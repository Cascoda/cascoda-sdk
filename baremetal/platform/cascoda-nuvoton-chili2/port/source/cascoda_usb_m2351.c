/*
 * Copyright (C) 2019  Cascoda, Ltd.
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
/*
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * USB Functions
*/
/* System */
#include <string.h>
/* Platform */
#include "M2351.h"
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

static uint8_t UsbBOSDescriptor[] = {LEN_BOS,                /* bLength */
                                     DESC_BOS,               /* bDescriptorType */
                                     (0x0C & 0x00FF),        /* wTotalLength */
                                     ((0x0C & 0xFF00) >> 8), /* bNumDeviceCaps */
                                     0x01,                   /* Device Capability */
                                     0x7,                    /* bLength */
                                     DESC_CAPABILITY,        /* bDescriptorType */
                                     CAP_USB20_EXT,          /* bDevCapabilityType */
                                     0x02,
                                     0x00,
                                     0x00,
                                     0x00}; /* bmAttributes */

static S_USBD_INFO_T hidInfo = {(uint8_t *)&HidDeviceDescriptor,
                                (uint8_t *)&HidConfDesc,
                                USB_STRINGS,
                                HidReport,
                                UsbBOSDescriptor,
                                HidReportLen,
                                ConfigHidDescIdx};

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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise USB interface
 *******************************************************************************
 ******************************************************************************/
void USB_Initialise(void)
{
	/* Enable PHY in Device Mode */
	SYS_UnlockReg();
	SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk;
	SYS_LockReg();

	/* Initialise HID mode */
	NextTransmitBufferToGo = NextTransmitBufferToFill = FullTransmitBuffers = 0;
	NextReceiveBufferToGo = NextReceiveBufferToFill = FullReceiveBuffers = 0;

	USBD_Open(&hidInfo, USB_ClassRequest, NULL);

	USBD->STBUFSEG = SETUP_BUF_BASE;

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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Deinitialise USB interface
 *******************************************************************************
 ******************************************************************************/
void USB_Deinitialise(void)
{
	/* Disable PHY */
	SYS_UnlockReg();
	USBD_DISABLE_USB();
	CLK_DisableModuleClock(USBD_MODULE);
	NVIC_DisableIRQ(USBD_IRQn);
	SYS->USBPHY &= ~SYS_USBPHY_OTGPHYEN_Msk;
	SYS_LockReg();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles USB plug/unplug events
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles USB state events (e.g. suspend/resume)
 *******************************************************************************
 ******************************************************************************/
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
		/* TODO: Do something here! Debug message that chunk was lost? */
		return;
	}
	Connected = 1;
	USBD_MemCopy(&ReceiveBuffer[NextReceiveBufferToFill][0], pu8EpBuf, HID_FRAGMENT_SIZE);
	if (++NextReceiveBufferToFill >= RBUFFS)
	{
		NextReceiveBufferToFill = 0;
	}
	FullReceiveBuffers++;
	/* TODO: If FullReceiveBuffers == RBUFFS then stall the HID device until freed */
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
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
	if (SerialGetCommand())
	{
		SerialRxPending = true;
	}
	if (TxStalled && FullTransmitBuffers)
		USB_SetInReport();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles USB transfer events (descriptors or application data)
 *******************************************************************************
 * \param u32INTSTS - Interrupt status bitmap
 *******************************************************************************
 ******************************************************************************/
void USB_UsbEvent(uint32_t u32INTSTS)
{
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

	if (u32INTSTS & USBD_INTSTS_EP4)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
	}

	if (u32INTSTS & USBD_INTSTS_EP5)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
	}

	if (u32INTSTS & USBD_INTSTS_EP6)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
	}

	if (u32INTSTS & USBD_INTSTS_EP7)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
	}
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
u8_t USB_Transmit(u8_t *pBuffer)
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
		USB_SetInReport();
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
u8_t USB_Receive(u8_t *pBuffer)
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set USB Connected Flag
 *******************************************************************************
 ******************************************************************************/
void USB_SetConnectedFlag(bool is_connected)
{
	Connected = is_connected;
}

#endif /* USE_USB */
