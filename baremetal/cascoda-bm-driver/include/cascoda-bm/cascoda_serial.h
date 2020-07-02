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
/**
 * @file
 * @brief  Function definitions for communicating with a host using UART/USB
 */

#ifndef CASCODA_SERIAL_H
#define CASCODA_SERIAL_H

#include <stdbool.h>
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(SERIAL_MAC_RX_LEN)
#define SERIAL_MAC_RX_LEN (200) //!< Serial transfer payload length
#endif

/** Structure of serial transfers */
struct SerialBuffer
{
	/** Header according to API specification */
	u8_t CmdId;
	u8_t CmdLen;
	/** Data payload */
	u8_t Data[SERIAL_MAC_RX_LEN];
};

/******************************************************************************/
/****** Global Variables for Serial Message Buffers                      ******/
/******************************************************************************/
extern struct SerialBuffer SerialRxBuffer;
extern volatile bool       SerialRxPending;

/** Function pointer called when a serial message is received. Should be
 *  populated by applications wishing to use the serial interface */
extern int (*cascoda_serial_dispatch)(u8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** Functions in cascoda_serial_uart.c / cascoda_serial_usb.c        ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Load next command into SerialRxBuffer if possible
 * Note: Not applicable to UART, as read into buffer via interrupt.
 *******************************************************************************
 * \return 1 if Command ready, 0 if not
 *******************************************************************************
 ******************************************************************************/
u8_t SerialGetCommand(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Read in next Command from Serial hardware
 *******************************************************************************
 * \return 1 if Command ready, 0 if not
 *******************************************************************************
 ******************************************************************************/
u8_t Serial_ReadInterface(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send MCPS or MLME confirm or indication Upstream
 *******************************************************************************
 * \param CommandId - command id of confirm or indication
 * \param Count - Number of Characters
 * \param pBuffer - Pointer to Character Buffer
 *******************************************************************************
 ******************************************************************************/
void MAC_Message_USB(u8_t CommandId, u8_t Count, const u8_t *pBuffer);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send MCPS or MLME confirm or indication Upstream
 *******************************************************************************
 * \param CommandId - command id of confirm or indication
 * \param Count - Number of Characters
 * \param pBuffer - Pointer to Character Buffer
 *******************************************************************************
 ******************************************************************************/
void MAC_Message_UART(u8_t CommandId, u8_t Count, const u8_t *pBuffer);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send EVBME_MESSAGE_Indication Upstream
 *******************************************************************************
 * \param pBuffer - Pointer to Character Buffer
 * \param Count - Number of Characters
 *******************************************************************************
 ******************************************************************************/
void EVBME_Message_USB(char *pBuffer, size_t Count);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send EVBME_MESSAGE_Indication Upstream
 *******************************************************************************
 * \param pBuffer - Pointer to Character Buffer
 * \param Count - Number of Characters
 *******************************************************************************
 ******************************************************************************/
void EVBME_Message_UART(char *pBuffer, size_t Count);
#if defined(USE_UART)
/**
 * Send an EVBME_RXRDY message to signal receive success
 */
void SerialSendRxRdy(void);

/**
 * Send an EVBME_RXFAIL message to signal receive fail
 */
void SerialSendRxFail(void);

/**
 * For an asynchronous (DMA) read, call this when the DMA transfer is complete.
 */
void SerialReadComplete(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // CASCODA_SERIAL_H
