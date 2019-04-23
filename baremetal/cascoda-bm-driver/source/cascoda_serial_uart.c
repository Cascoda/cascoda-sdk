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
#include "cascoda-bm/cascoda_types.h"

#if defined(USE_UART)

/******************************************************************************/
/****** Definitions for Serial State                                     ******/
/******************************************************************************/
enum serial_state
{
	SERIAL_INBETWEEN = 0,
	SERIAL_HEADER    = 1,
	SERIAL_DATA      = 2,
};

#define SERIAL_SOM (0xDE)

/******************************************************************************/
/****** Global Variables for Serial State                                ******/
/******************************************************************************/
u8_t              SerialCount;                      //!< Number of bytes read so far
u8_t              SerialRemainder;                  //!< Number of bytes left to receive
enum serial_state SerialIfState = SERIAL_INBETWEEN; //!< State of serial reading state machine

/******************************************************************************/
/****** Global Variables for Serial Message Buffers                      ******/
/******************************************************************************/
struct SerialBuffer SerialRxBuffer; //!< Global buffer for received serial messages
struct SerialBuffer SerialTxBuffer; //!< Global buffer for transmitted serial messages
volatile bool       SerialRxPending = false;

int (*cascoda_serial_dispatch)(u8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Find start of block from Serial input
 *******************************************************************************
 * \return 1 if start found , 0 if not
 *******************************************************************************
 ******************************************************************************/
u8_t SerialFindStart(void)
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
				SerialIfState   = SERIAL_HEADER;
				SerialRemainder = 2; //CmdId and CmdLen
				SerialCount     = 0;
				continue;
			}
			return 0;
		case SERIAL_HEADER:
			if ((Count = BSP_SerialRead(&SerialRxBuffer.CmdId + SerialCount, SerialRemainder)) != 0)
			{
				SerialCount += Count;
				SerialRemainder -= Count;
				if (SerialRemainder == 0)
				{
					SerialRemainder = SerialRxBuffer.CmdLen;
					SerialCount     = 0;
					if (SerialRemainder == 0)
					{
						SerialIfState = SERIAL_INBETWEEN; // exit if 0 API packet length !!
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
} // End of SerialGetCommand()

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
 * \brief Send EVBME_MESSAGE_Indication Upstream
 *******************************************************************************
 * \param pBuffer - Pointer to Character Buffer
 * \param Count - Number of Characters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void EVBME_Message_UART(char *pBuffer, size_t Count, struct ca821x_dev *pDeviceRef)
{
	uint8_t som = SERIAL_SOM;
	(void)pDeviceRef;

	SerialTxBuffer.CmdId  = 0xA0;
	SerialTxBuffer.CmdLen = Count;
	memcpy(SerialTxBuffer.Data, pBuffer, Count);
	BSP_SerialWriteAll(&som, 1);
	BSP_SerialWriteAll(&SerialTxBuffer.CmdId, Count + 2);

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
void MAC_Message_UART(u8_t CommandId, u8_t Count, u8_t *pBuffer)
{
	uint8_t som           = SERIAL_SOM;
	SerialTxBuffer.CmdId  = CommandId;
	SerialTxBuffer.CmdLen = Count;
	memcpy(SerialTxBuffer.Data, pBuffer, Count);
	BSP_SerialWriteAll(&som, 1);
	BSP_SerialWriteAll(&SerialTxBuffer.CmdId, Count + 2);

} // End of MAC_Message()

#endif
