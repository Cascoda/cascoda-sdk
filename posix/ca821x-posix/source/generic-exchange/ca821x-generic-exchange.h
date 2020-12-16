/*
 *  Copyright (c) 2019, Cascoda Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file
 * The common part of every exchange, handling message management and calling the interface-specific implementations.
 */

#ifndef CA821X_GENERIC_EXCHANGE_H
#define CA821X_GENERIC_EXCHANGE_H

#include "ca821x-posix/ca821x-types.h"

#define MAX_BUF_SIZE 256

/**
 * Initialise the generic part of a pDeviceRef.
 * @param pDeviceRef An allocated and partially initialised pDeviceRef struct
 * @return CA_ERROR_SUCCESS upon success, error upon failure
 */
ca_error init_generic(struct ca821x_dev *pDeviceRef);

/**
 * Deinitialise an initialised pDeviceRef struct.
 * @param pDeviceRef an initialised pDeviceRef struct.
 * @return CA_ERROR_SUCCESS upon success, error upon failure
 */
ca_error deinit_generic(struct ca821x_dev *pDeviceRef);

/**
 * Attempt to recover from an exchange error silently.
 * @param error The error code that caused the crash
 * @param pDeviceRef The initialised pDeviceRef struct related to the error.
 * @return CA_ERROR_SUCCESS
 */
ca_error exchange_handle_error(ca_error error, struct ca821x_dev *pDeviceRef);

/**
 * io worker thread function. Handles reads/writes to the exchange, buffering and debuffering messages as required.
 * @param arg an initialised pDeviceRef struct.
 * @return 0
 */
void *ca821x_io_worker(void *arg);

/**
 * Handle an exchange with the ca821x. Used as the downstream function for ca821x-api.
 * @param buf The buffer to send
 * @param len The length of the buffer to send, including header bytes
 * @param response The buffer to use for the response message to synchronous messages
 * @param pDeviceRef an initialised pDeviceRef struct.
 * @return status
 * @retval CA_ERROR_SUCCESS Success
 * @retval CA_ERROR_INVALID_STATE Invalid state, such as uninitialised.
 * @retval CA_ERROR_TIMEOUT Response was not received to synchronous command in reasonable timeframe.
 */
ca_error ca821x_exchange_commands(const uint8_t *buf, size_t len, uint8_t *response, struct ca821x_dev *pDeviceRef);

#endif
