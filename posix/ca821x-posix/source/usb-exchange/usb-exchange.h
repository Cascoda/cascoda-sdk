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
 * Cascoda posix exchange for communicating with ca821x via usb.
 */

#ifndef USB_EXCHANGE_H
#define USB_EXCHANGE_H

#include "ca821x-posix/ca821x-types.h"
#include "ca821x_api.h"
#define TEST_ENABLE 1

enum usb_exchange_errors
{
	usb_exchange_err_usb = 1, //!<Usb error - probably device removed and going to have to crash safely
	usb_exchange_err_ca821x,  //!< ca821x error - ca821x has been reset
	usb_exchange_err_generic
};

/**
 * Initialise the usb exchange
 *
 * If callback is specified, then it will be used to report any fatal errors back to the
 * application, which can react as required (i.e. crash gracefully or attempt to reset the ca821x)
 *
 * If path is specified, then the exchange will attempt to open that device, and fail if it cannot.
 * The path of a device can be obtained with usb_exchange_enumerate.
 * 
 * If serial_num is specified, then the exchange will run an additional check when iterating through
 * all devices, looking for one with matching serial number; Otherwise it will carry out the normal
 * iteration and try to open the first available device.
 *
 * @param[in]  callback    Function pointer to an error-handling callback (can be NULL)
 * @param[in]  path        String representing an exchange & system specific path to a device (can be NULL)
 * @param[in]  pDeviceRef  Pointer to initialised ca821x_device_ref struct
 * @param[in]  serial_num  Serial number of the target device (can be NULL)
 *
 * @retval CA_ERROR_SUCCESS  success
 * @retval CA_ERROR_NOT_FOUND No available devices found (or device at path not found)
 * @retval CA_ERROR_NO_ACCESS The specified path cannot be opened (e.g. no permissions, already in use)
 * @retval CA_ERROR_ALREADY The pDeviceRef is already initialised
 *
 */
ca_error usb_exchange_init(ca821x_errorhandler callback,
                           const char         *path,
                           struct ca821x_dev  *pDeviceRef,
                           char               *serial_num);

/**
 * Deinitialise the usb exchange, so that it can be reinitialised by another
 * process, or reopened later.
 *
 * @param pDeviceRef Pointer to initialised ca821x_device_ref struct
 *
 */
void usb_exchange_deinit(struct ca821x_dev *pDeviceRef);

/**
 * Send a hard reset to the ca821x. This should not be necessary, but is provided
 * in case the ca821x becomes unresponsive to spi.
 *
 * @param[in]  resettime   The length of time (in ms) to hold the reset pin
 *                         active for. 1ms is usually a suitable value for this.
 * @param[in]  pDeviceRef   Pointer to initialised ca821x_device_ref struct
 *
 */
int usb_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef);

/**
 * Function to enumerate all of the USB connected devices, calling aCallback with a struct
 * describing each one. The struct passed to aCallback will only be valid for the
 * duration that the function is called. This function will not return until every
 * callback has been called.
 *
 * @param aCallback The callback to call with each result
 * @param aContext  The generic void pointer to provide to the callback when it is called
 * @retval CA_ERROR_SUCCESS   Enumeration successful
 * @retval CA_ERROR_NOT_FOUND No devices found
 */
ca_error usb_exchange_enumerate(util_device_found aCallback, void *aContext);

#ifdef TEST_ENABLE
/**Run to test fragmentation. Crashes upon fail.*/
void test_frag_loopback();
#endif

#endif
