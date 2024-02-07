/**
 * @file
 * @brief  Serial Communication Driver Functions (USB)
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

#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "evbme_messages.h"

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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send confirm or ind via USB
 *******************************************************************************
 * \param Command - Command ID of message
 * \param pBuffer - Pointer to message Buffer
 * \param Count - Length of pBuffer
 *******************************************************************************
 ******************************************************************************/
static void SerialUSBSend(u8_t Command, const u8_t *pBuffer, u8_t Length)
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

u8_t SerialGetCommand(void)
{
	//TODO: Rval not useful
	u8_t        Count;
	u8_t       *UsbRxFrag;
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

void EVBME_Message_USB(char *pBuffer, size_t Count)
{
	/* check if interface is enabled */
	if (!BSP_IsCommsInterfaceEnabled())
		return;

	SerialUSBSend(Serial_Stdout, (u8_t *)pBuffer, Count);
} // End of EVBME_Message()

void MAC_Message_USB(u8_t CommandId, u8_t Count, const u8_t *pBuffer)
{
	/* check if interface is enabled */
	if (!BSP_IsCommsInterfaceEnabled())
		return;

	SerialUSBSend(CommandId, pBuffer, Count);

} // End of MAC_Message()

#endif // USE_USB
