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
/**
 * @file
 * Queue system for the ca821x-posix exchanges to use for message buffering & sorting
 */

#ifndef CA821X_QUEUE_H
#define CA821X_QUEUE_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "ca821x-posix/ca821x-types.h"

/**
 * Add a buffer onto the end of a non-waiting queue
 * @param head_buffer_queue A pointer to the head of a queue
 * @param buf_queue_mutex The mutex protecting the queue
 * @param buf The buffer to queue
 * @param len The length in bytes of the buffer
 * @param pDeviceRef The pDeviceRef that the buffer is relevant to
 */
void add_to_queue(struct buffer_queue **head_buffer_queue,
                  pthread_mutex_t *     buf_queue_mutex,
                  const uint8_t *       buf,
                  size_t                len,
                  struct ca821x_dev *   pDeviceRef);

/**
 * Add a buffer onto the end of a waiting queue that may have something waiting on it
 * @param head_buffer_queue A pointer to the head of a queue
 * @param buf_queue_mutex The mutex protecting the queue
 * @param queue_cond The condition variable for the waiting queue
 * @param buf The buffer to queue
 * @param len The length in bytes of the buffer
 * @param pDeviceRef The pDeviceRef that the buffer is relevant to
 */
void add_to_waiting_queue(struct buffer_queue **head_buffer_queue,
                          pthread_mutex_t *     buf_queue_mutex,
                          pthread_cond_t *      queue_cond,
                          const uint8_t *       buf,
                          size_t                len,
                          struct ca821x_dev *   pDeviceRef);

/**
 * Empty a queue into nothing
 * @param head_buffer_queue A pointer to the head of a queue
 * @param buf_queue_mutex The mutex protecting the queue
 */
void flush_queue(struct buffer_queue **head_buffer_queue, pthread_mutex_t *buf_queue_mutex);

/**
 * Reseat one queue onto the end of another
 * @param head_buffer_queue A pointer to the head of the queue to be cleared
 * @param head_buffer_queue2 A pointer to the head of the queue to have the items added
 * @param buf_queue_mutex The mutex protecting the queue
 * @param buf_queue_mutex2 The mutex protecting the queue2
 */
void reseat_queue(struct buffer_queue **head_buffer_queue,
                  struct buffer_queue **head_buffer_queue2,
                  pthread_mutex_t *     buf_queue_mutex,
                  pthread_mutex_t *     buf_queue_mutex2);

/**
 * Pop a buffer off a queue
 * @param head_buffer_queue A pointer to the head of a queue
 * @param buf_queue_mutex The mutex protecting the queue
 * @param[out] destBuf A pointer to a buffer to accept the dequeued data
 * @param maxlen The max size of the destBuf
 * @param[out] pDeviceRef_out Output parameter to store the pDeviceRef of the buffer
 * @return The length of the popped buffer
 */
size_t pop_from_queue(struct buffer_queue **head_buffer_queue,
                      pthread_mutex_t *     buf_queue_mutex,
                      uint8_t *             destBuf,
                      size_t                maxlen,
                      struct ca821x_dev **  pDeviceRef_out);

/**
 * Non-blocking function returning the length of the next buffer on the queue (or 0 if nothing)
 * @param head_buffer_queue A pointer to the head of a queue
 * @param buf_queue_mutex The mutex protecting the queue
 * @return The length of the peeked buffer, or 0 if there is no buffer in queue
 */
size_t peek_queue(struct buffer_queue *head_buffer_queue, pthread_mutex_t *buf_queue_mutex);

/**
 * Wait on a waiting queue, blocking until there is something available
 * @param head_buffer_queue A pointer to the head of a queue
 * @param buf_queue_mutex The mutex protecting the queue
 * @param queue_cond The condition variable for the waiting queue
 * @return The length of the next item in the queue, or -1 on error
 */
size_t wait_on_queue(struct buffer_queue **head_buffer_queue,
                     pthread_mutex_t *     buf_queue_mutex,
                     pthread_cond_t *      queue_cond);

#endif
