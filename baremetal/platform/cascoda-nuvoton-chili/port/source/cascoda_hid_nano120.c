/**
 * @file cascoda_hid_nano120.c
 * @brief USB HID code to port to Nuvoton platform
 * @author Peter Burnett
 * @date 27/08/14
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

//System
#include <string.h>

//Platform
#include "Nano100Series.h"
#include "sys.h"
#include "usbd.h"

//Cascoda
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_usb.h"
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili.h"
#include "cascoda_chili_usb.h"

#if defined(USE_USB)

#if defined(USBTRACE)
void SN(uint8_t Item);
void SA(uint8_t *Array, uint8_t Len);
void SS(uint8_t *str);
#else
#define SN(x)
#define SA(x, y)
#define SS(x)
#endif

#if defined(__GNUC__)
#define __disable_interrupt __disable_irq
#define __enable_interrupt __enable_irq
#endif

void HID_GetOutReport(uint8_t *pu8EpBuf, uint32_t u32Size);
void HID_SetInReport(void);

/*-------------------------------------------------------------*/
/* Define EP maximum packet size */
#define EP0_MAX_PKT_SIZE HID_CTRL_MAX_SIZE
#define EP1_MAX_PKT_SIZE EP0_MAX_PKT_SIZE
#define EP2_MAX_PKT_SIZE HID_FRAGMENT_SIZE
#define EP3_MAX_PKT_SIZE HID_FRAGMENT_SIZE

#define SETUP_BUF_BASE 0
#define SETUP_BUF_LEN 8
#define EP0_BUF_BASE (SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP0_BUF_LEN EP0_MAX_PKT_SIZE
#define EP1_BUF_BASE (SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP1_BUF_LEN EP1_MAX_PKT_SIZE
#define EP2_BUF_BASE (EP1_BUF_BASE + EP1_BUF_LEN)
#define EP2_BUF_LEN EP2_MAX_PKT_SIZE
#define EP3_BUF_BASE (EP2_BUF_BASE + EP2_BUF_LEN)
#define EP3_BUF_LEN EP3_MAX_PKT_SIZE

#define TBUFFS (8) //!< Number of HID transmit buffers
#define RBUFFS (8) //!< Number of HID receive buffers

extern u8_t *USB_STRINGS[]; //!< 2d array of USB string descriptors

u8_t Connected = 0;                             //!< True if the HID interface is active
u8_t TxStalled = 1;                             //!< True if there are no buffers left to transmit
u8_t TransmitBuffer[TBUFFS][HID_FRAGMENT_SIZE]; //!< Buffers for storing fragments to send
u8_t ReceiveBuffer[RBUFFS][HID_FRAGMENT_SIZE];  //!< Buffers for storing received fragments

u8_t NextTransmitBufferToGo;   //!< Index of TransmitBuffer to transmit next
u8_t NextTransmitBufferToFill; //!< Index of TransmitBuffer to fill next
u8_t FullTransmitBuffers;      //!< Number of populated indices in TransmitBuffer
u8_t NextReceiveBufferToGo;    //!< Index of ReceiveBuffer to read next
u8_t NextReceiveBufferToFill;  //!< Index of ReceiveBuffer to fill next
u8_t FullReceiveBuffers;       //!< Number of populated indices in ReceiveBuffer

u8_t HidReportSize = HID_RPT_SIZE;

uint8_t *HidReport[3] = {(uint8_t *)HidReportDescriptor, NULL, NULL};

uint32_t HidReportLen[3] = {sizeof(HidReportDescriptor), 0, 0};

uint32_t ConfigHidDescIdx[3] = {sizeof(UsbConfigurationDescriptor_t) + sizeof(UsbInterfaceDescriptor_t), 0, 0};

S_USBD_INFO_T hidInfo = {
    (uint8_t *)&HidDeviceDescriptor, (uint8_t *)&HidConfDesc, USB_STRINGS, HidReport, HidReportLen, ConfigHidDescIdx};

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialises the HID system
 *******************************************************************************
 ******************************************************************************/
void HID_Initialise(void)
{
	NextTransmitBufferToGo = NextTransmitBufferToFill = FullTransmitBuffers = 0;
	NextReceiveBufferToGo = NextReceiveBufferToFill = FullReceiveBuffers = 0;

	USBD_Open(&hidInfo, HID_ClassRequest, NULL);

	USBD->BUFSEG = SETUP_BUF_BASE;

	/* EP0 ==> control IN endpoint, address 0 */
	USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
	USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);
	/* EP1 ==> control OUT endpoint, address 0 */
	USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
	USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);
	/* EP2 ==> Interrupt IN endpoint, address 1 */
	USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | HID_IN_EP_NO);
	USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);
	/* EP3 ==> Interrupt OUT endpoint, address 2 */
	USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | HID_OUT_EP_NO);
	USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);

	NVIC_EnableIRQ(USBD_IRQn);
	USBD_Start();

} // End of HID_Initialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Process USB class request
 *******************************************************************************
 * SET_REPORT
 * SET_IDLE
 * SET_PROTOCOL (Not used)
 *******************************************************************************
 ******************************************************************************/
void HID_ClassRequest(void)
{
	uint8_t buf[8];

	USBD_GetSetupPacket(buf);

	switch (buf[1])
	{
	/* SET_REPORT */
	case HID_SET_REPORT:
	{
		if ((buf[6] != 0) || (buf[7] != 0))
			break;

		if (buf[3] == 2)
		{
			/* Request Type = Output */
			USBD_SET_DATA1(EP1);
			USBD_SET_PAYLOAD_LEN(EP1, buf[6]);

			USBD_PrepareCtrlIn(0, 0);
		}
		else if (buf[3] == 3)
		{
			/* Request Type = Feature */
			USBD_SET_DATA1(EP1);
			USBD_SET_PAYLOAD_LEN(EP1, 0);
		}
		else
		{
			/* Not support. Reply STALL. */
			USBD_SetStall(0);
			//                USBD_SetStall(1);
		}

		break;
	}

	/* SET_IDLE */
	case HID_SET_IDLE:
	{
		USBD_SET_DATA1(EP0);
		USBD_SET_PAYLOAD_LEN(EP0, 0);
		break;
	}

	default:
		/* Setup error, stall the device */
		USBD_SetStall(0);
		//            USBD_SetStall(1);
		break;
	}
} // End of USB_ClassRequest()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Interrupt pipe IN
 *******************************************************************************
 * This function is the ISR for when a HID packet has finished being transmitted
 * to the master.
 *******************************************************************************
 * \return 0
 *******************************************************************************
 ******************************************************************************/
uint32_t HID_Ep2Handler(void)
{
	SS("SEND");
	HID_SetInReport();
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Interrupt pipe OUT
 *******************************************************************************
 * This function is the ISR for when a HID packet has finished being received
 * from the master.
 *******************************************************************************
 * \return 0
 *******************************************************************************
 ******************************************************************************/
uint32_t HID_Ep3Handler(void)
{
	u8_t *ptr;
	/* Interrupt OUT */
	SS("RECV");
	ptr = (u8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
	HID_GetOutReport(ptr, USBD_GET_PAYLOAD_LEN(EP3));
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
	if (SerialGetCommand())
	{
		SerialRxPending = true;
	}
	if (TxStalled && FullTransmitBuffers)
		HID_SetInReport();
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Process incoming HID buffer (OUT of host)
 *******************************************************************************
 * \param pu8EpBuf - Pointer to received packet
 * \param u32Size - Size of packet in pu8EpBuf
 *******************************************************************************
 ******************************************************************************/
void HID_GetOutReport(uint8_t *pu8EpBuf, uint32_t u32Size)
{
	if (FullReceiveBuffers >= RBUFFS)
	{
		//No free buffers
		//TODO: Do something here! Debug message that chunk was lost?
		return;
	}
	Connected = 1;
	USBD_MemCopy(&ReceiveBuffer[NextReceiveBufferToFill][0], pu8EpBuf, HID_FRAGMENT_SIZE);
	SA(&ReceiveBuffer[NextReceiveBufferToFill][0], 16);
	if (++NextReceiveBufferToFill >= RBUFFS)
	{
		NextReceiveBufferToFill = 0;
	}
	FullReceiveBuffers++;
	/* TODO: If FullReceiveBuffers == RBUFFS then stall the HID device until
	 * freed */
	return;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Transmit next outgoing HID buffer (IN to host)
 *******************************************************************************
 ******************************************************************************/
void HID_SetInReport(void)
{
	u8_t *ptr;

	if (FullTransmitBuffers == 0)
	{
		// Nothing to go
		TxStalled = 1;
		return;
	}
	TxStalled = 0;
	ptr       = (u8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));
	USBD_MemCopy(ptr, &TransmitBuffer[NextTransmitBufferToGo][0], HID_FRAGMENT_SIZE);
	USBD_SET_PAYLOAD_LEN(EP2, HID_FRAGMENT_SIZE);
	SA(&TransmitBuffer[NextTransmitBufferToGo][0], 16);
	if (++NextTransmitBufferToGo >= TBUFFS)
	{
		NextTransmitBufferToGo = 0;
	}
	FullTransmitBuffers--;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Put HID fragment into transmit buffers and trigger transmission if
 *        stalled
 *******************************************************************************
 * \param pBuffer - Pointer to fragment to transmit
 *******************************************************************************
 * \return 1 if successful, 0 if buffers are full
 *******************************************************************************
 ******************************************************************************/
u8_t HID_Transmit(u8_t *pBuffer)
{
	__disable_interrupt();
	if (Connected && FullTransmitBuffers >= TBUFFS)
	{
		__enable_interrupt();
		return 0;
	}
	memcpy(&TransmitBuffer[NextTransmitBufferToFill][0], pBuffer, HID_FRAGMENT_SIZE);
	if (++NextTransmitBufferToFill >= TBUFFS)
	{
		NextTransmitBufferToFill = 0;
	}
	if (FullTransmitBuffers < TBUFFS)
	{
		FullTransmitBuffers++;
	}
	else if (++NextTransmitBufferToGo >= TBUFFS)
	{
		NextTransmitBufferToGo = 0;
	}
	if (Connected && TxStalled)
	{
		HID_SetInReport();
	}
	__enable_interrupt();
	return 1;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Extract next HID fragment from receive buffers
 *******************************************************************************
 * \param pBuffer - Pointer to populate with next fragment
 *******************************************************************************
 * \return 1 if successful, 0 if buffers are empty
 *******************************************************************************
 ******************************************************************************/
u8_t HID_Receive(u8_t *pBuffer)
{
	__disable_interrupt();
	if (FullReceiveBuffers == 0)
	{
		__enable_interrupt();
		return 0;
	}
	memcpy(pBuffer, &ReceiveBuffer[NextReceiveBufferToGo][0], HID_FRAGMENT_SIZE);
	if (++NextReceiveBufferToGo >= RBUFFS)
	{
		NextReceiveBufferToGo = 0;
	}
	FullReceiveBuffers--;
	__enable_interrupt();
	return 1;
}

void HID_SetConnectedFlag(bool is_connected)
{
	Connected = is_connected;
}
#endif
