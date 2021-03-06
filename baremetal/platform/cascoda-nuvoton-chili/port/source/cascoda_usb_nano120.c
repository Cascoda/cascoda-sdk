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
/*
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton Nano120
 * MODULE: Chili 1 (1.2, 1.3)
 * USB Functions
*/
/* System */
#include <string.h>
/* Platform */
#include "Nano100Series.h"
#include "sys.h"
#include "usbd.h"
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_usb.h"
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili.h"
#include "cascoda_chili_usb.h"

#if defined(USE_USB)

#if defined(__GNUC__)
#define __disable_interrupt __disable_irq
#define __enable_interrupt __enable_irq
#endif /* __GNUC__ */

/******************************************************************************/
/****** String descriptors for this device                               ******/
/******************************************************************************/
/* Language string descriptor index 0 */
static u8_t USB_LANGUAGE[] = {4, USB_DT_STRING, 0x09, 0x00};
/* Manufacturer string descriptor index 1 */
static u8_t USB_MANUFACTURER[] = {16, USB_DT_STRING, 'C', 0, 'a', 0, 's', 0, 'c', 0, 'o', 0, 'd', 0, 'a', 0};
/* Product string descriptor index 2 offset 20 */
static u8_t USB_PRODUCT[] = {26, USB_DT_STRING, 'C', 0,   'h', 0,   'i', 0,   'l', 0,   'i', 0,   ' ',
                             0,  'M',           0,   'o', 0,   'd', 0,   'u', 0,   'l', 0,   'e', 0};
/* SerialNo string descriptor index 3 offset 46 */
static u8_t USB_SERIAL[] = {
    34, USB_DT_STRING, 'C', 0,   'A', 0,   '5', 0,   'C', 0,   '0', 0,   'D', 0,   'A', 0,   '0',
    0,  '1',           0,   '2', 0,   '3', 0,   '4', 0,   '5', 0,   '6', 0,   '7', 0,   '8', 0};

/* 2d array of USB string descriptors */
static u8_t *USB_STRINGS[] = {USB_LANGUAGE, USB_MANUFACTURER, USB_PRODUCT, USB_SERIAL};

/******************************************************************************/
/****** HID Mode                                                         ******/
/******************************************************************************/
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

#define TBUFFS (8) /* Number of HID transmit buffers */
#define RBUFFS (8) /* Number of HID receive  buffers */

static u8_t Connected = 0;                             /* True if the HID interface is active */
static u8_t TxStalled = 1;                             /* True if there are no buffers left to transmit */
static u8_t RxStalled = 0;                             /* True if there are no buffers left to receive */
static u8_t TransmitBuffer[TBUFFS][HID_FRAGMENT_SIZE]; /* Buffers for storing fragments to send */
static u8_t ReceiveBuffer[RBUFFS][HID_FRAGMENT_SIZE];  /* Buffers for storing received fragments */

static u8_t NextTransmitBufferToGo;   /* Index of TransmitBuffer to transmit next */
static u8_t NextTransmitBufferToFill; /* Index of TransmitBuffer to fill next */
static u8_t FullTransmitBuffers;      /* Number of populated indices in TransmitBuffer */
static u8_t NextReceiveBufferToGo;    /* Index of ReceiveBuffer to read next */
static u8_t NextReceiveBufferToFill;  /* Index of ReceiveBuffer to fill next */
static u8_t FullReceiveBuffers;       /* Number of populated indices in ReceiveBuffer */

static uint8_t *HidReport[3]        = {(uint8_t *)HidReportDescriptor, NULL, NULL};
static uint32_t HidReportLen[3]     = {sizeof(HidReportDescriptor), 0, 0};
static uint32_t ConfigHidDescIdx[3] = {sizeof(UsbConfigurationDescriptor_t) + sizeof(UsbInterfaceDescriptor_t), 0, 0};

static S_USBD_INFO_T hidInfo =
    {(uint8_t *)&HidDeviceDescriptor, (uint8_t *)&HidConfDesc, USB_STRINGS, HidReport, HidReportLen, ConfigHidDescIdx};

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Process USB class request
 *******************************************************************************
 ******************************************************************************/
static void USB_ClassRequest(void)
{
	uint8_t buf[8];

	USBD_GetSetupPacket(buf);

	/* SET_REPORT */
	/* SET_IDLE */
	/* SET_PROTOCOL (Not used) */
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
		break;
	}
}

void USB_Initialise(void)
{
	/* Initialise HID mode */
	NextTransmitBufferToGo = NextTransmitBufferToFill = FullTransmitBuffers = 0;
	NextReceiveBufferToGo = NextReceiveBufferToFill = FullReceiveBuffers = 0;

	USBD_Open(&hidInfo, USB_ClassRequest, NULL);

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
}

void USB_FloatDetect(void)
{
	USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

	if (USBD_IS_ATTACHED())
	{
		/* USB Plug In */
		USBD_ENABLE_USB();
	}
	else
	{
		/* USB Un-plug */
		USBD_DISABLE_USB();
	}
}

void USB_BusEvent(void)
{
	uint32_t u32State = USBD_GET_BUS_STATE();

	/* Clear event flag */
	USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

	if (u32State & USBD_STATE_USBRST)
	{
		/* Bus reset */
		USBD_ENABLE_USB();
		USBD_SwReset();
	}
	if (u32State & USBD_STATE_SUSPEND)
	{
		/* Enable USB but disable PHY */
		USBD_DISABLE_PHY();
	}
	if (u32State & USBD_STATE_RESUME)
	{
		/* Enable USB and enable PHY */
		USBD_ENABLE_USB();
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Transmit next outgoing HID buffer (IN to host)
 *******************************************************************************
 ******************************************************************************/
static void USB_SetInReport(void)
{
	u8_t *ptr;

	if (FullTransmitBuffers == 0)
	{
		/* Nothing to go */
		TxStalled = 1;
		return;
	}
	TxStalled = 0;
	ptr       = (u8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));
	USBD_MemCopy(ptr, &TransmitBuffer[NextTransmitBufferToGo][0], HID_FRAGMENT_SIZE);
	USBD_SET_PAYLOAD_LEN(EP2, HID_FRAGMENT_SIZE);
	if (++NextTransmitBufferToGo >= TBUFFS)
	{
		NextTransmitBufferToGo = 0;
	}
	FullTransmitBuffers--;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Process incoming HID buffer (OUT of host)
 *******************************************************************************
 * \param pu8EpBuf - Pointer to received packet
 * \param u32Size - Size of packet in pu8EpBuf
 *******************************************************************************
 ******************************************************************************/
static void USB_GetOutReport(uint8_t *pu8EpBuf, uint32_t u32Size)
{
	(void)u32Size;

	if (FullReceiveBuffers >= RBUFFS)
	{
		/* No free buffers */
		RxStalled = 1;
		return;
	}
	Connected = 1;
	RxStalled = 0;
	USBD_MemCopy(&ReceiveBuffer[NextReceiveBufferToFill][0], pu8EpBuf, HID_FRAGMENT_SIZE);
	if (++NextReceiveBufferToFill >= RBUFFS)
	{
		NextReceiveBufferToFill = 0;
	}
	FullReceiveBuffers++;
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
	return;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles IN (to host) transfer events
 *******************************************************************************
 ******************************************************************************/
static void USB_EP2_Handler()
{
	/* HID packet has finished being transmitted to the master */
	USB_SetInReport();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles OUT (from host) transfer events
 *******************************************************************************
 ******************************************************************************/
static void USB_EP3_Handler()
{
	u8_t *ptr;
	/* Interrupt OUT */
	ptr = (u8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
	/* HID packet has finished being received from the master */
	USB_GetOutReport(ptr, USBD_GET_PAYLOAD_LEN(EP3));
}

void USB_UsbEvent(uint32_t u32INTSTS)
{
	//Clear all EP flags
	USBD_CLR_INT_FLAG(USBD_INTSTS_USB);
	if (u32INTSTS & USBD_INTSTS_SETUP)
	{
		/* Setup packet */
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

		/* Clear the data IN/OUT ready flag of control end-points */
		USBD_STOP_TRANSACTION(EP0);
		USBD_STOP_TRANSACTION(EP1);

		USBD_ProcessSetupPacket();
	}

	/* EP events */
	if (u32INTSTS & USBD_INTSTS_EP0)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
		/* control IN */
		USBD_CtrlIn();
	}

	if (u32INTSTS & USBD_INTSTS_EP1)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);
		/* control OUT */
		USBD_CtrlOut();
	}

	if (u32INTSTS & USBD_INTSTS_EP2)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
		/* Interrupt IN */
		USB_EP2_Handler();
	}

	if (u32INTSTS & USBD_INTSTS_EP3)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
		/* Interrupt OUT */
		USB_EP3_Handler();
	}
}

u8_t USB_Transmit(u8_t *pBuffer)
{
	__disable_interrupt();
	if (FullTransmitBuffers >= TBUFFS)
	{
		__enable_interrupt();
		return !Connected; //If we are connected, retry. If not, we don't care and will drop packet.
	}
	memcpy(&TransmitBuffer[NextTransmitBufferToFill][0], pBuffer, HID_FRAGMENT_SIZE);
	if (++NextTransmitBufferToFill >= TBUFFS)
	{
		NextTransmitBufferToFill = 0;
	}
	FullTransmitBuffers++;

	if (Connected && TxStalled)
	{
		USB_SetInReport();
	}
	__enable_interrupt();
	return 1;
}

u8_t *BSP_USBSerialRxPeek(void)
{
	__disable_interrupt();
	if (FullReceiveBuffers == 0)
	{
		__enable_interrupt();
		return NULL;
	}
	__enable_interrupt();
	return &ReceiveBuffer[NextReceiveBufferToGo][0];
}

void BSP_USBSerialRxDequeue(void)
{
	__disable_interrupt();
	if (++NextReceiveBufferToGo >= RBUFFS)
	{
		NextReceiveBufferToGo = 0;
	}
	FullReceiveBuffers--;
	if (RxStalled)
	{
		USB_EP3_Handler();
	}
	if (Connected && TxStalled)
	{
		USB_SetInReport();
	}
	__enable_interrupt();
}

void USB_SetConnectedFlag(bool is_connected)
{
	Connected = is_connected;
}

#endif /* USE_USB */
