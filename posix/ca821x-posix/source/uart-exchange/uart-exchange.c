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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "ca821x-generic-exchange.h"
#include "ca821x-posix-util-internal.h"
#include "ca821x-queue.h"
#include "ca821x_api.h"
#include "uart-exchange.h"

/** Private data for the UART Exchange */
struct uart_exchange_priv
{
	struct ca821x_exchange_base base;       //!< Exchange base structure
	int                         fd;         //!< UART device file descriptor
	uint8_t                     tx_stalled; //!< True if transmissions are stalled waiting for ack
	uint8_t         rx_buf[MAX_BUF_SIZE];   //!< Private buffer for buffering read data before full packet is received
	uint8_t         tx_buf[MAX_BUF_SIZE];   //!< Private buffer for buffering tx data (in case retransmit is required)
	size_t          offset;                 //!< Current offset for reading into buf
	struct timespec prev_send;              //!< Time that previous message was sent (for timing out ack)
	struct timespec rx_start;               //!< Time that current message receive started (for timing out receive)
	int             dummy_pipe_fd[2];       //!< Dummy pipe fds to release from select() call when write is due
};

/** struct for representing the contents of CASCODA_UART environment variable */
struct uart_device
{
	struct uart_device *next;   //!< Next device in linked list
	const char         *device; //!< path to uart device
	int                 baud;   //!< Baudrate to be used for uart comms
};

static int                 s_devcount         = 0;
static int                 s_initialised      = 0;
static char               *s_CASCODA_UART     = NULL;
static struct uart_device *s_uart_device_head = NULL;

//! Start of frame delimiter
static const uint8_t UART_SOM = 0xDE;

//! Timeout for waiting for ack = 1 second
static const struct timespec ack_timeout = {1, 0};
//! Timeout for waiting for full message = 0.2 seconds
static const struct timespec rx_timeout = {0, 200000000ULL};
//! Timeout for posix select call = 0.5 seconds
static const struct timespec select_timeout = {0, 500000000ULL};

static pthread_mutex_t devs_mutex = PTHREAD_MUTEX_INITIALIZER;

static void assert_uart_exchange(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;

	if (priv->base.exchange_type != ca821x_exchange_uart)
		abort();
}

static int uart_som_err_cnt = 0;
static int uart_timeout_err_cnt = 0;
static int uart_ack_err_cnt = 0;
static struct timespec time_last_uart_err_check = {};

static const int MAX_SOM_ERR = 5;
static const int MAX_TIMEOUT_ERR = 5;
static const int MAX_ACK_ERR = 5;
static const struct timespec uart_err_timeout = {60, 0};

uint8_t last_buf[MAX_BUF_SIZE];
size_t last_buf_len = 0;

/**
 * Get the amount of time that has passed since the last transmission.
 * @param priv exchange private state
 * @return the time that has passed since last UART transmission.
 */
static struct timespec get_tx_time_passed(const struct uart_exchange_priv *priv)
{
	struct timespec curTime;
	if (!clock_gettime(CLOCK_MONOTONIC, &curTime))
	{
		curTime = time_sub(&curTime, &priv->prev_send);
	}
	else
	{
		memset(&curTime, 0, sizeof(curTime));
	}
	return curTime;
}

/**
 * Get the amount of time that has passed since the last receive started.
 * @param priv exchange private state
 * @return the time that has passed since last UART receive begun.
 */
static struct timespec get_rx_time_passed(const struct uart_exchange_priv *priv)
{
	struct timespec curTime;
	if (!clock_gettime(CLOCK_MONOTONIC, &curTime))
	{
		curTime = time_sub(&curTime, &priv->rx_start);
	}
	else
	{
		memset(&curTime, 0, sizeof(curTime));
	}
	return curTime;
}


static struct timespec get_err_time_passed()
{
	struct timespec curTime;
	if (!clock_gettime(CLOCK_MONOTONIC, &curTime))
	{
		curTime = time_sub(&curTime, &time_last_uart_err_check);
	}
	else
	{
		memset(&curTime, 0, sizeof(curTime));
	}
	return curTime;
}
/**
 * Send a UART ACK or NACK packet over the interface
 * @param fd The UART device to use to send the ACK
 * @param rx_success True if receive was successful, False if receive failed (negative ack will be sent)
 * @return ca_error value
 */
static ca_error send_uart_ack(int fd, bool rx_success)
{
	ca_error error = CA_ERROR_SUCCESS;
	uint8_t  ack[] = {UART_SOM, EVBME_RXRDY, 0};
	int      rval, len;

	if (!rx_success)
		ack[1] = EVBME_RXFAIL;

	len = sizeof(ack);

	while (len > 0)
	{
		rval = write(fd, ack, len);
		if (rval < 0)
		{
			error = CA_ERROR_FAIL;
			break;
		}
		len -= rval;
	}

	return error;
}

static ca_error uart_try_write(const uint8_t *buffer, size_t len, struct ca821x_dev *pDeviceRef)
{
	int                        rval  = 0;
	ca_error                   error = CA_ERROR_SUCCESS;
	struct uart_exchange_priv *priv  = pDeviceRef->exchange_context;

	assert_uart_exchange(pDeviceRef);
	assert(len == (size_t)(buffer[1] + 2));

	//send SOM byte
	do
	{
		rval = write(priv->fd, &UART_SOM, 1);
		if (rval < 0)
		{
            // Return immediately. If can't send the SOM, then there
            // is no point (and it is even harmful) to send the rest of the
            // message.
            return CA_ERROR_FAIL;
		}
	} while (rval != 1);

	//send message
	while (len)
	{
		rval = write(priv->fd, buffer, len);
		if (rval < 0)
		{
			error = CA_ERROR_FAIL;
			break;
		}
		len -= rval;
		buffer += rval;
	}

	//store sent message
	if (buffer != priv->tx_buf)
		memcpy(priv->tx_buf, buffer, len);

	if (error == CA_ERROR_FAIL)
	{
		ca_log_crit("UART Send error!");
	}
	else
	{
		//Wait for ack
		ca_log_debg("Wrote to UART");
		priv->tx_stalled = 1;
		clock_gettime(CLOCK_MONOTONIC, &(priv->prev_send));
	}
	return error;
}

static bool is_valid_command(uint8_t cmdid)
{
    switch (cmdid)
    {
        // Upstream SPI messages
		//CA821x control bytes
		case SPI_IDLE:
		case SPI_NACK:
		// MAC MCPS
        case SPI_MCPS_DATA_INDICATION:
        case SPI_MCPS_DATA_CONFIRM:
        case SPI_MCPS_PURGE_CONFIRM:
// MAC PCPS
#if CASCODA_CA_VER >= 8211
		case SPI_PCPS_DATA_CONFIRM:
		case SPI_PCPS_DATA_INDICATION:
#endif // CASCODA_CA_VER >= 8211
		// MAC MLME
        case SPI_MLME_ASSOCIATE_INDICATION:
        case SPI_MLME_ASSOCIATE_CONFIRM:
		case SPI_MLME_DISASSOCIATE_INDICATION:
        case SPI_MLME_DISASSOCIATE_CONFIRM:
        case SPI_MLME_BEACON_NOTIFY_INDICATION:
        case SPI_MLME_GET_CONFIRM:
        case SPI_MLME_ORPHAN_INDICATION:
        case SPI_MLME_RESET_CONFIRM:
        case SPI_MLME_RX_ENABLE_CONFIRM:
        case SPI_MLME_SCAN_CONFIRM:
        case SPI_MLME_COMM_STATUS_INDICATION:
        case SPI_MLME_SET_CONFIRM:
        case SPI_MLME_START_CONFIRM:
        case SPI_MLME_SYNC_LOSS_INDICATION:
        case SPI_MLME_POLL_CONFIRM:
#if CASCODA_CA_VER >= 8212
		case SPI_MLME_IE_NOTIFY_INDICATION:
#endif // CASCODA_CA_VER >= 8212
#if CASCODA_CA_VER >= 8211
        case SPI_MLME_POLL_INDICATION:
#endif // CASCODA_CA_VER >= 8211
		// HWME
        case SPI_HWME_SET_CONFIRM:
        case SPI_HWME_GET_CONFIRM:
        case SPI_HWME_HAES_CONFIRM:
        case SPI_HWME_WAKEUP_INDICATION:
		// TDME
        case SPI_TDME_SETSFR_CONFIRM:
        case SPI_TDME_GETSFR_CONFIRM:
        case SPI_TDME_TESTMODE_CONFIRM:
        case SPI_TDME_SET_CONFIRM:
        case SPI_TDME_TXPKT_CONFIRM:
        case SPI_TDME_RXPKT_INDICATION:
        case SPI_TDME_EDDET_INDICATION:
        case SPI_TDME_ERROR_INDICATION:
        case SPI_TDME_LOTLK_CONFIRM:
        // Upstream EVBME messages
        case EVBME_GET_CONFIRM:
        case EVBME_SET_CONFIRM:
        case EVBME_MESSAGE_INDICATION:
        case EVBME_COMM_INDICATION:
        case EVBME_DFU_CMD:
        case EVBME_RXRDY:
        case EVBME_RXFAIL:
            return true;
    }

    return false;
}

static ssize_t uart_try_read(struct ca821x_dev *pDeviceRef, uint8_t *buf)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;
	ssize_t                    len;
	int                        error  = 0;
	uint8_t                   *som    = priv->rx_buf;
	uint8_t                   *cmdid  = priv->rx_buf + 1;
	uint8_t                   *cmdlen = priv->rx_buf + 2;

	assert_uart_exchange(pDeviceRef);

	if (!priv->offset)
	{
        fd_set          rx_block_fd_set;
        int             nfds;
        struct timespec timeout;
		uint8_t         dummybyte = 0;

        //Initialise fd set for blocking select()
        FD_ZERO(&rx_block_fd_set);
        FD_SET(priv->fd, &rx_block_fd_set);
        FD_SET(priv->dummy_pipe_fd[0], &rx_block_fd_set);
        nfds = priv->fd > priv->dummy_pipe_fd[0] ? priv->fd : priv->dummy_pipe_fd[0];
        nfds = nfds + 1;

        //Set up timeout, taking ack timeout into account. Max block time = select_timeout
        timeout = select_timeout;

        if (priv->tx_stalled)
        {
            timeout = get_tx_time_passed(priv);
            timeout = time_sub(&ack_timeout, &timeout);
            if (time_cmp(&timeout, &select_timeout) > 0)
                timeout = select_timeout;
        }

		//Block until activity required, then read potential dummy byte
		pselect(nfds, &rx_block_fd_set, NULL, NULL, &timeout, NULL);
		read(priv->dummy_pipe_fd[0], &dummybyte, 1);
	}

	//Read from the device if possible
	len = read(priv->fd, priv->rx_buf + priv->offset, sizeof(priv->rx_buf) - priv->offset);
	if (len < 0)
	{
		int cur_errno = errno;
		if (cur_errno == EAGAIN)
		{
			len = 0;
		}
		else
		{
			ca_log_warn("UART read error 0x%02x", cur_errno);
			error = -uart_exchange_err_uart;
			goto exit;
		}
	}

	//If we are just starting to receive, then register the start time
	if (len && !priv->offset)
	{
		clock_gettime(CLOCK_MONOTONIC, &(priv->rx_start));
	}

	priv->offset += len;

	//Catch SOM errors
	bool som_error = false;
	bool no_som_at_start = false;
	bool debug_dumped = false;

	// We enter the while loop if we have a non-empty rx_buf that doesn't begin with SOM,
	// in which case we have to search the rest of rx_buf for a SOM
	while (priv->offset && (*som != UART_SOM || !is_valid_command(*cmdid)))
	{
		/*
		 * Getting these very occasionally is ok, but if they are regular then either the
		 * line is too noisy, or there are some serious issues with receiving bytes
		 * and the baud rate should be decreased. If a stream of these occur in a row
		 * then it is likely a SOM has been missed and a packet has been lost.
		 */
		if (*som != UART_SOM)
		{
			ca_log_warn("UART No SOM, got 0x%02x", priv->rx_buf[0]);
		}
		else if (!is_valid_command(*cmdid))
		{
			ca_log_warn("Invalid command, got 0x%02x", priv->rx_buf[1]);
		}
		
		no_som_at_start = true; // This marks that the while loop has been entered

		if (!debug_dumped)
			{
				// Log priv->rx_buf contents
				char rx_buf_str[3 * MAX_BUF_SIZE + 50] = {0}; // one byte takes 3 chars, plus prefix
				size_t pos = 0;

				pos += snprintf(rx_buf_str, sizeof(rx_buf_str) - pos,
								"rx_buf (%zu bytes):", priv->offset);
				for (size_t i = 0; i < priv->offset; i++) {
					pos += snprintf(rx_buf_str + pos, sizeof(rx_buf_str) - pos,
									" %02x", priv->rx_buf[i]);
				}

				ca_log_warn("%s", rx_buf_str);

				// Log last_buf contents if it exists
				if (last_buf_len > 0)
				{
					char last_buf_str[3 * MAX_BUF_SIZE + 50] = {0}; // one byte takes 3 chars, plus prefix
					pos = 0;

					pos += snprintf(last_buf_str, sizeof(last_buf_str) - pos,
									"last_buf (%zu bytes, SOM excluded):", last_buf_len);
					for (size_t i = 0; i < last_buf_len; i++) {
						pos += snprintf(last_buf_str + pos, sizeof(last_buf_str) - pos,
										" %02x", last_buf[i]);
					}

					ca_log_warn("%s", last_buf_str);
				}

				debug_dumped = true;
			}

		priv->offset -= 1;
		memmove(priv->rx_buf, priv->rx_buf + 1, priv->offset);
	}

	// If the while loop was entered, check if it exited with or without finding a SOM
	if (no_som_at_start) {
		if (*som != UART_SOM || !is_valid_command(*cmdid))
		{
			// While loop exited without finding a SOM, so we mark it as a UART SOM error
			ca_log_warn("UART SOM Error!");
			som_error = true;
			uart_som_err_cnt++;
		}
		else
		{
			// We found the start of message, followed by a valid command id,
			// after printing some warning logs. Print the success status, as
			// in the else branch som == UART_SOM
			ca_log_warn("UART SOM Found, and cmdid valid (code 0x%x)", *cmdid);
		}
	}

	// If an incomplete packet has been received, hold off returning it up until the full
	// thing comes through.
	if ((priv->offset >= 3) && (priv->offset >= (*cmdlen + 3U)))
	{
		ssize_t framelen = *cmdlen + 3;

		//Skip SOM byte in copy
		len = framelen - 1;
		memcpy(buf, cmdid, len);
		memcpy(last_buf, buf, len);
		last_buf_len = len;
		// If extra data has been received, shuffle about
		priv->offset -= framelen;
		memmove(priv->rx_buf, priv->rx_buf + framelen, priv->offset);

		//Send ACK
		if (buf[0] != EVBME_RXRDY && send_uart_ack(priv->fd, true) != CA_ERROR_SUCCESS)
		{
			error = -uart_exchange_err_uart;
			goto exit;
		}
	}
	else
	{
		//If we fail to receive the entire packet within the receive timeout, then nack the entire packet and dump received data
		struct timespec timeDiff = get_rx_time_passed(priv);
		if ((priv->offset || som_error) && time_cmp(&timeDiff, &rx_timeout) > 0)
		{
			ca_log_warn("UART RX timed out");
			send_uart_ack(priv->fd, false);
			priv->offset = 0;
			uart_timeout_err_cnt++;
		}
		len = 0;
	}

	//Catch, process & discard UART ACKs
	if (len && buf[0] == EVBME_RXRDY)
	{
		priv->tx_stalled = 0;
		len              = 0;
		ca_log_debg("processed ACK");
	}

	//Catch, process & discard UART NACKs
	if (len && buf[0] == EVBME_RXFAIL)
	{
		len = 0;
		ca_log_debg("received NACK");
		if (uart_try_write(priv->tx_buf, priv->tx_buf[1] + 2, pDeviceRef))
		{
			ca_log_crit("UART failed to retransmit.");
			error = -uart_exchange_err_uart;
			goto exit;
		}
	}

	if (len >= 3 && buf[0] == 0xF0)
	{
		ca_log_crit("ERROR CODE 0x%02x", buf[2]);
		fflush(stderr);
		//Error packet indicating coprocessor has reset ca821x - let app know
		if (buf[3])
			error = -uart_exchange_err_ca821x;
		goto exit;
	}

	// keep track of the number of errors
	struct timespec errTimeDiff;
exit:
	errTimeDiff = get_err_time_passed();
	if (time_cmp(&errTimeDiff, &uart_err_timeout) > 0)
	{
		// timed out, check counters & reset
		if (uart_som_err_cnt >= MAX_SOM_ERR ||
            uart_timeout_err_cnt >= MAX_TIMEOUT_ERR ||
            uart_ack_err_cnt >= MAX_ACK_ERR)
		{
            ca_log_crit("Excessive UART errors in the last 60 seconds!");
			// kill all processes in the calling group of the POSIX exchange
			// kill(0, SIGKILL);
		}

        if((uart_som_err_cnt > 0) || (uart_timeout_err_cnt > 0) || (uart_ack_err_cnt > 0))
        {
            ca_log_warn("Number of UART errors in the last 60 seconds: %d/%d/%d", uart_som_err_cnt, uart_timeout_err_cnt, uart_ack_err_cnt);
        }

        uart_som_err_cnt = 0;
        uart_timeout_err_cnt = 0;
        uart_ack_err_cnt = 0;
        clock_gettime(CLOCK_MONOTONIC, &time_last_uart_err_check);
	}
	if (error < 0)
	{
		return error;
	}
	return len;
}

static void unblock_read(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;

	assert_uart_exchange(pDeviceRef);

	const uint8_t dummybyte = 0;
	write(priv->dummy_pipe_fd[1], &dummybyte, 1);
}

static ca_error uart_write_isready(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;

	// Check ack timeout
	if (priv->tx_stalled)
	{
		struct timespec timeDiff = get_tx_time_passed(priv);
		if (time_cmp(&timeDiff, &ack_timeout) > 0)
		{
			ca_log_warn("UART Ack missed");
			priv->tx_stalled = 0;
			uart_ack_err_cnt++;
		}
	}

	return priv->tx_stalled ? CA_ERROR_BUSY : CA_ERROR_SUCCESS;
}

static void flush_unread_uart(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;
	uint8_t                    junk_buf[100];
	int                        rval;

	assert_uart_exchange(pDeviceRef);

	do
	{
		rval = read(priv->fd, junk_buf, 100);
	} while (rval > 0);
}

/**
 * Fill the linkedlist at tailp with the information from the pathstr (which will be modified in the process).
 * @param tailp Pointer to the 'next' pointer of the tail of the linkedlist (Or pointer to head pointer)
 * @param pathstr Modifiable string encoded like "/dev/ttyS0,115200:/dev/ttyS1,9600:/dev/ttyS2,6000000"
 * @return status
 * @retval CA_ERROR_SUCCESS   Success
 * @retval CA_ERROR_NOT_FOUND No devices found in path
 */
static ca_error fill_uartdev_ll(struct uart_device **tailp, char *pathstr)
{
	char *saveptr1, *saveptr2;
	bool  found = false;

	do
	{
		// Get the next, major ':' delimited section
		char *majorStr = strtok_r(pathstr, ":", &saveptr1);
		char *device, *baudstr;

		pathstr = NULL;
		if (!majorStr)
			break;

		// Split by token
		device = strtok_r(majorStr, ",", &saveptr2);
		if (!device)
			continue;
		baudstr = strtok_r(NULL, ",", &saveptr2);
		if (!baudstr)
			continue;

		// Add to linked list
		(*tailp)         = malloc(sizeof(struct uart_device));
		(*tailp)->next   = NULL;
		(*tailp)->baud   = atoi(baudstr);
		(*tailp)->device = device;
		tailp            = &((*tailp)->next);
		found            = true;
	} while (1);

	if (found)
		return CA_ERROR_SUCCESS;
	return CA_ERROR_NOT_FOUND;
}

/**
 * Free a uart device linked list. Note that this does not free the underlying string buffer.
 * @param uartdev Pointer to the first item in the linkedlist
 */
static void free_uartdev_ll(struct uart_device *uartdev)
{
	while (uartdev)
	{
		struct uart_device *tmp = uartdev;
		uartdev                 = uartdev->next;
		free(tmp);
	}
}

static ca_error init_statics(void)
{
	ca_error    error              = CA_ERROR_SUCCESS;
	const char *CONST_CASCODA_UART = getenv("CASCODA_UART");
	//example: CASCODA_UART="/dev/ttyS0,115200:/dev/ttyS1,9600:/dev/ttyS2,6000000"
	size_t envLen;

	if (s_initialised)
		return CA_ERROR_SUCCESS;

	s_initialised = 1;

	if (!CONST_CASCODA_UART)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	// The value returned by getenv shouldn't be modified (no idea why it isn't const)
	// so make a static copy that can be tokenized and used in our linked list.
	envLen         = strlen(CONST_CASCODA_UART) + 1;
	s_CASCODA_UART = malloc(envLen);
	memcpy(s_CASCODA_UART, CONST_CASCODA_UART, envLen);

	error = fill_uartdev_ll(&s_uart_device_head, s_CASCODA_UART);

	if (error)
		goto exit;

exit:
	return error;
}

static int deinit_statics()
{
	struct uart_device *uartdev = s_uart_device_head;

	s_initialised = 0;

	s_uart_device_head = NULL;
	free_uartdev_ll(uartdev);
	free(s_CASCODA_UART);

	return 0;
}

static speed_t get_baud_code(int baudrate)
{
	/* Yes, this is annoying - may have to expand this in future
	 * or potentially gate some of the baud codes with #ifdef if
	 * we find a system that it doesn't compile on.
	 */
	switch (baudrate)
	{
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
#ifdef B460800
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
#endif
	default:
		return (speed_t)-1;
	}
}

static ca_error setup_port(int fd, int baudrate)
{
	struct termios port;
	speed_t        baudcode = get_baud_code(baudrate);

	if (tcgetattr(fd, &port) < 0)
		return CA_ERROR_INVALID;

	if (cfsetospeed(&port, baudcode) || cfsetispeed(&port, baudcode))
	{
		ca_log_crit("Errno %d, Failed setting baud rate %d", errno, baudrate);
		return CA_ERROR_INVALID_ARGS;
	}

	port.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
	port.c_cflag &= ~CSIZE;           /* 8-bit characters */
	port.c_cflag |= CS8;              /* 8-bit characters */
	port.c_cflag &= ~PARENB;          /* no parity bit */
	port.c_cflag &= ~CSTOPB;          /* only need 1 stop bit */
	port.c_cflag &= ~CRTSCTS;         /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	port.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	port.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	port.c_oflag &= ~OPOST;

	/* fetch bytes as they become available, nonblocking */
	port.c_cc[VMIN]  = 0;
	port.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSANOW, &port) != 0)
		return CA_ERROR_INVALID;

	return CA_ERROR_SUCCESS;
}

ca_error uart_exchange_init(ca821x_errorhandler callback, const char *path, struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = NULL;
	struct uart_device        *uartdev;
	struct uart_device        *path_uartdev = NULL;
	char                      *path_dup     = NULL;
	ca_error                   error        = 0;

	error = init_statics();
	if (error)
		return error;

	if (pDeviceRef->exchange_context)
		return CA_ERROR_ALREADY;

	pthread_mutex_lock(&devs_mutex);

	pDeviceRef->exchange_context = calloc(1, sizeof(struct uart_exchange_priv));
	priv                         = pDeviceRef->exchange_context;
	if (!priv)
	{
		ca_log_warn("Failed to allocate exchange context.");
		error = CA_ERROR_NO_BUFFER;
		goto exit;
	}

	priv->base.exchange_type      = ca821x_exchange_uart;
	priv->base.error_callback     = callback;
	priv->base.write_func         = &uart_try_write;
	priv->base.write_isready_func = &uart_write_isready;
	priv->base.signal_func        = &unblock_read;
	priv->base.read_func          = &uart_try_read;
	priv->base.flush_func         = &flush_unread_uart;
	priv->fd                      = -1;
	priv->offset                  = 0;

	// Use the path if that is supplied, or environment variable if not
	if (path)
	{
		size_t len = strlen(path) + 1;
		path_dup   = malloc(len);
		if (!path_dup)
		{
			error = CA_ERROR_NO_BUFFER;
			goto exit;
		}
		memcpy(path_dup, path, len);
		error   = fill_uartdev_ll(&path_uartdev, path_dup);
		uartdev = path_uartdev;
		if (error)
			goto exit;
	}
	else
	{
		uartdev = s_uart_device_head;
	}

	//Open and set up file descriptor if available
	while (uartdev)
	{
		priv->fd = open(uartdev->device, O_RDWR | O_NOCTTY | O_SYNC);

		if (priv->fd < 0)
			ca_log_debg("Failed to open device %s", uartdev->device);
		else if (ioctl(priv->fd, TIOCEXCL))
			ca_log_warn("Failed to lock device %s", uartdev->device);
		else if (setup_port(priv->fd, uartdev->baud) != CA_ERROR_SUCCESS)
			ca_log_crit("Failed setup for device %s", uartdev->device);
		else
			break;

		close(priv->fd);
		priv->fd = -1;

		uartdev = uartdev->next;
	}

	if (priv->fd < 0)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	//Initialise the dummy pipe for releasing from select()
	pipe(priv->dummy_pipe_fd);
	fcntl(priv->dummy_pipe_fd[0], F_SETFL, O_NONBLOCK);

	error = init_generic(pDeviceRef);

	if (error != 0)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	//Add the new device to the device list for io
	s_devcount++;

	// initialize the time since the last UART error check
	clock_gettime(CLOCK_MONOTONIC, &time_last_uart_err_check);

exit:
	if (error && pDeviceRef->exchange_context)
	{
		free(pDeviceRef->exchange_context);
		pDeviceRef->exchange_context = NULL;
	}
	if (s_devcount == 0)
		deinit_statics();
	pthread_mutex_unlock(&devs_mutex);
	free_uartdev_ll(path_uartdev);
	free(path_dup);
	if (error == CA_ERROR_SUCCESS)
		ca_log_info("Successfully started UART Exchange.");
	return error;
}

void uart_exchange_deinit(struct ca821x_dev *pDeviceRef)
{
	struct uart_exchange_priv *priv = pDeviceRef->exchange_context;

	assert_uart_exchange(pDeviceRef);
	deinit_generic(pDeviceRef);

	pthread_mutex_lock(&devs_mutex);
	s_devcount--;
	if (s_devcount == 0)
		deinit_statics();
	pthread_mutex_unlock(&devs_mutex);

	free(priv);
	pDeviceRef->exchange_context = NULL;
}

ca_error uart_exchange_enumerate(util_device_found aCallback, void *aContext)
{
	struct uart_device *uartdev;
	ca_error            status = CA_ERROR_NOT_FOUND;

	status = init_statics();
	if (status)
		return status;

	pthread_mutex_lock(&devs_mutex);
	//Increment the dev_count to prevent statics being deinitialised
	s_devcount++;
	pthread_mutex_unlock(&devs_mutex);

	uartdev = s_uart_device_head;
	while (uartdev)
	{
		struct ca_device_info devi    = {0};
		char                 *pathout = NULL;
		size_t pathlen = strlen(uartdev->device) + 20; //+20 is for the baud rate integer and colon and null terminator

		devi.exchange_type = ca821x_exchange_uart;
		devi.path = pathout = malloc(pathlen);

		snprintf(pathout, pathlen, "%s,%d", uartdev->device, uartdev->baud);

		//Notify the caller
		aCallback(&devi, aContext);

		free(pathout);
		uartdev = uartdev->next;
	}

	pthread_mutex_lock(&devs_mutex);
	s_devcount--;
	if (s_devcount == 0)
		deinit_statics();
	pthread_mutex_unlock(&devs_mutex);
	return status;
}

int uart_exchange_reset(unsigned long resettime, struct ca821x_dev *pDeviceRef)
{
	//For uart, the coprocessor will reset the ca821x if it isn't responsive.. so just rely on that
	(void)resettime;
	(void)pDeviceRef;

	return 0;
}
