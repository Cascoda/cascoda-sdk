/**
 * @file
 * @brief  USB definitions
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

#ifndef CASCODA_USB_H
#define CASCODA_USB_H

#include "cascoda-bm/cascoda_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/****** USB Descriptor Types                                             ******/
/******************************************************************************/
#define USB_DT_DEVICE (1)
#define USB_DT_CONFIGURATION (2)
#define USB_DT_STRING (3)
#define USB_DT_INTERFACE (4)
#define USB_DT_ENDPOINT (5)

#define USB_DT_BOS (15)
#define USB_DT_DEVICE_CAPABILITY (16)

#define USB_DT_HID (33)
#define USB_DT_HID_RPT (34)

/******************************************************************************/
/****** USB Device Capability Types                                      ******/
/******************************************************************************/
#define USB_DCT_WIRELESS (1)
#define USB_DCT_2_0_EXTENSION (2)

/** Device descriptor */
typedef struct UsbDeviceDescriptor
{
	u8_t bLength;            //!< Length in bytes
	u8_t bDescriptorType;    //!< USB_DT_DEVICE
	u8_t bcdUSB[2];          //!< usb spec release number in bcd
	u8_t bDeviceClass;       //!< Class code (from USB-IF)
	u8_t bDeviceSubClass;    //!< Subclass code (from USB-IF)
	u8_t bDeviceProtocol;    //!< Protocol code ( from USB-IF )
	u8_t bMaxPacketSize0;    //!< Max size for EP0 as exponent of 2
	u8_t idVendor[2];        //!< Vendor id
	u8_t idProduct[2];       //!< Product id
	u8_t bcdDevice[2];       //!< Device release number in bcd
	u8_t iManufacturer;      //!< Index of string desc for manufacturer
	u8_t iProduct;           //!< Index of string desc for product
	u8_t iSerialNumber;      //!< Index of string desc for serialnumber
	u8_t bNumConfigurations; //!< Number of configurations
} UsbDeviceDescriptor_t;

/**
 * \brief BOS descriptor
 *
 * Binary device Object Store is a root framework for storing
 * other related descriptors
 */
typedef struct UsbBOSDescriptor
{
	u8_t bLength;         //!< Length in bytes ( of this )
	u8_t bDescriptorType; //!< USB_DT_BOS
	u8_t wTotalLength[2]; //!< Length of this plus concatenated descs
	u8_t bNumDeviceCaps;  //!< No of capability descs following
} UsbBOSDescriptor_t;

/** Device Capability descriptor */
typedef struct UsbDeviceCapabilityDescriptor
{
	u8_t bLength;            //!< Length in bytes ( of this )
	u8_t bDescriptorType;    //!< USB_DT_CAPABILITY
	u8_t bDevCapabilityType; //!< See values above
} UsbDeviceCapabilityDescriptor_t;

/** Configuration descriptor */
typedef struct UsbConfigurationDescriptor
{
	u8_t bLength;             //!< Length in bytes ( of this )
	u8_t bDescriptorType;     //!< USB_DT_CONFIGURATION
	u8_t wTotalLength[2];     //!< Total length of concatenated descriptors
	u8_t bNumInterfaces;      //!< Number of interfaces this config
	u8_t bConfigurationValue; //!< Value to select this config
	u8_t iConfiguration;      //!< index of string desc for this config
	u8_t bmAttributes;        //!< attribute bitmap
	u8_t bMaxPower;           //!< in 2ma units
} UsbConfigurationDescriptor_t;

/** Interface descriptor */
typedef struct UsbInterfaceDescriptor
{
	u8_t bLength;            //!< Length in bytes ( of this )
	u8_t bDescriptorType;    //!< USB_DT_INTERFACE
	u8_t bInterfaceNumber;   //!< Interface Number
	u8_t bAlternateSetting;  //!< Alternate setting for this interface
	u8_t bNumEndpoints;      //!< Number of endpoints in this interface excluding endpoint 0
	u8_t iInterfaceClass;    //!< Class Code: HID
	u8_t iInterfaceSubClass; //!< SubClass Code
	u8_t bInterfaceProtocol; //!< Protocol Code
	u8_t iInterface;         //!< String index
} UsbInterfaceDescriptor_t;

/** Endpoint descriptor */
typedef struct UsbEndpointDescriptor
{
	u8_t bLength;           //!< Length in bytes ( of this )
	u8_t bDescriptorType;   //!< USB_DT_ENDPOINT
	u8_t bEndpointAddress;  //!< Endpoint address
	u8_t bmAttributes;      //!< Transfer Type
	u8_t wMaxPacketSize[2]; //!< Endpoint max transfer Size
	u8_t bInterval;         //!< Polling Interval
} UsbEndpointDescriptor_t;

/** String descriptor */
typedef struct UsbStringDescriptor
{
	u8_t *pu8String;        //!< String as ASCII
	u8_t *pu8UniCodeString; //!< String as Unicode
} UsbStringDescriptor_t;

/******************************************************************************/
/****** String indexes                                                   ******/
/******************************************************************************/
#define USB_ILANGUAGE (0)
#define USB_IMANUFACTURER (1)
#define USB_IPRODUCT (2)
#define USB_ISERIAL (3)
#define USB_ILAST (3)
#ifdef __cplusplus
}
#endif

#endif // CASCODA_USB_H
