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
/**
 * @file
 * @brief  Internal USB HID definitions
 */

#ifndef CASCODA_USBHID_H
#define CASCODA_USBHID_H

#include "cascoda-bm/cascoda-bm-config.h"
#include "cascoda-bm/cascoda_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

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

extern u8_t USB_StringDescs[];

#if defined(USE_USB)

#ifndef EP_INPUT
#define EP_INPUT (0x80)
#endif

#ifndef EP_OUTPUT
#define EP_OUTPUT (0x00)
#endif
/******************************************************************************/
/****** Device descriptor for HID device                                 ******/
/******************************************************************************/

static const UsbDeviceDescriptor_t HidDeviceDescriptor = {
    sizeof(UsbDeviceDescriptor_t), // bLength
    USB_DT_DEVICE,                 // bDescriptorType
    USB_BCDUSBVER,                 // bcdUSB version
    0,                             // bDeviceClass (in interface descriptor)
    0,                             // bDeviceSubClass
    0,                             // bDeviceProtocol
    HID_CTRL_MAX_SIZE,             // bMaxPacketSize0
    USB_IDVENDOR,                  // idVendor ( Nuvoton need Cascoda )
    USB_IDPRODUCT,                 // idProduct
    USB_BCDDEVVER,                 // bcdDevice version
    USB_IMANUFACTURER,             // iManufacture: String-Index of Manufacture
    USB_IPRODUCT,                  // iProduct: String-Index of Product
    USB_ISERIAL,                   // iSerialNumber: String-Index of Serial Number
    1                              // bNumConfigurations: Number of possible configurations
};

static const HidConfigurationDescriptor_t HidConfDesc = {
    {
        sizeof(UsbConfigurationDescriptor_t),      // bLength
        USB_DT_CONFIGURATION,                      // bDescriptorType
        {sizeof(HidConfigurationDescriptor_t), 0}, // wTotalLength
        0x01,                                      // bNumInterfaces: Number of interfaces
        0x01,                                      // iConfigurationValue: Number of this configuration
        0x00,                                      // iConfiguration: String index of this configuration
        0x80,                                      // bmAttributes: Bus-Powered, Remote-Wakeup not supported
        0x19                                       // MaxPower: (in 2mA) 25->50mA
    },
    {
        sizeof(UsbInterfaceDescriptor_t), // bLength
        USB_DT_INTERFACE,                 // bDescriptorType
        0x00,                             // bInterfaceNumber: Interface Number
        0x00,                             // bAlternateSetting: Alternate setting for this interface
        0x02,                             // bNumEndpoints: Number of endpoints in this interface excluding endpoint 0
        0x03,                             // iInterfaceClass: Class Code: HID
        0x00,                             // iInterfaceSubClass: SubClass Code
        0x00,                             // bInterfaceProtocol: Protocol Code
        0x00,                             // iInterface: String index
    },
    {
        sizeof(UsbHIDDescriptor_t), // bLength: Length of this descriptor
        USB_DT_HID,                 // bDescriptorType: HID descriptor
        {0x11, 0x01},               // bcdHID: version 1.11
        0x00,                       // bCountryCode
        0x01,                       // bNumDescriptors
        USB_DT_HID_RPT,             // bDescriptorType: Report desc type
        {HID_RPT_SIZE, 0x00}        // ReportDesc. Size 29 bytes
    },
    {
        sizeof(UsbEndpointDescriptor_t), // bLength: Length of this descriptor
        USB_DT_ENDPOINT,                 // bDescriptorType: Endpoint Descriptor Type
        HID_IN_EP_NO | EP_INPUT,         // bEndpointAddress: Endpoint address (IN,EP1)
        0x03,                            // bmAttributes: Transfer Type: INTERRUPT_TRANSFER
        {HID_FRAGMENT_SIZE, 0x00},       // wMaxPacketSize: Endpoint Size
        0x01                             // bInterval: Polling Interval
    },
    {
        sizeof(UsbEndpointDescriptor_t), // bLength: Length of this descriptor
        USB_DT_ENDPOINT,                 // bDescriptorType: Endpoint Descriptor Type
        HID_OUT_EP_NO | EP_OUTPUT,       // bEndpointAddress: Endpoint address (OUT,EP2)
        0x03,                            // bmAttributes: Transfer Type: INTERRUPT_TRANSFER
        {HID_FRAGMENT_SIZE, 0x00},       // wMaxPacketSize: Endpoint Size
        0x01                             // bInterval: Polling Interval
    }};

/******************************************************************************/
/****** Report descriptor for HID device                                 ******/
/******************************************************************************/

static const u8_t HidReportDescriptor[HID_RPT_SIZE] = {
    // Usage Page item specifying vendor defined
    0x06, // item tag bSize 2 bType 1 (global) bTag 0
    0x00,
    0xff, // data 0xFF00 vendor defined
    // Usage item specifying vendor usage 1
    0x09, // item tag bSize 1 bType 2 (local) bTag 0
    0x01, // data 0x01
    // Collection item specifying application
    0xA1, // item tag bSize 1 bType 0 (Main) bTag 0xA
    0x01, // data 0x01 (application)
    // Usage minimum item
    0x19, // item tag bSize 1 bType 2 (Local) bTag 0x1
    0x01, // data 0x01
    // Usage maximum item
    0x29, // item tag bSize 1 bType 2 (Local) bTag 0x2
    0x3F, // data 0x3F (63)
    // Logical Minimum (data bytes in the report may have minimum value = 0x00)
    0x15, // item tag bSize 1 bType 1 (Global) bTag 0x1
    0x00, // data 0x00
    // Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
    0x26, // item tag bSize 2 bType 1 (Global) bTag 0x2
    0xFF,
    0x00, // data 0x00FF
    // Report Size: 8-bit field size
    0x75, // item tag bSize 1 bType 1 (Global) btag 0x7
    0x08, // data 0x08
    // Report Count: Make sixty-four 8-bit fields (the next time the parser hits an "Input", "Output", or "Feature" item)
    0x95, // item tag bSize 1 bType 1 (Global) bTag 0x9
    0x40, // data 0x40 (64)
    // Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage.
    0x91, // item tag bSize 1 bType 0 (Main) bTag 0x9
    0x00, // 0x00
    // Usage Minimum
    0x19, // item tag bSize 1 bType 2 (Local) bTag 0x1
    0x01, // data 0x01
    // Usage maximum item
    0x29, // item tag bSize 1 bType 2 (Local) bTag 0x2
    0x3F, // data 0x3F (63)
    // Output (Data, Array, Abs): Instantiates output packet fields.  Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item.
    0x81, // item tag bSize 1 bType 0 (Main) bTag 0x8
    0x00, // data 0x00
    // End collection
    0xC0, // item tag bSize 0 bType 0 (Main) bTag 0xC
};

#endif // USB_HID

#ifdef __cplusplus
}
#endif

#endif // CASCODA_USBHID_H
