/**
 * @file   cascoda_spi.h
 * @brief  SPI Communication Driver Definitions/Declarations
 * @author Wolfgang Bruchner
 * @date   19/07/14
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
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
#include "cascoda-bm/cascoda_bm.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#ifndef CASCODA_SPI_H
#define CASCODA_SPI_H

/****** Definitions for SPI Handling                                     ******/

#define SPI_T_TIMEOUT 500   //!< Timeout for synchronous responses [ms]
#define SPI_T_BACKOFF 10    //!< Backoff Time when receiving SPI_NACK [us]
#define SPI_T_HOLD_8210 100 //!< Hold time before transmitting for the CA-8210 [us]
#define SPI_T_CSHOLD \
	50 /**< Time to hold the chip select active before
                               *   re-checking the IRQ [us]. This helps the
                               *   slave to better synchronise with the master
                               */

#define SPI_RX_FIFO_SIZE 3 //!< Maximum Size of Rx FIFO \ref SPI_Receive_Buffer

/****** Function Declarations for cascoda_spi.c                          ******/

/**
 * \brief Exchange Messages across SPI
 *
 * As the SPI is operating in full duplex mode, every exchange consists of both
 * a transmission and reception. This function transmits pTxBuffer while
 * populating an appropriate Rx buffer with the received message.
 *
 * \param pTxBuffer - Pointer to Transmit Buffer or NULLP
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return Status
 *
 */
ca_error SPI_Exchange(struct MAC_Message *pTxBuffer, struct ca821x_dev *pDeviceRef);

/**
 * \brief Send Request over SPI
 *
 * This function is the system-specific definition of ca821x_api_downstream. It
 * transmits a SAP command and if applicable, waits for a synchronous confirm.
 *
 * \param buf - Message to transmit
 * \param len - Length of message in buf
 * \param response - Buffer to fill with response
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return Status
 *
 */
ca_error SPI_Send(const uint8_t *buf, size_t len, u8_t *response, struct ca821x_dev *pDeviceRef);

/**
 * \brief Get a MAC_Message buffer containing a received SPI Message
 *
 * Calling this function does not remove the buffer from the queue, so after
 * the buffer has been processed, SPI_DequeueFullBuffer should be used.
 *
 * \retval A full SPI Buffer, or NULL if none are available.
 *
 */
struct MAC_Message *SPI_PeekFullBuf(void);

/**
 * \brief Remove a processed Full Buffer from the SPI Queue
 *
 * This frees up the buffer for receiving future SPI Messages and also knocks
 * over the queue so that the next message can be processed.
 *
 */
void SPI_DequeueFullBuf(void);

/**
 * \brief Query whether the SPI message FIFO is full or not
 *
 * If it is full, then messages should not be further read from the slave.
 *
 * \retval true if FIFO is full, false if there is space for another message
 *
 */
bool SPI_IsFifoFull(void);

/**
 * \brief Query whether the SPI message FIFO is almost full or not
 *
 * If it is almost full, then messages should not be further read from the slave asynchronously.
 *
 * \retval true if FIFO is almost full, false if there is space for another 2 messages
 *
 */
bool SPI_IsFifoAlmostFull(void);

/**
 * \brief Query whether the SPI message FIFO is empty or not
 *
 * \retval true if FIFO is empty, false if there are any messages stored
 *
 */
bool SPI_IsFifoEmpty(void);

/**
 * \brief Initialise SPI buffers and call BSP SPI init
 *
 */
void SPI_Initialise(void);

#endif // CASCODA_SPI_H
