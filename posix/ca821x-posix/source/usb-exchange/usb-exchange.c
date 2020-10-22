/*
 * Copyright (c) 2016, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "hidapi/hidapi.h"
#include "ca821x-generic-exchange.h"
#include "ca821x-posix-util-internal.h"
#include "ca821x-queue.h"
#include "ca821x_api.h"
#include "usb-exchange.h"

#define USB_VID 0x0416
#define USB_PID 0x5020

#define MAX_FRAG_SIZE 64
/** Max time to wait on rx data in milliseconds */
#define POLL_DELAY 2

#ifndef USB_MAX_DEVICES
#define USB_MAX_DEVICES 5
#endif

#define FRAG_LEN_MASK 0x3F
#define FRAG_LAST_MASK (1 << 7)
#define FRAG_FIRST_MASK (1 << 6)

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))

/**
 * The usb exchange private-data struct representing a single device.
 */
struct usb_exchange_priv
{
	struct ca821x_exchange_base base;          //!< Exchange base struct
	hid_device *                hid_dev;       //!< hidapi device reference struct
	wchar_t *                   serial_number; //!< chili serial number

#if CASCODA_RASPI_USB_WORKAROUND
	struct timespec prev_send; //!< The time that the previous usb packet was sent
#endif
#ifdef _WIN32
	HANDLE hid_mutex; //!< System mutex so devices are only controlled once
#endif
};

static struct ca821x_dev *s_devs[USB_MAX_DEVICES] = {0};
static int                s_devcount              = 0;
static int                s_initialised           = 0;

//Dynamic hid-api library
#ifndef _WIN32
static void *s_hid_lib_handle = NULL;
#endif
static struct hid_device_info *(*dhid_enumerate)(unsigned short, unsigned short);
static hid_device *(*dhid_open_path)(const char *);
static void (*dhid_close)(hid_device *);
static void (*dhid_free_enumeration)(struct hid_device_info *);
static int (*dhid_read_timeout)(hid_device *, unsigned char *, size_t, int);
static int (*dhid_write)(hid_device *, const unsigned char *, size_t);
static int (*dhid_exit)(void);

static ca_error reload_hid_device(struct ca821x_dev *pDeviceRef);

static pthread_mutex_t devs_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  devs_cond  = PTHREAD_COND_INITIALIZER;

/**
 * Assert that the device is a usb device, and crash if not.
 *
 * If this crashes the program, then a serious programmer error
 * is the cause.
 */
static void assert_usb_exchange(struct ca821x_dev *pDeviceRef)
{
	struct usb_exchange_priv *priv = pDeviceRef->exchange_context;

	if (priv->base.exchange_type != ca821x_exchange_usb)
		abort();
}

//returns 1 for non-final fragment, 0 for final
static int get_next_frag(const uint8_t *buf_in, uint8_t len_in, uint8_t *frag_out, uint8_t *offset)
{
	int     end_offset = *offset + MAX_FRAG_SIZE - 1;
	uint8_t is_first = 0, is_last = 0, frag_len = 0;
	is_first = (*offset == 0);

	if (end_offset >= len_in)
	{
		end_offset = len_in;
		is_last    = 1;
	}
	frag_len = end_offset - *offset;

	assert((frag_len & FRAG_LEN_MASK) == frag_len);

	frag_out[0] = 0;
	frag_out[1] = 0;
	frag_out[1] |= frag_len;
	frag_out[1] |= is_first ? FRAG_FIRST_MASK : 0;
	frag_out[1] |= is_last ? FRAG_LAST_MASK : 0;
	memcpy(&frag_out[2], &buf_in[*offset], frag_len);

	*offset = end_offset;

	if (is_last)
		*offset = 0;
	return !is_last;
}

/**
 * Assemble received USB fragments into a receive buffer.
 * @returns ca_error
 * @retval 1 for non-final fragment received
 * @retval 0 for final fragment or dropped packet
 */
static int assemble_frags(uint8_t *frag_in, uint8_t *buf_out, uint8_t *len_out, uint8_t *offset)
{
	uint8_t is_first = 0, is_last = 0, frag_len = 0;
	frag_len = frag_in[0] & FRAG_LEN_MASK;
	is_last  = !!(frag_in[0] & FRAG_LAST_MASK);
	is_first = !!(frag_in[0] & FRAG_FIRST_MASK);

	if ((is_first) != (*offset == 0))
	{
		if (is_first)
		{
			// Drop previous frame and start processing this one
			ca_log_crit("Dropped received frame: Missed 'last' packet. CmdId %x", buf_out[0]);
			*offset  = 0;
			*len_out = 0;
		}
		else
		{
			// Drop this frame, will be able to receive next packet
			ca_log_crit("Dropped received frame: Missed 'first' packet.");
			*offset  = 0;
			*len_out = 0;
			return 0;
		}
	}

	memcpy(&buf_out[*offset], &frag_in[1], frag_len);

	*offset += frag_len;
	*len_out = *offset;

	if (is_last)
		*offset = 0;
	return !is_last;
}

#ifdef TEST_ENABLE
void test_frag_loopback()
{
	uint8_t data_in[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	                     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	                     0x20, 0x21, 0x22, 0x23, 0xca, 0x5c, 0x0d, 0xaa, 0xca, 0x5c, 0x0d, 0xaa, 0x11, 0x11, 0x22, 0x33,
	                     0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x45, 0x56, 0x67, 0xe9,
	                     0xdc, 0x7c, 0x64, 0x56, 0x9a, 0x68, 0xe9, 0x86, 0xe8, 0xe2, 0xf1, 0x92, 0x9e, 0xc5, 0x92, 0x67,
	                     0x5f, 0x91, 0x65, 0xae, 0x9f, 0x01, 0x45, 0x12, 0xe5, 0xdb, 0xfb, 0x07, 0xf2, 0xe8, 0xfd, 0xb2,
	                     0x54, 0x26, 0x1d, 0xe8, 0xec, 0x3e, 0xf8, 0x25, 0xaa, 0xe6, 0x7e, 0xba, 0x5b, 0xa0, 0x6e, 0xfc,
	                     0xa3, 0xdf, 0x6d, 0x97, 0xbe, 0x7c, 0xf6, 0x51, 0x77, 0x7f, 0x28, 0x44, 0xda, 0x48, 0x4f, 0x2e,
	                     0x57, 0xc3, 0x81, 0x8e, 0x76, 0x22, 0x3d, 0x40, 0x5a, 0x69, 0x62, 0x91, 0x10, 0x87, 0x1d, 0x11,
	                     0x11, 0x11, 0xca, 0x5c, 0x0d, 0xaa, 0x11, 0x11};

	int     data_in_size = sizeof(data_in) / sizeof(data_in[0]);
	uint8_t data_out[MAX_BUF_SIZE];
	uint8_t frag_buf[MAX_FRAG_SIZE + 1];
	uint8_t len, rval, offset1 = 0, offset2 = 0;

	do
	{
		rval = get_next_frag(data_in, data_in_size, frag_buf, &offset1);
	} while (assemble_frags(frag_buf + 1, data_out, &len, &offset2));

	if (rval)
		abort(); //make sure both assembly and deconstruction thought this was last frag
	if (len != data_in_size)
		abort();

	rval = memcmp(data_in, data_out, len);
	assert(rval == 0);
}
#endif

/**
 * This function applies a workaround that is necessary on the Raspberry pi for
 * reliable USB communications. Essentially, on the raspberry pi, if there is not
 * regular USB transmission to the OUT endpoint, the next OUT transmission will
 * take up to 1000ms to complete. This behaviour has only been observed on the
 * Raspberry pi. The hotfix for this is to send a USB transmission every 1 second
 * on the raspi platform.
 */
void usb_apply_raspi_workaround(struct ca821x_dev *pDeviceRef)
{
#if CASCODA_RASPI_USB_WORKAROUND
	struct usb_exchange_priv *priv = pDeviceRef->exchange_context;
	struct timespec           curTime;

	if (clock_gettime(CLOCK_REALTIME, &curTime))
		return;

	curTime = time_sub(&curTime, &priv->prev_send);

	// Not enough time has passed, no need to send packet
	if (curTime.tv_sec < 1)
		return;

	// Add a USB packet (EVBME COMM_CHECK to the queue)
	uint8_t comm_check[] = {0xA1, 3, 0, 0, 0};
	add_to_queue(
	    &(priv->base.out_buffer_queue), &(priv->base.out_queue_mutex), comm_check, sizeof(comm_check), pDeviceRef);

	clock_gettime(CLOCK_REALTIME, &(priv->prev_send));
#else
	(void)pDeviceRef;
#endif
}

ssize_t usb_try_read(struct ca821x_dev *pDeviceRef, uint8_t *buf)
{
	struct usb_exchange_priv *priv = pDeviceRef->exchange_context;
	uint8_t                   frag_buf[MAX_FRAG_SIZE + 1]; //+1 for report ID
	uint8_t                   delay, offset, len = 0;
	int                       error;

	assert_usb_exchange(pDeviceRef);
	usb_apply_raspi_workaround(pDeviceRef);

	delay = POLL_DELAY;

	//Read from the device if possible
	offset = 0;
	do
	{
		error = dhid_read_timeout(priv->hid_dev, frag_buf, MAX_FRAG_SIZE, delay);
		if (error <= 0)
			break;
		delay = -1;
	} while (assemble_frags(frag_buf, buf, &len, &offset));

	if (error < 0)
	{
		error = -usb_exchange_err_usb;
		if (reload_hid_device(pDeviceRef) == CA_ERROR_SUCCESS) //usb disconnected - attempt to grab new device
		{
			len   = 0;
			error = 0;
		}
	}

	if (buf[0] == 0xF0 && len)
	{
		static int ecount = 0;
		if (ecount < 20)
		{
			ca_log_crit("ERROR CODE 0x%02x", buf[2]);
			fflush(stderr);
			ecount++;
		}
		//Error packet indicating coprocessor has reset ca821x - let app know
		if (buf[3])
			error = -usb_exchange_err_ca821x;
	}

	if (error <= 0)
	{
		return error;
	}
	return len;
}

ca_error usb_try_write(const uint8_t *buffer, size_t len, struct ca821x_dev *pDeviceRef)
{
	uint8_t                   offset = 0;
	uint8_t                   frag_buf[MAX_FRAG_SIZE + 1]; //+1 for report ID
	int                       rval, error;
	ca_error                  caerror = CA_ERROR_SUCCESS;
	struct usb_exchange_priv *priv    = pDeviceRef->exchange_context;

	assert_usb_exchange(pDeviceRef);

	do
	{
		uint8_t retries = 0;

		rval = get_next_frag(buffer, len, frag_buf, &offset);
		do
		{
			error = dhid_write(priv->hid_dev, frag_buf, MAX_FRAG_SIZE + 1);
		} while ((error < 0) && (retries++ < 50));
	} while (rval && (error >= 0));

	if (error < 0)
	{
		ca_log_crit("USB Send error!");
		caerror = CA_ERROR_FAIL;
		if (reload_hid_device(pDeviceRef) == CA_ERROR_SUCCESS)
		{
			caerror = usb_try_write(buffer, len, pDeviceRef); //usb disconnected - attempt to grab new device
		}
	}
#if CASCODA_RASPI_USB_WORKAROUND
	else
	{
		clock_gettime(CLOCK_REALTIME, &(priv->prev_send));
	}
#endif
	return caerror;
}

void flush_unread_usb(struct ca821x_dev *pDeviceRef)
{
	struct usb_exchange_priv *priv = pDeviceRef->exchange_context;
	uint8_t                   frag_buf[MAX_FRAG_SIZE + 1]; //+1 for report ID
	int                       rval;

	assert_usb_exchange(pDeviceRef);

	do
	{
		rval = dhid_read_timeout(priv->hid_dev, frag_buf, MAX_FRAG_SIZE, 10);
	} while (rval > 0);
}

#ifdef _WIN32
//No Dynamic library, use statically linked
static ca_error load_dlibs()
{
	dhid_enumerate        = &hid_enumerate;
	dhid_open_path        = &hid_open_path;
	dhid_close            = &hid_close;
	dhid_free_enumeration = &hid_free_enumeration;
	dhid_read_timeout     = &hid_read_timeout;
	dhid_write            = &hid_write;
	dhid_exit             = &hid_exit;

	return CA_ERROR_SUCCESS;
}
#else
//Use dynamic lib
static ca_error load_dlibs()
{
	ca_error error = CA_ERROR_SUCCESS;
	char *hidapi_lib[] = {
	    "libhidapi-libusb.so", "libhidapi-libusb.so.0", "libhidapi-hidraw.so", "libhidapi-hidraw.so.0"};

	ca_log_debg("Loading hidapi dlibs...");

	//Load the dynamic library
	for (size_t i = 0; i < ARRAY_LENGTH(hidapi_lib); i++)
	{
		ca_log_debg("Trying to open dlib %s", hidapi_lib[i]);
		s_hid_lib_handle = dlopen(hidapi_lib[i], RTLD_NOW);
		if (s_hid_lib_handle)
			break;
		ca_log_debg("Failed.");
	}

	if (s_hid_lib_handle == NULL)
	{
		ca_log_debg("Failed to open dynamic library for hidapi.");
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	//Clear the dl error, by reading it
	dlerror();

	dhid_enumerate = dlsym(s_hid_lib_handle, "hid_enumerate");
	dhid_open_path = dlsym(s_hid_lib_handle, "hid_open_path");
	dhid_close = dlsym(s_hid_lib_handle, "hid_close");
	dhid_free_enumeration = dlsym(s_hid_lib_handle, "hid_free_enumeration");
	dhid_read_timeout = dlsym(s_hid_lib_handle, "hid_read_timeout");
	dhid_write = dlsym(s_hid_lib_handle, "hid_write");
	dhid_exit = dlsym(s_hid_lib_handle, "hid_exit");

	if (dlerror() != NULL)
	{
		ca_log_debg("Failed to find required dynamic symbols.");
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

exit:
	if (error && (s_hid_lib_handle != NULL))
	{
		ca_log_debg("Error loading dynamic hidapi.");
		dlclose(s_hid_lib_handle);
	}
	return error;
}
#endif

static ca_error init_statics()
{
	ca_error error = CA_ERROR_SUCCESS;

	error = load_dlibs();
	if (error)
		goto exit;

	s_initialised = 1;

exit:
	return error;
}

static ca_error deinit_statics()
{
	dhid_exit();
	s_initialised = 0;
#ifndef _WIN32
	dlclose(s_hid_lib_handle);
#endif
	return CA_ERROR_SUCCESS;
}

int usb_exchange_init(struct ca821x_dev *pDeviceRef)
{
	return usb_exchange_init_withhandler(NULL, pDeviceRef);
}

static int is_serialno_in_use(wchar_t *path)
{
	int rval;
	for (int i = 0; i < USB_MAX_DEVICES; i++)
	{
		if (!s_devs[i])
			continue;
		struct usb_exchange_priv *cur = s_devs[i]->exchange_context;
		rval                          = wcscmp(path, cur->serial_number);
		if (rval == 0)
			return 1;
	}
	return 0;
}

static struct hid_device_info *get_next_hid(struct hid_device_info *hid_cur)
{
	while (hid_cur != NULL)
	{
		if (!is_serialno_in_use(hid_cur->serial_number))
			break;
		hid_cur = hid_cur->next;
	}

	return hid_cur;
}

static ca_error reload_hid_device(struct ca821x_dev *pDeviceRef)
{
	struct usb_exchange_priv *priv   = pDeviceRef->exchange_context;
	struct hid_device_info *  hid_ll = NULL, *hid_cur = NULL;
	ca_error                  error = CA_ERROR_SUCCESS;

	assert_usb_exchange(pDeviceRef);
	pthread_mutex_lock(&devs_mutex);

	ca_log_warn("Hid device dropped... attempting reload");

	dhid_close(priv->hid_dev);
	priv->hid_dev = NULL;

	sleep(1);

	//Iterate through compatible HIDs until matching serial number is found
	hid_ll  = dhid_enumerate(USB_VID, USB_PID);
	hid_cur = hid_ll;
	while (priv->hid_dev == NULL && hid_cur != NULL)
	{
		//Find matching serial number
		if (wcscmp(priv->serial_number, hid_cur->serial_number) == 0)
			priv->hid_dev = dhid_open_path(hid_cur->path);

		if (priv->hid_dev == NULL)
		{
			hid_cur = hid_cur->next;
		}
	}
	if (hid_cur == NULL)
	{ //Device not found
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

exit:
	if (hid_ll)
		dhid_free_enumeration(hid_ll);
	if (error)
		ca_log_crit("Failed to reload HID device");
	else
		ca_log_info("Successfully reloaded HID device");
	pthread_mutex_unlock(&devs_mutex);
	return error;
}

#ifdef _WIN32
static ca_error lock_device(wchar_t *serial, struct ca821x_dev *pDeviceRef)
{
	struct usb_exchange_priv *priv = pDeviceRef->exchange_context;
	char                      path[40];
	DWORD                     rval = 0;

	snprintf(path, sizeof(path), "Global\\casc-%ws", serial);
	priv->hid_mutex = CreateMutex(NULL, FALSE, path);

	if (priv->hid_mutex == NULL)
		return CA_ERROR_FAIL;

	rval = WaitForSingleObject(priv->hid_mutex, 0);

	if (rval == WAIT_OBJECT_0 || rval == WAIT_ABANDONED)
	{
		//We own mutex
		return CA_ERROR_SUCCESS;
	}
	else
	{
		//We don't own the mutex
		CloseHandle(priv->hid_mutex);
		priv->hid_mutex = NULL;
		return CA_ERROR_NO_ACCESS;
	}
}

static ca_error unlock_device(struct ca821x_dev *pDeviceRef)
{
	struct usb_exchange_priv *priv = pDeviceRef->exchange_context;

	if (priv->hid_mutex == NULL)
		return CA_ERROR_NOT_FOUND;

	ReleaseMutex(priv->hid_mutex);
	CloseHandle(priv->hid_mutex);
	priv->hid_mutex = NULL;

	return CA_ERROR_SUCCESS;
}
#else
static ca_error lock_device(wchar_t *serial, struct ca821x_dev *pDeviceRef)
{
	(void)serial;
	(void)pDeviceRef;
	return CA_ERROR_SUCCESS;
}

static ca_error unlock_device(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	return CA_ERROR_SUCCESS;
}
#endif

ca_error usb_exchange_init_withhandler(ca821x_errorhandler callback, struct ca821x_dev *pDeviceRef)
{
	struct hid_device_info *  hid_ll = NULL, *hid_cur = NULL;
	hid_device *              dev   = NULL;
	struct usb_exchange_priv *priv  = NULL;
	ca_error                  error = CA_ERROR_SUCCESS;
	size_t                    len   = 0;

	ca_log_debg("Trying USB Exchange");
	if (!s_initialised)
	{
		ca_log_debg("First time init, initialising statics (such as dynamic lib).");
		error = init_statics();
		if (error)
			return error;
	}

	if (pDeviceRef->exchange_context)
		return CA_ERROR_ALREADY;

	ca_log_debg("USB exchange static parts loaded, initialising device...");
	pthread_mutex_lock(&devs_mutex);
	if (s_devcount >= USB_MAX_DEVICES)
	{
		ca_log_warn("Aborting device search, maximum usb exchange count reached.");
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	pDeviceRef->exchange_context = calloc(1, sizeof(struct usb_exchange_priv));
	priv                         = pDeviceRef->exchange_context;

	if (!priv)
	{
		ca_log_warn("Failed to allocate exchange context.");
		error = CA_ERROR_NO_BUFFER;
		goto exit;
	}

	priv->base.exchange_type  = ca821x_exchange_usb;
	priv->base.error_callback = callback;
	priv->base.write_func     = usb_try_write;
	priv->base.read_func      = usb_try_read;
	priv->base.flush_func     = flush_unread_usb;

	ca_log_debg("USB callbacks loaded into exchange struct.");

	//Iterate through compatible HIDs until one is found that hasn't already
	//been opened.
	hid_ll  = dhid_enumerate(USB_VID, USB_PID);
	hid_cur = get_next_hid(hid_ll);
	while (dev == NULL && hid_cur != NULL)
	{
		ca_log_debg("Attempting to claim usb device %s", hid_cur->path);
		error = lock_device(hid_cur->serial_number, pDeviceRef);
		if (error == CA_ERROR_SUCCESS)
			dev = dhid_open_path(hid_cur->path);
		else
			dev = NULL;

		if (dev == NULL)
		{
			unlock_device(pDeviceRef);
			hid_cur = get_next_hid(hid_cur->next);
		}
	}
	if (hid_cur == NULL)
	{ //Device not found
		ca_log_debg("Failed to find unused usb device.");
		error = CA_ERROR_NOT_FOUND;
		free(pDeviceRef->exchange_context);
		pDeviceRef->exchange_context = NULL;
		goto exit;
	}

	len                 = (wcslen(hid_cur->serial_number) + 1) * sizeof(wchar_t);
	priv->serial_number = malloc(len);
	memcpy(priv->serial_number, hid_cur->serial_number, len);
	priv->hid_dev = dev;

	error = init_generic(pDeviceRef) ? CA_ERROR_FAIL : CA_ERROR_SUCCESS;

	if (error != CA_ERROR_SUCCESS)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	//Add the new device to the device list for io
	s_devcount++;
	pthread_cond_signal(&devs_cond);
	for (int i = 0; i < USB_MAX_DEVICES; i++)
	{
		if (s_devs[i] == NULL)
		{
			s_devs[i] = pDeviceRef;
			break;
		}
	}

exit:
	if (hid_ll)
		dhid_free_enumeration(hid_ll);
	if (error && pDeviceRef->exchange_context)
	{
		free(priv->serial_number);
		free(pDeviceRef->exchange_context);
		pDeviceRef->exchange_context = NULL;
	}
	pthread_mutex_unlock(&devs_mutex);
	if (error == CA_ERROR_SUCCESS)
		ca_log_info("Successfully started USB Exchange.");
	return error;
}

void usb_exchange_deinit(struct ca821x_dev *pDeviceRef)
{
	struct usb_exchange_priv *priv = pDeviceRef->exchange_context;

	assert_usb_exchange(pDeviceRef);
	deinit_generic(pDeviceRef);
	dhid_close(priv->hid_dev);
	unlock_device(pDeviceRef);

	pthread_mutex_lock(&devs_mutex);
	s_devcount--;
	if (s_devcount == 0)
		deinit_statics();
	pthread_cond_signal(&devs_cond);
	for (int i = 0; i < USB_MAX_DEVICES; i++)
	{
		if (s_devs[i] == pDeviceRef)
		{
			s_devs[i] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&devs_mutex);

	free(priv->serial_number);
	free(priv);
	pDeviceRef->exchange_context = NULL;
}

int usb_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef)
{
	//For usb, the coprocessor will reset the ca821x if it isn't responsive.. so just rely on that
	(void)resettime;
	(void)pDeviceRef;

	return 0;
}
