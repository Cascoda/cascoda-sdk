/**
 * @file   cascoda_serial.h
 * @brief  Serial Upstream Communication Driver Definitions/Declarations
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
#include <stdbool.h>
#include "cascoda-bm/cascoda_types.h"

#ifndef CASCODA_SERIAL_H
#define CASCODA_SERIAL_H

#define SERIAL_HDR_LEN (2) //!< Length of serial message header
#define SERIAL_CMD_ID (0)  //!< Position of command ID in serial header
#define SERIAL_CMD_LEN (1) //!< Position of length in serial header
#if !defined(SERIAL_MAC_RX_LEN)
#define SERIAL_MAC_RX_LEN (200) //!< Serial transfer payload length
#endif

#define SERIAL_RX_BUFFER_LEN (3)
#define SERIAL_SOM (0xDE)

/** Structure of serial transfers */
struct SerialBuffer
{
	/** Start of Message, should be 0xDE */
	u8_t SOM;
	/** Header according to API specification */
	u8_t Header[SERIAL_HDR_LEN];
	/** Data payload */
	u8_t Data[SERIAL_MAC_RX_LEN];
};

struct ca821x_dev;

/******************************************************************************/
/****** Global Variables for Serial Message Buffers                      ******/
/******************************************************************************/
extern u8_t                Serial_Stdout;
extern struct SerialBuffer SerialRxBuffer[SERIAL_RX_BUFFER_LEN];
extern volatile int        SerialRxWrIndex, SerialRxRdIndex, SerialRxCounter;
extern volatile bool       SerialRxPending;

/** Function pointer called when a serial message is received. Should be
 *  populated by applications wishing to use the serial interface */
extern int (*cascoda_serial_dispatch)(u8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** Functions in cascoda_serial_uart.c / cascoda_serial_usb.c        ******/
/******************************************************************************/
u8_t SerialGetCommand(void);
void MAC_Message_USB(u8_t CommandId, u8_t Count, u8_t *pBuffer);
void MAC_Message_UART(u8_t CommandId, u8_t Count, u8_t *pBuffer);
void EVBME_Message_USB(char *pBuffer, size_t Count, void *pDeviceRef);
void EVBME_Message_UART(char *pBuffer, size_t Count, void *pDeviceRef);

#endif // CASCODA_SERIAL_H
