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

/*
 * Must call ONE of the following functions in order to initialize driver communications
 *
 * Using uart_exchange_init will cause the program to crash if there is an error
 *
 * Using uart_exchange_init_withhandler and passing a callback function will cause
 * that callback function to execute in the case of an error. Passing a callback of NULL causes
 * the same behaviour as uart_exchange_init.
 */

/**
 * Initialise the uart exchange, with no callback for errors (program will
 * crash in the case of an error.
 *
 * @warning It is recommended to use the uart_exchange_init_withandler function
 * instead, so that any errors can be handled by your application.
 *
 * @retval CA_ERROR_SUCCESS Successful initialisation
 * @retval CA_ERROR_FAIL Could not initialise
 * @retval CA_ERROR_ALREADY The exchange was already initialised
 *
 */
ca_error uart_exchange_init(struct ca821x_dev *pDeviceRef);

/**
 * Initialise the uart exchange, using the supplied errorhandling callback to
 * report any errors back to the application, which can react as required
 * (i.e. crash gracefully or attempt to reset the ca8210)
 *
 * @param[in]  callback   Function pointer to an error-handling callback
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 * @retval CA_ERROR_SUCCESS for success
 * @retval CA_ERROR_NOT_FOUND for error
 * @retval CA_ERROR_ALREADY if the device was already initialised
 *
 */
ca_error uart_exchange_init_withhandler(ca821x_errorhandler callback, struct ca821x_dev *pDeviceRef);

/**
 * Deinitialise the uart exchange, so that it can be reinitialised by another
 * process, or reopened later.
 *
 */
void uart_exchange_deinit(struct ca821x_dev *pDeviceRef);

/**
 * Send a hard reset to the ca8210. This should not be necessary, but is provided
 * in case the ca8210 becomes unresponsive to spi.
 *
 * @param[in]  resettime   The length of time (in ms) to hold the reset pin
 *                         active for. 1ms is usually a suitable value for this.
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 */
int uart_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef);

#endif
