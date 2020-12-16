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
 * Cascoda posix exchange for communicating with ca821x via kernel driver.
 */

#ifndef KERNEL_EXCHANGE_H
#define KERNEL_EXCHANGE_H

#include "ca821x-posix/ca821x-types.h"
#include "ca821x_api.h"

/**
 * Initialise the kernel exchange, using the supplied errorhandling callback to
 * report any errors back to the application, which can react as required
 * (i.e. crash gracefully or attempt to reset the ca8210)
 *
 *
 * @param[in]  callback   Function pointer to an error-handling callback (can be NULL)
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 * @retval CA_ERROR_SUCCESS Successful initialisation
 * @retval CA_ERROR_NOT_FOUND Did not succeed
 *
 */
ca_error kernel_exchange_init(ca821x_errorhandler callback, struct ca821x_dev *pDeviceRef);

/**
 * Deinitialise the kernel exchange, so that it can be reinitialised by another
 * process, or reopened later.
 *
 * @param pDeviceRef Pointer to initialised ca821x_device_ref struct
 *
 */
void kernel_exchange_deinit(struct ca821x_dev *pDeviceRef);

/**
 * Send a hard reset to the ca8210. This should not be necessary, but is provided
 * in case the ca8210 becomes unresponsive to spi.
 *
 * @param[in]  resettime   The length of time (in ms) to hold the reset pin
 *                         active for. 1ms is usually a suitable value for this.
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 */
int kernel_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef);

/**
 * Function to enumerate all of the kernel devices configured (currently max single
 * device per system), calling aCallback with a struct describing each one. The
 * struct passed to aCallback will only be valid for the duration that the function
 * is called. This function will not return until every callback has been called.
 *
 * @param aCallback The callback to call with each result
 * @param aContext  The generic void pointer to provide to the callback when it is called
 * @retval CA_ERROR_SUCCESS   Enumeration successful
 * @retval CA_ERROR_NOT_FOUND No devices found
 */
ca_error kernel_exchange_enumerate(util_device_found aCallback, void *aContext);

#endif
