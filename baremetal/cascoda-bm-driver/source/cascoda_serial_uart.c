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
#if defined(USE_UART)

#include <string.h>
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"

/******************************************************************************/
/****** Definitions for Serial State                                     ******/
/******************************************************************************/
#define SERIAL_INBETWEEN (0)
#define SERIAL_HEADER (1)
#define SERIAL_DATA (2)

/******************************************************************************/
/****** Global Variables for Serial State                                ******/
/******************************************************************************/
u8_t SerialCount;                      //!< Number of bytes read so far
u8_t SerialRemainder;                  //!< Number of bytes left to receive
u8_t SerialIfState = SERIAL_INBETWEEN; //!< State of serial reading state machine

/******************************************************************************/
/****** Global Variables for Serial Message Buffers                      ******/
/******************************************************************************/
volatile struct SerialBuffer SerialRxBuffer[SERIAL_RX_BUFFER_LEN]; //!< Global buffer for received serial messages
struct SerialBuffer          SerialTxBuffer;                       //!< Global buffer for transmitted serial messages
volatile int                 SerialRxWrIndex = 0, SerialRxRdIndex = 0, SerialRxCounter = 0;
volatile bool                SerialRxPending = false;

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
 * \brief Read in next Command from Serial
 *******************************************************************************
 * \return 1 if Command ready, 0 if not
 *******************************************************************************
 ******************************************************************************/
u8_t SerialGetCommand(void)
{
	u8_t Count;

	while (1)
	{
		switch (SerialIfState)
		{
		case SERIAL_INBETWEEN:
			if (SerialFindStart())
			{
				if (SerialRxBuffer[SerialRxWrIndex].SOM == SERIAL_SOM)
				{
					SerialRxWrIndex++;
					if (SerialRxWrIndex >= SERIAL_RX_BUFFER_LEN)
					{
						SerialRxWrIndex = 0;
					}
					SerialRxCounter++;
					if (SerialRxCounter > SERIAL_RX_BUFFER_LEN)
					{
						SerialRxCounter = SERIAL_RX_BUFFER_LEN;
					}
				}
				SerialIfState                       = SERIAL_HEADER;
				SerialRemainder                     = SERIAL_HDR_LEN;
				SerialCount                         = 0;
				SerialRxBuffer[SerialRxWrIndex].SOM = SERIAL_SOM;
				continue;
			}
			return 0;
		case SERIAL_HEADER:
			if ((Count = BSP_SerialRead(SerialRxBuffer[SerialRxWrIndex].Header + SerialCount, SerialRemainder)) != 0)
			{
				SerialCount += Count;
				SerialRemainder -= Count;
				if (SerialRemainder == 0)
				{
					SerialRemainder = SerialRxBuffer[SerialRxWrIndex].Header[SERIAL_CMD_LEN];
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
			if ((Count = BSP_SerialRead(SerialRxBuffer[SerialRxWrIndex].Data + SerialCount, SerialRemainder)) != 0)
			{
				SerialCount += Count;
				SerialRemainder -= Count;
				if (SerialRemainder == 0)
				{
					SerialIfState   = SERIAL_INBETWEEN;
					SerialRemainder = 0;
					SerialCount     = 0;
					return 1;
				}
			}
			return 0;
		}
	}
} // End of SerialGetCommand()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send EVBME_MESSAGE_Indication Upstream
 *******************************************************************************
 * \param pBuffer - Pointer to Character Buffer
 * \param Count - Number of Characters
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void EVBME_Message_UART(char *pBuffer, size_t Count, void *pDeviceRef)
{
	SerialTxBuffer.Header[SERIAL_CMD_ID]  = 0xA0;
	SerialTxBuffer.Header[SERIAL_CMD_LEN] = Count;
	SerialTxBuffer.SOM                    = SERIAL_SOM;
	memcpy(SerialTxBuffer.Data, pBuffer, Count);
	BSP_SerialWriteAll(&SerialTxBuffer.SOM, Count + 1 + SERIAL_HDR_LEN);

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
	SerialTxBuffer.Header[SERIAL_CMD_ID]  = CommandId;
	SerialTxBuffer.Header[SERIAL_CMD_LEN] = Count;
	SerialTxBuffer.SOM                    = SERIAL_SOM;
	memcpy(SerialTxBuffer.Data, pBuffer, Count);
	BSP_SerialWriteAll(&SerialTxBuffer.SOM, Count + 1 + SERIAL_HDR_LEN);

} // End of MAC_Message()

#endif
