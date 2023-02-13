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
/**
 * @file
 * @brief Queue manipulation for ca821x-posix data exchange
 */

#include <errno.h>
#include <string.h>
#include <time.h>

#include "ca821x-queue.h"

void add_to_queue(struct buffer_queue *buffer_queue, const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	if (pthread_mutex_lock(&buffer_queue->q_mutex) == 0)
	{
		struct buffer_queue_item *nextbuf = buffer_queue->head;
		if (nextbuf == NULL)
		{
			//queue empty -> start new queue
			buffer_queue->head = malloc(sizeof(struct buffer_queue_item));
			memset(buffer_queue->head, 0, sizeof(struct buffer_queue_item));
			nextbuf = buffer_queue->head;
		}
		else
		{
			while (nextbuf->next != NULL)
			{
				nextbuf = nextbuf->next;
			}
			//allocate new buffer cell
			nextbuf->next = malloc(sizeof(struct buffer_queue_item));
			memset(nextbuf->next, 0, sizeof(struct buffer_queue_item));
			nextbuf = nextbuf->next;
		}

		nextbuf->len = len;
		nextbuf->buf = malloc(len);
		memcpy(nextbuf->buf, buf, len);
		nextbuf->pDeviceRef = pDeviceRef;
		pthread_cond_broadcast(&buffer_queue->q_cond);
		pthread_mutex_unlock(&buffer_queue->q_mutex);
	}
}

void flush_queue(struct buffer_queue *buffer_queue)
{
	struct ca821x_dev *junkDev = NULL;

	pthread_mutex_lock(&buffer_queue->q_mutex);
	while (buffer_queue->head != NULL)
	{
		pthread_mutex_unlock(&buffer_queue->q_mutex);

		pop_from_queue(buffer_queue, NULL, 0, &junkDev);

		pthread_mutex_lock(&buffer_queue->q_mutex);
	}
	pthread_mutex_unlock(&buffer_queue->q_mutex);
}

size_t pop_from_queue(struct buffer_queue *buffer_queue,
                      uint8_t             *destBuf,
                      size_t               maxlen,
                      struct ca821x_dev  **pDeviceRef_out)
{
	if (pthread_mutex_lock(&buffer_queue->q_mutex) == 0)
	{
		struct buffer_queue_item *current = buffer_queue->head;
		size_t                    len     = 0;

		if (buffer_queue->head != NULL)
		{
			buffer_queue->head = current->next;
			len                = current->len;

			if (len > maxlen || !destBuf)
				len = 0; //Invalid
			else
				memcpy(destBuf, current->buf, len);

			*pDeviceRef_out = current->pDeviceRef;

			pthread_cond_broadcast(&buffer_queue->q_cond);
			free(current->buf);
			free(current);
		}

		pthread_mutex_unlock(&buffer_queue->q_mutex);
		return len;
	}
	return 0;
}

//return the length of the next buffer in the queue if it exists, otherwise 0
size_t peek_queue(struct buffer_queue *buffer_queue)
{
	size_t next_len = 0;

	if (pthread_mutex_lock(&buffer_queue->q_mutex) == 0)
	{
		if (buffer_queue->head != NULL)
		{
			next_len = buffer_queue->head->len;
		}
		pthread_mutex_unlock(&buffer_queue->q_mutex);
	}
	return next_len;
}

//return the length of the next buffer in the queue, blocking until
//it arrives. Returns length of buffer (or -1 upon error).
size_t wait_on_queue(struct buffer_queue *buffer_queue, time_t timeout_s)
{
	size_t          in_queue = -1;
	struct timespec ts;

	if (timeout_s)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout_s;
	}

	if (pthread_mutex_lock(&buffer_queue->q_mutex) == 0)
	{
		do
		{
			if (buffer_queue->head != NULL)
			{
				in_queue = buffer_queue->head->len;
			}
			else if (!timeout_s)
			{
				pthread_cond_wait(&buffer_queue->q_cond, &buffer_queue->q_mutex);
			}
			else
			{
				if (pthread_cond_timedwait(&buffer_queue->q_cond, &buffer_queue->q_mutex, &ts) == ETIMEDOUT)
					in_queue = 0;
			}
		} while (in_queue == ((size_t)-1));
		pthread_mutex_unlock(&buffer_queue->q_mutex);
	}
	return in_queue;
}

ca_error wait_on_queue_empty(struct buffer_queue *buffer_queue, time_t timeout_s)
{
	ca_error        error = CA_ERROR_FAIL;
	struct timespec ts;

	if (timeout_s)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout_s;
	}

	if (pthread_mutex_lock(&buffer_queue->q_mutex) == 0)
	{
		do
		{
			if (buffer_queue->head == NULL)
			{
				error = CA_ERROR_SUCCESS;
			}
			else if (!timeout_s)
			{
				pthread_cond_wait(&buffer_queue->q_cond, &buffer_queue->q_mutex);
			}
			else
			{
				if (pthread_cond_timedwait(&buffer_queue->q_cond, &buffer_queue->q_mutex, &ts) == ETIMEDOUT)
					error = CA_ERROR_TIMEOUT;
			}
		} while (error != CA_ERROR_SUCCESS && error != CA_ERROR_TIMEOUT);
		pthread_mutex_unlock(&buffer_queue->q_mutex);
	}
	return error;
}
