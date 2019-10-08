/*
 * ca821x-types.h
 *
 *  Created on: 11 Sep 2017
 *      Author: ciaran
 */

#ifndef CA821X_TYPES_H_
#define CA821X_TYPES_H_

#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix-config.h"
#include "ca821x_api.h"
#include "ca821x_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Error callback
 *
 * Optional callback for the application layer
 * to handle any chip errors which would otherwise
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
	pthread_mutex_t sync_mutex;        //!< condition variable for synchronous exchanges
	//In queue = Device to host(us)
	//Out queue = Host(us) to device
	pthread_mutex_t      in_queue_mutex, out_queue_mutex;    //!< queue mutexes
	struct buffer_queue *in_buffer_queue, *out_buffer_queue; //!< queues

	//Error handling
	ca_error             error;         //!< Error code
	int                  restoreflag;   //!< is currently recovering from error
	pthread_t            rescue_thread; //!< recovery thread
	pthread_cond_t       restore_cond;  //!< restoration condition variable
	struct buffer_queue *restore_in_buffer_queue,
	    *restore_out_buffer_queue; //!< working queues to properly restore comms
};

/** Single index in a singly-linked list of data buffers */
struct buffer_queue
{
	size_t               len;        //!< Length of buffer
	uint8_t *            buf;        //!< Buffer pointer
	struct ca821x_dev *  pDeviceRef; //!< Data's target/originating device
	struct buffer_queue *next;       //!< Next queue item
};

#ifdef __cplusplus
}
#endif

#endif /* CA821X_TYPES_H_ */
