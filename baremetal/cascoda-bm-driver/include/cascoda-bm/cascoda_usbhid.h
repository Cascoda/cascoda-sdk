/**
 * @file   cascoda_usbhid.h
 * @brief  USB HID definitions
 * @author Peter Burnett
 * @date   27/08/14
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

#ifndef CASCODA_USBHID_H
#define CASCODA_USBHID_H

#include "cascoda-bm/cascoda_usb.h"

/******************************************************************************/
/****** Endpoints to be used                                             ******/
/******************************************************************************/
/** Endpoint number for IN events. IN is relative to the host in this case,
 *  which means this is actually *out* of this system. */
#define HID_IN_EP_NO (4)
/** Endpoint number forOUT events. OUT is relative to the host in this case,
 *  which means this is actually *in* to this system. */
#define HID_OUT_EP_NO (5)
/** Maximum size for messages exchanged over the CONTROL endpoints */
#define HID_CTRL_MAX_SIZE (64)
/** Size of fragments exchanged over the user data endpoints (\ref HID_IN_EP_NO
 *  and \ref HID_OUT_EP_NO) */
#define HID_FRAGMENT_SIZE (64)

/******************************************************************************/
/****** Size of HID report descriptor See below                          ******/
/******************************************************************************/

#define HID_RPT_SIZE (29)

/** HID descriptor */
typedef struct UsbHIDDescriptor
{
	u8_t bLength;            //!< Length in bytes ( of this )
	u8_t bDescriptorType;    //!< USB_DT_HID
	u8_t bcdHID[2];          //!< bcd HID spec version
	u8_t bCountryCode;       //!< Country code
	u8_t bNumDescriptors;    //!< HID class descs following
	u8_t bDescriptorTypeRpt; //!< Report descriptor type
	u8_t ReportDescSize[2];  //
} UsbHIDDescriptor_t;

/** Configuration descriptor for HID device */
typedef struct HidConfigurationDescriptor
{
	UsbConfigurationDescriptor_t Config;
	UsbInterfaceDescriptor_t     Interface;
	UsbHIDDescriptor_t           Hid;
	UsbEndpointDescriptor_t      EPIn;
	UsbEndpointDescriptor_t      EPOut;
} HidConfigurationDescriptor_t;

/******************************************************************************/
/****** HID Request codes                                                ******/
/******************************************************************************/
#define HID_GET_REPORT 0x01
#define HID_GET_IDLE 0x02
#define HID_GET_PROTOCOL 0x03
#define HID_SET_REPORT 0x09
#define HID_SET_IDLE 0x0A
#define HID_SET_PROTOCOL 0x0B

extern const UsbDeviceDescriptor_t        HidDeviceDescriptor;
extern const HidConfigurationDescriptor_t HidConfDesc;
extern const u8_t                         HidReportDescriptor[HID_RPT_SIZE];
extern u8_t                               USB_StringDescs[];

#endif // CASCODA_USBHID_H
