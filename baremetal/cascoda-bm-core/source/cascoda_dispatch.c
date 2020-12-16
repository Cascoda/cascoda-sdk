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
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "mac_messages.h"

enum MDR_Cache
{
	MDR_CacheSize = 15,
};

enum MDR_CacheType
{
	MDR_CacheTypePCPS = 0,
	MDR_CacheTypeMCPS = 1,
};

static struct MDR_CacheItem
{
	uint16_t mTimeout;           /**< Timeout for direct transmissions */
	uint8_t  mMsduHandle;        /**< MSDU Handle for the cached transmission */
	uint8_t  mIsIndirect : 1;    /**< Flag tracking indirect-ness */
	uint8_t  mIsCacheActive : 1; /**< Is the cache member currently valid and in use */
	uint8_t  mCacheType : 1;     /**< Cache type enum MDR_Cache (Only reason this is a uint8 is for struct packing) */
} MDR_Cache[MDR_CacheSize];

static bool             isPCPSFixActive       = false;
static uint8_t          isPCPSRxOn            = 0;
static volatile uint8_t isReadPending         = 0;
static bool             isCacheFlushFixActive = false;
static bool             isScanInProgress      = false;

static void DispatchFromCa821x(struct MAC_Message *aMessage, struct ca821x_dev *pDeviceRef);

/** Get the confirm command id from the cache type */
static uint8_t CacheTypeToCmdId(enum MDR_CacheType aCacheType)
{
	switch (aCacheType)
	{
	case MDR_CacheTypeMCPS:
		return SPI_MCPS_DATA_CONFIRM;
#if CASCODA_CA_VER == 8211
	case MDR_CacheTypePCPS:
		return SPI_PCPS_DATA_CONFIRM;
#endif
	default:
		abort();
	}

	return 0;
}

/** Purge the cache of all requests, optionally generating a confirm for each */
static void CachePurge(bool GenerateConfirms, struct ca821x_dev *pDeviceRef)
{
	struct staticMDC
	{
		uint8_t                       commandId;
		uint8_t                       len;
		struct MCPS_DATA_confirm_pset params;
	} genMdc;

	memset(&genMdc, 0, sizeof(genMdc));
	genMdc.len           = sizeof(genMdc.params);
	genMdc.params.Status = MAC_SYSTEM_ERROR;

	for (size_t i = 0; i < MDR_CacheSize; i++)
	{
		if (MDR_Cache[i].mIsCacheActive && GenerateConfirms)
		{
			genMdc.commandId         = CacheTypeToCmdId(MDR_Cache[i].mCacheType);
			genMdc.params.MsduHandle = MDR_Cache[i].mMsduHandle;
			DispatchFromCa821x((struct MAC_Message *)&genMdc, pDeviceRef);
		}
		MDR_Cache[i].mIsCacheActive = false;
	}
}

/** Check if there are currently any indirect messages in the cache at all */
static bool CacheIsIndirectActive(void)
{
	for (size_t i = 0; i < MDR_CacheSize; i++)
	{
		if (MDR_Cache[i].mIsCacheActive && MDR_Cache[i].mIsIndirect)
			return true;
	}
	return false;
}

/** Remove a particular cache message, identified by a handle */
static ca_error CacheRemoveItem(uint8_t aMsduHandle, enum MDR_CacheType aCacheType)
{
	ca_error error = CA_ERROR_NOT_FOUND;

	for (size_t i = 0; i < MDR_CacheSize; i++)
	{
		if (MDR_Cache[i].mIsCacheActive && MDR_Cache[i].mMsduHandle == aMsduHandle &&
		    MDR_Cache[i].mCacheType == aCacheType)
		{
			error                       = CA_ERROR_SUCCESS;
			MDR_Cache[i].mIsCacheActive = false;
			break;
		}
	}

	return error;
}

/** Add a cache message, identified by a handle */
static ca_error CacheAddItem(uint8_t aMsduHandle, bool aIsIndirect, enum MDR_CacheType aCacheType)
{
	ca_error error = CA_ERROR_NO_BUFFER;

	for (size_t i = 0; i < MDR_CacheSize; i++)
	{
		if (!MDR_Cache[i].mIsCacheActive)
		{
			MDR_Cache[i].mIsCacheActive = true;
			MDR_Cache[i].mIsIndirect    = aIsIndirect;
			MDR_Cache[i].mMsduHandle    = aMsduHandle;
			MDR_Cache[i].mCacheType     = aCacheType;
			MDR_Cache[i].mTimeout       = 500; //TODO: Figure out this value properly
			error                       = CA_ERROR_SUCCESS;
			break;
		}
	}

	return error;
}

/** Decay the timeouts in the cache by the given amount of time */
static ca_error CacheDecay(void)
{
	static uint32_t prevTime = 0;
	uint32_t        curTime  = TIME_ReadAbsoluteTime();
	uint32_t        delta    = curTime - prevTime;

	//No reason to check cache as no time has passed
	if (delta == 0)
		return CA_ERROR_SUCCESS;

	//No reason to check cache because it is currently being flushed
	if (isCacheFlushFixActive)
		return CA_ERROR_SUCCESS;

	for (size_t i = 0; i < MDR_CacheSize; i++)
	{
		if (MDR_Cache[i].mIsCacheActive && !MDR_Cache[i].mIsIndirect)
		{
			if (delta > MDR_Cache[i].mTimeout)
				return CA_ERROR_TIMEOUT;
			else
				MDR_Cache[i].mTimeout -= delta;
		}
	}
	prevTime = curTime;

	return CA_ERROR_SUCCESS;
}

/** Freeze the CA821x, flush the cache, reinitialise memory */
static void ApplyCacheFlushFix(struct ca821x_dev *pDeviceRef)
{
	uint8_t len, rxOnOld, rxOnNew = 0;

	if (isCacheFlushFixActive)
		return;

	isCacheFlushFixActive = true;
	MLME_GET_request_sync(macRxOnWhenIdle, 0, &len, &rxOnOld, pDeviceRef);
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxOnNew, pDeviceRef);
	WAIT_Callback(SPI_HWME_WAKEUP_INDICATION, 15, NULL, pDeviceRef);
	CachePurge(true, pDeviceRef);
	MLME_RESET_request_sync(0, pDeviceRef);
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxOnOld, pDeviceRef);
	isCacheFlushFixActive = false;

	CacheDecay();
}

/** Fix pcps allocations on memory boundaries */
static inline void ApplyPCPSHotFix(struct PCPS_DATA_request_pset *aPset, struct ca821x_dev *pDeviceRef)
{
	uint8_t len, rxOnNew = 0;
	uint8_t data1[3] = {0x93, 0x68, 0x52};
	uint8_t data2[3] = {0x93, 0x68, 0xFF};

	//TODO: This only works when using the hardware fcs
	switch (aPset->PsduLength)
	{
	case 9:
	case 17:
	case 33:
	case 65:
	case 129:
		//TODO: Disabling receiver is not necessary in PCPS-only receive mode
		MLME_GET_request_sync(macRxOnWhenIdle, 0, &len, &isPCPSRxOn, pDeviceRef);
		MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxOnNew, pDeviceRef);
		WAIT_Callback(SPI_HWME_WAKEUP_INDICATION, 5, NULL, pDeviceRef);
		//Disable txSM by forcing state to 0xFF
		HWME_SET_request_sync(0x88, 3, data1, pDeviceRef);
		HWME_SET_request_sync(0x8A, 3, data2, pDeviceRef);
		//Disable hardware Tx FCF
		TDME_SETSFR_request_sync(0, 0xD9, 0x0D, pDeviceRef);
		isPCPSFixActive = true;
		break;
	}
}

/** Fix pcps allocations on memory boundaries - after command */
static inline void ApplyPCPSPostFix(struct ca821x_dev *pDeviceRef)
{
	if (isPCPSFixActive)
	{
		uint8_t data[] = {0x93, 0x68, 0x00};

		isPCPSFixActive = false;
		//Enable hardware tx FCS
		TDME_SETSFR_request_sync(0, 0xD9, 0x0F, pDeviceRef);
		//Re-enable txSM By setting state to IDLE
		HWME_SET_request_sync(0x8A, 3, data, pDeviceRef);
		//Re-enable receiver
		MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &isPCPSRxOn, pDeviceRef);
	}
}

/** Hotfix for sending both direct and indirect transmissions */
static inline void ApplyMixedDirectHotfix(struct ca821x_dev *pDeviceRef)
{
	const uint8_t hotfix[] = {0x4D, 0x01, 0x00};
	uint8_t       reply[3];
	SPI_Send(hotfix, 3, reply, pDeviceRef);
}

/**
 * Check if MAC MLME SET/GET Command requires intervention (that isn't in the hard mac)
 *
 * @param msg - MAC message to process
 * @param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *
 * @returns ca_error value
 * @retval  CA_ERROR_SUCCESS  Message was not consumed (should go to CA821x)
 * @retval  CA_ERROR_ALREADY  Message was consumed, and workaround already applied
 */
static ca_error CheckSetGet(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	uint8_t            status, val;
	struct MAC_Message response;

	if (msg->CommandId == SPI_MLME_SET_REQUEST)
	{
		if (TDME_CheckPIBAttribute(msg->PData.SetReq.PIBAttribute,
		                           msg->PData.SetReq.PIBAttributeLength,
		                           msg->PData.SetReq.PIBAttributeValue))
		{
			response.CommandId                      = SPI_MLME_SET_CONFIRM;
			response.Length                         = 3;
			response.PData.SetCnf.Status            = MAC_INVALID_PARAMETER;
			response.PData.SetCnf.PIBAttribute      = msg->PData.SetReq.PIBAttribute;
			response.PData.SetCnf.PIBAttributeIndex = msg->PData.SetReq.PIBAttributeIndex;
			DispatchFromCa821x(&response, pDeviceRef);
			return CA_ERROR_ALREADY;
		}
	}

	// MLME set / get phyTransmitPower
	if ((msg->CommandId == SPI_MLME_SET_REQUEST) && (msg->PData.SetReq.PIBAttribute == phyTransmitPower))
	{
		status                                  = TDME_SetTxPower(msg->PData.SetReq.PIBAttributeValue[0], pDeviceRef);
		response.CommandId                      = SPI_MLME_SET_CONFIRM;
		response.Length                         = 3;
		response.PData.SetCnf.Status            = status;
		response.PData.SetCnf.PIBAttribute      = msg->PData.SetReq.PIBAttribute;
		response.PData.SetCnf.PIBAttributeIndex = msg->PData.SetReq.PIBAttributeIndex;
		DispatchFromCa821x(&response, pDeviceRef);
		return CA_ERROR_ALREADY;
	}
	if ((msg->CommandId == SPI_MLME_GET_REQUEST) && (msg->PData.SetReq.PIBAttribute == phyTransmitPower))
	{
		status                                     = TDME_GetTxPower(&val, pDeviceRef);
		response.CommandId                         = SPI_MLME_GET_CONFIRM;
		response.Length                            = 5;
		response.PData.GetCnf.Status               = status;
		response.PData.GetCnf.PIBAttribute         = msg->PData.GetReq.PIBAttribute;
		response.PData.GetCnf.PIBAttributeIndex    = msg->PData.GetReq.PIBAttributeIndex;
		response.PData.GetCnf.PIBAttributeLength   = 1;
		response.PData.GetCnf.PIBAttributeValue[0] = val;
		DispatchFromCa821x(&response, pDeviceRef);
		return CA_ERROR_ALREADY;
	}

	return CA_ERROR_SUCCESS;
}

/**
 * Check if the channel is being changed, and call TDME_ChannelInit accordingly.
 * @param msg  MAC message to process
 * @param pDeviceRef Cascoda Device reference
 */
static inline void CheckChannel(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	if (msg->CommandId == SPI_MLME_ASSOCIATE_REQUEST)
		TDME_ChannelInit(msg->PData.AssocReq.LogicalChannel, pDeviceRef);
	else if (msg->CommandId == SPI_MLME_START_REQUEST)
		TDME_ChannelInit(msg->PData.StartReq.LogicalChannel, pDeviceRef);
	else if ((msg->CommandId == SPI_MLME_SET_REQUEST) && (msg->PData.SetReq.PIBAttribute == phyCurrentChannel))
		TDME_ChannelInit(msg->PData.SetReq.PIBAttributeValue[0], pDeviceRef);
	else if ((msg->CommandId == SPI_TDME_SET_REQUEST) && (msg->PData.TDMESetReq.TDAttribute == TDME_CHANNEL))
		TDME_ChannelInit(msg->PData.TDMESetReq.TDAttributeValue[0], pDeviceRef);
}

/** Checks to run before sending a message to the CA821x */
static ca_error PreCheckToCA821x(const uint8_t *buf, struct ca821x_dev *pDeviceRef)
{
	ca_error            Status = CA_ERROR_SUCCESS;
	struct MAC_Message *msg    = (struct MAC_Message *)buf;

	if (isScanInProgress)
	{
		Status = CA_ERROR_SPI_SCAN_IN_PROGRESS;
		goto exit;
	}

	if ((Status = CheckSetGet(msg, pDeviceRef)))
		goto exit;

	CheckChannel(msg, pDeviceRef);

#if CASCODA_CA_VER == 8211
	if (msg->CommandId == SPI_PCPS_DATA_REQUEST)
	{
		ApplyPCPSHotFix(&(msg->PData.PhyDataReq), pDeviceRef);
	}
#endif

exit:
	return Status;
}

/** Checks to run after sending a message to the CA821x */
static ca_error PostCheckToCA821x(const uint8_t *buf, const uint8_t *rspbuf, struct ca821x_dev *pDeviceRef)
{
	ca_error            Status = CA_ERROR_SUCCESS;
	struct MAC_Message *msg    = (struct MAC_Message *)buf;
	struct MAC_Message *rsp    = (struct MAC_Message *)rspbuf;

	if (msg->CommandId == SPI_MCPS_DATA_REQUEST)
	{
		bool isIndirect = (msg->PData.DataReq.TxOptions & TXOPT_INDIRECT);

		CacheAddItem(msg->PData.DataReq.MsduHandle, isIndirect, MDR_CacheTypeMCPS);

		if ((CASCODA_CA_VER == 8211) && !isIndirect && CacheIsIndirectActive())
		{
			ApplyMixedDirectHotfix(pDeviceRef);
		}
	}

	if (msg->CommandId == SPI_MLME_SCAN_REQUEST)
	{
		isScanInProgress = true;
	}

#if CASCODA_CA_VER == 8211
	if (msg->CommandId == SPI_PCPS_DATA_REQUEST)
	{
		bool isIndirect = (msg->PData.PhyDataReq.TxOpts & TXOPT_INDIRECT);

		CacheAddItem(msg->PData.PhyDataReq.PsduHandle, isIndirect, MDR_CacheTypePCPS);
		if (!isIndirect && CacheIsIndirectActive())
		{
			ApplyMixedDirectHotfix(pDeviceRef);
		}
	}

	if (msg->CommandId == SPI_PCPS_DATA_REQUEST)
	{
		ApplyPCPSPostFix(pDeviceRef);
	}
#endif

	if (rsp && rsp->CommandId == SPI_MCPS_PURGE_CONFIRM && rsp->PData.PurgeCnf.Status == MAC_SUCCESS)
	{
		CacheRemoveItem(rsp->PData.PurgeCnf.MsduHandle, MDR_CacheTypeMCPS);
	}

	if (rsp && rsp->CommandId == SPI_MLME_RESET_CONFIRM && rsp->PData.Status == MAC_SUCCESS)
	{
		CachePurge(false, pDeviceRef);
	}

	return Status;
}

/** Checks to run after receiving command from CA821x, but before dispatching */
static ca_error PreCheckFromCA821x(struct MAC_Message *aMessage)
{
	ca_error error = CA_ERROR_SUCCESS;

	if (aMessage->CommandId == SPI_MCPS_DATA_CONFIRM)
	{
		CacheRemoveItem(aMessage->PData.DataCnf.MsduHandle, MDR_CacheTypeMCPS);
	}
	if (aMessage->CommandId == SPI_MLME_SCAN_CONFIRM || aMessage->CommandId == SPI_HWME_WAKEUP_INDICATION)
	{
		isScanInProgress = false;
	}
#if CASCODA_CA_VER == 8211
	else if (aMessage->CommandId == SPI_PCPS_DATA_CONFIRM)
	{
		CacheRemoveItem(aMessage->PData.PhyDataCnf.PsduHandle, MDR_CacheTypePCPS);
	}
#endif

	return error;
}

/** Dispatch a command as if it came from the ca821x */
static void DispatchFromCa821x(struct MAC_Message *aMessage, struct ca821x_dev *pDeviceRef)
{
	ca_error ret;

	ret = ca821x_downstream_dispatch(aMessage, pDeviceRef);
	if (ret != CA_ERROR_SUCCESS)
	{
		ca_log_crit("Err %s dispatching on SPI %02x!", ca_error_str(ret), aMessage->CommandId);
	}
}

void DISPATCH_ReadCA821x(struct ca821x_dev *pDeviceRef)
{
	uint8_t rfirq;

	BSP_DisableRFIRQ();
	rfirq = BSP_SenseRFIRQ();

	if (SPI_IsExchangeInProgress())
	{
		//Do nothing...
	}
	else if (!rfirq && !SPI_IsFifoAlmostFull() && !SPI_IsSyncChainInFlight())
	{
		SPI_Exchange(NULLP, pDeviceRef);
		isReadPending = 0;
	}
	else if (!rfirq)
	{
		isReadPending = 1;
	}
	else
	{
		isReadPending = 0;
	}

	BSP_EnableRFIRQ();
}

ca_error DISPATCH_ToCA821x(const uint8_t *buf, size_t len, u8_t *response, struct ca821x_dev *pDeviceRef)
{
	ca_error Status = CA_ERROR_SUCCESS;

	if (response)
		response[0] = SPI_IDLE;

	if (!Status)
		Status = PreCheckToCA821x(buf, pDeviceRef);
	if (!Status)
		Status = SPI_Send(buf, len, response, pDeviceRef);
	if (!Status)
		Status = PostCheckToCA821x(buf, response, pDeviceRef);

	if (Status == CA_ERROR_ALREADY)
		Status = CA_ERROR_SUCCESS;
	return Status;
}

ca_error ca821x_api_downstream(const uint8_t *buf, size_t len, uint8_t *response, struct ca821x_dev *pDeviceRef)
{
	return DISPATCH_ToCA821x(buf, len, response, pDeviceRef);
}

ca_error DISPATCH_FromCA821x(struct ca821x_dev *pDeviceRef)
{
	struct MAC_Message *RxMessage;
	static bool         isDispatching = false;

	if (isDispatching)
		return CA_ERROR_INVALID_STATE;

	isDispatching = true;
	while ((RxMessage = SPI_PeekFullBuf()) != NULL)
	{
		PreCheckFromCA821x(RxMessage);
		DispatchFromCa821x(RxMessage, pDeviceRef);
		SPI_DequeueFullBuf();
	}
	isDispatching = false;

	if (CacheDecay() == CA_ERROR_TIMEOUT)
	{
		ApplyCacheFlushFix(pDeviceRef);
	}

	//If stalled, read pending message
	if (isReadPending)
		DISPATCH_ReadCA821x(pDeviceRef);

	return CA_ERROR_SUCCESS;
}
