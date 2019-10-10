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
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "ca821x-generic-exchange.h"
#include "ca821x-queue.h"
#include "kernel-exchange.h"

/******************************************************************************/

#define DebugFSMount "/sys/kernel/debug"
#define DriverNode "/ca8210"
#define DriverFilePath (DebugFSMount DriverNode)

#define CA8210_IOCTL_HARD_RESET (0)

/** Max time to wait on rx data in seconds */
#define POLL_DELAY 1

/******************************************************************************/

static int             DriverFileDescriptor, DriverFDPipe[2];
static pthread_mutex_t tx_mutex      = PTHREAD_MUTEX_INITIALIZER;
static int             s_initialised = 0;

/******************************************************************************/

struct kernel_exchange_priv
{
	struct ca821x_exchange_base base;
};

/******************************************************************************/
static void assert_kernel_exchange(struct ca821x_dev *pDeviceRef)
{
	struct kernel_exchange_priv *priv = pDeviceRef->exchange_context;

	if (priv->base.exchange_type != ca821x_exchange_kernel)
		abort();
}

static ca_error ca8210_test_int_write(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int      remaining = len;
	int      attempts  = 0;
	ca_error error     = CA_ERROR_SUCCESS;

	assert_kernel_exchange(pDeviceRef);

	do
	{
		int returnvalue;

		returnvalue = write(DriverFileDescriptor, buf + len - remaining, remaining);
		if (returnvalue > 0)
		{
			remaining -= returnvalue;
		}

		if (returnvalue == -1)
		{
			error = CA_ERROR_FAIL;

			if (errno == EAGAIN) //If the error is that the device is busy, try again after a short wait
			{
				error = CA_ERROR_BUSY;
				if (attempts++ < 5)
				{
					struct timespec toSleep;
					toSleep.tv_sec  = 0;
					toSleep.tv_nsec = 50 * 1000000;
					nanosleep(&toSleep, NULL); //Sleep for ~50ms
					continue;
				}
			}
			break;
		}
	} while (remaining > 0);

	return error;
}

ssize_t kernel_exchange_try_read(struct ca821x_dev *pDeviceRef, uint8_t *buf)
{
	struct kernel_exchange_priv *priv = pDeviceRef->exchange_context;
	struct timeval               timeout;

	assert_kernel_exchange(pDeviceRef);

	if (!peek_queue(priv->base.out_buffer_queue, &(priv->base.out_queue_mutex)))
	{
		fd_set  rx_block_fd_set;
		int     nfds;
		uint8_t dummybyte = 0;

		FD_ZERO(&rx_block_fd_set);
		FD_SET(DriverFileDescriptor, &rx_block_fd_set);
		FD_SET(DriverFDPipe[0], &rx_block_fd_set);
		nfds            = DriverFileDescriptor > DriverFDPipe[0] ? DriverFileDescriptor : DriverFDPipe[0];
		nfds            = nfds + 1;
		timeout.tv_sec  = POLL_DELAY;
		timeout.tv_usec = 0;
		select(nfds, &rx_block_fd_set, NULL, NULL, &timeout);
		read(DriverFDPipe[0], &dummybyte, 1);
	}

	//Read from the device if possible
	return read(DriverFileDescriptor, buf, 0);
}

void flush_unread_ke(struct ca821x_dev *pDeviceRef)
{
	uint8_t buffer[MAX_BUF_SIZE];
	ssize_t rval;

	assert_kernel_exchange(pDeviceRef);

	do
	{
		rval = read(DriverFileDescriptor, buffer, 0);
	} while (rval > 0);
}

void unblock_read(struct ca821x_dev *pDeviceRef)
{
	assert_kernel_exchange(pDeviceRef);

	const uint8_t dummybyte = 0;
	write(DriverFDPipe[1], &dummybyte, 1);
}

static int init_statics()
{
	DriverFileDescriptor = -1;
	s_initialised        = 1;
	return 0;
}

static int deinit_statics()
{
	s_initialised = 0;
	return 0;
}

int kernel_exchange_init(struct ca821x_dev *pDeviceRef)
{
	return kernel_exchange_init_withhandler(NULL, pDeviceRef);
}

ca_error kernel_exchange_init_withhandler(ca821x_errorhandler callback, struct ca821x_dev *pDeviceRef)
{
	ca_error                     error;
	struct kernel_exchange_priv *priv = NULL;

	if (!s_initialised)
	{
		error = init_statics() ? CA_ERROR_NOT_FOUND : CA_ERROR_SUCCESS;
		if (error)
			return error;
	}

	if (pDeviceRef->exchange_context)
		return CA_ERROR_ALREADY;

	if (DriverFileDescriptor != -1)
		return CA_ERROR_ALREADY;

	DriverFileDescriptor = open(DriverFilePath, O_RDWR | O_NONBLOCK);

	if (DriverFileDescriptor == -1)
	{
		return CA_ERROR_NOT_FOUND;
	}

	pipe(DriverFDPipe);
	fcntl(DriverFDPipe[0], F_SETFL, O_NONBLOCK);

	pDeviceRef->exchange_context = calloc(1, sizeof(struct kernel_exchange_priv));
	priv                         = pDeviceRef->exchange_context;
	priv->base.exchange_type     = ca821x_exchange_kernel;
	priv->base.error_callback    = callback;
	priv->base.write_func        = ca8210_test_int_write;
	priv->base.signal_func       = unblock_read;
	priv->base.read_func         = kernel_exchange_try_read;
	priv->base.flush_func        = flush_unread_ke;

	error = init_generic(pDeviceRef) ? CA_ERROR_NOT_FOUND : CA_ERROR_SUCCESS;

	if (error != 0)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

exit:
	if (error && pDeviceRef->exchange_context)
	{
		free(pDeviceRef->exchange_context);
		pDeviceRef->exchange_context = NULL;
	}
	if (error == CA_ERROR_SUCCESS)
		ca_log_info("Successfully started Kernel Exchange.");
	return error;
}

void kernel_exchange_deinit(struct ca821x_dev *pDeviceRef)
{
	int ret;

	assert_kernel_exchange(pDeviceRef);

	deinit_generic(pDeviceRef);
	deinit_statics();

	//Lock all mutexes
	pthread_mutex_lock(&tx_mutex);

	//close the driver file
	do
	{
		ret = close(DriverFileDescriptor);
	} while (ret < 0 && errno == EINTR);
	free(pDeviceRef->exchange_context);
	pDeviceRef->exchange_context = NULL;

	//unlock all mutexes
	pthread_mutex_unlock(&tx_mutex);
}

int kernel_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef)
{
	assert_kernel_exchange(pDeviceRef);
	return ioctl(DriverFileDescriptor, CA8210_IOCTL_HARD_RESET, resettime);
}
