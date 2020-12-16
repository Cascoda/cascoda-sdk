/*
 * Copyright (c) 2020, Cascoda Ltd.
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
 * @brief Various types used by the cascoda posix api.
 */

#ifndef CA821X_TYPES_H_
#define CA821X_TYPES_H_

#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix-config.h"
#include "ca821x-posix/ca821x-posix-evbme.h"
#include "ca821x_api.h"
#include "ca821x_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Error callback
 *
 * Optional callback for the application layer
 * to handle any exchange errors which would otherwise
 * cause a crash.
 *
 * \returns Cascoda error code
 */
typedef ca_error (*ca821x_errorhandler)(ca_error error, struct ca821x_dev *pDeviceRef);

/**
 * @brief Optional Exchange User Callback
 *
 * Optional callback for the application layer
 * to handle any non-ca821x communication with
 * a device over the same protocol. Any
 * command IDs which are not recognised as
 * a valid ca821x SPI command will be passed
 * to this callback.
 *
 * \returns Cascoda error code
 */
typedef ca_error (*exchange_user_callback)(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/**
 *  \brief Exchange write function
 *
 * Function for the exchange to implement. The implementation should
 * send the data in 'buf' to the ca821x.
 *
 * \param buf buffer containing the message to send to the ca821x
 * \param len length of the buffer data
 * \param pDeviceRef a Pointer to the relevant pDeviceRef struct
 *
 * \returns Cascoda error code
 */
typedef ca_error (*exchange_write)(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/**
 * \brief Exchange write isready function
 *
 * Function for the exchange to implement. The implementation should
 * return true (nonzero) if it is ready to send, or 0 if not.
 *
 * \param pDeviceRef a Pointer to the relevant pDeviceRef struct
 *
 * \returns Status
 * \retval CA_ERROR_SUCCESS The exchange is ready to send a packet
 * \retval CA_ERROR_BUSY The exhange is not ready to send a packet
 */
typedef ca_error (*exchange_write_isready)(struct ca821x_dev *pDeviceRef);

/**
 * \brief Exchange read function
 *
 * Function for the exchange to implement. The implementation should
 * read from the ca821x and copy the data into buf. This function
 * should block as necessary to reduce CPU load (although returning
 * a length of zero is still functionally correct).
 *
 * If the function does block, it must be able to be woken by a call
 * to the associated exchange_signal_read implementation.
 *
 * \param buf buffer containing the message read from the ca821x
 * \param pDeviceRef a Pointer to the relevant pDeviceRef struct
 *
 * \returns the length of the returned data
 */
typedef ssize_t (*exchange_read)(struct ca821x_dev *pDeviceRef, uint8_t *buf);

/**
 *  \brief Exchange signalling function to trigger read return
 *
 * Function for the exchange to implement. The implementation should
 * cause the associated exchange_read function to stop blocking and
 * return.
 *
 * \param pDeviceRef a Pointer to the relevant pDeviceRef struct
 *
 */
typedef void (*exchange_signal_read)(struct ca821x_dev *pDeviceRef);

/**
 *  \brief Exchange signalling function to flush external buffers
 *
 * Function for the exchange to implement. The implementation should
 * clear all external buffers, discarding any messages that were left
 * behind from past executions.
 *
 * This function will be called when the device is opened.
 *
 * \param pDeviceRef a Pointer to the relevant pDeviceRef struct
 *
 */
typedef void (*exchange_flush_unread)(struct ca821x_dev *pDeviceRef);

/** Enumeration for identifying the underlying exchange interface type */
enum ca821x_exchange_type
{
	ca821x_exchange_kernel = 1, //!< kernel driver's debugfs node
	ca821x_exchange_usb,        //!< USB HID device
	ca821x_exchange_uart,       //!< UART device
};

/** Single item in a singly-linked list of data buffers */
struct buffer_queue_item
{
	size_t                    len;        //!< Length of buffer
	uint8_t *                 buf;        //!< Buffer pointer
	struct ca821x_dev *       pDeviceRef; //!< Data's target/originating device
	struct buffer_queue_item *next;       //!< Next queue item
};

/** Queue struct for singly-linked list of buffer_queue_items */
struct buffer_queue
{
	struct buffer_queue_item *head;
	pthread_mutex_t           q_mutex;
	pthread_cond_t            q_cond;
};

/** Base structure for exchange private data collections */
struct ca821x_exchange_base
{
	enum ca821x_exchange_type exchange_type; //!< Which exchange is connected to this device

	ca821x_errorhandler    error_callback;     //!< Exchange error callback
	exchange_user_callback user_callback;      //!< User unhandled command callback
	exchange_write         write_func;         //!< Exchange write callback
	exchange_write_isready write_isready_func; //!< Exchange write isready callback
	exchange_signal_read   signal_func;        //!< Exchange write signalling callback
	exchange_read          read_func;          //!< Exchange read callback
	exchange_flush_unread  flush_func;         //!< Exchange flush callback

	//Synchronous queue
	pthread_t       io_thread;         //!< Thread for io handling
	int             io_thread_runflag; //!< flag to shutdown io thread
	pthread_mutex_t flag_mutex;        //!< mutex for generic flag handling
	pthread_cond_t  sync_cond;         //!< condition variable for synchronous exchanges
	pthread_mutex_t sync_mutex;        //!< mutex for synchronous exchanges

	//In queue = Device to host(us)
	//Out queue = Host(us) to device
	struct buffer_queue in_buffer_queue, out_buffer_queue; //!< queues

	struct EVBME_callbacks evbme_callbacks; //!< EVBME Callback struct
};

/**
 * Struct for getting info of connected devices (primarily for enumerating them)
 */
struct ca_device_info
{
	enum ca821x_exchange_type exchange_type; //!< Exchange type for this device
	const char *              path;          //!< Exchange & system specific 'path', unique to this device
	const char *              device_name;   //!< Name of the device, eg 'Chili2'
	const char *              app_name;      //!< Name of the application running on the device, eg 'ot-cli'
	const char *              version;       //!< Version string of the device
	const char *              serialno;      //!< Serial number of the device
	bool                      available; //!< Is the device available for use (or not, eg. in use by other application)
};

/**
 * Function type for handling enumerated devices when finding them.
 * @param aDeviceInfo The information about the found device
 * @param aContext    Generic context pointer requested when callback was provided
 */
typedef void (*util_device_found)(struct ca_device_info *aDeviceInfo, void *aContext);

#ifdef __cplusplus
}
#endif

#endif /* CA821X_TYPES_H_ */
