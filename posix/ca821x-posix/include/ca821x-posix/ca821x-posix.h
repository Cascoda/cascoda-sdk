/*
 *  Copyright (c) 2020, Cascoda Ltd.
 *  All rights reserved.
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
 * @brief Utility functions for using the cascoda sdk on posix.
 */
/**
 * @ingroup posix
 * @defgroup posix-core Posix Core
 * @brief  Core functions for getting a connected CA-821x/Chili device and establishing a communications link
 *
 * @{
 */

#ifndef CA821X_POSIX_H
#define CA821X_POSIX_H 1

#include "ca821x-posix/ca821x-types.h"
#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generic function to initialise an available ca821x device. This includes
 * initialisation of the api and an exchange. Use of these generic functions
 * over using a specific exchange allows more flexibility.
 *
 * Calling twice on the same pDeviceRef without a deinit produces undefined
 * behaviour.
 *
 * Also see ca821x_util_init_path for initialising a specific device.
 *
 * @param[in]   pDeviceRef   Device reference to be initialised. Must point to
 *                           allocated memory, but does not have to be
 *                           initialised. The memory is cleared and initialised
 *                           internally.
 *
 * @param[in]   errorHandler A function pointer to an error handling function.
 *                           This callback will be triggered in the event of an
 *                           unrecoverable error.
 * 
 * @param[in]   serial_num   Serial number of the target device (can be NULL).
 *
 * @retval CA_ERROR_SUCCESS Device was initialised successfully
 * @retval CA_ERROR_ALREADY Device was already initialised
 * @retval CA_ERROR_NOT_FOUND Device could not be initialised
 *
 */
ca_error ca821x_util_init(struct ca821x_dev *pDeviceRef, ca821x_errorhandler errorHandler, char *serial_num);

/**
 * Generic function to initialise a specific device as found via ca821x_util_enumerate
 * or other mechanism. See ca821x_util_init for basic description, as this function
 * carries out the same purpose, but for a specific device.
 *
 * @param[in]   pDeviceRef   Device reference to be initialised. Must point to
 *                           allocated memory, but does not have to be
 *                           initialised. The memory is cleared and initialised
 *                           internally.
 *
 * @param[in]   errorHandler A function pointer to an error handling function.
 *                           This callback will be triggered in the event of an
 *                           unrecoverable error.
 *
 * @param exchangeType       The exchange type (eg usb, uart, kernel) to be used.
 *                           Obtained from ca821x_util_enumerate.
 *
 * @param path               The exchange-specific path to the desired device.
 *                           Obtained from ca821x_util_enumerate.
 *
 * @retval CA_ERROR_SUCCESS  Device was initialised successfully
 * @retval CA_ERROR_ALREADY  Device was already initialised
 * @retval CA_ERROR_NOT_FOUND  Device could not be initialised
 * @retval CA_ERROR_INVALID_ARGS  Exchange type not recognised
 */
ca_error ca821x_util_init_path(struct ca821x_dev *       pDeviceRef,
                               ca821x_errorhandler       errorHandler,
                               enum ca821x_exchange_type exchangeType,
                               const char *              path);

/**
 * Generic function to deinitialise an initialised ca821x device. This will
 * free any resources that were allocated by ca821x_util_init.
 *
 * Calling on an uninitialised pDeviceRef produces undefined behaviour.
 *
 * @param[in]   pDeviceRef   Device reference to be deinitialised.
 *
 */
void ca821x_util_deinit(struct ca821x_dev *pDeviceRef);

/**
 * Function to enumerate all of the connected devices, calling aCallback with a struct
 * describing each one. The struct passed to aCallback will only be valid for the
 * duration that the function is called. This function will not return until every
 * callback has been called.
 *
 * @param aCallback The callback to call with each result
 * @param aContext  The generic void pointer to provide to the callback when it is called
 * @retval CA_ERROR_SUCCESS   Enumeration successful
 * @retval CA_ERROR_NOT_FOUND No devices found
 */
ca_error ca821x_util_enumerate(util_device_found aCallback, void *aContext);

/**
 * Generic function to attempt a hard reset of the ca821x chip.
 *
 * Calling on an uninitialised pDeviceRef produces undefined behaviour.
 *
 * @param[in]   pDeviceRef   Device reference for device to be reset.
 *
 * @retval CA_ERROR_SUCCESS The device has been reset successfully
 * @retval CA_ERROR_FAIL The device could not be reset
 *
 */
ca_error ca821x_util_reset(struct ca821x_dev *pDeviceRef);

/**
 * Generic function to poll the receive queue and call callbacks for received
 * commands. This function should only be used if ca821x_util_start_upstream_dispatch_worker
 * has not been called. If ca821x_util_start_upstream_dispatch_worker has been
 * called, the callbacks will be called immediately and asynchronously from another thread.
 *
 * It is recommended that if the return value is CA_ERROR_SUCCESS, this function should be
 * called again.
 *
 * @returns status
 * @retval CA_ERROR_SUCCESS A message was pending and has been processed
 * @retval CA_ERROR_NOT_FOUND No messages pending dispatch
 * @retval CA_ERROR_INVALID_STATE This function cannot be used as the exchange is in
 *                                asynchronous mode - all dispatch callbacks will be
 *                                called from a separate thread.
 *
 */
ca_error ca821x_util_dispatch_poll();

/**
 * Start the upstream_dispatch worker, which asynchronously calls the message callbacks
 * (such as MCPS_DATA_indication) as they are received. These callbacks will be triggered
 * from a separate posix thread. Appropriate pthread locking should be used by the application
 * or else the application logic could suffer from threading related issues.
 *
 * This should not be used in conjunction with ca821x_util_dispatch_poll. Use
 * ca821x_util_dispatch_poll if you don't want to deal with pthreads, at the expense of
 * having to poll a function in the application.
 *
 * @returns status
 * @retval CA_ERROR_ALREADY upstream dispatch worker already running.
 * @retval CA_ERROR_FAIL Failed to start thread
 * @retval CA_ERROR_SUCCESS Success
 */
ca_error ca821x_util_start_upstream_dispatch_worker();

/**
 * Stop the upstream_dispatch worker, so callbacks will no longer be triggered
 * from a separate thread.
 *
 * @returns status
 * @retval CA_ERROR_ALREADY upstream dispatch worker already stopped.
 * @retval CA_ERROR_FAIL Failed to stop thread
 * @retval CA_ERROR_SUCCESS Success
 */
ca_error ca821x_util_stop_upstream_dispatch_worker();

/**
 * Registers the callback to call for any non-ca821x commands that are sent over
 * the interface. Commands are still limited to the cascoda tlv format, and must
 * use a command ID that is not currently used by the ca821x-spi protocol.
 *
 * @param[in]  callback   Function pointer to an user-command-handling callback
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 * @retval CA_ERROR_SUCCESS Callback successfully registered
 * @retval CA_ERROR_FAIL Could not register callback
 *
 */
ca_error exchange_register_user_callback(exchange_user_callback callback, struct ca821x_dev *pDeviceRef);

/**
 * Query whether the given exchange has any messages pending being sent in its send queue.
 * @param timeout_s timeout in seconds, or 0 for nonblocking mode
 * @param pDeviceRef Device reference to check the exchange of.
 * @retval CA_ERROR_SUCCESS All pending transmissions to the device are complete
 * @retval CA_ERROR_BUSY    All pending transmissions are not complete (returned due to timeout or non-blocking)
 */
ca_error exchange_wait_send_complete(time_t timeout_s, struct ca821x_dev *pDeviceRef);

/**
 * Sends a user-defined command over the connected interface. This is not useful for direct CA-821x
 * communication, as the api commands handle this. This can be used to implement custom commands
 * on the chili modules, when communicating over USB or UART, for example.
 *
 * Synchronous commands are not supported using this mechanism.
 *
 * @param[in]  cmdid   Command ID to be used by command
 * @param[in]  cmdlen  Length of the payload
 * @param[in]  payload  Pointer to a buffer containing the payload data with length cmdlen
 * @param[in]  pDeviceRef  The device reference to communicate with
 *
 * @returns ca_error
 *
 */
ca_error exchange_user_command(uint8_t cmdid, uint8_t cmdlen, uint8_t *payload, struct ca821x_dev *pDeviceRef);

#ifdef __cplusplus
}
#endif

#endif //CA821X_POSIX_H

/**
 * @}
 */
