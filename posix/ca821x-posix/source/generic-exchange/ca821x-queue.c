/**
 * @file
 * @brief Queue manipulation for ca821x-posix data exchange
 */
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

#include <string.h>

#include "ca821x-queue.h"

void add_to_queue(struct buffer_queue **head_buffer_queue,
                  pthread_mutex_t *     buf_queue_mutex,
                  const uint8_t *       buf,
                  size_t                len,
                  struct ca821x_dev *   pDeviceRef)
{
	add_to_waiting_queue(head_buffer_queue, buf_queue_mutex, NULL, buf, len, pDeviceRef);
}

void add_to_waiting_queue(struct buffer_queue **head_buffer_queue,
                          pthread_mutex_t *     buf_queue_mutex,
                          pthread_cond_t *      queue_cond,
                          const uint8_t *       buf,
                          size_t                len,
                          struct ca821x_dev *   pDeviceRef)
{
	if (pthread_mutex_lock(buf_queue_mutex) == 0)
	{
		struct buffer_queue *nextbuf = *head_buffer_queue;
		if (nextbuf == NULL)
		{
			//queue empty -> start new queue
			*head_buffer_queue = malloc(sizeof(struct buffer_queue));
			memset(*head_buffer_queue, 0, sizeof(struct buffer_queue));
			nextbuf = *head_buffer_queue;
		}
		else
		{
			while (nextbuf->next != NULL)
			{
				nextbuf = nextbuf->next;
			}
			//allocate new buffer cell
			nextbuf->next = malloc(sizeof(struct buffer_queue));
			memset(nextbuf->next, 0, sizeof(struct buffer_queue));
			nextbuf = nextbuf->next;
		}

		nextbuf->len = len;
		nextbuf->buf = malloc(len);
		memcpy(nextbuf->buf, buf, len);
		nextbuf->pDeviceRef = pDeviceRef;
		if (queue_cond)
			pthread_cond_broadcast(queue_cond);
		pthread_mutex_unlock(buf_queue_mutex);
	}
}

void flush_queue(struct buffer_queue **head_buffer_queue, pthread_mutex_t *buf_queue_mutex)
{
	struct ca821x_dev *junkDev = NULL;
	uint8_t *          junk    = NULL;

	pthread_mutex_lock(buf_queue_mutex);
	while (*head_buffer_queue != NULL)
	{
		pthread_mutex_unlock(buf_queue_mutex);

		pop_from_queue(head_buffer_queue, buf_queue_mutex, junk, 0, &junkDev);

		pthread_mutex_lock(buf_queue_mutex);
	}
	pthread_mutex_unlock(buf_queue_mutex);
}

void reseat_queue(struct buffer_queue **head_buffer_queue,
                  struct buffer_queue **head_buffer_queue2,
                  pthread_mutex_t *     buf_queue_mutex,
                  pthread_mutex_t *     buf_queue_mutex2)
{
	struct buffer_queue *tomove;
	struct buffer_queue *endbuf;

	pthread_mutex_lock(buf_queue_mutex);
	tomove             = *head_buffer_queue;
	*head_buffer_queue = NULL;
	pthread_mutex_unlock(buf_queue_mutex);

	pthread_mutex_lock(buf_queue_mutex2);
	endbuf = *head_buffer_queue2;
	if (endbuf == NULL)
	{
		//queue empty -> simple move
		*head_buffer_queue2 = tomove;
	}
	else
	{
		while (endbuf->next != NULL)
		{
			endbuf = endbuf->next;
		}
		//add on to end
		endbuf->next = tomove;
	}
	pthread_mutex_unlock(buf_queue_mutex2);
}

size_t pop_from_queue(struct buffer_queue **head_buffer_queue,
                      pthread_mutex_t *     buf_queue_mutex,
                      uint8_t *             destBuf,
                      size_t                maxlen,
                      struct ca821x_dev **  pDeviceRef_out)
{
	if (pthread_mutex_lock(buf_queue_mutex) == 0)
	{
		struct buffer_queue *current = *head_buffer_queue;
		size_t               len     = 0;

		if (*head_buffer_queue != NULL)
		{
			*head_buffer_queue = current->next;
			len                = current->len;

			if (len > maxlen)
				len = 0; //Invalid

			memcpy(destBuf, current->buf, len);
			*pDeviceRef_out = current->pDeviceRef;

			free(current->buf);
			free(current);
		}

		pthread_mutex_unlock(buf_queue_mutex);
		return len;
	}
	return 0;
}

//return the length of the next buffer in the queue if it exists, otherwise 0
size_t peek_queue(struct buffer_queue *head_buffer_queue, pthread_mutex_t *buf_queue_mutex)
{
	size_t in_queue = 0;

	if (pthread_mutex_lock(buf_queue_mutex) == 0)
	{
		if (head_buffer_queue != NULL)
		{
			in_queue = head_buffer_queue->len;
		}
		pthread_mutex_unlock(buf_queue_mutex);
	}
	return in_queue;
}

//return the length of the next buffer in the queue, blocking until
//it arrives. Returns length of buffer (or -1 upon error).
size_t wait_on_queue(struct buffer_queue **head_buffer_queue,
                     pthread_mutex_t *     buf_queue_mutex,
                     pthread_cond_t *      queue_cond)
{
	size_t in_queue = -1;

	if (pthread_mutex_lock(buf_queue_mutex) == 0)
	{
		do
		{
			if (*head_buffer_queue != NULL)
			{
				in_queue = (*head_buffer_queue)->len;
			}
			else
			{
				pthread_cond_wait(queue_cond, buf_queue_mutex);
			}
		} while (in_queue == ((size_t)-1));
		pthread_mutex_unlock(buf_queue_mutex);
	}
	return in_queue;
}
