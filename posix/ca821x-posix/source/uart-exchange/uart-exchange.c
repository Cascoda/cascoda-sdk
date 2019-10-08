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
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "ca821x-generic-exchange.h"
#include "ca821x-queue.h"
#include "ca821x_api.h"
#include "uart-exchange.h"

#ifndef UART_MAX_DEVICES
#define UART_MAX_DEVICES 5
#endif

struct uart_exchange_priv
{
	struct ca821x_exchange_base base;
	int                         fd;
	uint8_t                     tx_stalled;
	uint8_t                     buf[MAX_BUF_SIZE];
	size_t                      offset;
};

struct uart_device
{
	struct uart_device *next;
	const char *        device;
	int                 baud;
};

static struct ca821x_dev * s_devs[UART_MAX_DEVICES] = {0};
static int                 s_devcount               = 0;
static int                 s_initialised            = 0;
static char *              s_CASCODA_UART           = NULL;
static struct uart_device *s_uart_device_head       = NULL;

static const uint8_t UART_SOM = 0xDE;
static const uint8_t UART_ACK = 0xAA;

static pthread_mutex_t devs_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  devs_cond  = PTHREAD_COND_INITIALIZER;

static void assert_uart_exchange(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;

	if (priv->base.exchange_type != ca821x_exchange_uart)
		abort();
}

static ca_error send_uart_ack(int fd)
{
	ca_error error = CA_ERROR_SUCCESS;
	uint8_t  ack[] = {UART_SOM, UART_ACK, 0};
	int      rval, len;

	len = sizeof(ack);

	while (len > 0)
	{
		rval = write(fd, ack, len);
		if (rval < 0)
		{
			error = CA_ERROR_FAIL;
			break;
		}
		len -= rval;
	}

	return error;
}

ssize_t uart_try_read(struct ca821x_dev *pDeviceRef, uint8_t *buf)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;
	ssize_t                    len;
	int                        error  = 0;
	uint8_t *                  som    = priv->buf;
	uint8_t *                  cmdid  = priv->buf + 1;
	uint8_t *                  cmdlen = priv->buf + 2;

	assert_uart_exchange(pDeviceRef);

	//Read from the device if possible
	len = read(priv->fd, priv->buf + priv->offset, sizeof(priv->buf) - priv->offset);
	if (len < 0)
	{
		if (errno == EAGAIN)
		{
			len = 0;
		}
		else
		{
			error = -uart_exchange_err_uart;
			goto exit;
		}
	}
	priv->offset += len;

	//Catch SOM errors
	while (priv->offset && *som != UART_SOM)
	{
		/*
		 * Getting these very occasionally is ok, but if they are regular then either the
		 * line is too noisy, or there are some serious issues with receiving bytes
		 * and the baud rate should be decreased. If a stream of these occur in a row
		 * then it is likely a SOM has been missed and a packet has been lost.
		 */
		ca_log_warn("No SOM, got 0x%02x", priv->buf[0]);
		memmove(priv->buf, priv->buf + 1, priv->offset);
		priv->offset -= 1;
	}

	// If an incomplete packet has been received, hold off returning it up until the full
	// thing comes through.
	if ((priv->offset >= 3) && (priv->offset >= (*cmdlen + 3U)))
	{
		ssize_t framelen = *cmdlen + 3;

		//Skip SOM byte in copy
		len = framelen - 1;
		memcpy(buf, cmdid, len);
		// If extra data has been received, shuffle about
		memmove(priv->buf, priv->buf + framelen, (priv->offset - framelen));
		priv->offset -= framelen;

		//Send ACK
		if (buf[0] != UART_ACK && send_uart_ack(priv->fd) != CA_ERROR_SUCCESS)
		{
			error = -uart_exchange_err_uart;
			goto exit;
		}
	}
	else
	{
		len = 0;
	}

	//Catch, process & discard UART ACKs
	if (len && buf[0] == UART_ACK)
	{
		priv->tx_stalled = 0;
		len              = 0;
	}

	if (len >= 3 && buf[0] == 0xF0)
	{
		ca_log_crit("ERROR CODE 0x%02x", buf[2]);
		fflush(stderr);
		//Error packet indicating coprocessor has reset ca821x - let app know
		if (buf[3])
			error = -uart_exchange_err_ca821x;
		goto exit;
	}

exit:
	if (error < 0)
	{
		return error;
	}
	return len;
}

ca_error uart_write_isready(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;

	return priv->tx_stalled ? CA_ERROR_BUSY : CA_ERROR_SUCCESS;
}

ca_error uart_try_write(const uint8_t *buffer, size_t len, struct ca821x_dev *pDeviceRef)
{
	int                        rval  = 0;
	ca_error                   error = CA_ERROR_SUCCESS;
	struct uart_exchange_priv *priv  = pDeviceRef->exchange_context;

	assert_uart_exchange(pDeviceRef);

	//send SOM byte
	do
	{
		rval = write(priv->fd, &UART_SOM, 1);
		if (rval < 0)
		{
			error = CA_ERROR_FAIL;
			break;
		}
	} while (rval != 1);

	//send message
	while (len)
	{
		rval = write(priv->fd, buffer, len);
		if (rval < 0)
		{
			error = CA_ERROR_FAIL;
			break;
		}
		len -= len;
		buffer += rval;
	}

	if (error < 0)
	{
		ca_log_crit("UART Send error!");
		error = CA_ERROR_FAIL;
	}
	else
	{
		//Wait for ack
		priv->tx_stalled = 1;
	}
	return error;
}

void flush_unread_uart(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;
	uint8_t                    junk_buf[100];
	int                        rval;

	assert_uart_exchange(pDeviceRef);

	do
	{
		rval = read(priv->fd, junk_buf, 100);
	} while (rval > 0);
}

static ca_error init_statics()
{
	ca_error    error = CA_ERROR_SUCCESS;
	char *      saveptr1, *saveptr2;
	char *      masterStr;
	const char *CONST_CASCODA_UART = getenv("CASCODA_UART");
	//example: CASCODA_UART="/dev/ttyS0,115200:/dev/ttyS1,9600:/dev/ttyS2,6000000"
	size_t               envLen;
	struct uart_device **oldTail = &s_uart_device_head;

	s_initialised = 1;

	if (!CONST_CASCODA_UART)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	// The value returned by getenv shouldn't be modified (no idea why it isn't const)
	// so make a static copy that can be tokenized and used in our linked list.
	envLen         = strlen(CONST_CASCODA_UART) + 1;
	s_CASCODA_UART = malloc(envLen);
	memcpy(s_CASCODA_UART, CONST_CASCODA_UART, envLen);
	s_CASCODA_UART[envLen - 1] = '\0';
	masterStr                  = s_CASCODA_UART;

	do
	{
		// Get the next, major ':' delimited section
		char *majorStr = strtok_r(masterStr, ":", &saveptr1);
		char *device, *baudstr;

		masterStr = NULL;
		if (!majorStr)
			break;

		// Split by token
		device = strtok_r(majorStr, ",", &saveptr2);
		if (!device)
			continue;
		baudstr = strtok_r(NULL, ",", &saveptr2);
		if (!baudstr)
			continue;

		// Add to linked list
		(*oldTail)         = malloc(sizeof(struct uart_device));
		(*oldTail)->next   = NULL;
		(*oldTail)->baud   = atoi(baudstr);
		(*oldTail)->device = device;
		oldTail            = &((*oldTail)->next);
	} while (1);

	if (!s_uart_device_head)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

exit:
	return error;
}

static int deinit_statics()
{
	struct uart_device *uartdev = s_uart_device_head;

	s_initialised = 0;

	s_uart_device_head = NULL;
	while (uartdev)
	{
		struct uart_device *tmp = uartdev;
		uartdev                 = uartdev->next;
		free(tmp);
	}
	free(s_CASCODA_UART);

	return 0;
}

static speed_t get_baud_code(int baudrate)
{
	/* Yes, this is annoying - may have to expand this in future
	 * or potentially gate some of the baud codes with #ifdef if
	 * we find a system that it doesn't compile on.
	 */
	switch (baudrate)
	{
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
	default:
		return (speed_t)-1;
	}
}

static ca_error setup_port(int fd, int baudrate)
{
	struct termios port;
	speed_t        baudcode = get_baud_code(baudrate);

	if (tcgetattr(fd, &port) < 0)
	{
		return CA_ERROR_INVALID;
	}

	if (cfsetospeed(&port, baudcode) || cfsetispeed(&port, baudcode))
	{
		ca_log_crit("Errno %d, Failed setting baud rate %d", errno, baudrate);
		return CA_ERROR_INVALID_ARGS;
	}

	port.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
	port.c_cflag &= ~CSIZE;           /* 8-bit characters */
	port.c_cflag |= CS8;              /* 8-bit characters */
	port.c_cflag &= ~PARENB;          /* no parity bit */
	port.c_cflag &= ~CSTOPB;          /* only need 1 stop bit */
	port.c_cflag &= ~CRTSCTS;         /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	port.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	port.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	port.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	port.c_cc[VMIN]  = 1;
	port.c_cc[VTIME] = 1;

	if (tcsetattr(fd, TCSANOW, &port) != 0)
	{
		return CA_ERROR_INVALID;
	}

	return CA_ERROR_SUCCESS;
}

ca_error uart_exchange_init(struct ca821x_dev *pDeviceRef)
{
	return uart_exchange_init_withhandler(NULL, pDeviceRef);
}

ca_error uart_exchange_init_withhandler(ca821x_errorhandler callback, struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = NULL;
	struct uart_device *       uartdev;
	ca_error                   error = 0;

	if (!s_initialised)
	{
		error = init_statics();
		if (error)
			return error;
	}

	if (pDeviceRef->exchange_context)
		return CA_ERROR_ALREADY;

	pthread_mutex_lock(&devs_mutex);
	if (s_devcount >= UART_MAX_DEVICES)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	pDeviceRef->exchange_context  = calloc(1, sizeof(struct uart_exchange_priv));
	priv                          = pDeviceRef->exchange_context;
	priv->base.exchange_type      = ca821x_exchange_uart;
	priv->base.error_callback     = callback;
	priv->base.write_func         = uart_try_write;
	priv->base.write_isready_func = uart_write_isready;
	priv->base.read_func          = uart_try_read;
	priv->base.flush_func         = flush_unread_uart;
	priv->fd                      = -1;
	priv->offset                  = 0;

	//Open and set up file descriptor if available
	uartdev = s_uart_device_head;
	while (uartdev)
	{
		priv->fd = open(uartdev->device, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
		if (priv->fd < 0)
			continue;

		if (setup_port(priv->fd, uartdev->baud) != CA_ERROR_SUCCESS)
		{
			ca_log_crit("Failed setup for device %s", uartdev->device);
			close(priv->fd);
			priv->fd = -1;
		}
		else
		{
			break;
		}

		uartdev = uartdev->next;
	}

	if (priv->fd < 0)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	error = init_generic(pDeviceRef);

	if (error != 0)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	//Add the new device to the device list for io
	s_devcount++;
	pthread_cond_signal(&devs_cond);
	for (int i = 0; i < UART_MAX_DEVICES; i++)
	{
		if (s_devs[i] == NULL)
		{
			s_devs[i] = pDeviceRef;
			break;
		}
	}

exit:
	if (error && pDeviceRef->exchange_context)
	{
		free(pDeviceRef->exchange_context);
		pDeviceRef->exchange_context = NULL;
	}
	pthread_mutex_unlock(&devs_mutex);
	if (error == CA_ERROR_SUCCESS)
		ca_log_info("Successfully started UART Exchange.");
	return error;
}

void uart_exchange_deinit(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;

	assert_uart_exchange(pDeviceRef);
	deinit_generic(pDeviceRef);

	pthread_mutex_lock(&devs_mutex);
	s_devcount--;
	if (s_devcount == 0)
		deinit_statics();
	pthread_cond_signal(&devs_cond);
	for (int i = 0; i < UART_MAX_DEVICES; i++)
	{
		if (s_devs[i] == pDeviceRef)
		{
			s_devs[i] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&devs_mutex);

	free(priv);
	pDeviceRef->exchange_context = NULL;
}

int uart_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef)
{
	//For uart, the coprocessor will reset the ca821x if it isn't responsive.. so just rely on that
	(void)resettime;
	(void)pDeviceRef;

	return 0;
}
