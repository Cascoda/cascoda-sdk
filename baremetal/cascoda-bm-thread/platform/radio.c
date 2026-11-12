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
 *   This file implements the OpenThread platform abstraction for radio
 *   communication.
 *
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "openthread/platform/logging.h"
#include "openthread/platform/radio-mac.h"
#include "openthread/random_noncrypto.h"
#include "openthread/thread.h"

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "code_utils.h"
#include "ieee_802_15_4.h"
#include "mac_messages.h"
#include "platform.h"

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))

static uint8_t sIeeeEui64[8];

//For cascoda API
static struct ca821x_dev *pDeviceRef = NULL;

static int8_t sNoiseFloor = 127;

static uint8_t sRadioInitialised      = 0;
static volatile uint8_t sIsExpectingIndication = 0;

#if CASCODA_CA_VER <= 8211
static volatile uint8_t sIsDataPollInProgress = 0;
static volatile uint8_t sUnicastReceivedDuringDataPoll = 0;

static ca_tasklet sHandleUnicastIndTasklet;
static ca_tasklet sResetDataPollInProgressTasklet;
static otDataIndication sUnicastDataIndDuringPoll = {0};

struct M_KeyDescriptor_thread
{
	struct M_KeyTableEntryFixed Fixed;
	struct M_KeyIdLookupDesc    KeyIdLookupList[1];
	uint8_t                     flags[40]; //TODO: Provisional length
	                                       //struct M_KeyDeviceDesc         KeyDeviceList[count];
	                                       //struct M_KeyUsageDesc          KeyUsageList[2];
};
#endif // CASCODA_CA_VER <= 8211

struct ca821x_dev *PlatformGetDeviceRef()
{
	return pDeviceRef;
}

static otError ConvertErrorMacToOt(ca_mac_status aMacError)
{
	otError error;

	switch (aMacError)
	{
	case MAC_SUCCESS:
	case MAC_NO_DATA:
		error = OT_ERROR_NONE;
		break;
	case MAC_NO_ACK:
		error = OT_ERROR_NO_ACK;
		break;
	case MAC_CHANNEL_ACCESS_FAILURE:
		error = OT_ERROR_CHANNEL_ACCESS_FAILURE;
		break;
	case MAC_TRANSACTION_OVERFLOW:
		error = OT_ERROR_NO_BUFS;
		break;
	case MAC_INVALID_PARAMETER:
	case MAC_UNSUPPORTED_ATTRIBUTE:
	case MAC_INVALID_INDEX:
	case MAC_FRAME_TOO_LONG:
		error = OT_ERROR_INVALID_ARGS;
		break;
	case MAC_NO_SHORT_ADDRESS:
	case MAC_UNAVAILABLE_KEY:
		error = OT_ERROR_INVALID_STATE;
		break;
	case MAC_READ_ONLY:
		error = OT_ERROR_NOT_CAPABLE;
		break;
	case MAC_INVALID_HANDLE:
		error = OT_ERROR_ALREADY;
		break;
	case MAC_SYSTEM_ERROR:
		error = OT_ERROR_FAILED;
		break;
	default:
		ca_log_debg("MAC Error %x unconverted", aMacError);
		error = OT_ERROR_ABORT;
		break;
	}

	return error;
}

#if CASCODA_CA_VER <= 8211
static ca_error dataPollDone(void)
{
	uint8_t rxidl = 0;
    ca_mac_status status = CA_ERROR_SUCCESS;

    // Done with the data poll, turn the receiver off
	if ((status = MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxidl, pDeviceRef)))
    {
        ca_log_crit("pollDone: FAILED TO TURN RECEIVER BACK OFF! err: %d", status);
    }
    ca_log_debg("pollDone: RECEIVER TURNED OFF");

    // Update the state after at least a 30ms delay, to allow for correct handling
    // (i.e. broadcast filtering) of any final messages that might still be coming from
    // the MAC, due to having been received during the processing time that it took
    // to request the receiver to turn off.
    TASKLET_ScheduleDelta(&sResetDataPollInProgressTasklet, 30, NULL);

    return status;
}

static ca_error handleUnicastIndDuringPoll(void *context)
{
    (void)context;
    ca_log_debg("handleUni: sIsExpectingIndication %d -> 0", sIsExpectingIndication);
    sIsExpectingIndication = 0;
    otPlatMcpsDataIndication(OT_INSTANCE, &sUnicastDataIndDuringPoll);
    return CA_ERROR_SUCCESS;
}

static ca_error resetDataPollInProgress(void *context)
{
    (void) context;
    ca_log_debg("pollDone: sIsDataPollInProgress %d -> 0", sIsDataPollInProgress);
    sIsDataPollInProgress = 0;
    return CA_ERROR_SUCCESS;
}
#endif // CASCODA_CA_VER <= 8211


otError otPlatMlmeGet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t *aLen, uint8_t *aBuf)
{
	(void)aInstance;
	uint8_t error;
	otError otErr;

#if CASCODA_CA_VER <= 8211 // TODO: Implement equivalent for CA8212 security
	//Adaption for security table
	if (aAttr == OT_PIB_MAC_KEY_TABLE)
	{
		struct M_KeyDescriptor_thread caKeyDesc  = {0};
		otKeyTableEntry              *otKeyDesc  = (otKeyTableEntry *)aBuf;
		uint8_t                       flagOffset = 0;

		error = MLME_GET_request_sync(aAttr, aIndex, aLen, (uint8_t *)(&caKeyDesc), pDeviceRef);

		//Convert to ot format
		otKeyDesc->mKeyIdLookupListEntries = caKeyDesc.Fixed.KeyIdLookupListEntries;
		otKeyDesc->mKeyDeviceListEntries   = caKeyDesc.Fixed.KeyDeviceListEntries;
		otKeyDesc->mKeyUsageListEntries    = caKeyDesc.Fixed.KeyUsageListEntries;

		otEXPECT_ACTION(otKeyDesc->mKeyIdLookupListEntries <= ARRAY_LENGTH(otKeyDesc->mKeyIdLookupDesc),
		                otErr = OT_ERROR_GENERIC);
		otEXPECT_ACTION(otKeyDesc->mKeyDeviceListEntries <= ARRAY_LENGTH(otKeyDesc->mKeyDeviceDesc),
		                otErr = OT_ERROR_GENERIC);
		otEXPECT_ACTION(otKeyDesc->mKeyUsageListEntries <= ARRAY_LENGTH(otKeyDesc->mKeyUsageDesc),
		                otErr = OT_ERROR_GENERIC);

		memcpy(otKeyDesc->mKey, caKeyDesc.Fixed.Key, sizeof(otKeyDesc->mKey));
		otKeyDesc->mKeyIdLookupDesc[0] = *((struct otKeyIdLookupDesc *)&(caKeyDesc.KeyIdLookupList[0]));

		for (int i = 0; i < otKeyDesc->mKeyDeviceListEntries; i++, flagOffset++)
		{
			otKeyDesc->mKeyDeviceDesc[i].mDeviceDescriptorHandle =
			    caKeyDesc.flags[flagOffset] & KDD_DeviceDescHandleMask;
			otKeyDesc->mKeyDeviceDesc[i].mUniqueDevice = !!(caKeyDesc.flags[flagOffset] & KDD_UniqueDeviceMask);
			otKeyDesc->mKeyDeviceDesc[i].mBlacklisted  = !!(caKeyDesc.flags[flagOffset] & KDD_BlacklistedMask);
#if OPENTHREAD_CONFIG_EXTERNAL_MAC_SHARED_DD && (CASCODA_CA_VER != 8210)
			otKeyDesc->mKeyDeviceDesc[i].mNew = !!(caKeyDesc.flags[flagOffset] & KDD_NewMask);
#endif
		}

		for (int i = 0; i < otKeyDesc->mKeyUsageListEntries; i++, flagOffset++)
		{
			uint8_t flag                           = caKeyDesc.flags[flagOffset];
			otKeyDesc->mKeyUsageDesc[i].mFrameType = flag & KUD_FrameTypeMask;
			otKeyDesc->mKeyUsageDesc[i].mCommandFrameId =
			    (flag & KUD_CommandFrameIdentifierMask) >> KUD_CommandFrameIdentifierShift;
		}

		*aLen = sizeof(otKeyTableEntry);
	}
	else
#endif // CASCODA_CA_VER <= 8211
	{
		error = MLME_GET_request_sync(aAttr, aIndex, aLen, aBuf, pDeviceRef);
	}

	otErr = ConvertErrorMacToOt(error);

exit:
	return otErr;
}

otError otPlatMlmeSet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t aLen, const uint8_t *aBuf)
{
	(void)aInstance;
	uint8_t error;
	otError otErr;

#if CASCODA_CA_VER <= 8211
	//Adaption for security table
	if (aAttr == OT_PIB_MAC_KEY_TABLE)
	{
		struct M_KeyDescriptor_thread caKeyDesc  = {0};
		const otKeyTableEntry        *otKeyDesc  = (otKeyTableEntry *)aBuf;
		uint8_t                       flagOffset = 0;

		caKeyDesc.Fixed.KeyIdLookupListEntries = otKeyDesc->mKeyIdLookupListEntries;
		caKeyDesc.Fixed.KeyDeviceListEntries   = otKeyDesc->mKeyDeviceListEntries;
		caKeyDesc.Fixed.KeyUsageListEntries    = otKeyDesc->mKeyUsageListEntries;

		memcpy(caKeyDesc.Fixed.Key, otKeyDesc->mKey, sizeof(otKeyDesc->mKey));
		caKeyDesc.KeyIdLookupList[0] = *((struct M_KeyIdLookupDesc *)&(otKeyDesc->mKeyIdLookupDesc[0]));

		for (int i = 0; i < otKeyDesc->mKeyDeviceListEntries; i++, flagOffset++)
		{
			const otKeyDeviceDesc *devDesc = &(otKeyDesc->mKeyDeviceDesc[i]);
			caKeyDesc.flags[flagOffset]    = devDesc->mDeviceDescriptorHandle & KDD_DeviceDescHandleMask;
			caKeyDesc.flags[flagOffset] |= devDesc->mUniqueDevice ? KDD_UniqueDeviceMask : 0;
			caKeyDesc.flags[flagOffset] |= devDesc->mBlacklisted ? KDD_BlacklistedMask : 0;
#if OPENTHREAD_CONFIG_EXTERNAL_MAC_SHARED_DD && (CASCODA_CA_VER != 8210)
			caKeyDesc.flags[flagOffset] |= devDesc->mNew ? KDD_NewMask : 0;
#endif
		}

		for (int i = 0; i < otKeyDesc->mKeyUsageListEntries; i++, flagOffset++)
		{
			const otKeyUsageDesc *useDesc = &(otKeyDesc->mKeyUsageDesc[i]);
			uint8_t               flag;

			flag = useDesc->mFrameType & KUD_FrameTypeMask;
			flag |= ((useDesc->mCommandFrameId << KUD_CommandFrameIdentifierShift) & KUD_CommandFrameIdentifierMask);

			caKeyDesc.flags[flagOffset] = flag;
		}

		aLen  = sizeof(caKeyDesc) + flagOffset - sizeof(caKeyDesc.flags);
		error = MLME_SET_request_sync(aAttr, aIndex, aLen, (uint8_t *)(&caKeyDesc), pDeviceRef);
	}
	else
#endif // CASCODA_CA_VER <= 8211
	{
		//TODO: Need to also adapt for security table when using CA8212 as well.
		error = MLME_SET_request_sync(aAttr, aIndex, aLen, aBuf, pDeviceRef);
	}

	otErr = ConvertErrorMacToOt(error);

	return otErr;
}

otError otPlatMlmeReset(otInstance *aInstance, bool setDefaultPib)
{
	(void)aInstance;
	ca_mac_status error;

	error = MLME_RESET_request_sync(setDefaultPib, pDeviceRef);
	if(!error)
		error = TDME_ReconfigurePIB(setDefaultPib, pDeviceRef);

	return ConvertErrorMacToOt(error);
}

otError otPlatMlmeStart(otInstance *aInstance, otStartRequest *aStartReq)
{
	(void)aInstance;
	ca_mac_status error;
	otError       otErr;

	error = MLME_START_request_sync(aStartReq->mPanId,
	                                aStartReq->mLogicalChannel,
	                                aStartReq->mBeaconOrder,
	                                aStartReq->mSuperframeOrder,
	                                aStartReq->mPanCoordinator,
	                                aStartReq->mBatteryLifeExtension,
	                                aStartReq->mCoordRealignment,
	                                (struct SecSpec *)&(aStartReq->mCoordRealignSecurity),
	                                (struct SecSpec *)&(aStartReq->mBeaconSecurity),
	                                pDeviceRef);

	otErr = ConvertErrorMacToOt(error);

	return otErr;
}

otError otPlatMlmeScan(otInstance *aInstance, otScanRequest *aScanRequest)
{
	(void)aInstance;
	ca_mac_status error;

	error = MLME_SCAN_request(aScanRequest->mScanType,
	                          aScanRequest->mScanChannelMask,
	                          aScanRequest->mScanDuration,
	                          (struct SecSpec *)&(aScanRequest->mSecSpec),
	                          pDeviceRef);

	return ConvertErrorMacToOt(error);
}

otError otPlatMlmePollRequest(otInstance *aInstance, otPollRequest *aPollRequest)
{
	(void)aInstance;
	ca_mac_status error;
	uint8_t sSendUnicastIndToOpenthread = 0;

#if CASCODA_CA_VER >= 8212
	error = MLME_POLL_request(*((struct FullAddr *)&(aPollRequest->mCoordAddress)),
	                          0,
	                          (struct SecSpec *)&(aPollRequest->mSecurity),
	                          pDeviceRef);
#else

    // Only allow one poll request at a time
    if (sIsDataPollInProgress)
    {
        ca_log_debg("poll_req: error: OT_ERROR_ALREADY");
        return OT_ERROR_ALREADY;
    }

    uint8_t attlen;
    uint16_t waittime = 0;
	uint8_t rxidl = 1;

    // Find out the rxon timeout duration from the MAC PIB (in number of symbols)
    if ((error = MLME_GET_request_sync(macMaxFrameTotalWaitTime, 0, &attlen, &waittime, pDeviceRef)))
    {
        ca_log_crit("poll_req: MLME GET macMaxFrameTotalWaitTime FAILED");
        goto exit;
    }
    ca_log_debg("poll_req: waittime in num of symbols: %d", waittime);

    // Convert to ms (1 symbol = 16 us, as defined in the MAC)
    waittime = (waittime * 16) / 1000;
    ca_log_debg("poll_req: waittime_ms: %d", waittime);

    // Starting states
    ca_log_debg("poll_req: sIsDataPollInProgress %d -> 1", sIsDataPollInProgress);
    sIsDataPollInProgress = 1;
    ca_log_debg("poll_req: sUnicastReceivedDuringDataPoll %d -> 0", sUnicastReceivedDuringDataPoll);
    sUnicastReceivedDuringDataPoll = 0;

    // About to start a data poll, turn the receiver on
	if ((error = MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxidl, pDeviceRef)))
    {
        ca_log_crit("poll_req: MLME SET macRxOnWhenIdle FAILED");
        goto exit;
    }
    ca_log_debg("poll_req: RECEIVER TURNED ON");

    // Start the data poll
#if CASCODA_CA_VER == 8211
	error = MLME_POLL_request_sync(
	    *((struct FullAddr *)&(aPollRequest->mCoordAddress)), (struct SecSpec *)&(aPollRequest->mSecurity), pDeviceRef);
#else // CASCODA_CA_VER == 8210
	uint8_t interval[2] = {0, 0};
	error               = MLME_POLL_request_sync(*((struct FullAddr *)&(aPollRequest->mCoordAddress)),
                                   interval,
                                   (struct SecSpec *)&(aPollRequest->mSecurity),
                                   pDeviceRef);
#endif // CASCODA_CA_VER == 8211
#endif // CASCODA_CA_VER >= 8212

#if CASCODA_CA_VER < 8212
	/* For versions older than CA8212, MLME POLL is a synchronous command
    and the return value error already contains the confirm status. That is not
	the case for the CA8212, where the MLME POLL is an asynchronous command. In
	that case, the confirm status will be available via a callback, as is the case
	with all other asynchronous messages (e.g. MCPS-DATA). */
    if (error == MAC_SUCCESS)
    {
        ca_log_debg("poll_req: MAC_SUCCESS");
        uint32_t startticks = TIME_ReadAbsoluteTime();

        // MAC_SUCCESS means that some data has been received (either broadcast or not)
        // We need to wait until timeout, or until a data indication has come through,
        // in order to decide what to do next
        while (true)
        {
            cascoda_io_handler(pDeviceRef);

            if (sUnicastReceivedDuringDataPoll)
            {
                ca_log_debg("poll_req: sUnicastReceivedDuringDataPoll");

				// Signal that the received unicast should be sent to openthread ASAP after returning from this function
				sSendUnicastIndToOpenthread = 1;

                // Let the platform know that an indication is expected (e.g. this is used
                // by other functions to determine whether we can go to sleep or not)
                ca_log_debg("poll_req: sIsExpectingIndication %d -> 1", sIsExpectingIndication);
                sIsExpectingIndication = 1;

                break;
            }

            if ((TIME_ReadAbsoluteTime() - startticks) >= waittime)
            {
                // The data poll has timed out without receiving any data
                ca_log_debg("poll_req: MAC_NO_DATA");
                error = MAC_NO_DATA;
                break;
            }

        }
    }
    else
    {
        ca_log_debg("poll_req: ERROR: 0x%x", error);
    }

#endif // CASCODA_CA_VER < 8212

exit:
#if CASCODA_CA_VER <= 8211
    // Signal that the data poll is done (turns receiver off and updates the state)
    dataPollDone();
    // Wait for the state to reset
    uint32_t start_time = BSP_ReadAbsoluteTime();
    while (sIsDataPollInProgress)
    {
        BSP_ResetOnLoopStuck(start_time); // Protection against infinite loop
        cascoda_io_handler(pDeviceRef); // Allows handling of io and tasklets
    }

	// Schedule the received unicast to be sent to openthread ASAP after returning
	// from this function. (It has to be done in this order, because the return of this function
	// acts as the MLME POLL confirm. And that should go to Thread before the data indication)
	if (sSendUnicastIndToOpenthread)
	{
		ca_log_debg("poll_req: Scheduling sHandleUnicastIndTasklet");
		TASKLET_ScheduleDelta(&sHandleUnicastIndTasklet, 0, NULL);
		sSendUnicastIndToOpenthread = 0;
	}
#endif

	return ConvertErrorMacToOt(error);
}

otError otPlatMcpsDataRequest(otInstance *aInstance, otDataRequest *aDataRequest)
{
	(void)aInstance;
	ca_mac_status error;

#if CASCODA_CA_VER >= 8212
	uint8_t tx_op[2] = {0x00, 0x00};
	tx_op[0] |= aDataRequest->mTxOptions;
	error = MCPS_DATA_request(aDataRequest->mSrcAddrMode,                   /* SrcAddrMode */
	                          *(struct FullAddr *)&aDataRequest->mDst,      /* DstAddr */
	                          0,                                            /* HeaderIELength */
	                          0,                                            /* PayloadIELength */
	                          aDataRequest->mMsduLength,                    /* MsduLength */
	                          aDataRequest->mMsdu,                          /* pMsdu */
	                          aDataRequest->mMsduHandle,                    /* MsduHandle */
	                          tx_op,                                        /* pTxOptions */
	                          0,                                            /* SchTimestamp */
	                          0,                                            /* SchPeriod */
	                          0,                                            /* TxChannel */
	                          NULL,                                         /* pHeaderIEList */
	                          NULL,                                         /* pPayloadIEList */
	                          (struct SecSpec *)&(aDataRequest->mSecurity), /* pSecurity */
	                          pDeviceRef);                                  /* pDeviceRef */
#else
	error = MCPS_DATA_request(aDataRequest->mSrcAddrMode,
	                          *(struct FullAddr *)&aDataRequest->mDst,
	                          aDataRequest->mMsduLength,
	                          aDataRequest->mMsdu,
	                          aDataRequest->mMsduHandle,
	                          aDataRequest->mTxOptions,
	                          (struct SecSpec *)&(aDataRequest->mSecurity),
	                          pDeviceRef);
#endif // CASCODA_CA_VER >= 8212

	return ConvertErrorMacToOt(error);
}

otError otPlatMcpsPurge(otInstance *aInstance, uint8_t aMsduHandle)
{
	(void)aInstance;
	ca_mac_status error;

	error = MCPS_PURGE_request_sync(&aMsduHandle, pDeviceRef);

	return ConvertErrorMacToOt(error);
}

static ca_error handleDataIndication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *aDeviceRef)
{
	int16_t rssi;
	uint8_t csvallp;
	uint8_t len;
	//TODO: Move this off the stack
	otDataIndication dataInd = {0};

#if CASCODA_CA_VER <= 8211
    if (!sIsDataPollInProgress)
    {
        ca_log_debg("handleInd: !sIsDataPollInProgress");
        // If a data poll is in progress, we don't want to reset the state
        // of expecting indication yet, we need to do that once we are about
        // to pass up a unicast to Thread.
        ca_log_debg("handleInd: sIsExpectingIndication %d -> 0", sIsExpectingIndication);
        sIsExpectingIndication = 0;
    }
#endif // CASCODA_CA_VER <= 8211

	// Get the Carrier Sense value for the last packet now. We are doing it here,
	// because this is the earliest point at which this can be done. This minimizes
	// the chance that a newer PHY frame is received by the CA821x before we get the chance to extract
	// the Carrier Sense for the frame which is being handled here.

	/* get cs value for last data packet */
	HWME_GET_request_sync(HWME_CSVALLP, &len, &csvallp, aDeviceRef);

	dataInd.mSrc              = *((struct otFullAddr *)&(params->Src));
	dataInd.mDst              = *((struct otFullAddr *)&(params->Dst));

#if CASCODA_CA_VER <= 8211
    if (sIsDataPollInProgress)
    {
        ca_log_debg("handleInd: sIsDataPollInProgress");
        if (dataInd.mDst.mAddressMode == OT_MAC_ADDRESS_MODE_SHORT &&
            dataInd.mDst.mAddress[0] == 0xff && dataInd.mDst.mAddress[1] == 0xff)
        {
            ca_log_debg("handleInd: Broadcast Rejected");


            // Silently reject broadcasts
            goto exit;
        }
    }
    else
    {
        if (dataInd.mDst.mAddressMode == OT_MAC_ADDRESS_MODE_SHORT &&
            dataInd.mDst.mAddress[0] == 0xff && dataInd.mDst.mAddress[1] == 0xff)
        {
            ca_log_debg("handleInd: BROADCAST RECEIVED, BUT sIsDataPollInProgress is FALSE! (OK for non-sleepy devices)");
        }
    }
#endif // CASCODA_CA_VER <= 8211

	dataInd.mMsduLength       = params->MsduLength;
	rssi                      = ((int16_t)params->MpduLinkQuality - 256) / 2; //convert to rssi
	dataInd.mMpduLinkQuality  = rssi;
	dataInd.mLinkCarrierSense = csvallp;
	dataInd.mDSN              = params->DSN;
#if CASCODA_CA_VER >= 8212
	//TODO: Uncomment lines in this section once otDataIndication extra parameters are implemented in OpenThread.

	// dataInd.mHeaderIELength  = params->HeaderIELength;
	// dataInd.mPayloadIELength = params->PayloadIELength;

	union DataIndVariable
	{
		uint8_t *ptr;
		uint8_t *pHieList;
		uint8_t *pPieList;
		uint8_t *pMsdu;
		uint8_t *pSec;
	} data; //Moving pointer

	data.ptr = params->Data;

	// memcpy(dataInd.mHeaderIEList, data.pHieList, dataInd.mHeaderIELength);
	data.ptr += params->HeaderIELength;

	// memcpy(dataInd.mPayloadIELength, data.pPieList, dataInd.mPayloadIELength);
	data.ptr += params->PayloadIELength;

	memcpy(dataInd.mMsdu, data.pMsdu, dataInd.mMsduLength);
	data.ptr += dataInd.mMsduLength;

	memcpy(&(dataInd.mSecurity), data.pSec, sizeof(dataInd.mSecurity));
#else
	memcpy(dataInd.mMsdu, params->Msdu, dataInd.mMsduLength);
	memcpy(&(dataInd.mSecurity), params->Msdu + params->MsduLength, sizeof(dataInd.mSecurity));
#endif // CASCODA_CA_VER >= 8212

#if CASCODA_CA_VER >= 8211
	dataInd.mIsFramePending = params->FramePending;
#endif // CASCODA_CA_VER >= 8211

	if (dataInd.mSecurity.mSecurityLevel == 0)
	{
		memset(&(dataInd.mSecurity), 0, sizeof(dataInd.mSecurity));
	}

	ca_log_debg("MAC Rx Data Indication");

#if CASCODA_CA_VER <= 8211
    if (sIsDataPollInProgress)
    {
        ca_log_debg("handleInd: sIsDataPollInProgress");
        // If a data poll is in progress, do not send the indication to OpenThread yet.
        // Save it, to be sent after the the Poll Confirm.
        ca_log_debg("handleInd: sUnicastReceivedDuringDataPoll %d -> 1", sUnicastReceivedDuringDataPoll);
        sUnicastReceivedDuringDataPoll = 1;
        sUnicastDataIndDuringPoll = dataInd;
        goto exit;
    }
#endif // CASCODA_CA_VER <= 8211

	otPlatMcpsDataIndication(OT_INSTANCE, &dataInd);

exit:
	return CA_ERROR_SUCCESS;
}

static ca_error handlePollIndication(struct MLME_POLL_indication_pset *params, struct ca821x_dev *aDeviceRef)
{
	(void)aDeviceRef;
	//TODO: Move this off the stack
	otPollIndication pollInd = {0};

	pollInd.mSrc = *((struct otFullAddr *)&(params->Src));
	pollInd.mDst = *((struct otFullAddr *)&(params->Dst));
	pollInd.mLQI = params->LQI;
	pollInd.mDSN = params->DSN;
#if CASCODA_CA_VER >= 8212
	// TODO: Uncomment this once the Timestamp is implemented in otPollIndication.
	// memset(&(pollInd.Timestamp), params->Timestamp, sizeof(pollInd.Timestamp));
#endif // CASCODA_CA_VER >= 8212
	memcpy(&(pollInd.mSecurity), &(params->Security), sizeof(pollInd.mSecurity));

	if (pollInd.mSecurity.mSecurityLevel == 0)
	{
		memset(&(pollInd.mSecurity), 0, sizeof(pollInd.mSecurity));
	}

	otPlatMlmePollIndication(OT_INSTANCE, &pollInd);

	return CA_ERROR_SUCCESS;
}

#if CASCODA_CA_VER >= 8212
static ca_error handlePollConfirm(struct MLME_POLL_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	otError error = ConvertErrorMacToOt((ca_mac_status)params->Status);

	// TODO: Uncomment the line below when otPlatMlmePollConfirm() gets implemented in OpenThread.
	// otPlatMlmePollConfirm(OT_INSTANCE, error);

	sIsExpectingIndication = 1;

	return CA_ERROR_SUCCESS;
}
#endif // CASCODA_CA_VER >= 8212

static ca_error handleCommStatusIndication(struct MLME_COMM_STATUS_indication_pset *params,
                                           struct ca821x_dev                       *pDeviceRef)
{
	(void)pDeviceRef;
	//TODO: Move this off the stack
	otCommStatusIndication commInd = {0};

	memcpy(commInd.mPanId, params->PANId, sizeof(commInd.mPanId));
	commInd.mDstAddrMode = params->DstAddrMode;
	commInd.mSrcAddrMode = params->SrcAddrMode;
	memcpy(commInd.mDstAddr, params->DstAddr, sizeof(commInd.mDstAddr));
	memcpy(commInd.mSrcAddr, params->SrcAddr, sizeof(commInd.mSrcAddr));
	memcpy(&commInd.mSecurity, &params->Security, sizeof(commInd.mSecurity));

	commInd.mStatus = params->Status;

	if (commInd.mSecurity.mSecurityLevel == 0)
	{
		memset(&(commInd.mSecurity), 0, sizeof(commInd.mSecurity));
	}

	otPlatMlmeCommStatusIndication(OT_INSTANCE, &commInd);

	return CA_ERROR_SUCCESS;
}

static ca_error handleDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *aDeviceRef) //Async
{
	(void)aDeviceRef;
	otError error = ConvertErrorMacToOt((ca_mac_status)params->Status);

	otPlatMcpsDataConfirm(OT_INSTANCE, params->MsduHandle, error);

	return CA_ERROR_SUCCESS;
}

static ca_error handleBeaconNotify(struct MLME_BEACON_NOTIFY_indication_pset *params,
                                   struct ca821x_dev                         *pDeviceRef) //Async
{
	(void)pDeviceRef;
	//TODO: Move this off the stack
	otBeaconNotify beaconNotify = {0};
	uint8_t        sduLenOffset;

	{
		uint8_t addrField  = ((uint8_t *)params)[23];
		uint8_t shortaddrs = addrField & 0x07;
		uint8_t extaddrs   = (addrField >> 4) & 0x07;
		sduLenOffset       = (24 + (2 * shortaddrs) + (8 * extaddrs));
	}

	beaconNotify.BSN            = params->BSN;
	beaconNotify.mPanDescriptor = *((struct otPanDescriptor *)&(params->PanDescriptor));
	beaconNotify.mSduLength     = ((uint8_t *)params)[sduLenOffset];
	memcpy(beaconNotify.mSdu, &(((uint8_t *)params)[sduLenOffset + 1]), beaconNotify.mSduLength);

	otPlatMlmeBeaconNotifyIndication(OT_INSTANCE, &beaconNotify);

	return CA_ERROR_SUCCESS;
}

static ca_error handleScanConfirm(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *aDeviceRef) //Async
{
	(void)aDeviceRef;
	otPlatMlmeScanConfirm(OT_INSTANCE, (otScanConfirm *)params);

	return CA_ERROR_SUCCESS;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
	(void)aInstance;
	memcpy(aIeeeEui64, sIeeeEui64, sizeof(sIeeeEui64));
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
	(void)aInstance;
	return -95;
}

//static int driverErrorCallback(int error_number)
//{
//	otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_MAC, "DRIVER FAILED WITH ERROR %d\n\r", error_number);
//
//	//TODO: Fail gracefully
//
//	abort();
//	return 0;
//}

void PlatformRadioStop(void)
{
	if (sRadioInitialised)
	{
		//Reset the MAC to a default state
		otPlatLog(OT_LOG_LEVEL_INFO, OT_LOG_REGION_MAC, "Resetting & Stopping Radio...\n\r");
		MLME_RESET_request_sync(1, pDeviceRef);
		sRadioInitialised = false;
	}
}

void initIeeeEui64()
{
	PUTLE64(BSP_GetUniqueId(), sIeeeEui64);
	sIeeeEui64[0] &= ~1; //Unset Group bit
	sIeeeEui64[0] |= 2;  //Set local bit
}

static ca_error handleWakeupIndication(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *aDeviceRef)
{
	(void)aDeviceRef;
	ca_log_info("Woke Up with status %02x", params->WakeUpCondition);
	return CA_ERROR_SUCCESS;
}

static int platformRadioInitWithDevNoEui(struct ca821x_dev *apDeviceRef)
{
	pDeviceRef = apDeviceRef;

	pDeviceRef->callbacks.MCPS_DATA_indication          = &handleDataIndication;
	pDeviceRef->callbacks.MLME_COMM_STATUS_indication   = &handleCommStatusIndication;
	pDeviceRef->callbacks.MCPS_DATA_confirm             = &handleDataConfirm;
	pDeviceRef->callbacks.MLME_BEACON_NOTIFY_indication = &handleBeaconNotify;
	pDeviceRef->callbacks.MLME_SCAN_confirm             = &handleScanConfirm;
	pDeviceRef->callbacks.HWME_WAKEUP_indication        = &handleWakeupIndication;
#if CASCODA_CA_VER >= 8211
	pDeviceRef->callbacks.MLME_POLL_indication = &handlePollIndication;
#endif // CASCODA_CA_VER >= 8211
#if CASCODA_CA_VER >= 8212
	pDeviceRef->callbacks.MLME_POLL_confirm = &handlePollConfirm;
#endif // CASCODA_CA_VER >= 8211

	//Initialise the alarm subsystem too
	PlatformAlarmInit();

	//Reset the MAC to a default state
	otPlatMlmeReset(NULL, true);

	sRadioInitialised = 1;

#if CASCODA_CA_VER <= 8211
    // Initialise the tasklets used for mlme poll
    TASKLET_Init(&sHandleUnicastIndTasklet, &handleUnicastIndDuringPoll);
    TASKLET_Init(&sResetDataPollInProgressTasklet, &resetDataPollInProgress);
#endif // CASCODA_CA_VER <= 8211

	return 0;
}

int PlatformRadioInitWithDev(struct ca821x_dev *apDeviceRef)
{
	int error = platformRadioInitWithDevNoEui(apDeviceRef);
	initIeeeEui64();

	return error;
}

int PlatformRadioInitWithDevEui64(struct ca821x_dev *apDeviceRef, uint8_t *pEui64)
{
	int error = platformRadioInitWithDevNoEui(apDeviceRef);
	memcpy(sIeeeEui64, pEui64, sizeof(sIeeeEui64));

	return error;
}

int PlatformRadioInit(void)
{
	static struct ca821x_dev sDeviceRef;
	int                      status;

	status = ca821x_api_init(&sDeviceRef);
	if (status < 0)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_PLATFORM, "No ca821x device found");
		return status;
	}

	return PlatformRadioInitWithDev(&sDeviceRef);
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	(void)aInstance;
	return sNoiseFloor;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
	(void)aInstance;
	otError error = OT_ERROR_NONE;
	OT_INSTANCE   = aInstance;

	return error;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
	(void)aInstance;
	return (OT_INSTANCE != NULL);
}

int PlatformIsExpectingIndication()
{
	return sIsExpectingIndication;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

// SOF Cascoda low level cli access

otError otPlatGetSFR(otInstance *aInstance, uint8_t aPage, uint8_t aAddr, uint8_t *aVal)
{
	(void)aInstance;
	uint8_t error;
	otError otErr;

	error = TDME_GETSFR_request_sync(aPage, aAddr, aVal, pDeviceRef);
	otErr = ConvertErrorMacToOt(error);

	return otErr;
}

otError otPlatSetSFR(otInstance *aInstance, uint8_t aPage, uint8_t aAddr, uint8_t aVal)
{
	(void)aInstance;
	uint8_t error;
	otError otErr;

	error = TDME_SETSFR_request_sync(aPage, aAddr, aVal, pDeviceRef);
	otErr = ConvertErrorMacToOt(error);

	return otErr;
}

otError otPlatHwmeGet(otInstance *aInstance, uint8_t aAttr, uint8_t *aLen, uint8_t *aBuf)
{
	(void)aInstance;
	uint8_t error;
	otError otErr;

	error = HWME_GET_request_sync(aAttr, aLen, aBuf, pDeviceRef);
	otErr = ConvertErrorMacToOt(error);

	return otErr;
}

otError otPlatHwmeSet(otInstance *aInstance, uint8_t aAttr, uint8_t aLen, uint8_t *aBuf)
{
	(void)aInstance;
	uint8_t error;
	otError otErr;

	error = HWME_SET_request_sync(aAttr, aLen, aBuf, pDeviceRef);
	otErr = ConvertErrorMacToOt(error);

	return otErr;
}

otError otPlatPowerUpRadio(otInstance *aInstance)
{
    (void)aInstance;
    uint8_t error;
    otError otErr;

    error = EVBME_PowerUpRadio(pDeviceRef);
    otErr = ConvertErrorMacToOt(error);

    return otErr;
}

// EOF Cascoda low level cli access
