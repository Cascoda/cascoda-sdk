/**
 * @file   cascoda_wait.h
 * @brief  Helper 'wait' framework for blocking functions
 * @author Ciaran Woodward
 * @date   31/01/19
 *//*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "cascoda-bm/cascoda_types.h"
#include "mac_messages.h"

#ifndef CASCODA_WAIT_H
#define CASCODA_WAIT_H

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
 * \param aTargetCallback - Pointer to the callback function in the pDeviceRef callback struct that is to be waited upon
 * \param aTimeoutMs - Timeout in milliseconds to wait for message before giving up
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return Status  EVBME_SUCCESS, 0 - Success
 *                 EVBME_SPI_WAIT_TIMEOUT, 1 - Timed out waiting for message
 *
 */
int WAIT_Callback(union ca821x_api_callback *aTargetCallback,
                  int                        timeout,
                  void *                     aCallbackContext,
                  struct ca821x_dev *        pDeviceRef);

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

/**
 * \brief Legacy function for waiting - not recommended to use for any new purpose
 *
 * This function registers a callback which copies the command buffer into the
 * waiting buffer. Any old callback is swapped out then back in again while this
 * happens. This is to implement the legacy ca821x_wait_for_command function,
 * and isn't recommended for any real application.
 *
 * \deprecated Unsupported, inefficient & encourages bad design
 *
 * \return Status  EVBME_SUCCESS, 0 - Success
 *                 EVBME_SPI_WAIT_TIMEOUT, 1 - Timed out waiting for message
 *                 EVBME_FAIL, 1 - Failed
 */
int WAIT_Legacy(uint8_t cmdid, int timeout_ms, uint8_t *buf, struct ca821x_dev *pDeviceRef);

#endif //CASCODA_WAIT_H
