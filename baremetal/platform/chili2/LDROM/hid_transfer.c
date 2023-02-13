/*
 *  Copyright (c) 2020, Cascoda Ltd.
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

#include <stdio.h>
#include <string.h>

#include "NuMicro.h"

#include "cascoda-bm/cascoda_usb.h"
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda-util/cascoda_hash.h"

#include "ldrom_hid.h"

/* Define EP maximum packet size */
#define EP0_MAX_PKT_SIZE HID_CTRL_MAX_SIZE
#define EP1_MAX_PKT_SIZE HID_CTRL_MAX_SIZE
#define EP2_MAX_PKT_SIZE HID_FRAGMENT_SIZE
#define EP3_MAX_PKT_SIZE HID_FRAGMENT_SIZE

/* Define the EP number */
#define INT_IN_EP_NUM HID_IN_EP_NO
#define INT_OUT_EP_NUM HID_OUT_EP_NO

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

struct SerialBuf gRxBuffer;
struct SerialBuf gTxBuffer;

/******************************************************************************/
/****** String descriptors for this device                               ******/
/******************************************************************************/
/* Language string descriptor index 0 */
static u8_t USB_LANGUAGE[] = {4, USB_DT_STRING, 0x09, 0x00};
/* Manufacturer string descriptor index 1 */
static u8_t USB_MANUFACTURER[] = {16, USB_DT_STRING, 'C', 0, 'a', 0, 's', 0, 'c', 0, 'o', 0, 'd', 0, 'a', 0};
/* Product string descriptor index 2 offset 20 */
static u8_t USB_PRODUCT[] = {22, USB_DT_STRING, 'C', 0,   'h', 0,   'i', 0,   'l', 0,   'i',
                             0,  '2',           0,   ':', 0,   'D', 0,   'F', 0,   'U', 0};
/* SerialNo string descriptor index 3 offset 46 */
static u8_t USB_SERIAL[34] = {};

/* 2d array of USB string descriptors */
static u8_t *USB_STRINGS[] = {USB_LANGUAGE, USB_MANUFACTURER, USB_PRODUCT, USB_SERIAL};

uint8_t gu8BOSDescriptor[] = {
    LEN_BOS,  /* bLength */
    DESC_BOS, /* bDescriptorType */
    /* wTotalLength */
    0x0C & 0x00FF,
    ((0x0C & 0xFF00) >> 8),
    0x01, /* bNumDeviceCaps */

    /* Device Capability */
    LEN_BOSCAP,      /* bLength */
    DESC_CAPABILITY, /* bDescriptorType */
    CAP_USB20_EXT,   /* bDevCapabilityType */
    0x02,
    0x00,
    0x00,
    0x00 /* bmAttributes */
};

#define MAX_FRAG_SIZE 64
#define FRAG_LEN_MASK 0x3F
#define FRAG_LAST_MASK (1 << 7)
#define FRAG_FIRST_MASK (1 << 6)

static uint8_t *HidReport[3]        = {(uint8_t *)HidReportDescriptor, NULL, NULL};
static uint32_t HidReportLen[3]     = {sizeof(HidReportDescriptor), 0, 0};
static uint32_t ConfigHidDescIdx[3] = {sizeof(UsbConfigurationDescriptor_t) + sizeof(UsbInterfaceDescriptor_t), 0, 0};

const S_USBD_INFO_T gsInfo = {(uint8_t *)&HidDeviceDescriptor,
                              (uint8_t *)&HidConfDesc,
                              USB_STRINGS,
                              HidReport,
                              gu8BOSDescriptor,
                              HidReportLen,
                              ConfigHidDescIdx};

/**
 * Assemble fragments into gRxBuffer
 * @param frag_in Incoming USB fragment
 * @return 0 if we are expecting more in buffer, 1 if app needs to process first
 */
static int assemble_frags(uint8_t *frag_in)
{
	static uint8_t offset = 0, len = 0;
	uint8_t        is_first = 0, is_last = 0, frag_len = 0;
	frag_len = frag_in[0] & FRAG_LEN_MASK;
	is_last  = !!(frag_in[0] & FRAG_LAST_MASK);
	is_first = !!(frag_in[0] & FRAG_FIRST_MASK);

	if (is_first && offset != 0)
	{
		// Invalid internal state: Drop previous frame and start processing this one
		offset = 0;
		len    = 0;
	}
	else if (!is_first && offset == 0)
	{
		// Invalid internal state: Drop this frame, will be able to receive next packet
		len = 0;
		return 0;
	}

	memcpy(&gRxBuffer.cmdid + offset, &frag_in[1], frag_len);

	offset += frag_len;
	len = offset;

	if (is_last)
	{
		gRxBuffer.isReady = true;
		offset            = 0;
	}
	return is_last;
}

/**
 * Get the next fragment from gTxBuffer
 * @param frag_out The buffer to build the fragment into
 */
static void get_next_frag(uint8_t *frag_out)
{
	static uint8_t offset     = 0;
	uint8_t        len_in     = gTxBuffer.len + 2;
	int            end_offset = offset + MAX_FRAG_SIZE - 1;
	uint8_t        is_first = 0, is_last = 0, frag_len = 0;
	is_first = (offset == 0);

	if (end_offset >= len_in)
	{
		end_offset = len_in;
		is_last    = 1;
	}
	frag_len = end_offset - offset;

	frag_out[0] = 0;
	frag_out[0] |= frag_len;
	frag_out[0] |= is_first ? FRAG_FIRST_MASK : 0;
	frag_out[0] |= is_last ? FRAG_LAST_MASK : 0;
	memcpy(&frag_out[1], &gTxBuffer.cmdid + offset, frag_len);

	offset = end_offset;

	if (is_last)
	{
		gTxBuffer.isReady = true;
		offset            = 0;
	}
}

void EP2_Handler(void) /* Interrupt IN handler */
{
	uint8_t *ptr;
	ptr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));

	//If the buffer is waiting to be loaded, there is nothing to send.
	if (gTxBuffer.isReady)
		return;

	//We have not sent complete packet, load next fragment
	get_next_frag(ptr);
	USBD_SET_PAYLOAD_LEN(EP2, EP2_MAX_PKT_SIZE);
}

void EP3_Handler(void) /* Interrupt OUT handler */
{
	uint8_t *ptr;
	ptr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));

	//Unstall receive if we have not got complete packet
	if (!assemble_frags(ptr))
		USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
}

void RxHandled(void)
{
	gRxBuffer.isReady = false;
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
}

void TxReady(void)
{
	gTxBuffer.isReady = false;
	EP2_Handler();
}

uint64_t HASH_fnv1a_64(const void *data_in, size_t num_bytes)
{
	uint64_t       hash = basis64;
	const uint8_t *data = data_in;

	for (size_t i = 0; i < num_bytes; i++)
	{
		hash = (data[i] ^ hash) * prime64;
	}
	return hash;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Populate the USB serial number with the device's unique ID.
 *******************************************************************************
 ******************************************************************************/
static void USB_PopulateSerial(void)
{
	//Unrolled version for LDROM
	u8_t  hex_symbol;
	u8_t  ascii_shift;
	u32_t UID[3];
	u64_t uniqueID = 0;

	FMC_Open();
	UID[0] = FMC_ReadUID(0);
	UID[1] = FMC_ReadUID(1);
	UID[2] = FMC_ReadUID(2);
	FMC_Close();

	uniqueID      = HASH_fnv1a_64(&UID, sizeof(UID));
	USB_SERIAL[0] = 34;
	USB_SERIAL[1] = USB_DT_STRING;

	for (int i = 2; i < 34; i += 2)
	{
		hex_symbol    = ((uniqueID >> (16 - i / 2) * 4) & 0xF);
		ascii_shift   = (hex_symbol > 9) ? '7' : '0';
		USB_SERIAL[i] = hex_symbol + ascii_shift;
	}
}

/*--------------------------------------------------------------------------*/
/**
  * USBD Endpoint Config.
  */
void HID_Init(void)
{
	USB_PopulateSerial();
	/* Init setup packet buffer */
	/* Buffer range for setup packet -> [0 ~ 0x7] */
	USBD->STBUFSEG = SETUP_BUF_BASE;
	/*****************************************************/
	/* EP0 ==> control IN endpoint, address 0 */
	USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
	/* Buffer range for EP0 */
	USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);
	/* EP1 ==> control OUT endpoint, address 0 */
	USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
	/* Buffer range for EP1 */
	USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);
	/*****************************************************/
	/* EP2 ==> Interrupt IN endpoint, address 1 */
	USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | INT_IN_EP_NUM);
	/* Buffer range for EP2 */
	USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);
	/* EP3 ==> Interrupt OUT endpoint, address 2 */
	USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | INT_OUT_EP_NUM);
	/* Buffer range for EP3 */
	USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
	/* trigger to receive OUT data */
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);

	gTxBuffer.isReady = true;
}
