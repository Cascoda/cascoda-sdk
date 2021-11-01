/*
 *  Copyright (c) 2016, Nest Labs, Inc.
 *  Modifications copyright (c) 2020, Cascoda Ltd.
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
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "openthread/platform/logging.h"
#include "openthread/platform/radio-mac.h"
#include "openthread/random_noncrypto.h"
#include "openthread/thread.h"

#include "ca821x-posix-thread/posix-platform.h"
#include "ca821x-posix/ca821x-posix-settings.h"
#include "ca821x-posix/ca821x-posix.h"
#include "cascoda-util/cascoda_rand.h"
#include "ca821x_api.h"
#include "code_utils.h"
#include "ieee_802_15_4.h"
#include "mac_messages.h"
#include "selfpipe.h"

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))

//BARRIER
/*
 * The following functions create a thread safe system for allowing the worker
 * thread to access openthread functions safely. The main thread always has
 * priority and must explicitly give control to the worker thread while locking
 * itself. This has ONLY been designed to work with ONE worker and ONE main, in
 * a way that causes the worker thread to run one operation per poll cycle as
 * openthread was designed for.
 */
static pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  barrier_cond  = PTHREAD_COND_INITIALIZER;

static enum barrier_waiting { NOT_WAITING, WAITING, GREENLIGHT, DONE } mbarrier_waiting;

static inline void barrier_main_letWorkerWork(void);
static inline void barrier_worker_waitForMain(void);
static inline void barrier_worker_endWork(void);
//END BARRIER

static const char IeeeEuiFile[] = "otEui";
static uint8_t    sIeeeEui64[8];

//For cascoda API
static struct ca821x_dev  s_pDeviceRef;
static struct ca821x_dev *pDeviceRef = NULL;

static int8_t sNoiseFloor = 127;

static otInstance *OT_INSTANCE       = NULL;
static uint8_t     sRadioInitialised = 0;

struct M_KeyDescriptor_thread
{
	struct M_KeyTableEntryFixed Fixed;
	struct M_KeyIdLookupDesc    KeyIdLookupList[1];
	uint8_t                     flags[40]; //TODO: Provisional length
	                                       //struct M_KeyDeviceDesc         KeyDeviceList[count];
	                                       //struct M_KeyUsageDesc          KeyUsageList[2];
};

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

otError otPlatMlmeGet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t *aLen, uint8_t *aBuf)
{
	uint8_t error;
	otError otErr;

	//Adaption for security table
	if (aAttr == OT_PIB_MAC_KEY_TABLE)
	{
		struct M_KeyDescriptor_thread caKeyDesc  = {0};
		otKeyTableEntry *             otKeyDesc  = (otKeyTableEntry *)aBuf;
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
	{
		error = MLME_GET_request_sync(aAttr, aIndex, aLen, aBuf, pDeviceRef);
	}

	otErr = ConvertErrorMacToOt(error);

exit:
	return otErr;
}

otError otPlatMlmeSet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t aLen, const uint8_t *aBuf)
{
	uint8_t error;
	otError otErr;

	//Adaption for security table
	if (aAttr == OT_PIB_MAC_KEY_TABLE)
	{
		struct M_KeyDescriptor_thread caKeyDesc  = {0};
		const otKeyTableEntry *       otKeyDesc  = (otKeyTableEntry *)aBuf;
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
	{
		error = MLME_SET_request_sync(aAttr, aIndex, aLen, aBuf, pDeviceRef);
	}

	otErr = ConvertErrorMacToOt(error);

	return otErr;
}

otError otPlatMlmeReset(otInstance *aInstance, bool setDefaultPib)
{
	ca_mac_status error;

	error = MLME_RESET_request_sync(setDefaultPib, pDeviceRef);

	// Tx Power to max
	uint8_t txPow = 8;
	MLME_SET_request_sync(phyTransmitPower, 0, 1, &txPow, pDeviceRef);

	// Enable poll indications when no data confirm is triggered.
#if CASCODA_CA_VER == 8211
	uint8_t pollIndMode = 2;
	HWME_SET_request_sync(HWME_POLLINDMODE, 1, &pollIndMode, pDeviceRef);
#endif

	if (setDefaultPib)
	{
		//Disable low LQI rejection @ MAC Layer
		uint8_t disable = 0;
		HWME_SET_request_sync(0x11, 1, &disable, pDeviceRef);

		//LQI values should be derived from receive energy
		uint8_t LQImode = HWME_LQIMODE_ED;
		HWME_SET_request_sync(HWME_LQIMODE, 1, &LQImode, pDeviceRef);

#if CASCODA_CA_VER == 8211
		//Increase the max indirect queue length to 8
		uint8_t maxInd = 8;
		HWME_SET_request_sync(HWME_MAXINDIRECTS, 1, &maxInd, pDeviceRef);
#endif
	}

	return ConvertErrorMacToOt(error);
}

otError otPlatMlmeStart(otInstance *aInstance, otStartRequest *aStartReq)
{
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
	ca_mac_status error;

#if CASCODA_CA_VER == 8210
	uint8_t interval[2] = {0, 0};
	error               = MLME_POLL_request_sync(*((struct FullAddr *)&(aPollRequest->mCoordAddress)),
                                   interval,
                                   (struct SecSpec *)&(aPollRequest->mSecurity),
                                   pDeviceRef);
#else
	error = MLME_POLL_request_sync(
	    *((struct FullAddr *)&(aPollRequest->mCoordAddress)), (struct SecSpec *)&(aPollRequest->mSecurity), pDeviceRef);
#endif

	return ConvertErrorMacToOt(error);
}

otError otPlatMcpsDataRequest(otInstance *aInstance, otDataRequest *aDataRequest)
{
	ca_mac_status error;

	error = MCPS_DATA_request(aDataRequest->mSrcAddrMode,
	                          *(struct FullAddr *)&aDataRequest->mDst,
	                          aDataRequest->mMsduLength,
	                          aDataRequest->mMsdu,
	                          aDataRequest->mMsduHandle,
	                          aDataRequest->mTxOptions,
	                          (struct SecSpec *)&(aDataRequest->mSecurity),
	                          pDeviceRef);

	ca_log_debg("MCPS Data req - Error %d, Ind: %d, MH: %02x",
	            error,
	            aDataRequest->mTxOptions & OT_MAC_TX_OPTION_INDIRECT,
	            aDataRequest->mMsduHandle);

	return ConvertErrorMacToOt(error);
}

otError otPlatMcpsPurge(otInstance *aInstance, uint8_t aMsduHandle)
{
	ca_mac_status error;

	error = MCPS_PURGE_request_sync(&aMsduHandle, pDeviceRef);

	ca_log_debg("MCPS Purge req - Error %d, MH: %02x", error, aMsduHandle);

	return ConvertErrorMacToOt(error);
}

static ca_error handleDataIndication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	int16_t rssi;
	//TODO: Move this off the stack
	otDataIndication dataInd = {0};

	dataInd.mSrc             = *((struct otFullAddr *)&(params->Src));
	dataInd.mDst             = *((struct otFullAddr *)&(params->Dst));
	dataInd.mMsduLength      = params->MsduLength;
	rssi                     = ((int16_t)params->MpduLinkQuality - 256) / 2; //convert to rssi
	dataInd.mMpduLinkQuality = rssi;
	dataInd.mDSN             = params->DSN;
	memcpy(dataInd.mMsdu, params->Msdu, dataInd.mMsduLength);
	memcpy(&(dataInd.mSecurity), params->Msdu + params->MsduLength, sizeof(dataInd.mSecurity));

#if CASCODA_CA_VER == 8211
	dataInd.mIsFramePending = params->FramePending;
#endif

	if (dataInd.mSecurity.mSecurityLevel == 0)
	{
		memset(&(dataInd.mSecurity), 0, sizeof(dataInd.mSecurity));
	}

	barrier_worker_waitForMain();
	otPlatMcpsDataIndication(OT_INSTANCE, &dataInd);
	barrier_worker_endWork();

	return CA_ERROR_SUCCESS;
}

static ca_error handlePollIndication(struct MLME_POLL_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	//TODO: Move this off the stack
	otPollIndication pollInd = {0};

	pollInd.mSrc = *((struct otFullAddr *)&(params->Src));
	pollInd.mDst = *((struct otFullAddr *)&(params->Dst));
	pollInd.mLQI = params->LQI;
	pollInd.mDSN = params->DSN;
	memcpy(&(pollInd.mSecurity), &(params->Security), sizeof(pollInd.mSecurity));

	if (pollInd.mSecurity.mSecurityLevel == 0)
	{
		memset(&(pollInd.mSecurity), 0, sizeof(pollInd.mSecurity));
	}

	barrier_worker_waitForMain();
	otPlatMlmePollIndication(OT_INSTANCE, &pollInd);
	barrier_worker_endWork();

	return CA_ERROR_SUCCESS;
}

static ca_error handleCommStatusIndication(struct MLME_COMM_STATUS_indication_pset *params,
                                           struct ca821x_dev *                      pDeviceRef)
{
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

	barrier_worker_waitForMain();
	otPlatMlmeCommStatusIndication(OT_INSTANCE, &commInd);
	barrier_worker_endWork();

	return CA_ERROR_SUCCESS;
}

static ca_error handleDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef) //Async
{
	otError error = ConvertErrorMacToOt((ca_mac_status)params->Status);

	ca_log_debg("Data Confirm handle %x, status %x", params->MsduHandle, params->Status);

	barrier_worker_waitForMain();
	otPlatMcpsDataConfirm(OT_INSTANCE, params->MsduHandle, error);
	barrier_worker_endWork();

	return CA_ERROR_SUCCESS;
}

static ca_error handleBeaconNotify(struct MLME_BEACON_NOTIFY_indication_pset *params,
                                   struct ca821x_dev *                        pDeviceRef) //Async
{
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

	barrier_worker_waitForMain();
	otPlatMlmeBeaconNotifyIndication(OT_INSTANCE, &beaconNotify);
	barrier_worker_endWork();

	return CA_ERROR_SUCCESS;
}

static ca_error handleScanConfirm(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef) //Async
{
	barrier_worker_waitForMain();
	otPlatMlmeScanConfirm(OT_INSTANCE, (otScanConfirm *)params);
	barrier_worker_endWork();

	return CA_ERROR_SUCCESS;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
	memcpy(aIeeeEui64, sIeeeEui64, sizeof(sIeeeEui64));

	(void)aInstance;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
	return -105;
}

static ca_error driverErrorCallback(ca_error error, struct ca821x_dev *pDeviceRef)
{
	otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_MAC, "DRIVER FAILED WITH ERROR %d\n\r", (int)error);

	if (!sRadioInitialised)
		exit(EXIT_FAILURE);

	otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_MAC, "Attempting restart...\n\r", (int)error);

	if (ca821x_util_reset(pDeviceRef) == 0)
	{
		otInstanceReset(OT_INSTANCE);
	}

	abort();
	return CA_ERROR_SUCCESS;
}

void PlatformRadioStop(void)
{
	if (sRadioInitialised)
	{
		//Reset the MAC to a default state
		otPlatLog(OT_LOG_LEVEL_INFO, OT_LOG_REGION_MAC, "Resetting & Stopping Radio...\n\r");
		MLME_RESET_request_sync(1, pDeviceRef);
		ca821x_util_deinit(pDeviceRef);
		sRadioInitialised = false;
	}
}

void initIeeeEui64()
{
	int     file;
	uint8_t create      = false;
	char *  dataDir     = posixGetDataDir(NODE_ID);
	size_t  fileNameLen = sizeof(IeeeEuiFile) + strlen(dataDir) + 1; //"datadir/filename"
	char    fileName[fileNameLen];

	snprintf(fileName, fileNameLen, "%s/%s", dataDir, IeeeEuiFile);

	if (!access(fileName, R_OK))
	{
		uint8_t ret = 0;

		file = open(fileName, O_RDONLY);
		ret  = read(file, sIeeeEui64, 8);
		if (ret != 8)
		{
			close(file);
			create = true;
		}
	}
	else
	{
		create = true;
	}

	if (create)
	{
		file = open(fileName, O_RDWR | O_CREAT, 0666);
		RAND_GetBytes(sizeof(sIeeeEui64), sIeeeEui64);
		sIeeeEui64[0] &= ~1; //Unset Group bit
		sIeeeEui64[0] |= 2;  //Set local bit
		write(file, sIeeeEui64, 8);
	}
	close(file);
	free(dataDir);
}

static ca_error handleWakeupIndication(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	ca_log_info("Woke Up with status %02x", params->WakeUpCondition);
	return CA_ERROR_SUCCESS;
}

int PlatformRadioInitWithDev(struct ca821x_dev *apDeviceRef)
{
	pDeviceRef = apDeviceRef;

	atexit(&PlatformRadioStop);
	selfpipe_init();

	pDeviceRef->callbacks.MCPS_DATA_indication          = &handleDataIndication;
	pDeviceRef->callbacks.MLME_COMM_STATUS_indication   = &handleCommStatusIndication;
	pDeviceRef->callbacks.MCPS_DATA_confirm             = &handleDataConfirm;
	pDeviceRef->callbacks.MLME_BEACON_NOTIFY_indication = &handleBeaconNotify;
	pDeviceRef->callbacks.MLME_SCAN_confirm             = &handleScanConfirm;
	pDeviceRef->callbacks.HWME_WAKEUP_indication        = &handleWakeupIndication;
#if CASCODA_CA_VER == 8211
	pDeviceRef->callbacks.MLME_POLL_indication = &handlePollIndication;
#endif

	//Reset the MAC to a default state
	otPlatMlmeReset(NULL, true);

	initIeeeEui64();
	sRadioInitialised = 1;

	return 0;
}

int PlatformRadioInit(void)
{
	if (ca821x_util_init(&s_pDeviceRef, driverErrorCallback) == CA_ERROR_NOT_FOUND)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_PLATFORM, "No ca821x device found");
		return -1;
	}

	if (PlatformRadioInitWithDev(&s_pDeviceRef))
	{
		otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_PLATFORM, "Failed radio initialisation");
		return -1;
	}

	if (ca821x_util_start_downstream_dispatch_worker())
	{
		otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_PLATFORM, "Failed worker thread start");
		return -1;
	}

	return 0;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	return sNoiseFloor;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
	otError error = OT_ERROR_NONE;
	OT_INSTANCE   = aInstance;

	return error;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
	return (OT_INSTANCE != NULL);
}

int PlatformRadioProcess(void)
{
	barrier_main_letWorkerWork();
	return 0;
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

//Lets the worker thread work synchronously if there is synchronous work to do
static inline void barrier_main_letWorkerWork()
{
	pthread_mutex_lock(&barrier_mutex);

	if (mbarrier_waiting == WAITING)
	{
		mbarrier_waiting = GREENLIGHT;
		pthread_cond_broadcast(&barrier_cond);

		while (mbarrier_waiting != DONE)
		{
			pthread_cond_wait(&barrier_cond, &barrier_mutex); //The worker thread does work now
		}
	}

	mbarrier_waiting = NOT_WAITING;
	pthread_cond_broadcast(&barrier_cond);
	pthread_mutex_unlock(&barrier_mutex);
}

static inline void barrier_worker_waitForMain()
{
	pthread_mutex_lock(&barrier_mutex);

	while (mbarrier_waiting != NOT_WAITING)
	{
		pthread_cond_wait(&barrier_cond, &barrier_mutex);
	}

	selfpipe_push();
	mbarrier_waiting = WAITING;
	pthread_cond_broadcast(&barrier_cond);

	while (mbarrier_waiting != GREENLIGHT)
	{
		pthread_cond_wait(&barrier_cond, &barrier_mutex); //wait for the main thread to signal worker to run
	}
}

static inline void barrier_worker_endWork()
{
	mbarrier_waiting = DONE;
	pthread_cond_broadcast(&barrier_cond);
	pthread_mutex_unlock(&barrier_mutex);
}
