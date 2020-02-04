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
 * @brief  Helper 'wait' framework for blocking functions
 */

#ifndef CASCODA_WAIT_H
#define CASCODA_WAIT_H

#include "ca821x_api.h"
#include "mac_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Wait for an asynchronous callback to be triggered
 *
 * This is a helper function which blocks until a specific SPI indication/confirm
 * message has been received from the CA821x. Think carefully before using this
 * function, as overuse can lead to programs which are inflexible in their flow.
 * It has been designed specifically for use with asynchronous messages, and can
 * correctly handle other commands received out of order. This function preserves
 * message order, so every callback will be triggered in order, regardless of whether
 * it is the one being waited on or not.
 *
 * This function is strictly not re-entrant.
 * It will fail if you try to call it while it is already waiting.
 * Do not use from an IRQ context.
 *
 * \param aCommandId - Asynchronous, incoming command ID that is to be waited upon
 * \param aTimeoutMs - Timeout in milliseconds to wait for message before giving up
 * \param aCallbackContext - Generic pointer that can be retreived in the callback using WAIT_GetContext()
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return Status
 * \retval CA_ERROR_SUCCESS Success
 * \retval CA_ERROR_INVALID_STATE System in invalid state - did you try and use from a callback?
 * \retval CA_ERROR_SPI_WAIT_TIMEOUT Timed out waiting for message
 *
 */
ca_error WAIT_Callback(uint8_t aCommandId, int aTimeoutMs, void *aCallbackContext, struct ca821x_dev *pDeviceRef);

/**
 * \brief Wait for an asynchronous callback to be triggered, with a given callback swapped in
 *
 * This is a helper function which blocks until a specific SPI indication/confirm
 * message has been received from the CA821x. Think carefully before using this
 * function, as overuse can lead to programs which are inflexible in their flow.
 * It has been designed specifically for use with asynchronous messages, and can
 * correctly handle other commands received out of order. This function preserves
 * message order, so every callback will be triggered in order, regardless of whether
 * it is the one being waited on or not.
 *
 * The callback should return a value following the normal callback rules defined in
 * ca821x_api.c (return 1 to consume the callback, return 0 to not consume, return a
 * negative number for error).
 *
 * This function is strictly not re-entrant.
 * It will fail if you try to call it while it is already waiting.
 * Do not use from an IRQ context.
 *
 * \param aCommandId - The commandId of the Asynchronous upstream command to be captured
 * \param aCallback  - A function pointer to the callback to be swapped in during the wait
 * \param aTimeoutMs - Timeout in milliseconds to wait for message before giving up
 * \param aCallbackContext - Provide a pointer to a context, which can be retrieved in the callback using WAIT_GetContext.
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return Status
 * \retval CA_ERROR_SUCCESS Success
 * \retval CA_ERROR_INVALID_STATE System in invalid state - did you try and use from a callback?
 * \retval CA_ERROR_SPI_WAIT_TIMEOUT Timed out waiting for message
 *
 */
ca_error WAIT_CallbackSwap(uint8_t                 aCommandId,
                           ca821x_generic_callback aCallback,
                           int                     aTimeoutMs,
                           void *                  aCallbackContext,
                           struct ca821x_dev *     pDeviceRef);

/**
 * \brief Get the callback context from within a callback being waited for
 *
 * This is a helper function for \ref WAIT_Callback, which allows the callback function
 * being waited for to retrieve a single pointer. It is only valid for use inside the
 * relevent callback function, and will otherwise return NULL.
 *
 * \return Pointer to context or NULL
 *
 */
void *WAIT_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif //CASCODA_WAIT_H
