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
/**
 * @file
 * @brief Definitions relating to EVBME API messages.
 */

#ifndef EVBME_MESSAGES_H
#define EVBME_MESSAGES_H

#include <stdint.h>

#include "ca821x_config.h"
#include "ca821x_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** EVBME Command IDs */
enum evbme_command_ids
{
	EVBME_GET_CONFIRM = 0x5C, //!< M<-S Response containing EVBME parameter data (sync resp)
	EVBME_GET_REQUEST = 0x5D, //!< M->S Get an EVBME parameter (sync req)
	EVBME_SET_CONFIRM = 0x5E, //!< M<-S Response including status for EVBME set (sync resp)
	EVBME_SET_REQUEST = 0x5F, //!< M->S Set an EVBME parameter (sync req)

	EVBME_GUI_CONNECTED      = 0x81, //!< M->S Notification from host that connection is established
	EVBME_GUI_DISCONNECTED   = 0x82, //!< M->S Notification from host that connection is about to be terminated
	EVBME_MESSAGE_INDICATION = 0xA0, //!< M<-S Text message to be printed by host
	EVBME_COMM_CHECK         = 0xA1, //!< M->S Communication check message from host that generates COMM_INDICATIONS
	EVBME_COMM_INDICATION    = 0xA2, //!< M<-S Communication check indication from slave to master as requested
	EVBME_DFU_CMD            = 0xA3, //!< M<>S DFU Commands for Device Firmware Upgrade in system

	EVBME_RXRDY = 0xAA, //!< M<>S RXRDY signal, used for interfaces without built in flow control like raw UART
};

/** EVBME attribute ids for use with EVBME_SET_REQUEST and EVBME_GET_REQUEST*/
enum evbme_attribute
{
	EVBME_RESETRF  = 0x00, //!< ResetRF - Write only
	EVBME_CFGPINS  = 0x01, //!< CfgPins - Write only
	EVBME_WAKEUPRF = 0x02, //!< Wakeup CA8211 - Write only

	EVBME_VERSTRING   = 0x80, //!< Version string - Read only
	EVBME_PLATSTRING  = 0x81, //!< Platform string - Read only
	EVBME_APPSTRING   = 0x82, //!< Application string - Read only
	EVBME_SERIALNO    = 0x83, //!< SerialNo 64 bit - Read only
	EVBME_OT_EUI64    = 0x84, //!< Openthread EUI64 for commissioning - Read only
	EVBME_OT_JOINCRED = 0x85, //!< Openthread joining credential for commissioning - Read only
};

/**
 * Command IDs for DFU commands
 */
enum evbme_dfu_cmdid
{
	DFU_REBOOT = 0, //!< Reboot into DFU or non-dfu mode
	DFU_ERASE  = 1, //!< Erase given flash pages
	DFU_WRITE  = 2, //!< Write given data to already erased flash
	DFU_CHECK  = 3, //!< Check flash checksum in given range
	DFU_STATUS = 4, //!< Status command returned from chili to host
};

/**
 * Reboot mode
 */
enum evbme_dfu_rebootmode
{
	EVBME_DFU_REBOOT_APROM = 0, //!< Reboot into application
	EVBME_DFU_REBOOT_DFU   = 1, //!< Reboot into Device Firmware Upgrade mode
};

/**
 * Reboot command to boot into DFU or APROM
 */
struct evbme_dfu_reboot_cmd
{
	uint8_t rebootMode; //!< 0=APROM, 1=DFU - see evbme_dfu_rebootmode
};

/**
 * Erase command to erase whole pages
 */
struct evbme_dfu_erase_cmd
{
	uint8_t startAddr[4]; //!< startAddr to write to - must be page-aligned
	uint8_t eraseLen[4];  //!< amount of data to erase - must be whole pages
};

/**
 * Write command to write words of data
 */
struct evbme_dfu_write_cmd
{
	uint8_t startAddr[4]; //!< Start address for writing - must be word aligned
	uint8_t data[];       //!< Data to write, must be whole words. Max 244 bytes.
};

/**
 * Check command to validate flash against a checksum
 */
struct evbme_dfu_check_cmd
{
	uint8_t startAddr[4]; //!< startAddr to check - must be page-aligned
	uint8_t checkLen[4];  //!< amount of data to check - must be whole pages
	uint8_t checksum[4];  //!< checksum to validate against
};

/**
 * Status command used as a reply from the Chili2 to host.
 */
struct evbme_dfu_status_cmd
{
	uint8_t status; //!< ca_error status
};

/**
 * Union of all DFU sub-commands
 */
union evbme_dfu_sub_cmd
{
	struct evbme_dfu_reboot_cmd reboot_cmd;
	struct evbme_dfu_erase_cmd  erase_cmd;
	struct evbme_dfu_write_cmd  write_cmd;
	struct evbme_dfu_check_cmd  check_cmd;
	struct evbme_dfu_status_cmd status_cmd;
};

/**
 * EVBME Get data confirm structure
 */
struct EVBME_GET_confirm
{
	uint8_t mStatus;
	uint8_t mAttributeId;
	uint8_t mAttributeLen;
	uint8_t mAttribute[];
};

/**
 * EVBME Get data request structure
 */
struct EVBME_GET_request
{
	uint8_t mAttributeId;
};

/**
 * EVBME Set data confirm structure
 */
struct EVBME_SET_confirm
{
	uint8_t mStatus;
};

/**
 * EVBME Set data request structure
 */
struct EVBME_SET_request
{
	uint8_t mAttributeId;
	uint8_t mAttributeLen;
	uint8_t mAttribute[];
};

/**
 * EVBME Message indication structure
 */
struct EVBME_MESSAGE_indication
{
	char mMessage[1]; //Flexible length, but need at least one member
};

/** Structure of the EVBME_COMM_CHECK message that can be used to test comms by host. */
struct EVBME_COMM_CHECK_request
{
	uint8_t mHandle;    //!< Handle identifying this comm check
	uint8_t mDelay;     //!< Delay before sending responses
	uint8_t mIndCount;  //!< Number of indications to send up
	uint8_t mIndSize;   //!< Size of the indications to send
	uint8_t mPayload[]; //!< Redundant payload to stress the interface
};

/** Structure of the EVBME_COMM_INDICATION message that is used in response to EVBME_COMM_CHECK_request. */
struct EVBME_COMM_indication
{
	uint8_t mHandle;    //!< Handle identifying this comm check
	uint8_t mPayload[]; //!< Additional data to stress comm link as specified by mIndSize in EVBME_COMM_CHECK_request
};

/**
 * EVBME DFU command holder, contains various sub-dfu commands from evbme_dfu_sub_cmd
 */
struct EVBME_DFU_cmd
{
	uint8_t                 mDfuSubCmdId; //!< DFU sub command ID evbme_dfu_cmdid
	union evbme_dfu_sub_cmd mSubCmd;      //!< Dfu subcommand data
};

/**
 * EVBME Message command in Cascoda TLV format.
 */
struct EVBME_Message
{
	uint8_t mCmdId; //!< see evbme_command_ids
	uint8_t mLen;   //!< Length of the EVBME command data
	union
	{
		struct EVBME_GET_confirm        GET_confirm;
		struct EVBME_GET_request        GET_request;
		struct EVBME_SET_confirm        SET_confirm;
		struct EVBME_SET_request        SET_request;
		struct EVBME_MESSAGE_indication MESSAGE_indication;
		struct EVBME_COMM_CHECK_request COMM_CHECK_request;
		struct EVBME_COMM_indication    COMM_indication;
		struct EVBME_DFU_cmd            DFU_cmd;
		uint8_t                         data[254]; //!< To access as raw data
	} EVBME;
};

#ifdef __cplusplus
}
#endif

#endif // EVBME_MESSAGES_H
