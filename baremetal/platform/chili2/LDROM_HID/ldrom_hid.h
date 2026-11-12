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

#ifndef BAREMETAL_PLATFORM_CASCODA_NUVOTON_CHILI2_LDROM_HID_LDROM_HID_H_
#define BAREMETAL_PLATFORM_CASCODA_NUVOTON_CHILI2_LDROM_HID_LDROM_HID_H_

#include <stdint.h>

#include "ca821x_error.h"
#include "evbme_messages.h"

/* Nuvoton BSP update from v3.00.003 to v3.00.007:                         */
/* New macros switching between secure and non-secure peripheral pointers. */
/* This causes ldrom_hid size to rise above limit of 4k.                   */
/* All used macros re-named to USBD_NOTZ_*                                 */
#define USBD_NOTZ_GET_EP_BUF_ADDR(ep)        ((uint32_t)*((__IO uint32_t *) ((uint32_t)&USBD->EP[0].BUFSEG + (uint32_t)((ep) << 4))))
#define USBD_NOTZ_SET_PAYLOAD_LEN(ep, size)  (*((__IO uint32_t *) ((uint32_t)&USBD->EP[0].MXPLD + (uint32_t)((ep) << 4))) = (size))
#define USBD_NOTZ_CONFIG_EP(ep, config)      (*((__IO uint32_t *) ((uint32_t)&USBD->EP[0].CFG + (uint32_t)((ep) << 4))) = (config))
#define USBD_NOTZ_SET_EP_BUF_ADDR(ep, offset)    (*((__IO uint32_t *) ((uint32_t)&USBD->EP[0].BUFSEG + (uint32_t)((ep) << 4))) = (offset))
#define USBD_NOTZ_GET_INT_FLAG()         ((uint32_t)(USBD->INTSTS))
#define USBD_NOTZ_CLR_INT_FLAG(flag)     (USBD->INTSTS = (flag))

/**
 * Write command to write words of data - aligned for codespace and speed
 */
struct dfu_write_cmd_aligned
{
	uint8_t  startAddr[4]; //!< Start address for writing - must be word aligned
	uint32_t data[61];     //!< Data to write, must be whole words.
};
//Assert struct is packed even with uint32_t data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs" //Suppresses warnings
ca_static_assert(sizeof(struct dfu_write_cmd_aligned) == 248);
#pragma GCC diagnostic pop

/**
 * Union of all DFU commands - version with dfu_write_cmd_aligned
 */
union dfu_cmd_aligned
{
	struct evbme_dfu_reboot_cmd  reboot_cmd;
	struct evbme_dfu_erase_cmd   erase_cmd;
	struct dfu_write_cmd_aligned write_cmd;
	struct evbme_dfu_check_cmd   check_cmd;
	struct evbme_dfu_status_cmd  status_cmd;
};

/**
 * Cascoda serialbuffer structure.
 */
struct SerialBuf
{
	volatile uint8_t      isReady;   //!< Is ready to: Tx-Write, Rx-Read
	uint8_t               cmdid;     //!< Cascoda command ID = EVBME_DFU_CMD (0xA3)
	uint8_t               len;       //!< Length of data
	uint8_t               dfu_cmdid; //!< DFU cmdid
	union dfu_cmd_aligned dfu_cmd;   //!< dfu command
};

//Assert SerialBuf struct is packed
ca_static_assert(sizeof(struct SerialBuf) == sizeof(struct dfu_write_cmd_aligned) + 4);

extern struct SerialBuf gRxBuffer;
extern struct SerialBuf gTxBuffer;

void HID_Init(void);
void EP2_Handler(void);
void EP3_Handler(void);

//Functions to unblock IO:
void RxHandled(void); //Call when a Rx message has been fully processed
void TxReady(void);   //Call when a Tx is ready to send

#endif /* BAREMETAL_PLATFORM_CASCODA_NUVOTON_CHILI2_LDROM_HID_LDROM_HID_H_ */
