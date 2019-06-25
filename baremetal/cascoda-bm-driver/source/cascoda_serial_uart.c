/**
 * @file   cascoda_serial_uart.c
 * @brief  Serial Communication Driver Functions (UART)
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

#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"

#if defined(USE_UART)

/******************************************************************************/
/****** Definitions for Serial State                                     ******/
/******************************************************************************/
enum serial_state
{
	SERIAL_INBETWEEN = 0,
	SERIAL_CMDID     = 1,
	SERIAL_CMDLEN    = 2,
	SERIAL_DATA      = 3,
};

#define SERIAL_SOM (0xDE)

#define EVBME_UART_RXRDY (0xAA) /* serial rx_rdy handshake command (both ways) */
#define SERIAL_TIMEOUT 1000     /* serial rx_rdy timeout in [ms] */

/******************************************************************************/
/****** Global Variables for Serial State                                ******/
/******************************************************************************/
static u8_t       SerialCount;                      //!< Number of bytes read so far
static u8_t       SerialRemainder;                  //!< Number of bytes left to receive
enum serial_state SerialIfState = SERIAL_INBETWEEN; //!< State of serial reading state machine

/* combine Serial Buffer and Framing for UART transfers */
struct SerialUARTBuffer
{
	u8_t                SofPkt;    /* Start of Packet Delimiter */
	struct SerialBuffer SerialBuf; /* Serial Buffer */
};

/******************************************************************************/
/****** Global Variables for Serial Message Buffers                      ******/
/******************************************************************************/
struct SerialBuffer            SerialRxBuffer;          //!< Global buffer for received serial messages
static struct SerialUARTBuffer SerialTxBuffer;          //!< Global buffer for transmitted serial messages
volatile bool                  SerialRxPending = false; //!< Receive Packet is ready
static volatile bool           SerialTxStalled = false; //!< Transmit waiting for RxRdy
static u32_t                   SerialTxTimeout = 0;     //!< RxRdy timeout

static u8_t SerialCmdId  = 0xFF;
static u8_t SerialCmdLen = 0;

/* Local Functions */
static u8_t SerialFindStart(void);
static u8_t SerialReceivedRxRdy(void);
static void SerialSetTxTimeout(void);
static void SerialCheckTxTimeout(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Find start of block from Serial input
 *******************************************************************************
 * \return 1 if start found , 0 if not
 *******************************************************************************
 ******************************************************************************/
static u8_t SerialFindStart(void)
{
	u8_t InputChar;

	if (BSP_SerialRead(&InputChar, 1) != 0)
	{
		if (InputChar == SERIAL_SOM)
		{
			return 1;
		}
	}
	return 0;
} // End of SerialFindStart()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Read in next Command from Serial hardware
 *******************************************************************************
 * \return 1 if Command ready, 0 if not
 *******************************************************************************
 ******************************************************************************/
u8_t Serial_ReadInterface(void)
{
	u8_t Count;

	while (1)
	{
		switch (SerialIfState)
		{
		case SERIAL_INBETWEEN:
			if (SerialFindStart())
			{
				SerialIfState = SERIAL_CMDID;
				continue;
			}
			return 0;
		case SERIAL_CMDID:
			if ((Count = BSP_SerialRead(&SerialCmdId, 1)) != 0)
			{
				SerialIfState = SERIAL_CMDLEN;
				continue;
			}
			return 0;
		case SERIAL_CMDLEN:
			if ((Count = BSP_SerialRead(&SerialCmdLen, 1)) != 0)
			{
				if (SerialReceivedRxRdy())
				{
					SerialIfState = SERIAL_INBETWEEN;
					return 1;
				}
				else
				{
					SerialRxBuffer.CmdId  = SerialCmdId;
					SerialRxBuffer.CmdLen = SerialCmdLen;
					SerialRemainder       = SerialCmdLen;
					SerialCount           = 0;
					if (SerialRemainder == 0)
					{
						SerialIfState   = SERIAL_INBETWEEN; // exit if 0 API packet length !!
						SerialRxPending = true;
						return 1;
					}
					else
					{
						SerialIfState = SERIAL_DATA;
						continue;
					}
				}
			}
			return 0;
		case SERIAL_DATA:
			if ((Count = BSP_SerialRead(SerialRxBuffer.Data + SerialCount, SerialRemainder)) != 0)
			{
				SerialCount += Count;
				SerialRemainder -= Count;
				if (SerialRemainder == 0)
				{
					SerialIfState   = SERIAL_INBETWEEN;
					SerialRemainder = 0;
					SerialCount     = 0;
					SerialRxPending = true;
					return 1;
				}
			}
			return 0;
		}
	}
} // End of Serial_ReadInterface()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Load next command into SerialRxBuffer if possible
 * Note: Not applicable to UART, as read into buffer via interrupt.
 *******************************************************************************
 * \return 1 if Command ready, 0 if not
 *******************************************************************************
 ******************************************************************************/
u8_t SerialGetCommand(void)
{
	return SerialRxPending;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Serial Read
 *******************************************************************************
 ******************************************************************************/
void SerialReadComplete(void)
{
	SerialIfState   = SERIAL_INBETWEEN;
	SerialRemainder = 0;
	SerialCount     = 0;
	SerialRxPending = true;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send RX_RDY Handshake Packet
 *******************************************************************************
 ******************************************************************************/
void SerialSendRxRdy(void)
{
	uint8_t buf[3];

	buf[0] = SERIAL_SOM;       /* start-of-packet delimiter */
	buf[1] = EVBME_UART_RXRDY; /* CmdId  = EVBME_UART_RXRDY */
	buf[2] = 0x00;             /* CmdLen = 0 */
	BSP_SerialWriteAll(buf, 3);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send RX_RDY Handshake Packet
 *******************************************************************************
 * \return 1 if RX_RDY received, 0 if not
 *******************************************************************************
 ******************************************************************************/
static u8_t SerialReceivedRxRdy(void)
{
	if ((SerialCmdId == EVBME_UART_RXRDY) && (SerialCmdLen == 0))
	{
		SerialTxStalled = false;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets RX_RDY Handshake Tx Timeout
 *******************************************************************************
 ******************************************************************************/
static void SerialSetTxTimeout(void)
{
	SerialTxStalled = true;
	SerialTxTimeout = TIME_ReadAbsoluteTime();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks RX_RDY Handshake Tx Timeout
 *******************************************************************************
 ******************************************************************************/
static void SerialCheckTxTimeout(void)
{
	while (SerialTxStalled == true)
	{
		if ((TIME_ReadAbsoluteTime() - SerialTxTimeout) > SERIAL_TIMEOUT)
		{
			SerialTxStalled = false;
			break;
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send EVBME_MESSAGE_Indication Upstream
 *******************************************************************************
 * \param pBuffer - Pointer to Character Buffer
 * \param Count - Number of Characters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void EVBME_Message_UART(char *pBuffer, size_t Count, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	SerialCheckTxTimeout();
	SerialTxBuffer.SofPkt           = SERIAL_SOM;
	SerialTxBuffer.SerialBuf.CmdId  = 0xA0;
	SerialTxBuffer.SerialBuf.CmdLen = Count;
	memcpy(SerialTxBuffer.SerialBuf.Data, pBuffer, Count);
	BSP_SerialWriteAll(&SerialTxBuffer.SofPkt, Count + 3);
	SerialSetTxTimeout();
} // End of EVBME_Message()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send MCPS or MLME confirm or indication Upstream
 *******************************************************************************
 * \param CommandId - command id of confirm or indication
 * \param Count - Number of Characters
 * \param pBuffer - Pointer to Character Buffer
 *******************************************************************************
 ******************************************************************************/
void MAC_Message_UART(u8_t CommandId, u8_t Count, const u8_t *pBuffer)
{
	SerialCheckTxTimeout();
	SerialTxBuffer.SofPkt           = SERIAL_SOM;
	SerialTxBuffer.SerialBuf.CmdId  = CommandId;
	SerialTxBuffer.SerialBuf.CmdLen = Count;
	memcpy(SerialTxBuffer.SerialBuf.Data, pBuffer, Count);
	BSP_SerialWriteAll(&SerialTxBuffer.SofPkt, Count + 3);
	SerialSetTxTimeout();
} // End of MAC_Message()

#endif
