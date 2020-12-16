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
 * Add a buffer onto the end of a queue
 * @param buffer_queue A pointer to the queue
 * @param buf The buffer to queue
 * @param len The length in bytes of the buffer
 * @param pDeviceRef The pDeviceRef that the buffer is relevant to
 */
void add_to_queue(struct buffer_queue *buffer_queue, const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/**
 * Empty a queue into nothing
 * @param buffer_queue A pointer to the head of a queue
 */
void flush_queue(struct buffer_queue *buffer_queue);

/**
 * Pop a buffer off a queue
 * @param buffer_queue A pointer to the queue
 * @param[out] destBuf A pointer to a buffer to accept the dequeued data
 * @param maxlen The max size of the destBuf
 * @param[out] pDeviceRef_out Output parameter to store the pDeviceRef of the buffer
 * @return The length of the popped buffer
 */
size_t pop_from_queue(struct buffer_queue *buffer_queue,
                      uint8_t *            destBuf,
                      size_t               maxlen,
                      struct ca821x_dev ** pDeviceRef_out);

/**
 * Non-blocking function returning the length of the next buffer on the queue (or 0 if nothing)
 * @param buffer_queue A pointer to the queue
 * @return The length of the peeked buffer, or 0 if there is no buffer in queue
 */
size_t peek_queue(struct buffer_queue *buffer_queue);

/**
 * Wait on a queue, blocking until there is something available
 * @param buffer_queue A pointer to the queue
 * @param timeout_s  The wait timeout in seconds, or 0 for no timeout
 * @return The length of the next item in the queue, 0 on timeout, or -1 on error
 */
size_t wait_on_queue(struct buffer_queue *buffer_queue, time_t timeout_s);

/**
 * Wait on a queue, blocking until it is empty
 * @param buffer_queue A pointer to the queue
 * @param timeout_s  The wait timeout in seconds, or 0 for no timeout
 * @return CA_ERROR_SUCCESS upon success, CA_ERROR_TIMEOUT if operation timed out
 */
ca_error wait_on_queue_empty(struct buffer_queue *buffer_queue, time_t timeout_s);

#endif
