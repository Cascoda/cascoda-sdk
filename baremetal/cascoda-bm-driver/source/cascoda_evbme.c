/**
 * @file
 * @brief  EvaBoard Management Entity (EVBME) functions
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
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_bm.h"
#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_os.h"
#include "cascoda-bm/cascoda_ota_upgrade.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-bm/chili_test.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "cascoda-util/cascoda_rand.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "cascoda_bm_internal.h"
#include "cascoda_external_flash.h"
#include "evbme_messages.h"
#include "mac_messages.h"

/******************************************************************************/
/****** Global Parameters that can by set via EVBME_SET_request          ******/
/******************************************************************************/
uint8_t EVBME_HasReset = 0; //!< Used to notify apps that radio has been reset
uint8_t EVBME_UseMAC   = 0; //!< Use MAC functionality during phy tests

static const char *app_name;    //!< String describing initialised app
static u8_t        CLKExternal; //!< Nonzero if the CA821x is supplying a 4MHz clock

/** Function pointer for sending ASCII reporting messages upstream */
void (*EVBME_Message)(char *message, size_t len);
/** Function pointer for sending API commands upstream. */
void (*MAC_Message)(u8_t CommandId, u8_t Count, const u8_t *pBuffer);
/** Function pointer for reinitialising the CA821X upon a restart/reset **/
int (*cascoda_reinitialise)(struct ca821x_dev *pDeviceRef);

// serial dispatch function
int (*cascoda_serial_dispatch)(u8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

// static function prototypes
static ca_error EVBME_WakeupCallback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_ResetRF(uint8_t ms, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_Connect(const char *aAppName, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_ExternalClock(u8_t on_offb, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_Wakeup_callback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_Wakeup(enum powerdown_mode mode, u32_t timeout_ms, struct ca821x_dev *pDeviceRef);
static void     EVBME_WakeUpRF(void);

#if defined(USE_USB) || defined(USE_UART)
static void     EVBME_COMM_CHECK_request(struct EVBME_Message *rxBuf);
static int      EVBMEUpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
static void     EVBMESendDownStream(const uint8_t *buf, struct ca821x_dev *pDeviceRef);
static void     EVBME_Disconnect(void);
static ca_error EVBME_GET_request(struct EVBME_GET_request *req);
static ca_error EVBME_SET_request(struct EVBME_SET_request *req, struct ca821x_dev *pDeviceRef);

static void EVBME_COMM_CHECK_request(struct EVBME_Message *rxBuf)
{
	struct EVBME_COMM_CHECK_request *msg = &rxBuf->EVBME.COMM_CHECK_request;

	if (msg->mIndSize == 0 || rxBuf->mLen == 3)
		msg->mIndSize = 1;

	uint8_t buf[msg->mIndSize];

	WAIT_ms(msg->mDelay);

	memset(buf, 0, sizeof(buf));
	buf[0] = msg->mHandle;

	if (!MAC_Message)
		return;

	for (uint8_t i = 0; i < msg->mIndCount; i++)
	{
		MAC_Message(EVBME_COMM_INDICATION, msg->mIndSize, buf);
	}
}

static ca_error EVBME_DFU_reboot(struct EVBME_DFU_cmd *dfuCmd)
{
	if (dfuCmd->mSubCmd.reboot_cmd.rebootMode)
		BSP_SystemReset(SYSRESET_DFU);
	else
		BSP_SystemReset(SYSRESET_APROM);
	//Should not return, as above functions reset program.
	return CA_ERROR_FAIL;
}

static ca_error EVBME_DFU_erase(struct EVBME_DFU_cmd *dfuCmd)
{
	ca_error status    = CA_ERROR_INVALID_ARGS;
	uint32_t eraseLen  = GETLE32(dfuCmd->mSubCmd.erase_cmd.eraseLen);
	uint32_t startAddr = GETLE32(dfuCmd->mSubCmd.erase_cmd.startAddr);

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	ExternalFlashInfo external_flash_info;
	BSP_ExternalFlashGetInfo(&external_flash_info);

	if (startAddr >= external_flash_info.baseAddress)
	{
		startAddr -= external_flash_info.baseAddress;
		status = ota_handle_erase(startAddr, eraseLen, &external_flash_evbme_send_upstream);
	}
	else
#endif
	{
		struct ca_flash_info flash_info;

		BSP_GetFlashInfo(&flash_info);

		for (uint32_t offset = 0; offset < eraseLen; offset += flash_info.pageSize)
		{
			status = BSP_FlashErase(startAddr + offset);
			if (status)
				break;
		}
	}

	return status;
}

static ca_error EVBME_DFU_write(struct EVBME_Message *rxMsg)
{
	struct EVBME_DFU_cmd *dfuCmd = &rxMsg->EVBME.DFU_cmd;
	uint32_t writeLen  = rxMsg->mLen - sizeof(dfuCmd->mDfuSubCmdId) - sizeof(dfuCmd->mSubCmd.write_cmd.startAddr);
	uint32_t startAddr = GETLE32(dfuCmd->mSubCmd.write_cmd.startAddr);
	uint8_t *data;

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	ca_error status = CA_ERROR_INVALID_ARGS;

	ExternalFlashInfo external_flash_info;
	BSP_ExternalFlashGetInfo(&external_flash_info);

	if (startAddr >= external_flash_info.baseAddress)
	{
		//DFU_WRITE_MAX_LEN = 244;
		static uint8_t data_buff[244]; //TODO: We may want to reconsider how this is allocated in future.
		memcpy(data_buff, dfuCmd->mSubCmd.write_cmd.data, writeLen);
		data = data_buff;

		startAddr -= external_flash_info.baseAddress;
		status = ota_handle_write(startAddr, writeLen, data, external_flash_info, &external_flash_evbme_send_upstream);
	}
	else
#endif
	{
		data = dfuCmd->mSubCmd.write_cmd.data;
		return BSP_FlashWriteInitial(startAddr, data, writeLen);
	}

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	return status;
#endif
}

static ca_error EVBME_DFU_check(struct EVBME_DFU_cmd *dfuCmd)
{
	u32_t startaddr = GETLE32(dfuCmd->mSubCmd.check_cmd.startAddr);
	u32_t checklen  = GETLE32(dfuCmd->mSubCmd.check_cmd.checkLen);
	u32_t checksum  = GETLE32(dfuCmd->mSubCmd.check_cmd.checksum);

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	ca_error status = CA_ERROR_INVALID_ARGS;

	struct ExternalFlashInfo external_flash_info;
	BSP_ExternalFlashGetInfo(&external_flash_info);

	if (startaddr >= external_flash_info.baseAddress)
	{
		startaddr -= external_flash_info.baseAddress;
		status =
		    ota_handle_check(startaddr, checklen, checksum, external_flash_info, &external_flash_evbme_send_upstream);
	}
	else
#endif

	{
		return BSP_FlashCheck(startaddr, checklen, checksum);
	}

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	return status;
#endif
}

static ca_error EVBME_DFU_bootmode(struct EVBME_DFU_cmd *dfuCmd)
{
	return BSP_SetBootMode(dfuCmd->mSubCmd.bootmode_cmd.bootMode);
}

/**
 * Handle dfu command at chili layer.
 * @param rxMsg The received EVBME message
 */
static void EVBME_DFU_cmd(struct EVBME_Message *rxMsg)
{
	ca_error              status                 = CA_ERROR_FAIL;
	bool                  skip_upstream_response = false;
	struct EVBME_DFU_cmd *dfuCmd                 = &rxMsg->EVBME.DFU_cmd;
	struct EVBME_DFU_cmd  dfuRsp;

	switch (dfuCmd->mDfuSubCmdId)
	{
	case DFU_REBOOT:
		status = EVBME_DFU_reboot(dfuCmd);
		break;
	case DFU_ERASE:
		status = EVBME_DFU_erase(dfuCmd);
		if (status == CA_ERROR_BUSY)
			skip_upstream_response = true;
		break;
	case DFU_WRITE:
		status = EVBME_DFU_write(rxMsg);
		if (status == CA_ERROR_BUSY)
			skip_upstream_response = true;
		break;
	case DFU_CHECK:
		status = EVBME_DFU_check(dfuCmd);
		if (status == CA_ERROR_BUSY)
			skip_upstream_response = true;
		break;
	case DFU_BOOTMODE:
		status = EVBME_DFU_bootmode(dfuCmd);
		break;
	default:
		status = CA_ERROR_INVALID_STATE;
		break;
	}
	if (!skip_upstream_response)
	{
		dfuRsp.mDfuSubCmdId              = DFU_STATUS;
		dfuRsp.mSubCmd.status_cmd.status = (uint8_t)status;
		MAC_Message(EVBME_DFU_CMD, 2, (u8_t *)&dfuRsp);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch upstream messages for EVBME
 *******************************************************************************
 * \param SerialRxBuffer - Serial message to process
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return  0: Message was not consumed by EVBME (should go downstream)
 *         >0: Message was consumed by EVBME
 *******************************************************************************
 ******************************************************************************/
static int EVBMEUpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	int                   ret     = 0;
	struct EVBME_Message *rxEvbme = (struct EVBME_Message *)SerialRxBuffer;

	switch (rxEvbme->mCmdId)
	{
	case EVBME_GET_REQUEST:
		ret = 1;
		EVBME_GET_request(&rxEvbme->EVBME.GET_request);
		break;
	case EVBME_SET_REQUEST:
		ret = 1;
		EVBME_SET_request(&rxEvbme->EVBME.SET_request, pDeviceRef);
		break;
	case EVBME_HOST_CONNECTED:
		ret = 1;
		EVBME_Connect(app_name, pDeviceRef);
		break;
	case EVBME_HOST_DISCONNECTED:
		ret = 1;
		EVBME_Disconnect();
		break;
	case EVBME_MESSAGE_INDICATION:
		ret = 1;
		break;
	case EVBME_COMM_CHECK:
		ret = 1;
		EVBME_COMM_CHECK_request(rxEvbme);
		break;
	case EVBME_DFU_CMD:
		ret = 1;
		EVBME_DFU_cmd(rxEvbme);
		break;
	default:
		break;
	}

	return ret;
} // End of EVBMEUpStreamDispatch()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends UpStream Command from Serial DownStream to SPI
 *******************************************************************************
 * \param buf - Message to send DownStream, Length is encoded in Cascoda TLV format
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
static void EVBMESendDownStream(const uint8_t *buf, struct ca821x_dev *pDeviceRef)
{
	struct MAC_Message response;
	ca_error           status;

	status = DISPATCH_ToCA821x(buf, &response.CommandId, pDeviceRef);

	if (status == CA_ERROR_SUCCESS)
	{
#if CASCODA_CA_VER == 8210
		struct MAC_Message *cmd = (struct MAC_Message *)buf;
		switch (cmd->CommandId)
		{
		case SPI_MLME_SET_REQUEST:
			if (cmd->PData.SetReq.PIBAttribute == macShortAddress)
				pDeviceRef->shortaddr = GETLE16(cmd->PData.SetReq.PIBAttributeValue);
			else if (cmd->PData.SetReq.PIBAttribute == nsIEEEAddress)
				memcpy(pDeviceRef->extaddr, cmd->PData.SetReq.PIBAttributeValue, 8);
			break;
		case SPI_MLME_RESET_REQUEST:
			if (cmd->PData.u8Param)
				pDeviceRef->shortaddr = 0xFFFF;
			break;
		case SPI_HWME_SET_REQUEST:
			if (cmd->PData.HWMESetReq.HWAttribute == HWME_LQIMODE)
				pDeviceRef->lqi_mode = cmd->PData.HWMESetReq.HWAttributeValue[0];
			break;
		}
#endif // CASCODA_CA_VER == 8210

		// Synchronous Confirms have to be sent upstream for interrupt driven handlers
		if ((buf[0] != SPI_IDLE) && (buf[0] != SPI_NACK) && (buf[0] & SPI_SYN))
		{
			EVBME_NotHandled(&response, pDeviceRef);
		}
	}
} // End of EVBMESendDownStream()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset Test Modes and RF when GUI is disconnected
 *******************************************************************************
 ******************************************************************************/
static void EVBME_Disconnect(void)
{
	EVBME_HasReset = 1;
	BSP_ResetRF(50); // reset RF for 50 ms
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_GET_request according to EVBME Spec
 *******************************************************************************
 * \param req The Evbme get request received
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_GET_request(struct EVBME_GET_request *req)
{
	ca_error                  status = CA_ERROR_SUCCESS;
	uint8_t                   internalbuf[30]; //Only used to size the VLA at end of below struct.
	struct EVBME_GET_confirm *getInd     = (struct EVBME_GET_confirm *)internalbuf;
	size_t                    maxAttrLen = sizeof(internalbuf) - sizeof(struct EVBME_GET_confirm);

	switch (req->mAttributeId)
	{
	case EVBME_VERSTRING:
	{
		const char *verString = ca821x_get_version_nodate();
		size_t      verLen    = strlen(verString) + 1;
		uint8_t     attrlen   = verLen > maxAttrLen ? maxAttrLen : verLen;

		getInd->mAttributeLen = attrlen;
		memcpy(getInd->mAttribute, verString, getInd->mAttributeLen);
		getInd->mAttribute[attrlen - 1] = '\0';
		break;
	}
	case EVBME_PLATSTRING:
	{
		const char *devString = BSP_GetPlatString();
		size_t      devLen    = strlen(devString) + 1;
		uint8_t     attrlen   = devLen > maxAttrLen ? maxAttrLen : devLen;

		getInd->mAttributeLen = attrlen;
		memcpy(getInd->mAttribute, devString, getInd->mAttributeLen);
		getInd->mAttribute[attrlen - 1] = '\0';
		break;
	}
	case EVBME_APPSTRING:
	{
		size_t  appLen  = strlen(app_name) + 1;
		uint8_t attrlen = appLen > maxAttrLen ? maxAttrLen : appLen;

		getInd->mAttributeLen = attrlen;
		memcpy(getInd->mAttribute, app_name, getInd->mAttributeLen);
		getInd->mAttribute[attrlen - 1] = '\0';
		break;
	}
	case EVBME_SERIALNO:
	{
		uint64_t serialNo = BSP_GetUniqueId();

		getInd->mAttributeLen = sizeof(serialNo);
		PUTLE64(serialNo, getInd->mAttribute);
		break;
	}
	case EVBME_EXTERNAL_FLASH_AVAILABLE:
	{
		getInd->mAttributeLen = 1;
#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
		getInd->mAttribute[0] = 1;
#else
		getInd->mAttribute[0] = 0;
#endif
		break;
	}
	case EVBME_OT_EUI64:
	case EVBME_OT_JOINCRED:
		getInd->mAttributeLen = maxAttrLen;
		status                = EVBME_GET_OT_Attrib(req->mAttributeId, &(getInd->mAttributeLen), getInd->mAttribute);
		break;
	case EVBME_ONBOARD_FLASH_INFO:
	{
		struct ca_flash_info flash_info;
		BSP_GetFlashInfo(&flash_info);
		getInd->mAttributeLen = sizeof(flash_info);
		memcpy(getInd->mAttribute, &flash_info, getInd->mAttributeLen);
		break;
	}
	default:
		status = CA_ERROR_UNKNOWN;
		break;
	}

	//Set status and ID before sending indication response
	getInd->mStatus      = status;
	getInd->mAttributeId = req->mAttributeId;

	if (status == CA_ERROR_SUCCESS)
	{
		MAC_Message(EVBME_GET_CONFIRM, getInd->mAttributeLen + sizeof(*getInd), internalbuf);
	}
	else
	{
		getInd->mAttributeLen = 0;
		MAC_Message(EVBME_GET_CONFIRM, sizeof(*getInd), internalbuf);
	}

	return status;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_SET_request according to EVBME Spec
 *******************************************************************************
 * \param req - The EVBME set request received
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_SET_request(struct EVBME_SET_request *req, struct ca821x_dev *pDeviceRef)
{
	struct EVBME_SET_confirm setConfirm;
	ca_error                 status;

	/* branch for EVBME-SET Attributes */
	switch (req->mAttributeId)
	{
	case EVBME_RESETRF:
		if (req->mAttributeLen != 1)
		{
			status = CA_ERROR_INVALID;
		}
		else
		{
			status         = EVBME_ResetRF(req->mAttribute[0], pDeviceRef);
			EVBME_HasReset = req->mAttribute[0];
		}
		break;

	case EVBME_CFGPINS:
		if ((req->mAttributeLen != 1) || (req->mAttribute[0] > 0x03))
		{
			status = CA_ERROR_INVALID;
		}
		else
		{
			EVBME_UseMAC = req->mAttribute[0];
			status       = CA_ERROR_SUCCESS;
		}
		break;

	case EVBME_WAKEUPRF:
		status = CA_ERROR_SUCCESS;
		EVBME_WakeUpRF();
		break;

	default:
		status = CA_ERROR_UNKNOWN; /* what's that ??? */
		break;
	}

	//Send the confirm
	setConfirm.mStatus = status;
	MAC_Message(EVBME_SET_CONFIRM, sizeof(setConfirm), (uint8_t *)&setConfirm);

	return status;
}

#else // defined(USE_USB) || defined(USE_UART)

/** dummy stub function if no serial interface is defined */
static void EVBME_Message_DUMMY(char *pBuffer, size_t Count)
{
	(void)pBuffer;
	(void)Count;
}

/** dummy stub function if no serial interface is defined */
static void MAC_Message_DUMMY(u8_t CommandId, u8_t Count, const u8_t *pBuffer)
{
	(void)CommandId;
	(void)Count;
	(void)pBuffer;
}

#endif // defined(USE_USB) || defined(USE_UART)

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for Wakeup indication from CA-821X after reset
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_WakeupCallback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	ca_error  status;
	ca_error *context = (ca_error *)WAIT_GetContext();
	uint8_t   chipid[2];
	uint8_t   attlen;

	if (params->WakeUpCondition != HWME_WAKEUP_POWERUP)
	{
		ca_log_warn("CA-821X wakeup condition not power-up, check status");
		status = CA_ERROR_FAIL;
		goto exit;
	}

	if (TDME_ChipInit(pDeviceRef))
	{
		ca_log_crit("CA-821X initialisation failed\n");
		status = CA_ERROR_FAIL;
		goto exit;
	}

	if (HWME_GET_request_sync(HWME_CHIPID, &attlen, chipid, pDeviceRef))
	{
		ca_log_warn("CA-821X ID Failed");
		status = CA_ERROR_FAIL;
	}
	else
	{
		status = CA_ERROR_SUCCESS;
		ca_log_info("CA-821X V%d.%d", chipid[0], chipid[1]); // product id.version number
		// MAC_MPW for V0.x
		if (chipid[0] == 0)
			pDeviceRef->MAC_MPW = 1;
		else
			pDeviceRef->MAC_MPW = 0;
	}

exit:
	if (context)
		*context = status;
	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset RF device and check connection to RF
 *******************************************************************************
 * \param ms - Number of milliseconds to hold reset pin low for
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_ResetRF(uint8_t ms, struct ca821x_dev *pDeviceRef)
{
	ca_error status   = CA_ERROR_SUCCESS;
	ca_error cbStatus = CA_ERROR_FAIL;
	u8_t     CLKExternal_saved;

	// save external clock status
	CLKExternal_saved = CLKExternal;

	// switch off external clock
	EVBME_SwitchClock(pDeviceRef, 0);

	if (ms > 1) // reset RF
		BSP_ResetRF(ms);
	else
		BSP_ResetRF(2);

	status = WAIT_CallbackSwap(
	    SPI_HWME_WAKEUP_INDICATION, (ca821x_generic_callback)&EVBME_WakeupCallback, 100, &cbStatus, pDeviceRef);

	if (status == CA_ERROR_SUCCESS)
		status = cbStatus;

	if (status == CA_ERROR_SPI_WAIT_TIMEOUT)
	{
		ca_log_crit("CA-821X connection timed out waiting for wakeup indication, check hardware");
		goto exit;
	}

	// restore external clock status
	if (status == CA_ERROR_SUCCESS)
		EVBME_SwitchClock(pDeviceRef, CLKExternal_saved);

exit:
	return status;
} // End of EVBME_ResetRF()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset RF device and check connection to RF when GUI is connected
 *******************************************************************************
 * \param aAppName - App name string
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_Connect(const char *aAppName, struct ca821x_dev *pDeviceRef)
{
	ca_error    status;
	u64_t       device_id = BSP_GetUniqueId();
	const char *appname   = aAppName ? aAppName : "UNKNAPP";

	EVBME_HasReset = 1; // initialise/reset PIBs on higher layers
	ca_log_note("EVBME connected, %s, %s", appname, ca821x_get_version());

	if (device_id)
	{
		ca_log_note("Device ID: %08X%08X", (uint32_t)(device_id >> 32ULL), (uint32_t)device_id);
	}

	status = EVBME_ResetRF(50, pDeviceRef); // reset RF for 50 ms

	return status;
} // End of EVBME_Connect()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Switch External Clock from CAX on or off
 *******************************************************************************
 * \param on_offb - 0: Off 1: On
 * \param pDeviceRef   Pointer to initialised ca821x_device_ref struct
 * \retval CA_ERROR_SUCCESS for success, CA_ERROR_FAIL for failure
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_ExternalClock(u8_t on_offb, struct ca821x_dev *pDeviceRef)
{
	u8_t clkparam[2];

	if (on_offb)
	{
		clkparam[0] = 0x03; // 4 MHz
		clkparam[1] = 0x02; // GPIO2
	}
	else
	{
		clkparam[0] = 0x00; // turn clock off/
		clkparam[1] = 0x00;
	}
	if (HWME_SET_request_sync(HWME_SYSCLKOUT, 2, clkparam, pDeviceRef))
	{
		return CA_ERROR_FAIL;
	}
	return CA_ERROR_SUCCESS;
} // End of EVBME_CAX_ExternalClock()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Program CAX Low-Power Mode
 *******************************************************************************
 * \param mode - the required power down mode
 * \param sleeptime_ms - milliseconds to sleep for
 * \param pDeviceRef - a pointer to the ca821x_dev struct
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef)
{
	u8_t          pdparam[5];
	ca_mac_status status;
	u8_t          i;

	if ((mode == PDM_POWEROFF) || (mode == PDM_DPD))
		pdparam[0] = 0x1C; // power-off mode 1, wake-up by sleep timer
	else if (mode == PDM_POWERDOWN)
		pdparam[0] = 0x2A; // power-off mode 0, wake-up by gpio (ssb)
	else if (mode == PDM_STANDBY)
		pdparam[0] = 0x24; // standby mode,     wake-up by gpio (ssb)
	else
		return CA_ERROR_SUCCESS; // nothing to do, remain active

	pdparam[1] = LS0_BYTE(sleeptime_ms);
	pdparam[2] = LS1_BYTE(sleeptime_ms);
	pdparam[3] = LS2_BYTE(sleeptime_ms);
	pdparam[4] = LS3_BYTE(sleeptime_ms);

	if ((status = HWME_SET_request_sync(HWME_POWERCON, 5, pdparam, pDeviceRef)))
	{
		ca_log_crit("CA-821X PowerDown: %02X", status);
		return CA_ERROR_FAIL;
	}

	/* check max. 10 times if device has gone into power-down mode */
	for (i = 0; i < 10; ++i)
	{
		BSP_WaitUs(600);
		if (BSP_SenseRFIRQ()) /* device in power-down ? */
			return CA_ERROR_SUCCESS;
	}

	ca_log_crit("CA-821X No PowerDown");
	return CA_ERROR_FAIL;
} // End of EVBME_CAX_PowerDown()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for Wakeup indication from CA-821X after wakeup
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_Wakeup_callback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	u8_t condition = params->WakeUpCondition;
	u8_t mode;

	if (!WAIT_GetContext())
		return 0;
	mode = *((u8_t *)WAIT_GetContext());

	if ((((mode == PDM_STANDBY) && (condition != HWME_WAKEUP_STBY_GPIO)) ||
	     ((mode == PDM_POWERDOWN) && (condition != HWME_WAKEUP_POFF_GPIO)) ||
	     ((mode == PDM_POWEROFF) && (condition != HWME_WAKEUP_POFF_SLT))) &&
	    (condition != HWME_WAKEUP_POWERUP))
	{
		ca_log_warn("CA-821X wakeup: mode: %02X; condition: %02X", mode, condition);
	}

	if ((mode == PDM_POWEROFF) || (mode == PDM_POWERDOWN))
	{
		TDME_ChipInit(pDeviceRef);
	}

	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wait for CAX wakeup after power-down
 *******************************************************************************
 * CAX GPIO wake-up by sending dummy SPI packet, wait for wakeup message.
 *******************************************************************************
 * \param mode - Power-Down Mode
 * \param timeout_ms - Timeout for the wakeup indication (ms)
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_Wakeup(enum powerdown_mode mode, u32_t timeout_ms, struct ca821x_dev *pDeviceRef)
{
	ca_error status = CA_ERROR_SUCCESS;

	// wake up device by gpio by toggling chip select
	if ((mode == PDM_STANDBY) || (mode == PDM_POWERDOWN))
	{
		EVBME_WakeUpRF();
	}
	DISPATCH_ReadCA821x(pDeviceRef);

	status = WAIT_CallbackSwap(
	    SPI_HWME_WAKEUP_INDICATION, (ca821x_generic_callback)&EVBME_CAX_Wakeup_callback, timeout_ms, &mode, pDeviceRef);

	if (status)
	{
		ca_log_crit("CA-821X wakeup: status: %02X", status);
	}

	return status;
} // End of EVBME_CAX_Wakeup()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wake Up CAX RF Chip by toggling GPIO
 *******************************************************************************
 * requires several 32kHz cycles on SPI CSB
 *******************************************************************************
 ******************************************************************************/
static void EVBME_WakeUpRF(void)
{
	BSP_SetRFSSBLow();
	BSP_WaitUs(100);
	BSP_SetRFSSBHigh();
}

void cascoda_io_handler(struct ca821x_dev *pDeviceRef)
{
	CA_OS_LockAPI();
	DISPATCH_FromCA821x(pDeviceRef);
	TASKLET_Process();

#if defined(USE_USB) || defined(USE_UART)
	SerialGetCommand();
	if (SerialRxPending && SPI_IsFifoEmpty())
	{
		bool handled = false;
		if (cascoda_serial_dispatch)
		{
			if (cascoda_serial_dispatch(&SerialRxBuffer.CmdId, SerialRxBuffer.CmdLen + 2, pDeviceRef) > 0)
			{
				handled = true;
			}
		}
		if (EVBMEUpStreamDispatch(&SerialRxBuffer, pDeviceRef) > 0)
			handled = true;
		// production test (CHILI_TEST) and PHY test (TEST15_4) included
		if (TEST15_4_UpStreamDispatch(&SerialRxBuffer, pDeviceRef) > 0)
			handled = true;
		if (CHILI_TEST_UpStreamDispatch(&SerialRxBuffer, pDeviceRef) > 0)
			handled = true;
		if (!handled)
		{
			EVBMESendDownStream(&SerialRxBuffer.CmdId, pDeviceRef);
		}
#if defined(USE_UART)
		/* send RX_RDY */
		SerialSendRxRdy();
#endif /* USE_UART */
		SerialRxPending = false;
	}
#endif /* USE_UART || USE_USB */
	CA_OS_UnlockAPI();
	CA_OS_Yield();

	// production test (CHILI_TEST) and PHY test (TEST15_4) included
	if (CHILI_TEST_IsInTestMode())
		CHILI_TEST_Handler(pDeviceRef);
	else
		TEST15_4_Handler(pDeviceRef);
}

ca_error EVBME_NotHandled(const struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	if (MAC_Message)
	{
		MAC_Message(msg->CommandId, msg->Length, msg->PData.Payload);
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_NOT_HANDLED;
}

void EVBME_SwitchClock(struct ca821x_dev *pDeviceRef, u8_t useExternalClock)
{
	if (CLKExternal == useExternalClock)
		return;
	CLKExternal = useExternalClock;

	if (!CLKExternal)
	{
		// switch before turning external clock off
		BSP_UseExternalClock(CLKExternal);
	}

	if (EVBME_CAX_ExternalClock(CLKExternal, pDeviceRef))
	{
		//Failed
		CLKExternal = 0;
		BSP_UseExternalClock(CLKExternal);
		return;
	}

	if (CLKExternal)
	{
		// switch after turning external clock on
		BSP_UseExternalClock(CLKExternal);
	}

} // End of EVBME_SwitchClock()

void EVBME_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef)
{
	u8_t CLKExternal_saved;
	u8_t restarted = 0;
	u8_t framecounter[4];
	u8_t attlen;
	u8_t dsn[1];

	// preserve running counter values from MAC
	if ((mode == PDM_POWERDOWN) || (mode == PDM_POWEROFF))
	{
		MLME_GET_request_sync(macFrameCounter, 0, &attlen, framecounter, pDeviceRef);
		MLME_GET_request_sync(macDSN, 0, &attlen, dsn, pDeviceRef);
	}

#if defined(USE_USB)
	BSP_DisableUSB();
#endif /* USE_USB */
#if defined(USE_USB) || defined(USE_UART)
	BSP_SystemConfig(BSP_GetSystemFrequency(), 0);
#endif /* USE_UART || USE_USB */

	// save external clock status
	CLKExternal_saved = CLKExternal;

	// switch off external clock
	EVBME_SwitchClock(pDeviceRef, 0);

	// power down CAX
	EVBME_CAX_PowerDown(mode, sleeptime_ms, pDeviceRef);

	// power down
	if (mode == PDM_DPD) // mode has to use CAX sleep timer
	{
		BSP_PowerDown(sleeptime_ms, 0, 1);
		DISPATCH_ReadCA821x(pDeviceRef); /* read downstream message that has woken up device */
	}
	else if (mode == PDM_POWEROFF) // mode has to use CAX sleep timer
	{
		BSP_PowerDown(sleeptime_ms, 0, 0);
		DISPATCH_ReadCA821x(pDeviceRef); /* read downstream message that has woken up device */
	}
	else if (mode == PDM_ALLON)
		WAIT_ms(sleeptime_ms);
	else
		BSP_PowerDown(sleeptime_ms, 1, 0);

	// wake up CAX
	if ((mode == PDM_POWEROFF) && BSP_IsWatchdogTriggered())
	{
		EVBME_CAX_Restart(pDeviceRef);
		restarted = 1;
	}
	else
	{
		if ((mode != PDM_ACTIVE) && (mode != PDM_ALLON))
		{
			if (EVBME_CAX_Wakeup(mode, sleeptime_ms, pDeviceRef) != CA_ERROR_SUCCESS)
			{
				EVBME_CAX_Restart(pDeviceRef);
				restarted = 1;
			}
		}
	}

	// restore external clock status
	EVBME_SwitchClock(pDeviceRef, CLKExternal_saved);

	// call application specific re-initialisation
	if ((mode == PDM_POWERDOWN) || (mode == PDM_POWEROFF) || restarted)
	{
		if (cascoda_reinitialise)
			cascoda_reinitialise(pDeviceRef);
	}

#if defined(USE_USB) || defined(USE_UART)
	BSP_SystemConfig(BSP_GetSystemFrequency(), 1);
#endif /* USE_UART || USE_USB */
#if defined(USE_USB)
	BSP_EnableUSB();
#endif /* USE_USB */

	// restore running counter values to MAC
	if ((mode == PDM_POWERDOWN) || (mode == PDM_POWEROFF))
	{
		MLME_SET_request_sync(macFrameCounter, 0, 4, framecounter, pDeviceRef);
		MLME_SET_request_sync(macDSN, 0, 1, dsn, pDeviceRef);
	}

} // End of EVBME_PowerDown()

void EVBME_CAX_Restart(struct ca821x_dev *pDeviceRef)
{
	ca_error status = CA_ERROR_SUCCESS;

	EVBME_SwitchClock(pDeviceRef, 0);

	BSP_ResetRF(5);

	status = WAIT_Callback(SPI_HWME_WAKEUP_INDICATION, 20, NULL, pDeviceRef);

	if (status)
	{
		ca_log_crit("CA-821X restart: status: %02X", status);
		return;
	}

	if (TDME_ChipInit(pDeviceRef))
	{
		ca_log_crit("CA-821X restart: ChipInit fail");
		return;
	}
}

ca_error EVBMEInitialise(const char *aAppName, struct ca821x_dev *pDeviceRef)
{
	ca_error status;

	app_name = aAppName;
#ifdef stdout
	setbuf(stdout, NULL);
#endif

	BSP_Initialise(pDeviceRef); // initialise chip

	SPI_Initialise();

	CA_OS_Init();

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	BSP_ExternalFlashInit(); // Initialisation for using the external flash
#endif

	EVBME_HasReset = 1; // initialise/reset PIBs on higher layers

	// function pointer assignments
#if defined(USE_USB)
	EVBME_Message = EVBME_Message_USB;
	MAC_Message   = MAC_Message_USB;
#elif defined(USE_UART)
	EVBME_Message = EVBME_Message_UART;
	MAC_Message   = MAC_Message_UART;
#else
	EVBME_Message = EVBME_Message_DUMMY;
	MAC_Message   = MAC_Message_DUMMY;
#endif

	status = EVBME_Connect(aAppName, pDeviceRef); // reset and connect RF

	pDeviceRef->callbacks.generic_dispatch = &EVBME_NotHandled;

	// EVBME connected, so use the radio for the clock reference
	EVBME_SwitchClock(pDeviceRef, 1);

	if (status == CA_ERROR_SUCCESS)
	{
		RAND_SetCryptoEntropyDev(pDeviceRef);
	}

	// production test (CHILI_TEST) and PHY test (TEST15_4) included
	CHILI_TEST_Initialise(status, pDeviceRef); /* status used for test fail so keep outside check */
	if (!status)
		TEST15_4_Initialise(pDeviceRef);

	return status;
}

const char *EVBME_GetAppName(void)
{
	return app_name;
}

CA_TOOL_WEAK ca_error EVBME_GET_OT_Attrib(enum evbme_attribute aAttrib, uint8_t *aOutBufLen, uint8_t *aOutBuf)
{
	(void)aAttrib;
	(void)aOutBufLen;
	(void)aOutBuf;
	return CA_ERROR_UNKNOWN;
}
