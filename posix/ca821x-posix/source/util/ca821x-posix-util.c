/*
 * Copyright (c) 2017, Cascoda
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ca821x-posix/ca821x-posix.h"
#include "cascoda-util/cascoda_rand.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x-generic-exchange.h"
#include "ca821x-posix-util-internal.h"
#include "kernel-exchange.h"
#include "uart-exchange.h"
#include "usb-exchange.h"

/** Static start time of program */
static struct timespec sStartTime = {0, 0};

uint32_t TIME_ReadAbsoluteTime(void)
{
	struct timespec curTime = {0, 0};
	uint32_t        time_ms;

	clock_gettime(CLOCK_MONOTONIC, &curTime);
	curTime = time_sub(&curTime, &sStartTime);

	time_ms = (int32_t)(curTime.tv_sec * 1000);
	time_ms += (int32_t)(curTime.tv_nsec / 1000000);

	return time_ms;
}

static void initStatic(void)
{
	if (sStartTime.tv_sec == 0 && sStartTime.tv_nsec == 0)
	{
		clock_gettime(CLOCK_MONOTONIC, &sStartTime);
		RAND_Seed((uint64_t)time(NULL));
		ca_log_note("Host Cascoda SDK %s", ca821x_get_version());
	}
}

ca_error ca821x_util_init(struct ca821x_dev *pDeviceRef, ca821x_errorhandler errorHandler, char *serial_num)
{
	ca_error error = CA_ERROR_SUCCESS;

	initStatic();
	error = ca821x_api_init(pDeviceRef);
	if (error)
		goto exit;
#ifdef _WIN32
	error = usb_exchange_init(errorHandler, NULL, pDeviceRef, serial_num);
#else
	error = kernel_exchange_init(errorHandler, pDeviceRef);

	if (error)
	{
		error = uart_exchange_init(errorHandler, NULL, pDeviceRef);
	}

	if (error)
	{
		error = usb_exchange_init(errorHandler, NULL, pDeviceRef, serial_num);
	}
#endif
exit:
	return error;
}

ca_error ca821x_util_init_path(struct ca821x_dev        *pDeviceRef,
                               ca821x_errorhandler       errorHandler,
                               enum ca821x_exchange_type exchangeType,
                               const char               *path)
{
	ca_error error = CA_ERROR_SUCCESS;

	initStatic();
	error = ca821x_api_init(pDeviceRef);
	if (error)
		goto exit;

	switch (exchangeType)
	{
	case ca821x_exchange_usb:
		error = usb_exchange_init(errorHandler, path, pDeviceRef, NULL);
		break;
#ifndef _WIN32
	case ca821x_exchange_uart:
		error = uart_exchange_init(errorHandler, path, pDeviceRef);
		break;
	case ca821x_exchange_kernel:
		error = kernel_exchange_init(errorHandler, pDeviceRef);
		break;
#endif
	default:
		error = CA_ERROR_INVALID_ARGS;
		break;
	}

exit:
	return error;
}

void ca821x_util_deinit(struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *base = pDeviceRef->exchange_context;

	if (base == NULL)
		return;

	switch (base->exchange_type)
	{
#ifdef _WIN32
	case ca821x_exchange_kernel:
	case ca821x_exchange_uart:
		break;
#else
	case ca821x_exchange_kernel:
		kernel_exchange_deinit(pDeviceRef);
		break;
	case ca821x_exchange_uart:
		uart_exchange_deinit(pDeviceRef);
		break;
#endif
	case ca821x_exchange_usb:
		usb_exchange_deinit(pDeviceRef);
		break;
	}
}

struct dev_info_context
{
	util_device_found callback; //!< User callback
	void             *context;  //!< Context to provide to user callback
	ca_error          status;   //!< Status of the enumeration
};

/**
 * Get an evbme Property into an allocated buffer
 * @param destp Pointer to string pointer to set to allocated buffer
 * @param aAttrId EVBME attribute to get
 * @returns The size of the buffer allocated to destp
 */
static int evbme_getprop_alloc(char **destp, enum evbme_attribute aAttrId, struct ca821x_dev *pDeviceRef)
{
	ca_error  status   = CA_ERROR_NO_BUFFER;
	const int buf_size = 100;
	uint8_t   len      = 0;

	*destp = malloc(buf_size);
	if (*destp)
	{
		status        = EVBME_GET_request_sync(aAttrId, buf_size - 1, (uint8_t *)*destp, &len, pDeviceRef);
		(*destp)[len] = '\0';
		if (status)
		{
			free(*destp);
			*destp = NULL;
			return 0;
		}
		return buf_size;
	}
	return 0;
}

static void enumerate_callback(struct ca_device_info *aDeviceInfo, void *aContext)
{
	ca_error                 status  = CA_ERROR_INVALID_ARGS;
	struct dev_info_context *context = aContext;
	struct ca821x_dev        tDevice;

	char *devNameBuf           = NULL;
	char *appNameBuf           = NULL;
	char *verBuf               = NULL;
	char *serBuf               = NULL;
	char *extFlashAvailableBuf = NULL;

	// Try to open the device to extract evbme info
	status = ca821x_util_init_path(&tDevice, NULL, aDeviceInfo->exchange_type, aDeviceInfo->path);

	//If successfully opened, extract the missing info. If not, leave it missing
	if (status == CA_ERROR_SUCCESS)
	{
		aDeviceInfo->available = true;

		if (evbme_getprop_alloc(&extFlashAvailableBuf, EVBME_EXTERNAL_FLASH_AVAILABLE, &tDevice))
			aDeviceInfo->external_flash_available = extFlashAvailableBuf[0];

		if (!aDeviceInfo->device_name)
		{
			evbme_getprop_alloc(&devNameBuf, EVBME_PLATSTRING, &tDevice);
			aDeviceInfo->device_name = devNameBuf;
		}
		if (!aDeviceInfo->app_name)
		{
			evbme_getprop_alloc(&appNameBuf, EVBME_APPSTRING, &tDevice);
			aDeviceInfo->app_name = appNameBuf;
		}
		if (!aDeviceInfo->version)
		{
			evbme_getprop_alloc(&verBuf, EVBME_VERSTRING, &tDevice);
			aDeviceInfo->version = verBuf;
		}
		if (!aDeviceInfo->serialno)
		{
			int bufsize = evbme_getprop_alloc(&serBuf, EVBME_SERIALNO, &tDevice);
			if (bufsize >= 8)
			{
				uint64_t serialno;
				serialno = GETLE64((uint8_t *)serBuf);
				snprintf(serBuf, bufsize, "%016llx", serialno);
				aDeviceInfo->serialno = serBuf;
			}
		}

		ca821x_util_deinit(&tDevice);
	}

	//Call the user callback
	context->callback(aDeviceInfo, context->context);
	context->status = CA_ERROR_SUCCESS;

	free(devNameBuf);
	free(appNameBuf);
	free(verBuf);
	free(serBuf);
}

ca_error ca821x_util_enumerate(util_device_found aCallback, void *aContext)
{
	struct dev_info_context context = {aCallback, aContext, CA_ERROR_NOT_FOUND};
#ifndef _WIN32
	kernel_exchange_enumerate(&enumerate_callback, &context);
	uart_exchange_enumerate(&enumerate_callback, &context);
#endif
	usb_exchange_enumerate(&enumerate_callback, &context);
	return context.status;
}

ca_error ca821x_util_reset(struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *base  = pDeviceRef->exchange_context;
	ca_error                     error = CA_ERROR_FAIL;

	if (base == NULL)
		return CA_ERROR_FAIL;

	switch (base->exchange_type)
	{
#ifdef _WIN32
	case ca821x_exchange_kernel:
	case ca821x_exchange_uart:
		break;
#else
	case ca821x_exchange_kernel:
		error = kernel_exchange_reset(1, pDeviceRef) ? CA_ERROR_FAIL : CA_ERROR_SUCCESS;
		break;
	case ca821x_exchange_uart:
		error = uart_exchange_reset(1, pDeviceRef) ? CA_ERROR_FAIL : CA_ERROR_SUCCESS;
		break;
#endif
	case ca821x_exchange_usb:
		error = usb_exchange_reset(1, pDeviceRef) ? CA_ERROR_FAIL : CA_ERROR_SUCCESS;
		break;
	}

	return error;
}

struct timespec time_sub(const struct timespec *t1, const struct timespec *t2)
{
	struct timespec curTime = {t1->tv_sec, t1->tv_nsec};
	//Get time difference
	curTime.tv_sec -= t2->tv_sec;
	curTime.tv_nsec -= t2->tv_nsec;

	// Normalize the struct in case the subtraction made negative nsec
	if (curTime.tv_nsec < 0)
	{
		curTime.tv_nsec += 1000000000L;
		curTime.tv_sec -= 1;
	}

	return curTime;
}

int time_cmp(const struct timespec *t1, const struct timespec *t2)
{
	if (t1->tv_sec == t2->tv_sec && t1->tv_nsec == t2->tv_nsec)
		return 0;

	if (t1->tv_sec < t2->tv_sec)
		return -1;
	if (t1->tv_sec > t2->tv_sec)
		return 1;
	if (t1->tv_nsec < t2->tv_nsec)
		return -1;
	if (t1->tv_nsec > t2->tv_nsec)
		return 1;

	return 0;
}
