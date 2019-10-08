/**
 * @file
 * @brief  SPI Communication Driver Definitions/Declarations
 */
/*
 *  Copyright (c) 2019, Cascoda Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CASCODA_SPI_H
#define CASCODA_SPI_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/****** Definitions for SPI Handling                                     ******/

#define SPI_T_TIMEOUT 500   //!< Timeout for synchronous responses [ms]
#define SPI_T_BACKOFF 10    //!< Backoff Time when receiving SPI_NACK [us]
#define SPI_T_HOLD_8210 100 //!< Hold time before transmitting for the CA-8210 [us]
#define SPI_T_CSHOLD 50     //!< Time to hold the chip select active before re-checking the IRQ [us].

#define SPI_RX_FIFO_SIZE 7 //!< Maximum Size of Rx FIFO \ref SPI_Receive_Buffer
#define SPI_RX_FIFO_RESV 4 //!< Number of SPI RX FIFOs to be reserved for piggyback messages

#if !(SPI_RX_FIFO_RESV < SPI_RX_FIFO_SIZE)
#error "SPI_RX_FIFO_RESV must be less than SPI_RX_FIFO_SIZE"
#endif

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
ca_error SPI_Exchange(const struct MAC_Message *pTxBuffer, struct ca821x_dev *pDeviceRef);

/**
 * \brief Send Request over SPI
 *
 * This function is used by DISPATCH_ToCA821x as the downstream path to interface
 * with the CA-821x.
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
 * This is controlled by the SPI_RX_FIFO_RESV preprocessor macro.
 *
 * \retval true if FIFO is almost full, false if there is space for more async messages
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
 * \brief Query whether the SPI driver is currently locked in a Sync chain.
 *
 * Not for application usage.
 *
 * A Sync chain is a method to restrict the incoming messages from the CA-821x
 * on a constrained platform. If this is in action, then it is important that
 * messages are not received via IRQ. This function is used in cascoda_dispatch.c
 * to prevent sync chain responses from being processed.
 *
 * \retval true if sync chain is locked, false if unlocked.
 *
 */
bool SPI_IsSyncChainInFlight(void);

/**
 * Start a 'Sync Chain' For more efficient and safer chains of sync message comms
 * with the CA821x. This function should be called before starting to send a group
 * of synchronous messages, and SPI_StopSyncChain must be called afterwards.
 *
 * Asynchronous requests MUST NOT be made while the sync chain is active.
 * This prevents any asynchronous messages from being received, making timing and
 * buffers more consistent.
 *
 * @param pDeviceRef Pointer to initialised \ref ca821x_dev struct
 */
void SPI_StartSyncChain(struct ca821x_dev *pDeviceRef);

/**
 * Stop the sync chain after starting with SPI_StartSyncChain.
 *
 * @param pDeviceRef Pointer to initialised \ref ca821x_dev struct
 */
void SPI_StopSyncChain(struct ca821x_dev *pDeviceRef);

/**
 * \brief Initialise SPI buffers and call BSP SPI init
 *
 */
void SPI_Initialise(void);

#ifdef __cplusplus
}
#endif

#endif // CASCODA_SPI_H
