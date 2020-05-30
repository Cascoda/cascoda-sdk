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

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix.h"
#include "ca821x-generic-exchange.h"
#include "ca821x-posix-evbme-internal.h"
#include "ca821x-queue.h"
#include "ca821x_api.h"

/** Is the downstream dispatch thread supposed to be running? */
static int dd_run_flag = 0;

/** Is the generic dispatch initialised (statics)? */
static int generic_initialised = 0;

/** Mutex for protecting static flags */
static pthread_mutex_t s_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Queue of buffers to be processed by downstream dispatch */
static struct buffer_queue *downstream_dispatch_queue = NULL;

/** mutex protecting downstream_dispatch_queue */
static pthread_mutex_t downstream_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Thread for running the downstream dispatch functions on the dd queue */
static pthread_t dd_thread;

/** Condition variable for the dd queue */
static pthread_cond_t dd_cond = PTHREAD_COND_INITIALIZER;

void (*wake_hw_worker)(void);

static void     init_generic_statics(void);
static ca_error deinit_generic_statics(void);

static int ca821x_run_downstream_dispatch()
{
	struct ca821x_dev *          pDeviceRef;
	struct ca821x_exchange_base *priv;
	uint8_t                      buffer[MAX_BUF_SIZE];
	ca_error                     rval;
	int                          len;

	len = pop_from_queue(&downstream_dispatch_queue, &downstream_queue_mutex, buffer, MAX_BUF_SIZE, &pDeviceRef);

	if (len > 0)
	{
		priv = pDeviceRef->exchange_context;
		rval = ca821x_downstream_dispatch(buffer, len, pDeviceRef);

		if (rval != CA_ERROR_SUCCESS)
		{
			rval = ca821x_evbme_dispatch(buffer, len, pDeviceRef);
		}

		if (rval != CA_ERROR_SUCCESS && priv->user_callback)
		{
			priv->user_callback(buffer, len, pDeviceRef);
		}
	}

	return len;
}

ca_error ca821x_util_dispatch_poll()
{
	int is_async;

	pthread_mutex_lock(&s_flag_mutex);
	is_async = dd_run_flag;
	pthread_mutex_unlock(&s_flag_mutex);

	if (is_async)
		return CA_ERROR_INVALID_STATE;

	if (ca821x_run_downstream_dispatch())
		return CA_ERROR_SUCCESS;
	else
		return CA_ERROR_NOT_FOUND;
}

static void *ca821x_downstream_dispatch_worker(void *arg)
{
	(void)arg;

	pthread_mutex_lock(&s_flag_mutex);
	while (dd_run_flag)
	{
		pthread_mutex_unlock(&s_flag_mutex);

		wait_on_queue(&downstream_dispatch_queue, &downstream_queue_mutex, &dd_cond);

		ca821x_run_downstream_dispatch();

		pthread_mutex_lock(&s_flag_mutex);
	}
	pthread_mutex_unlock(&s_flag_mutex);

	return 0;
}

ca_error ca821x_util_start_downstream_dispatch_worker()
{
	int rval = 0;
	int old_runflag;

	pthread_mutex_lock(&s_flag_mutex);
	old_runflag = dd_run_flag;
	pthread_mutex_unlock(&s_flag_mutex);

	if (old_runflag)
		return CA_ERROR_ALREADY;

	pthread_mutex_lock(&s_flag_mutex);
	dd_run_flag = 1;
	pthread_mutex_unlock(&s_flag_mutex);

	rval = pthread_create(&dd_thread, PTHREAD_CREATE_JOINABLE, &ca821x_downstream_dispatch_worker, NULL);

	return rval ? CA_ERROR_FAIL : CA_ERROR_SUCCESS;
}

ca_error ca821x_util_stop_downstream_dispatch_worker()
{
	int rval = 0;
	int old_runflag;

	pthread_mutex_lock(&s_flag_mutex);
	old_runflag = dd_run_flag;
	pthread_mutex_unlock(&s_flag_mutex);

	if (!old_runflag)
		return CA_ERROR_ALREADY;

	pthread_mutex_lock(&s_flag_mutex);
	dd_run_flag = 0;
	pthread_mutex_unlock(&s_flag_mutex);

	//Wake the downstream dispatch thread up so that it dies cleanly
	add_to_waiting_queue(&downstream_dispatch_queue, &downstream_queue_mutex, &dd_cond, NULL, 0, NULL);

	rval = pthread_join(dd_thread, NULL);

	return rval ? CA_ERROR_FAIL : CA_ERROR_SUCCESS;
}

ca_error init_generic(struct ca821x_dev *pDeviceRef)
{
	ca_error                     error = CA_ERROR_SUCCESS;
	struct ca821x_exchange_base *base  = pDeviceRef->exchange_context;

	ca_log_debg("Initialising generic part of exchange...");

	init_generic_statics();

	pthread_mutex_init(&(base->flag_mutex), NULL);
	pthread_mutex_init(&(base->sync_mutex), NULL);
	pthread_mutex_init(&(base->in_queue_mutex), NULL);
	pthread_mutex_init(&(base->out_queue_mutex), NULL);
	pthread_cond_init(&(base->sync_cond), NULL);
	pthread_cond_init(&(base->restore_cond), NULL);

	pthread_mutex_lock(&base->flag_mutex);
	base->io_thread_runflag = 1;
	pthread_mutex_unlock(&base->flag_mutex);

	ca_log_debg("Initialising io thread.");
	if (pthread_create(&(base->io_thread), PTHREAD_CREATE_JOINABLE, &ca821x_io_worker, pDeviceRef))
	{
		error = CA_ERROR_FAIL;
		ca_log_warn("Failed to start io thread!");
	}

	return error;
}

ca_error deinit_generic(struct ca821x_dev *pDeviceRef)
{
	ca_error                     error = CA_ERROR_SUCCESS;
	struct ca821x_exchange_base *priv  = pDeviceRef->exchange_context;

	pthread_mutex_lock(&priv->flag_mutex);
	priv->io_thread_runflag = 0;
	pthread_mutex_unlock(&priv->flag_mutex);

	pthread_join(priv->io_thread, NULL);

	flush_queue(&priv->in_buffer_queue, &priv->in_queue_mutex);
	flush_queue(&priv->out_buffer_queue, &priv->out_queue_mutex);

	pthread_mutex_destroy(&(priv->flag_mutex));
	pthread_mutex_destroy(&(priv->sync_mutex));
	pthread_mutex_destroy(&(priv->in_queue_mutex));
	pthread_mutex_destroy(&(priv->out_queue_mutex));
	pthread_cond_destroy(&(priv->sync_cond));

	priv->error_callback = NULL;

	error = deinit_generic_statics();

	return error;
}

static void init_generic_statics()
{
	generic_initialised++;
}

static ca_error deinit_generic_statics()
{
	ca_error error = CA_ERROR_SUCCESS;

	if (!(--generic_initialised))
		goto exit;

	error = ca821x_util_stop_downstream_dispatch_worker();

	flush_queue(&downstream_dispatch_queue, &downstream_queue_mutex);

exit:
	return error;
}

ca_error exchange_register_user_callback(exchange_user_callback callback, struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *priv = pDeviceRef->exchange_context;

	if (priv->user_callback)
		return CA_ERROR_FAIL;

	priv->user_callback = callback;

	return CA_ERROR_SUCCESS;
}

ca_error exchange_user_command(uint8_t cmdid, uint8_t cmdlen, uint8_t *payload, struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *priv  = pDeviceRef->exchange_context;
	ca_error                     error = CA_ERROR_SUCCESS;
	uint8_t                      buf[(size_t)cmdlen + 2];

	if (cmdid & SPI_SYN)
	{
		error = CA_ERROR_INVALID_ARGS;
		goto exit;
	}

	buf[0] = cmdid;
	buf[1] = cmdlen;
	memcpy(buf + 2, payload, cmdlen);

	add_to_queue(&(priv->out_buffer_queue), &(priv->out_queue_mutex), buf, cmdlen + 2, pDeviceRef);

	if (priv->signal_func)
		priv->signal_func(pDeviceRef);

exit:
	return error;
}

static void *ca821x_recovery_worker(void *arg)
{
	struct ca821x_dev *          pDeviceRef = arg;
	struct ca821x_exchange_base *priv       = pDeviceRef->exchange_context;

	pthread_mutex_lock(&priv->flag_mutex);
	priv->rescue_thread = pthread_self();
	pthread_mutex_unlock(&priv->flag_mutex);

	if (priv->error_callback)
	{
		priv->error_callback(priv->error, pDeviceRef);
	}
	else
	{
		abort();
	}

	pthread_mutex_lock(&priv->flag_mutex);
	priv->restoreflag = 0;
	pthread_cond_broadcast(&priv->restore_cond);
	pthread_mutex_unlock(&priv->flag_mutex);

	//Swap contents restore buffers back into queues:
	flush_queue(&priv->out_buffer_queue, &priv->out_queue_mutex);

	flush_queue(&priv->in_buffer_queue, &priv->in_queue_mutex);

	//Signal the sync queue just in case there is something waiting
	pthread_mutex_lock(&(priv->in_queue_mutex));
	pthread_cond_signal(&priv->sync_cond);
	pthread_mutex_unlock(&(priv->in_queue_mutex));

	return NULL;
}

ca_error exchange_handle_error(ca_error error, struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *priv = pDeviceRef->exchange_context;
	int                          rval = 0;

	priv->error = error;

	//Swap contents of queues into restore buffers:
	reseat_queue(
	    &priv->out_buffer_queue, &priv->restore_out_buffer_queue, &priv->out_queue_mutex, &priv->out_queue_mutex);

	reseat_queue(&priv->in_buffer_queue, &priv->restore_in_buffer_queue, &priv->in_queue_mutex, &priv->in_queue_mutex);

	pthread_mutex_lock(&priv->flag_mutex);
	if (priv->restoreflag)
		abort(); //Failed during recovery - give up
	else
		priv->restoreflag = 1;
	pthread_mutex_unlock(&priv->flag_mutex);

	if ((rval = pthread_mutex_trylock(&priv->sync_mutex)) == 0)
	{
		//No sync command, release
		pthread_mutex_unlock(&priv->sync_mutex);
	}
	else if (rval == EBUSY)
	{
		uint8_t buffer[] = {0};
		//Push a fake response to unblock sync waiter
		add_to_waiting_queue(
		    &(priv->in_buffer_queue), &(priv->in_queue_mutex), &(priv->sync_cond), buffer, 0, pDeviceRef);
	}

	pthread_create(&priv->rescue_thread, NULL, &ca821x_recovery_worker, pDeviceRef);

	return CA_ERROR_SUCCESS;
}

static inline ca_error ca821x_try_read(struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *priv = pDeviceRef->exchange_context;
	uint8_t                      buffer[MAX_BUF_SIZE];
	ssize_t                      len;

	len = priv->read_func(pDeviceRef, buffer);
	assert(len < MAX_BUF_SIZE);
	if (len > 0)
	{
		if (buffer[0] & SPI_SYN)
		{
			//Add to queue for synchronous processing
			add_to_waiting_queue(
			    &(priv->in_buffer_queue), &(priv->in_queue_mutex), &(priv->sync_cond), buffer, len, pDeviceRef);
		}
		else
		{
			//Add to queue for dispatching downstream
			add_to_waiting_queue(
			    &downstream_dispatch_queue, &downstream_queue_mutex, &dd_cond, buffer, len, pDeviceRef);
		}
		return CA_ERROR_SUCCESS;
	}
	else if (len < 0)
	{
		exchange_handle_error(CA_ERROR_FAIL, pDeviceRef);
	}
	return CA_ERROR_NOT_FOUND;
}

static inline ca_error ca821x_try_write(struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *priv = pDeviceRef->exchange_context;
	uint8_t                      buffer[MAX_BUF_SIZE];
	ssize_t                      len;

	if (priv->write_isready_func && priv->write_isready_func(pDeviceRef))
	{
		return CA_ERROR_BUSY;
	}

	len = pop_from_queue(&(priv->out_buffer_queue), &(priv->out_queue_mutex), buffer, MAX_BUF_SIZE, &pDeviceRef);

	if (len > 0)
	{
		ca_error error;

		error = priv->write_func(buffer, len, pDeviceRef);
		if (error)
		{
			exchange_handle_error(error, pDeviceRef);
			return CA_ERROR_FAIL;
		}
		return CA_ERROR_SUCCESS;
	}

	return CA_ERROR_NOT_FOUND;
}

void *ca821x_io_worker(void *arg)
{
	struct ca821x_dev *          pDeviceRef = arg;
	struct ca821x_exchange_base *priv       = pDeviceRef->exchange_context;

	priv->flush_func(pDeviceRef);

	pthread_mutex_lock(&priv->flag_mutex);
	while (priv->io_thread_runflag)
	{
		pthread_mutex_unlock(&priv->flag_mutex);

		if (ca821x_try_read(pDeviceRef) == CA_ERROR_NOT_FOUND)
		{
			//If no reads left, we can start writing.
			ca821x_try_write(pDeviceRef);
		}

		pthread_mutex_lock(&priv->flag_mutex);
	}

	pthread_mutex_unlock(&priv->flag_mutex);
	return 0;
}

ca_error ca821x_exchange_commands(const uint8_t *buf, size_t len, uint8_t *response, struct ca821x_dev *pDeviceRef)
{
	const uint8_t                isSynchronous = ((buf[0] & SPI_SYN) && response);
	struct ca821x_exchange_base *priv          = pDeviceRef->exchange_context;
	struct ca821x_dev *          ref_out;
	size_t                       success    = 0;
	uint8_t                      is_rescuer = 0;

	if (!generic_initialised)
		return CA_ERROR_INVALID_STATE;
	//Synchronous must execute synchronously
	//Get sync responses from the in queue
	//Send messages by adding them to the out queue

	//If in error state, wait until restored
	pthread_mutex_lock(&priv->flag_mutex);
	if (priv->restoreflag && pthread_equal(priv->rescue_thread, pthread_self()))
	{
		is_rescuer = 1;
	}
	while (priv->restoreflag && !is_rescuer)
	{
		pthread_cond_wait(&priv->restore_cond, &priv->flag_mutex);
	}
	pthread_mutex_unlock(&priv->flag_mutex);

	if (isSynchronous && !is_rescuer)
		pthread_mutex_lock(&(priv->sync_mutex));

	while (success == 0) //Retry loop
	{
		//If in error state, wait until restored
		pthread_mutex_lock(&priv->flag_mutex);
		while (priv->restoreflag && !is_rescuer)
		{
			pthread_cond_wait(&priv->restore_cond, &priv->flag_mutex);
		}
		pthread_mutex_unlock(&priv->flag_mutex);

		add_to_queue(&(priv->out_buffer_queue), &(priv->out_queue_mutex), buf, len, pDeviceRef);

		if (priv->signal_func)
			priv->signal_func(pDeviceRef);

		if (!isSynchronous)
			return CA_ERROR_SUCCESS;

		//A rval of zero here is an error packet notifying of a driver error during
		//sync command. The original command will be resent after recovery so sync
		//behaviour should be upheld.
		success = wait_on_queue(&(priv->in_buffer_queue), &(priv->in_queue_mutex), &(priv->sync_cond));

		pop_from_queue(
		    &(priv->in_buffer_queue), &(priv->in_queue_mutex), response, sizeof(struct MAC_Message), &ref_out);
	}

	assert(ref_out == pDeviceRef);
	if (!is_rescuer)
		pthread_mutex_unlock(&(priv->sync_mutex));

	return CA_ERROR_SUCCESS;
}

ca_error ca821x_api_downstream(const uint8_t *buf, size_t len, uint8_t *response, struct ca821x_dev *pDeviceRef)
{
	return ca821x_exchange_commands(buf, len, response, pDeviceRef);
}
