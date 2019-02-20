/**
 * @file   cascoda_usbhid.c
 * @brief  USB HID code
 * @author Peter Burnett
 * @date   13/08/14
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

#include <stdint.h>

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_usb.h"
#include "cascoda-bm/cascoda_usbhid.h"

#if defined(USE_USB)

#define EP_INPUT (0x80)
#define EP_OUTPUT (0x00)
/******************************************************************************/
/****** Device descriptor for HID device                                 ******/
/******************************************************************************/

const UsbDeviceDescriptor_t HidDeviceDescriptor = {
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

const HidConfigurationDescriptor_t HidConfDesc = {
    {
        sizeof(UsbConfigurationDescriptor_t),      // bLength
        USB_DT_CONFIGURATION,                      // bDescriptorType
        {sizeof(HidConfigurationDescriptor_t), 0}, // wTotalLength
        0x01,                                      // bNumInterfaces: Number of interfaces
        0x01,                                      // iConfigurationValue: Number of this configuration
        0x00,                                      // iConfiguration: String index of this configuration
        0x80,                                      // bmAttributes: Bus-Powered, Remote-Wakeup not supported
        0xFA                                       // MaxPower: (in 2mA) 250->500mA
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

const u8_t HidReportDescriptor[HID_RPT_SIZE] = {
    // Usage Page item specifying vendor defined
    0x06,       // item tag bSize 2 bType 1 (global) bTag 0
    0x00, 0xff, // data 0xFF00 vendor defined
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
    0x26,       // item tag bSize 2 bType 1 (Global) bTag 0x2
    0xFF, 0x00, // data 0x00FF
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
