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
 * Cascoda posix exchange for communicating with ca821x via uart.
 */

#ifndef UART_EXCHANGE_H
#define UART_EXCHANGE_H

#include "ca821x-posix/ca821x-types.h"
#include "ca821x_api.h"
#define TEST_ENABLE 1

enum uart_exchange_errors
{
	uart_exchange_err_uart = 1, //uart error - probably device removed and going to have to crash safely
	uart_exchange_err_ca821x,   //ca821x error - ca821x has been reset
	uart_exchange_err_generic
};

#ifdef _WIN32
/**
 * Initialise the uart exchange, using the supplied errorhandling callback to
 * report any errors back to the application, which can react as required
 * (i.e. crash gracefully or attempt to reset the ca821x)
 * 
 * This Windows version of the function differs from the non-Windows version, in that instead
 * of taking a path as an argument, it takes a more flexible argument of type 
 * "union ca821x_util_init_extra_arg". This argument can be used to specify a serial number,
 * a com port number, or simply the --force flag (or it can be set to NULL).
 *
 * @param[in]  callback   Function pointer to an error-handling callback (can be NULL)
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 * @param[in]  arg Flexible argument
 *
 * @retval CA_ERROR_SUCCESS for success
 * @retval CA_ERROR_NOT_FOUND for error
 * @retval CA_ERROR_ALREADY if the device was already initialised
 *
 */
ca_error uart_exchange_init(ca821x_errorhandler              callback,
                            struct ca821x_dev               *pDeviceRef,
                            union ca821x_util_init_extra_arg arg);
#else
/**
 * Initialise the uart exchange, using the supplied errorhandling callback to
 * report any errors back to the application, which can react as required
 * (i.e. crash gracefully or attempt to reset the ca821x)
 *
 * Regarding the path argument, it can be manually selected, or obtained from
 * calling uart_exchange enumerate. The string is a colon-delimited list of
 * comma delimited 'device,baud' pairs. Unlike the USB version of this function,
 * the path argument can be used to specify devices that wouldn't otherwise
 * be available (i.e. they don't need to exist in the CASCODA_UART environment
 * variable).
 *
 * @param[in]  callback   Function pointer to an error-handling callback (can be NULL)
 * @param[in]  path        String representing possible UART devices and baud rates in the form "/dev/ttyS0,115200:/dev/ttyS1,9600:/dev/ttyS2,6000000"
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 * @retval CA_ERROR_SUCCESS for success
 * @retval CA_ERROR_NOT_FOUND for error
 * @retval CA_ERROR_ALREADY if the device was already initialised
 *
 */
ca_error uart_exchange_init(ca821x_errorhandler callback, const char *path, struct ca821x_dev *pDeviceRef);
#endif

/**
 * Deinitialise the uart exchange, so that it can be reinitialised by another
 * process, or reopened later.
 *
 *@param pDeviceRef Pointer to initialised ca821x_device_ref struct
 */
void uart_exchange_deinit(struct ca821x_dev *pDeviceRef);

/**
 * Function to enumerate all of the UART devices configured (in the CASCODA_UART
 * environment variable), calling aCallback with a struct describing each one. The
 * struct passed to aCallback will only be valid for the duration that the function
 * is called. This function will not return until every callback has been called.
 *
 * @param aCallback The callback to call with each result
 * @param aContext  The generic void pointer to provide to the callback when it is called
 * @retval CA_ERROR_SUCCESS   Enumeration successful
 * @retval CA_ERROR_NOT_FOUND No devices found
 */
ca_error uart_exchange_enumerate(util_device_found aCallback, void *aContext);

/**
 * Send a hard reset to the ca821x. This should not be necessary, but is provided
 * in case the ca821x becomes unresponsive to spi.
 *
 * @param[in]  resettime   The length of time (in ms) to hold the reset pin
 *                         active for. 1ms is usually a suitable value for this.
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 */
int uart_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef);

#endif
