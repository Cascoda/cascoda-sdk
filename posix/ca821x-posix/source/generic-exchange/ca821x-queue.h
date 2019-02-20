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

#ifndef CA821X_QUEUE_H
#define CA821X_QUEUE_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "ca821x-posix/ca821x-types.h"

//Add a buffer onto the end of a non-waiting queue
void add_to_queue(struct buffer_queue **head_buffer_queue,
                  pthread_mutex_t *     buf_queue_mutex,
                  const uint8_t *       buf,
                  size_t                len,
                  struct ca821x_dev *   pDeviceRef);

//Add a buffer onto the end of a queue that may have something waiting on it
void add_to_waiting_queue(struct buffer_queue **head_buffer_queue,
                          pthread_mutex_t *     buf_queue_mutex,
                          pthread_cond_t *      queue_cond,
                          const uint8_t *       buf,
                          size_t                len,
                          struct ca821x_dev *   pDeviceRef);

//Empty a queue into nothing
void flush_queue(struct buffer_queue **head_buffer_queue, pthread_mutex_t *buf_queue_mutex);

//Reseat one queue onto the end of another
void reseat_queue(struct buffer_queue **head_buffer_queue,
                  struct buffer_queue **head_buffer_queue2,
                  pthread_mutex_t *     buf_queue_mutex,
                  pthread_mutex_t *     buf_queue_mutex2);

//Pop a buffer off a queue
size_t pop_from_queue(struct buffer_queue **head_buffer_queue,
                      pthread_mutex_t *     buf_queue_mutex,
                      uint8_t *             destBuf,
                      size_t                maxlen,
                      struct ca821x_dev **  pDeviceRef_out);

//Non-blocking function returning the length of the next buffer on the queue (or 0 if nothing)
size_t peek_queue(struct buffer_queue *head_buffer_queue, pthread_mutex_t *buf_queue_mutex);

//Wait on a queue, blocking until there is something available
size_t wait_on_queue(struct buffer_queue **head_buffer_queue,
                     pthread_mutex_t *     buf_queue_mutex,
                     pthread_cond_t *      queue_cond);

#endif
