/**
 * @file cascoda_usb_m2351.c
 * @brief Board Support Package (BSP) USB Code\n
 *        Micro: Nuvoton M2351\n
 *        Board: Chili
 * @author Wolfgang Bruchner
 * @date 09/08/15
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
//Platform
#include "M2351.h"
#include "usbd.h"

//Cascoda
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_usb.h"
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili.h"
#include "cascoda_chili_usb.h"

/******************************************************************************/
/****** String descriptors for this device                               ******/
/******************************************************************************/
u8_t USB_LANGUAGE[] = {
    // Language string descriptor index 0
    4, USB_DT_STRING, 0x09, 0x00};
u8_t USB_MANUFACTURER[] = {
    // Manufacturer string descriptor index 1
    16, USB_DT_STRING, 'C', 0, 'a', 0, 's', 0, 'c', 0, 'o', 0, 'd', 0, 'a', 0};
u8_t USB_PRODUCT[] = {
    // Product string descriptor index 2 offset 20
    26, USB_DT_STRING, 'C', 0, 'h', 0, 'i', 0, 'l', 0, 'i', 0, ' ', 0, 'M', 0, 'o', 0, 'd', 0, 'u', 0, 'l', 0, 'e', 0};
u8_t USB_SERIAL[] = {
    //SerialNo string descriptor index 3 offset 46
    34, USB_DT_STRING, 'C', 0,   'A', 0,   '5', 0,   'C', 0,   '0', 0,   'D', 0,   'A', 0,   '0',
    0,  '1',           0,   '2', 0,   '3', 0,   '4', 0,   '5', 0,   '6', 0,   '7', 0,   '8', 0};

u8_t *USB_STRINGS[] = {USB_LANGUAGE, USB_MANUFACTURER, USB_PRODUCT, USB_SERIAL};

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise USB interface
 *******************************************************************************
 ******************************************************************************/
void USB_Initialise(void)
{
	// Enable PHY in Device Mode
	SYS_UnlockReg();
	SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk;
	SYS_LockReg();

#if defined(USE_USB)
	HID_Initialise();
#endif // USE_USB
} // End of USB_Initialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Deinitialise USB interface
 *******************************************************************************
 ******************************************************************************/
void USB_Deinitialise(void)
{
	// Enable PHY in Device Mode
	SYS_UnlockReg();
	USBD_DISABLE_USB();
	CLK_DisableModuleClock(USBD_MODULE);
	NVIC_DisableIRQ(USBD_IRQn);
	SYS->USBPHY &= ~SYS_USBPHY_OTGPHYEN_Msk;
	SYS_LockReg();

} // End of USB_Denitialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Process USB class request
 *******************************************************************************
 * \return 1 for standard request, 0 for error
 *******************************************************************************
 * SET_REPORT\n
 * SET_IDLE\n
 * SET_PROTOCOL (Not used)
 *******************************************************************************
 ******************************************************************************/
uint32_t USB_ClassRequest(void)
{
#if defined(USE_USB)
	HID_ClassRequest();
	return 1;
#else
	return 0;
#endif // USE_USB
} // End of USB_ClassRequest()

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
 * \brief Handles IN (to host) transfer events
 *******************************************************************************
 ******************************************************************************/
static void EP2_Handler()
{
#if defined(USE_USB)
	HID_Ep2Handler();
#endif
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles OUT (from host) transfer events
 *******************************************************************************
 ******************************************************************************/
static void EP3_Handler()
{
#if defined(USE_USB)
	HID_Ep3Handler();
#endif
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
		// Setup packet
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

		/* Clear the data IN/OUT ready flag of control end-points */
		USBD_STOP_TRANSACTION(EP0);
		USBD_STOP_TRANSACTION(EP1);

		USBD_ProcessSetupPacket();
	}

	// EP events
	if (u32INTSTS & USBD_INTSTS_EP0)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
		// control IN
		USBD_CtrlIn();
	}

	if (u32INTSTS & USBD_INTSTS_EP1)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);
		// control OUT
		USBD_CtrlOut();
	}

	if (u32INTSTS & USBD_INTSTS_EP2)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
		// Interrupt IN
		EP2_Handler();
	}

	if (u32INTSTS & USBD_INTSTS_EP3)
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
		// Interrupt OUT
		EP3_Handler();
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
