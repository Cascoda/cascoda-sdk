/*
 * Copyright (c) 2023, Cascoda
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

#include "ca821x-generic-exchange.h"
#include "ca821x-posix-util-internal.h"
#include "uart-exchange.h"

#include <Windows.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

#define CASCODA_UART_BAUDRATE 115200    // serial port baud rate
#define SMALLEST_ALLOWED_PORT_NUMBER 2  // COM1 is reserved
#define LARGEST_ALLOWED_PORT_NUMBER 255 // Windows allows for 256 serial ports

// ================
// TYPE DEFINITIONS
// ================

/** Private data for the UART exchange */
struct uart_exchange_info
{
	struct ca821x_exchange_base base;                 //!< Exchange base structure
	HANDLE                      port_handle;          //!< Serial port handle
	bool                        tx_stalled;           //!< True if transmissions are stalled waiting for ack
	uint8_t                     rx_buf[MAX_BUF_SIZE]; //!< Buffer for buffering read data before full packet is received
	uint8_t                     tx_buf[MAX_BUF_SIZE]; //!< Buffer for buffering tx data (in case retransmit is required)
	size_t                      offset;               //!< Current offset for reading into buf
	struct timespec             prev_send_time;       //!< Time that previous message was sent (for timing out ack)
	struct timespec             rx_start_time; //!< Time that current message receive started (for timing out receive)
	HANDLE     events[2];    //!< Event handles for signalling and allowing WaitForMultipleObjects() to unblock
	OVERLAPPED port_overlap; //!< Overlapped struct to pass in ReadFile function for Overlapped I/O
};

/**
 * Structure representing a node in a linked list, used to store the port number
 * of every detected serial device. A linked list is used (as opposed to an array)
 * because it can scale accordingly depending on the number of detected device,
 * which is not known in advance.
 */
struct uart_dev_ll_node
{
	struct uart_dev_ll_node *next;     //!< Pointer to the next node
	int                      port_num; //!< Port number of a connected serial device
};

/** The two types of events that are watched */
enum events_array_index
{
	UNBLOCKING_READ_EVENT = 0,
	SERIAL_ACTIVITY_EVENT,
	NUM_EVENTS,
};

/** Different ways a serial read or write operation can occur */
enum serial_op_completion_info
{
	OPERATION_PERFORMED_SYNCHRONOUSLY = 0,
	OPERATION_PERFORMED_ASYNCHRONOUSLY,
	OPERATION_FAILED,
};

// =========
// CONSTANTS
// =========

//! Start of message
static const uint8_t UART_SOM = 0xDE;
//! Timeout for waiting for ack = 1 second
static const struct timespec ACK_TIMEOUT = {1, 0};
//! Timeout for waiting for full message = 0.2 seconds
static const struct timespec RX_TIMEOUT = {0, 200000000ULL};
//! Timeout for wait_for_event call = 0.5 seconds
static const struct timespec BLOCKING_WAIT_TIMEOUT = {0, 500000000ULL};

// =============
// PROGRAM STATE
// =============

//! Port number explicitly set (optional) via argument to uart_exchange_init().
static int s_com_port_number = 0;
//! Whether the serial device with the lowest port number will be automatically grabbed.
static bool s_grab_first_port = false;
//! Mutex for preventing race conditions due to unwanted concurrent code execution.
static pthread_mutex_t s_devs_mutex = PTHREAD_MUTEX_INITIALIZER;
//! Number of serial devices currently in use.
static int s_devcount = 0;
//! Whether the program state has already been initialised or not.
static bool s_initialised = 0;
//! Pointer to the head node of the linked list which stores the port number of serial devices.
static struct uart_dev_ll_node *s_uart_dev_ll_head = NULL;

// ==================
// INTERNAL FUNCTIONS
// ==================

/**
 * Assert that the device exchange type is UART. Aborts if it is not.
 * @param pDeviceRef Represents a ca821x device
 */
static void assert_uart_exchange(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_info *info = pDeviceRef->exchange_context;

	if (info->base.exchange_type != ca821x_exchange_uart)
		abort();
}

/**
 * Get the amount of time that has passed since the last transmission.
 * @param pInfo uart exchange state
 * @return the time that has passed since last UART transmission.
 */
static struct timespec get_tx_time_passed(const struct uart_exchange_info *pInfo)
{
	struct timespec curTime;
	if (!clock_gettime(CLOCK_REALTIME, &curTime))
		curTime = time_sub(&curTime, &pInfo->prev_send_time);
	else
		memset(&curTime, 0, sizeof(curTime));
	return curTime;
}

/**
 * Get the amount of time that has passed since the last receive started.
 * @param pInfo uart exchange state
 * @return the time that has passed since last UART receive begun.
 */
static struct timespec get_rx_time_passed(const struct uart_exchange_info *pInfo)
{
	struct timespec curTime;
	if (!clock_gettime(CLOCK_REALTIME, &curTime))
		curTime = time_sub(&curTime, &pInfo->rx_start_time);
	else
		memset(&curTime, 0, sizeof(curTime));
	return curTime;
}

/**
 * Insert a new node at the end of the linked list which stores port numbers of 
 * serial devices.
 * @param port_num The port number stored by that new node which is being inserted.
 */
static void uart_dev_ll_insert(int port_num)
{
	// Create a new node and populate its elements
	struct uart_dev_ll_node *new_node = malloc(sizeof(struct uart_dev_ll_node));
	new_node->port_num                = port_num;
	new_node->next                    = NULL;

	// If this is the first node, make the head point to it.
	if (s_uart_dev_ll_head == NULL)
	{
		s_uart_dev_ll_head = new_node;
	}
	else // Otherwise, find the tail node and add the new node after it
	{
		struct uart_dev_ll_node *tail_node = s_uart_dev_ll_head;
		while (tail_node->next != NULL) tail_node = tail_node->next;
		tail_node->next = new_node;
	}
}

/**
 * Detects all connected serial devices, and adds them to the linked list which
 * stores the port numbers of all connected devices.
 * 
 * @return ca_error indicating whether at least one serial device was detected.
 * @retval CA_ERROR_SUCCESS - at least one device was detected
 * @retval CA_ERROR_NOT_FOUND - no devices were detected
 */
static ca_error fill_uart_dev_ll(void)
{
	bool found = false;

	for (int i = SMALLEST_ALLOWED_PORT_NUMBER; i <= LARGEST_ALLOWED_PORT_NUMBER; ++i)
	{
		char port_path[20] = "\\\\.\\COM";
		char port_number[4];
		sprintf(port_number, "%d", i);
		strcat(port_path, port_number);

		HANDLE Port = CreateFile(port_path,                    // Name of the Port to be Opened
		                         GENERIC_READ | GENERIC_WRITE, // Read/Write Access
		                         0,                            // No Sharing, ports cant be shared
		                         NULL,                         // No Security
		                         OPEN_EXISTING,                // Open existing port only
		                         FILE_FLAG_OVERLAPPED,         // Overlapped I/O
		                         NULL);                        // Null for Comm Devices)

		if (Port != INVALID_HANDLE_VALUE)
		{
			uart_dev_ll_insert(i);
			found = true;
		}

		// NOTE: The ports here are only opened to check if something is connected to them.
		// They are then closed again immediately, because they will actually be opened
		// for use in the function serial_open_port(). If they are not closed here,
		// this will result in an error when serial_open_port() tries to open them.
		CloseHandle(Port);
	}

	if (found)
		return CA_ERROR_SUCCESS;

	return CA_ERROR_NOT_FOUND;
}

/**
 * Frees all the memory currently allocated for the linked list, and resets the
 * head pointer.
 */
static void free_uart_dev_ll(void)
{
	struct uart_dev_ll_node *cur_node = s_uart_dev_ll_head;
	struct uart_dev_ll_node *tmp;

	s_uart_dev_ll_head = NULL;

	while (cur_node != NULL)
	{
		tmp      = cur_node;
		cur_node = cur_node->next;
		free(tmp);
	}
}

/**
 * Lists the COM ports of all the connected serial devices which were detected
 * and added to the linked list. 
 */
static void list_uart_dev_ll(void)
{
	printf("Available COM ports:\n\t");
	struct uart_dev_ll_node *cur_node = s_uart_dev_ll_head;

	while (cur_node != NULL)
	{
		printf("COM%d ", cur_node->port_num);
		cur_node = cur_node->next;
	}
	printf("\n");
}

/**
 * Prints a helpful message explaining the different correct ways in which serial-adapter
 * can be called, in case the user did so incorrectly.
 */
static void print_help(void)
{
	ca_log_warn("Please specify which of the above listed COM ports you would like serial-adapter to connect to.");
	ca_log_warn(
	    "You can do this by specifying the port number as an argument to serial-adapter. E.g., to connect to COM8, type:");
	ca_log_warn("serial-adapter 8");
	ca_log_warn(
	    "Alternatively, pass the flag --force to serial-adapter and it will connect to the first COM port listed:");
	ca_log_warn("serial-adapter --force");
}

/**
 * Helper function to interpret the argument and determine what to do
 * based on its value.
 * 
 * @param arg Argument being interpreted.
 * @return ca_error Status indicating whether the argument was invalid or not.
 * @retval CA_ERROR_INVALID_ARGS Argument is invalid
 * @retval CA_ERROR_SUCCESS Argument was valid, and has been handled.
 */
static ca_error handle_extra_arg(union ca821x_util_init_extra_arg arg)
{
	if (arg.generic == NULL)
	{
		ca_log_warn("=== ERROR: COM port not specified ===");
		print_help();
		return CA_ERROR_INVALID_ARGS;
	}

	if (strcmp(arg.force, "--force") == 0)
	{
		s_grab_first_port = true;
		return CA_ERROR_SUCCESS;
	}

	if ((s_com_port_number = atoi(arg.com_port_num)))
	{
		s_grab_first_port = false;
		return CA_ERROR_SUCCESS;
	}

	ca_log_warn("=== ERROR: Invalid argument");
	print_help();
	return CA_ERROR_INVALID_ARGS;
}

/**
 * Initialises program state, and creates a linked list of all
 * connected serial devices which were detected.
 * 
 * @return ca_error Status of the function.
 */
static ca_error init_statics(void)
{
	ca_error error = CA_ERROR_SUCCESS;

	if (s_initialised)
		return CA_ERROR_SUCCESS;

	s_initialised = 1;

	// Creates a linked list of all available serial ports
	error = fill_uart_dev_ll();
	if (!error)
		list_uart_dev_ll();

	return error;
}

/**
 * Deinitialises program state, and frees the linked list.
 */
static void deinit_statics(void)
{
	s_initialised = 0;
	free_uart_dev_ll();
}

/**
 * This function performs a call to the Windows WriteFile() function, in order to
 * send data to the serial interface. It returns appropriate information to the
 * caller so that it can determine what to do next.
 * 
 * NOTE: This function is non-blocking, and by the time it returns, data may or
 * may not have been actually sent to the serial interface. It is the responsibility
 * of the caller to handle this ambiguity by making use of the return value of this
 * function.
 * 
 * @param pDeviceRef Represents a ca821x device
 * @param pBuffer Data to send
 * @param bufferSize Number of bytes to send
 * @param pBytesWritten The number of bytes that actually get written will be stored
 * at this location.
 * @return enum serial_op_completion_info 
 */
static enum serial_op_completion_info _uart_write(struct ca821x_dev *pDeviceRef,
                                                  const uint8_t     *pBuffer,
                                                  uint32_t           bufferSize,
                                                  int               *pBytesWritten)
{
	ca_log_debg("_uart_write()");

	struct uart_exchange_info *info = pDeviceRef->exchange_context;

	// info->port_handle was opened for OVERLAPPED I/O, so WriteFile() can either perform
	// synchronously or asynchronounsly. The application needs to be able to handle both
	// scenarios, because we can't predict which one will occur.
	// NOTE: If WriteFile() performs synchronously, then we can get the number of bytes
	// written via pBytesWritten. HOWEVER, if WriteFile() performs asynchronously,
	// this should be ignored, and the number of bytes written will instead be available
	// via a call to GetOverlappedResult().
	if (!WriteFile(info->port_handle, pBuffer, bufferSize, (LPDWORD)pBytesWritten, &info->port_overlap))
	{
		// Can't rely on pBytesWritten, see comment above.
		pBytesWritten          = NULL;
		DWORD write_file_error = GetLastError();

		if (write_file_error == ERROR_IO_PENDING)
		{
			ca_log_debg("WriteFile() performed asynchronously.");
			return OPERATION_PERFORMED_ASYNCHRONOUSLY;
		}
		else
		{
			ca_log_warn("WriteFile() operation FAILED, with error %x", write_file_error);
			return OPERATION_FAILED;
		}
	}
	else
	{
		ca_log_debg("WriteFile() performed synchronously, sent %d bytes to serial", *pBytesWritten);
		return OPERATION_PERFORMED_SYNCHRONOUSLY;
	}
}

/**
 * This function performs a call to the Windows ReadFile() function, in order to
 * read data from the serial interface. It returns appropriate information to the
 * caller so that it can determine what to do next.
 * 
 * NOTE: This function is non-blocking, and by the time it returns, data may or
 * may not have been actually received from the serial interface. It is the responsibility
 * of the caller to handle this ambiguity by making use of the return value of this
 * function.
 * 
 * @param pDeviceRef Represents a ca821x device
 * @param pBuffer The data that is read will be stored at this location
 * @param bufferSize Number of bytes to read 
 * @param pBytesRead The number of bytes that actually get read will be stored
 * at this location.
 * @return enum serial_op_completion_info 
 */
static enum serial_op_completion_info _uart_read(struct ca821x_dev *pDeviceRef,
                                                 uint8_t           *pBuffer,
                                                 uint32_t           bufferSize,
                                                 int               *pBytesRead)
{
	ca_log_debg("_uart_read()");

	struct uart_exchange_info *info = pDeviceRef->exchange_context;

	// info->port_handle was opened for OVERLAPPED I/O, so ReadFile() can either perform
	// synchronously or asynchronounsly. The application needs to be able to handle both
	// scenarios.
	// NOTE: If ReadFile() performs synchronously, then we can get the number of bytes
	// read via pBytesRead. HOWEVER, if ReadFile() performs asynchronously,
	// this should be ignored, and the number of bytes read will instead be available
	// via a call to GetOverlappedResult().
	if (!ReadFile(info->port_handle, pBuffer, bufferSize, (LPDWORD)pBytesRead, &info->port_overlap))
	{
		// Can't rely on pBytesRead, see comment above.
		pBytesRead            = NULL;
		DWORD read_file_error = GetLastError();

		if (read_file_error == ERROR_IO_PENDING)
		{
			ca_log_debg("ReadFile() performed asynchronously.");
			return OPERATION_PERFORMED_ASYNCHRONOUSLY;
		}
		else
		{
			ca_log_debg("ReadFile() operation FAILED, with error %x", read_file_error);
			return OPERATION_FAILED;
		}
	}
	else
	{
		ca_log_debg("ReadFile() performed synchronously, received %d bytes from serial", *pBytesRead);
		return OPERATION_PERFORMED_SYNCHRONOUSLY;
	}
}

/**
 * This function calls the _uart_write() function and does the proper handling to
 * actually guarantee (failures aside) that data is written to the serial interface
 * by the time it returns.
 * 
 * @param pDeviceRef Represents a ca821x device
 * @param pBuffer Data to send
 * @param bufferSize Number of bytes to send
 * @param pBytesWritten The number of bytes that actually get written will be stored
 * at this location.
 * @return ca_error 
 */
static ca_error uart_write_blocking(struct ca821x_dev *pDeviceRef,
                                    const uint8_t     *pBuffer,
                                    ssize_t            bufferSize,
                                    int               *pBytesWritten)
{
	ca_log_debg("uart_write_blocking()");

	struct uart_exchange_info     *info = pDeviceRef->exchange_context;
	DWORD                          wait_result;
	enum serial_op_completion_info op_info;

	op_info = _uart_write(pDeviceRef, pBuffer, bufferSize, pBytesWritten);

	if (op_info == OPERATION_FAILED)
		return CA_ERROR_FAIL;

	if (op_info == OPERATION_PERFORMED_SYNCHRONOUSLY)
		return CA_ERROR_SUCCESS;

	// Process the asynchronous operation
	wait_result = WaitForSingleObject(info->events[SERIAL_ACTIVITY_EVENT], INFINITE);
	switch (wait_result)
	{
	case WAIT_OBJECT_0:
		ca_log_debg("Asynchronous WriteFile() complete.");
		if (!GetOverlappedResult(info->port_handle, &info->port_overlap, (LPDWORD)pBytesWritten, FALSE))
		{
			ca_log_warn("GetOverlapped() FAILED! 0x%x", GetLastError());
			return CA_ERROR_FAIL;
		}
		else
		{
			ca_log_debg("Sent %d bytes!", *pBytesWritten);
			return CA_ERROR_SUCCESS;
		}
		break;
	case WAIT_TIMEOUT:
		ca_log_warn("WaitForSingleObject() Timed Out");
		return CA_ERROR_FAIL;
	case WAIT_FAILED:
		ca_log_warn("WaitForSingleObject() Failed");
		return CA_ERROR_FAIL;
	case WAIT_ABANDONED:
		ca_log_warn("WaitForSingleObject() Abandoned");
		return CA_ERROR_FAIL;
	default:
		ca_log_warn("WaitForSingleObject() Unknown Error???");
		return CA_ERROR_FAIL;
	}
}

/**
 * This function calls the _uart_read() function and does the proper handling to
 * actually guarantee (failures aside) that data is read from the serial interface
 * by the time it returns.
 * 
 * @param pDeviceRef Represents a ca821x device
 * @param pBuffer The data that is read will be stored at this location
 * @param bufferSize Number of bytes to read
 * @param pBytesRead The number of bytes that actually get read will be stored
 * at this location.
 * @param timeout How long to wait for data to be read, before returning with a
 * CA_ERROR_TIMEOUT error.
 * @return ca_error 
 */
static ca_error uart_read_blocking(struct ca821x_dev *pDeviceRef,
                                   uint8_t           *pBuffer,
                                   ssize_t            bufferSize,
                                   ssize_t           *pBytesRead,
                                   int                timeout)
{
	ca_log_debg("uart_read_blocking()");

	struct uart_exchange_info     *info = pDeviceRef->exchange_context;
	DWORD                          wait_result;
	enum serial_op_completion_info op_info;

	op_info = _uart_read(pDeviceRef, pBuffer, bufferSize, (int *)pBytesRead);

	if (op_info == OPERATION_FAILED)
		return CA_ERROR_FAIL;

	if (op_info == OPERATION_PERFORMED_SYNCHRONOUSLY)
		return CA_ERROR_SUCCESS;

	// Process the asynchronous operation
	wait_result = WaitForSingleObject(info->events[SERIAL_ACTIVITY_EVENT], timeout);
	switch (wait_result)
	{
	case WAIT_OBJECT_0:
		ca_log_debg("Asynchronous ReadFile() complete.");
		if (!GetOverlappedResult(info->port_handle, &info->port_overlap, (LPDWORD)pBytesRead, FALSE))
		{
			ca_log_warn("GetOverlapped() FAILED!");
			return CA_ERROR_FAIL;
		}
		else
		{
			ca_log_debg("Read %d bytes!", *pBytesRead);
			return CA_ERROR_SUCCESS;
		}
	case WAIT_TIMEOUT:
		ca_log_warn("WaitForSingleObject() Timed Out");
		return CA_ERROR_TIMEOUT;
	case WAIT_FAILED:
		ca_log_warn("WaitForSingleObject() Failed");
		return CA_ERROR_FAIL;
	case WAIT_ABANDONED:
		ca_log_warn("WaitForSingleObject() Abandoned");
		return CA_ERROR_FAIL;
	default:
		ca_log_warn("WaitForSingleObject() Unknown Error???");
		return CA_ERROR_FAIL;
	}
}

/**
 * This function waits until either data is ready to be read from the serial
 * interface, or until a signal is sent specifically to unblock the wait (by the
 * function unblock_read()), or until the timeout is expired.
 * If it stops waiting due to data being ready to be read, then it also reads it.
 * 
 * @param pDeviceRef Represents a ca821x device
 * @param timeout How long to wait before moving on from the wait
 * @param pBytesRead The number of bytes that actually get read will be stored
 * at this location.
 * @return int 
 */
static int wait_for_event_and_try_to_read(struct ca821x_dev *pDeviceRef, struct timespec timeout, int *pBytesRead)
{
	ca_log_debg("wait_for_event_and_try_to_read()\n");

	struct uart_exchange_info     *info = pDeviceRef->exchange_context;
	DWORD                          wait_result;
	long int                       timeout_ms = timeout.tv_sec * 1000 + timeout.tv_nsec / 1000000;
	enum serial_op_completion_info op_info;

	op_info = _uart_read(pDeviceRef, info->rx_buf + info->offset, sizeof(info->rx_buf) - info->offset, pBytesRead);

	// If the read operation is performed synchronously, then that is equivalent
	// to waiting until a byte is available, and then reading it. Except the wait
	// was instantaneous because there was already data to be read.
	if (op_info == OPERATION_PERFORMED_SYNCHRONOUSLY)
	{
		ca_log_debg("Wait done due to synchronous read. Read %d bytes!", &pBytesRead);
		return CA_ERROR_SUCCESS;
	}

	// If the read operation fails, then there is no point waiting for data,
	// so we just wait for the UNBLOCKING_READ_EVENT
	if (op_info == OPERATION_FAILED)
	{
		wait_result = WaitForSingleObject(info->events[UNBLOCKING_READ_EVENT], timeout_ms);
	}
	// If the read operation is pending, then we need to wait for either this operation to complete or for the UNBLOCKING_READ_EVENT.
	else
	{
		// This will block until either SERIAL_ACTIVITY_EVENT or UNBLOCKING_READ_EVENT
		// are triggered, or until the timeout_ms is expired.
		wait_result = WaitForMultipleObjects(NUM_EVENTS,   // number of objects in array
		                                     info->events, // array of objects
		                                     FALSE,        // Wait for any object
		                                     timeout_ms);  // Timeout in ms
	}

	switch (wait_result)
	{
	case WAIT_OBJECT_0 + UNBLOCKING_READ_EVENT:
		ca_log_debg("Wait done due to UNBLOCKGIN_READ_EVENT signal.");
		return CA_ERROR_SUCCESS;
	case WAIT_OBJECT_0 + SERIAL_ACTIVITY_EVENT:
		ca_log_debg("Wait done due to SERIAL_ACTIVITY_EVENT signal.");
		if (!GetOverlappedResult(info->port_handle, &info->port_overlap, (LPDWORD)pBytesRead, FALSE))
		{
			ca_log_warn("Reading from the device failed!");
			return CA_ERROR_FAIL;
		}
		else
		{
			ca_log_debg("Read %d bytes!", *pBytesRead);
			return CA_ERROR_SUCCESS;
		}
	case WAIT_TIMEOUT:
		ca_log_info("Wait done. Timed Out");
		return CA_ERROR_TIMEOUT;
	case WAIT_FAILED:
		ca_log_warn("Wait done. Failed");
		return CA_ERROR_FAIL;
	case WAIT_ABANDONED:
		ca_log_warn("Wait done. Abandoned");
		return CA_ERROR_FAIL;
	default:
		ca_log_warn("Wait done. Unknown Error???");
		return CA_ERROR_FAIL;
	}
}

static ca_error uart_write_isready(struct ca821x_dev *pDeviceRef)
{
	ca_log_debg("uart_write_isready()");

	struct uart_exchange_info *info = pDeviceRef->exchange_context;

	// Check ack timeout
	if (info->tx_stalled)
	{
		struct timespec timeDiff = get_tx_time_passed(info);
		if (time_cmp(&timeDiff, &ACK_TIMEOUT) > 0)
		{
			ca_log_warn("UART Ack missed");
			info->tx_stalled = 0;
		}
	}

	return info->tx_stalled ? CA_ERROR_BUSY : CA_ERROR_SUCCESS;
}

static void unblock_read(struct ca821x_dev *pDeviceRef)
{
	ca_log_debg("unblock_read()");

	struct uart_exchange_info *info = pDeviceRef->exchange_context;

	assert_uart_exchange(pDeviceRef);

	if (!SetEvent(info->events[UNBLOCKING_READ_EVENT]))
		ca_log_warn("SetEvent failed (%d)\n", GetLastError());
	else
		ca_log_debg("Successfully set off event to unblock read");
}

static ca_error send_uart_ack(struct ca821x_dev *pDeviceRef, bool rxSuccess)
{
	ca_log_debg("send_uart_ack(), %d", rxSuccess);

	ca_error error = CA_ERROR_SUCCESS;
	uint8_t  ack[] = {UART_SOM, EVBME_RXRDY, 0};
	int      len_to_send, len_sent;

	if (!rxSuccess)
		ack[1] = EVBME_RXFAIL;

	len_to_send = sizeof(ack);

	while (len_to_send > 0)
	{
		if (uart_write_blocking(pDeviceRef, ack, len_to_send, &len_sent))
		{
			error = CA_ERROR_FAIL;
			break;
		}
		len_to_send -= len_sent;
	}

	return error;
}

static ca_error uart_try_write(const uint8_t *buffer, size_t len, struct ca821x_dev *pDeviceRef)
{
	ca_log_debg("uart_try_write()");

	int                        len_sent = 0;
	ca_error                   error    = CA_ERROR_SUCCESS;
	struct uart_exchange_info *info     = pDeviceRef->exchange_context;

	assert_uart_exchange(pDeviceRef);
	assert(len == (size_t)(buffer[1] + 2));

	//send SOM byte
	do
	{
		if (uart_write_blocking(pDeviceRef, &UART_SOM, 1, &len_sent))
		{
			error = CA_ERROR_FAIL;
			break;
		}
	} while (len_sent != 1);

	//send message
	while (len)
	{
		if (uart_write_blocking(pDeviceRef, buffer, len, &len_sent))
		{
			error = CA_ERROR_FAIL;
			break;
		}
		len -= len_sent;
		buffer += len_sent;
	}

	//store sent message
	if (buffer != info->tx_buf)
		memcpy(info->tx_buf, buffer, len);

	if (error == CA_ERROR_FAIL)
	{
		ca_log_crit("UART Send error!");
	}
	else
	{
		//Wait for ack
		ca_log_debg("Wrote to UART");
		info->tx_stalled = 1;
		clock_gettime(CLOCK_REALTIME, &(info->prev_send_time));
	}
	return error;
}

static ssize_t uart_try_read(struct ca821x_dev *pDeviceRef, uint8_t *buf)
{
	ca_log_debg("uart_try_read()");

	struct uart_exchange_info *info = pDeviceRef->exchange_context;
	struct timespec            timeout;
	uint8_t                   *som    = info->rx_buf;
	uint8_t                   *cmdid  = info->rx_buf + 1;
	uint8_t                   *cmdlen = info->rx_buf + 2;
	int                        uart_error;
	ca_error                   status;
	ssize_t                    len = 0;

	assert_uart_exchange(pDeviceRef);

	// Set up timeout, taking ack timeout into account.
	if (info->tx_stalled)
	{
		timeout = get_tx_time_passed(info);
		timeout = time_sub(&ACK_TIMEOUT, &timeout);
		if (time_cmp(&timeout, &BLOCKING_WAIT_TIMEOUT) > 0)
			timeout = BLOCKING_WAIT_TIMEOUT;
	}
	else
	{
		timeout = BLOCKING_WAIT_TIMEOUT;
	}

	if (!info->offset)
		status = wait_for_event_and_try_to_read(pDeviceRef, timeout, (int *)&len);
	else
		// Just try to read, without waiting
		status =
		    uart_read_blocking(pDeviceRef, info->rx_buf + info->offset, sizeof(info->rx_buf) - info->offset, &len, 0);

	// Read from device if possible
	if (status == CA_ERROR_TIMEOUT)
	{
		len = 0;
	}
	else if (status != CA_ERROR_SUCCESS)
	{
		ca_log_warn("UART read error");
		uart_error = -uart_exchange_err_uart;
		goto exit;
	}

	//If we are just starting to receive, then register the start time
	if (len && !info->offset)
	{
		clock_gettime(CLOCK_REALTIME, &(info->rx_start_time));
	}

	info->offset += len;

	//Catch SOM (Start Of Message) errors
	while (info->offset && *som != UART_SOM)
	{
		/*
		 * Getting these very occasionally is ok, but if they are regular then either the
		 * line is too noisy, or there are some serious issues with receiving bytes
		 * and the baud rate should be decreased. If a stream of these occur in a row
		 * then it is likely a SOM has been missed and a packet has been lost.
		 */
		ca_log_warn("No SOM, got 0x%02x", info->rx_buf[0]);
		memmove(info->rx_buf, info->rx_buf + 1, info->offset);
		info->offset -= 1;
	}

	// If an incomplete packet has been received, hold off returning it up until the full
	// thing comes through.
	if ((info->offset >= 3) && (info->offset >= (*cmdlen + 3U)))
	{
		ssize_t framelen = *cmdlen + 3;

		//Skip SOM byte in copy
		len = framelen - 1;
		memcpy(buf, cmdid, len);
		// If extra data has been received, shuffle about
		memmove(info->rx_buf, info->rx_buf + framelen, (info->offset - framelen));
		info->offset -= framelen;

		//Send ACK
		if (buf[0] != EVBME_RXRDY && send_uart_ack(pDeviceRef, true) != CA_ERROR_SUCCESS)
		{
			uart_error = -uart_exchange_err_uart;
			goto exit;
		}
	}
	else
	{
		//If we fail to receive the entire packet within the receive timeout, then nack the entire packet and dump received data
		struct timespec timeDiff = get_rx_time_passed(info);
		if (info->offset && time_cmp(&timeDiff, &RX_TIMEOUT) > 0)
		{
			ca_log_warn("UART RX timed out");
			send_uart_ack(pDeviceRef, false);
			info->offset = 0;
		}
		len = 0;
	}

	//Catch, process & discard UART ACKs
	if (len && buf[0] == EVBME_RXRDY)
	{
		info->tx_stalled = 0;
		len              = 0;
		ca_log_debg("processed ACK");
	}

	//Catch, process & discard UART NACKs
	if (len && buf[0] == EVBME_RXFAIL)
	{
		len = 0;
		ca_log_debg("received NACK");
		if (uart_try_write(info->tx_buf, info->tx_buf[1] + 2, pDeviceRef))
		{
			ca_log_crit("UART failed to retransmit.");
			uart_error = -uart_exchange_err_uart;
			goto exit;
		}
	}

	if (len >= 3 && buf[0] == 0xF0)
	{
		ca_log_crit("ERROR CODE 0x%02x", buf[2]);
		fflush(stderr);
		//Error packet indicating coprocessor has reset ca821x - let app know
		if (buf[3])
			uart_error = -uart_exchange_err_ca821x;
		goto exit;
	}

exit:
	if (uart_error < 0)
		return uart_error;

	return len;
}

static void flush_unread_uart(struct ca821x_dev *pDeviceRef)
{
	ca_log_debg("flush_unread_uart()");

	uint8_t junk_buf[100];
	int     len;

	assert_uart_exchange(pDeviceRef);

	do
	{
		uart_read_blocking(pDeviceRef, junk_buf, sizeof(junk_buf), (ssize_t *)&len, 0);
	} while (len > 0);
}

/**
 * Opens and configures the serial port.
 * 
 * @param port_handle The handle created from opening the port will be stored
 * at this location.
 * @return ca_error 
 */
static ca_error serial_open_port(HANDLE *port_handle)
{
	ca_log_debg("serial_open_port()");
	DCB          port_params   = {0}; // Device Control Block struct for port parameter initialisation
	COMMTIMEOUTS port_timeouts = {0}; // Port timeout parameters
	BOOL         status;
	char         port_path[20] = "\\\\.\\COM";
	char         port_num_str[4];
	int          port_num_int;

	if (s_grab_first_port)
		port_num_int = s_uart_dev_ll_head->port_num;
	else
		port_num_int = s_com_port_number;

	sprintf(port_num_str, "%d", port_num_int);
	strcat(port_path, port_num_str);

	// open serial port
	(*port_handle) = CreateFile(port_path,                    // Name of the Port to be Opened
	                            GENERIC_READ | GENERIC_WRITE, // Read/Write Access
	                            0,                            // No Sharing, ports cant be shared
	                            NULL,                         // No Security
	                            OPEN_EXISTING,                // Open existing port only
	                            FILE_FLAG_OVERLAPPED,         // Overlapped I/O
	                            NULL);                        // Null for Comm Devices

	if ((*port_handle) == INVALID_HANDLE_VALUE)
	{
		ca_log_warn("Failed to open device %s", port_path);
		return CA_ERROR_NOT_FOUND;
	}

	// configure serial port parameterf
	port_params.DCBlength = sizeof(port_params);

	status = GetCommState((*port_handle), &port_params); /* get current settings */
	if (status == FALSE)
	{
		ca_log_warn("Failed to get serial port state");
		return CA_ERROR_FAIL;
	}

	port_params.BaudRate = CASCODA_UART_BAUDRATE; // Setting BaudRate
	port_params.ByteSize = 8;                     // Setting ByteSize = 8
	port_params.StopBits = ONESTOPBIT;            // Setting StopBits = 1
	port_params.Parity   = NOPARITY;              // Setting Parity = None

	status = SetCommState((*port_handle), &port_params); /* configure serial port */
	if (status == FALSE)
	{
		ca_log_warn("Failed to configure serial port");
		return CA_ERROR_FAIL;
	}

	// set serial port timeout parameters in [ms]
	port_timeouts.ReadIntervalTimeout         = MAXDWORD;
	port_timeouts.ReadTotalTimeoutConstant    = 0;
	port_timeouts.ReadTotalTimeoutMultiplier  = 0;
	port_timeouts.WriteTotalTimeoutConstant   = 0;
	port_timeouts.WriteTotalTimeoutMultiplier = 0;

	status = SetCommTimeouts((*port_handle), &port_timeouts);
	if (status == FALSE)
	{
		ca_log_warn("Failed to configure serial port timeouts");
		return CA_ERROR_FAIL;
	}

	printf("Port %s sucessfully opened; baud rate %u\n", port_path, port_params.BaudRate);

	return CA_ERROR_SUCCESS;
}

/**
 * Initialises the two events which are to be monitored via the various calls
 * to the Windows Wait functions.
 * 
 * @param pDeviceRef Represents a ca821x device
 */
static void init_signalling_events(struct ca821x_dev *pDeviceRef)
{
	ca_log_debg("init_signalling_events()");

	struct uart_exchange_info *info = pDeviceRef->exchange_context;

	// Create first event for the sole purpose of unblocking (e.g. in case a write
	// operation needs to be performed).
	// Create second event to signal that something was written to COM port.
	for (int i = 0; i < 2; ++i)
	{
		info->events[i] = CreateEvent(NULL,  // default security attributes
		                              FALSE, // auto-reset event object
		                              FALSE, // initial state is nonsignaled
		                              NULL); // unnamed object

		if (info->events[i] == NULL)
		{
			ca_log_warn("CreateEvent error %d", GetLastError());
			ExitProcess(0);
		}
		else
		{
			ca_log_debg("Successfully created event no. %d with handle %d", i, info->events[i]);
		}
	}

	// Set this event in an overlap struct which can be used to issue an asynchronous read!
	// Note: this is not done for the UNBLOCKING_READ_EVENT because that event doesn't
	// indicate that anything is ready for reading. It is instead used as a signal which
	// can be arbitrarily sent for any reason whatsoever.
	info->port_overlap.hEvent = info->events[SERIAL_ACTIVITY_EVENT];
}

// ==================
// EXTERNAL FUNCTIONS
// ==================

ca_error uart_exchange_init(ca821x_errorhandler              callback,
                            struct ca821x_dev               *pDeviceRef,
                            union ca821x_util_init_extra_arg arg)
{
	ca_log_debg("uart_exchange_init()");

	ca_error                   error = CA_ERROR_SUCCESS;
	struct uart_exchange_info *info  = NULL;

	if ((error = init_statics()))
		return error;

	if ((error = handle_extra_arg(arg)))
		return error;

	if (pDeviceRef->exchange_context)
		return CA_ERROR_ALREADY;

	pthread_mutex_lock(&s_devs_mutex);

	pDeviceRef->exchange_context = calloc(1, sizeof(struct uart_exchange_info));
	info                         = pDeviceRef->exchange_context;
	if (!info)
	{
		ca_log_warn("Failed to allocate exchange context.");
		error = CA_ERROR_NO_BUFFER;
		goto exit;
	}

	info->base.exchange_type      = ca821x_exchange_uart;
	info->base.error_callback     = callback;
	info->base.write_func         = &uart_try_write;
	info->base.write_isready_func = &uart_write_isready;
	info->base.signal_func        = &unblock_read;
	info->base.read_func          = &uart_try_read;
	info->base.flush_func         = &flush_unread_uart;
	info->port_handle             = INVALID_HANDLE_VALUE;
	info->offset                  = 0;

	if ((error = serial_open_port(&info->port_handle)))
	{
		CloseHandle(info->port_handle);
		info->port_handle = INVALID_HANDLE_VALUE;
		goto exit;
	}

	// Initialises Events that will be used for signalling to the
	// WaitForMultipleObjects() to unblock.
	init_signalling_events(pDeviceRef);

	if ((error = init_generic(pDeviceRef)))
		goto exit;

	//Add the new device to the device list for io
	s_devcount++;

exit:
	if (error && pDeviceRef->exchange_context)
	{
		free(pDeviceRef->exchange_context);
		pDeviceRef->exchange_context = NULL;
	}

	if (s_devcount == 0)
		deinit_statics();

	pthread_mutex_unlock(&s_devs_mutex);

	if (error == CA_ERROR_SUCCESS)
		ca_log_info("Successfully started UART Exchange.");

	return error;
}

void uart_exchange_deinit(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_info *info = pDeviceRef->exchange_context;

	CloseHandle(info->port_handle);

	assert_uart_exchange(pDeviceRef);
	deinit_generic(pDeviceRef);

	pthread_mutex_lock(&s_devs_mutex);
	s_devcount--;
	if (s_devcount == 0)
		deinit_statics();
	pthread_mutex_unlock(&s_devs_mutex);

	free(info);
	pDeviceRef->exchange_context = NULL;
}

ca_error uart_exchange_enumerate(util_device_found aCallback, void *aContext)
{
	ca_log_debg("uart_exchange_enumerate()");

	struct uart_dev_ll_node *cur_node;
	ca_error                 status = CA_ERROR_NOT_FOUND;

	if ((status = init_statics()))
		return status;

	pthread_mutex_lock(&s_devs_mutex);
	//Increment the dev_count to prevent statics being deinitialised
	s_devcount++;
	pthread_mutex_unlock(&s_devs_mutex);

	cur_node = s_uart_dev_ll_head;
	while (cur_node)
	{
		struct ca_device_info devi          = {0};
		char                  port_path[20] = "\\\\.\\COM";
		char                  port_num_str[4];

		sprintf(port_num_str, "%d", cur_node->port_num);
		strcat(port_path, port_num_str);

		devi.exchange_type = ca821x_exchange_uart;
		devi.path          = port_path;

		//Notify the caller
		aCallback(&devi, aContext);

		cur_node = cur_node->next;
	}

	pthread_mutex_lock(&s_devs_mutex);
	s_devcount--;
	if (s_devcount == 0)
		deinit_statics();
	pthread_mutex_unlock(&s_devs_mutex);

	return status;
}

int uart_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef)
{
	//For uart, the coprocessor will reset the ca821x if it isn't responsive.. so just rely on that
	(void)resettime;
	(void)pDeviceRef;

	return 0;
}