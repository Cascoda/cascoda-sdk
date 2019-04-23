/**
 * @file   cascoda_serial_usb.c
 * @brief  Serial Communication Driver Functions (USB)
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

#if defined(USE_USB)

/******************************************************************************/
/****** Message id for standard messages to Wing commander               ******/
/****** EVBME_MESSAGE_INDICATION for log console                         ******/
/****** EVBME_LOG_INDICATION for terminal console                        ******/
/******************************************************************************/
u8_t Serial_Stdout = EVBME_MESSAGE_INDICATION;

/******************************************************************************/
/****** Local Defines for fragmented USB Packets                         ******/
/******************************************************************************/
#define USB_FRAG_SIZE (64)
#define USB_MAX_DATA (USB_FRAG_SIZE - 1)
#define USB_FRAG_FIRST (0x40)
#define USB_FRAG_LAST (0x80)

/******************************************************************************/
/****** Global Variables for buffering fragmented USB Packets            ******/
/******************************************************************************/
u8_t UsbTxFrag[USB_FRAG_SIZE];

/******************************************************************************/
/****** Global Variables for Serial Message Buffers                      ******/
/******************************************************************************/
struct SerialBuffer SerialRxBuffer; //Must be protected by locks (enable/disable serial IRQ)
volatile bool       SerialRxPending = false;

// dispatch function
int (*cascoda_serial_dispatch)(u8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send confirm or ind via USB
 *******************************************************************************
 * \param Command - Command ID of message
 * \param pBuffer - Pointer to message Buffer
 * \param Count - Length of pBuffer
 *******************************************************************************
 ******************************************************************************/
void SerialUSBSend(u8_t Command, u8_t *pBuffer, u8_t Length)
{
	u8_t FragSize;
	u8_t FragType;
	u8_t FragOffset;
	u8_t SizeLeft;

	UsbTxFrag[1] = Command;
	UsbTxFrag[2] = Length;
	FragOffset   = 3;

	for (FragType = USB_FRAG_FIRST; Length; FragType = 0, FragOffset = 1)
	{
		SizeLeft = USB_FRAG_SIZE - FragOffset;
		FragSize = (Length < SizeLeft) ? Length : SizeLeft;

		memcpy(UsbTxFrag + FragOffset, pBuffer, FragSize);
		memset(UsbTxFrag + FragOffset + FragSize, 0, SizeLeft - FragSize);
		Length -= FragSize;
		pBuffer += FragSize;
		FragType |= (FragSize + FragOffset - 1);
		if (Length == 0)
		{
			FragType |= USB_FRAG_LAST;
		}
		UsbTxFrag[0] = FragType;
		BSP_USBSerialWrite(UsbTxFrag);
	}
} // End of SerialUSBSend()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Read in next Command from Serial
 *******************************************************************************
 * \param buf - Buffer pointer to populate with received buffer location
 * \return 1 if Command ready, 0 if not
 *******************************************************************************
 ******************************************************************************/
u8_t SerialGetCommand(void)
{
	//TODO: Rval not useful
	u8_t        Count;
	u8_t *      UsbRxFrag;
	static u8_t SerialCount = 0;

	if (SerialRxPending)
		return 1;

	while (1)
	{
		u8_t ControlByte;
		if ((UsbRxFrag = BSP_USBSerialRxPeek()) == NULL)
		{
			return 0;
		}
		ControlByte = UsbRxFrag[0];
		Count       = ControlByte & 0x3F;

		if (ControlByte & USB_FRAG_FIRST)
		{
			SerialRxBuffer.CmdId  = UsbRxFrag[1];
			SerialRxBuffer.CmdLen = UsbRxFrag[2];
			SerialCount           = Count - 2; // take off header size
			memcpy(SerialRxBuffer.Data, UsbRxFrag + 3, SerialCount);
		}
		else
		{
			if (((u16_t)SerialCount + Count) > sizeof(SerialRxBuffer.Data))
				return 0;
			memcpy(SerialRxBuffer.Data + SerialCount, UsbRxFrag + 1, Count);
			SerialCount += Count;
		}

		BSP_USBSerialRxDequeue();
		if (ControlByte & USB_FRAG_LAST)
		{
			SerialRxPending = true;
			SerialCount     = 0;
			return 1;
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
void EVBME_Message_USB(char *pBuffer, size_t Count, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	SerialUSBSend(Serial_Stdout, (u8_t *)pBuffer, Count);
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
void MAC_Message_USB(u8_t CommandId, u8_t Count, u8_t *pBuffer)
{
	SerialUSBSend(CommandId, pBuffer, Count);

} // End of MAC_Message()

#endif // USE_USB
