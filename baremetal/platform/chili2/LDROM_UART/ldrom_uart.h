/*
 *  Copyright (c) 2023, Cascoda Ltd.
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

#ifndef BAREMETAL_PLATFORM_CASCODA_NUVOTON_CHILI2_LDROM_UART_LDROM_UART_H_
#define BAREMETAL_PLATFORM_CASCODA_NUVOTON_CHILI2_LDROM_UART_LDROM_UART_H_

#include <stdbool.h>
#include <stdint.h>

#include "ca821x_error.h"
#include "cascoda_chili_config.h"
#include "evbme_messages.h"

#if (UART_CHANNEL == 0)
#define CLK_APBCLK_MASK CLK_APBCLK0_UART0CKEN_Msk
#define UART UART0
#define UART_IRQn UART0_IRQn
#elif (UART_CHANNEL == 1)
#define CLK_APBCLK_MASK CLK_APBCLK0_UART1CKEN_Msk
#define UART UART1
#define UART_IRQn UART1_IRQn
#elif (UART_CHANNEL == 2)
#define CLK_APBCLK_MASK CLK_APBCLK0_UART2CKEN_Msk
#define UART UART2
#define UART_IRQn UART2_IRQn
#elif (UART_CHANNEL == 4)
#define CLK_APBCLK_MASK CLK_APBCLK0_UART4CKEN_Msk
#define UART UART4
#define UART_IRQn UART4_IRQn
#elif (UART_CHANNEL == 5)
#define CLK_APBCLK_MASK CLK_APBCLK0_UART5CKEN_Msk
#define UART UART5
#define UART_IRQn UART5_IRQn
#endif

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
 * Structure of a DFU message
 */
struct __attribute__((packed)) dfu_cmd_msg
{
	uint8_t               dfu_cmdid; //!< DFU cmdid
	union dfu_cmd_aligned dfu_cmd;   //!< dfu command
};

/**
 * Evbme GET request struct (for received messages)
 */
struct __attribute__((packed)) evbme_get_request
{
	uint8_t attributeId;
};

/**
 * Evbme GET confirm struct (to respond to received GET requests)
 */
struct __attribute__((packed)) evbme_get_confirm
{
	uint8_t status;
	uint8_t attributeId;
	uint8_t attributeLen;
	uint8_t attribute[];
};

/**
 * Union of GET request and confirm structs 
 */
union evbme_msg
{
	struct evbme_get_request req;
	struct evbme_get_confirm cnf;
};

/**
 * Union of all types of "data" (which commes after "cmdid" and "len")
 */
union serial_data
{
	union evbme_msg    evbme;
	struct dfu_cmd_msg dfu;
	uint8_t            generic;
};

/**
 * Cascoda serialbuffer structure.
 */
struct __attribute__((packed)) SerialUARTBuf
{
	volatile uint8_t  isReady; //!< Is ready to: Tx-Write, Rx-Read
	uint8_t           SofPkt;  //!< Start of frame
	uint8_t           cmdid;   //!< Cascoda command ID = EVBME_DFU_CMD (0xA3)
	uint8_t           len;     //!< Length of data
	union serial_data data;    //!< Data
};

/******************************************************************************/
/****** Definitions for Serial State                                     ******/
/******************************************************************************/
enum serial_state
{
	SERIAL_INBETWEEN = 0,
	SERIAL_CMDID     = 1,
	SERIAL_CMDLEN    = 2,
	SERIAL_DATA      = 3,
};

#define SERIAL_SOM (0xDE)
//Assert SerialUARTBuf struct is packed
ca_static_assert(sizeof(struct SerialUARTBuf) == (5 + sizeof(struct dfu_write_cmd_aligned)));

extern struct SerialUARTBuf       gRxBuffer;
extern struct SerialUARTBuf       gTxBuffer;
extern volatile enum serial_state gSerialRxState;
extern volatile bool              gSerialTxStalled;

void UART_Init(void);

//Functions to unblock IO:
void RxHandled(void); //Call when a Rx message has been fully processed
void TxReady(void);   //Call when a Tx is ready to send

#endif /* BAREMETAL_PLATFORM_CASCODA_NUVOTON_CHILI2_LDROM_UART_LDROM_UART_H_ */