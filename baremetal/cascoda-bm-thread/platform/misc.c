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

#include <assert.h>
#include <string.h>

#include "openthread/dataset.h"
#include "openthread/joiner.h"
#include "openthread/link.h"
#include "openthread/platform/misc.h"
#include "openthread/platform/radio.h"
#include "openthread/platform/settings.h"
#include "openthread/random_crypto.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "platform.h"

enum
{
	JOINER_CREDENTIAL_LEN    = 8,
	JOINER_CREDENTIAL_MAXLEN = 32, // Thread Spec
	JOINER_CREDENTIAL_MINLEN = 6,  // Thread Spec
};

static char       joiner_credential[JOINER_CREDENTIAL_MAXLEN + 1];
static const char joiner_alphabet[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
                                       'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y'};

bool SleepEnabled = 1;

void otPlatReset(otInstance *aInstance)
{
	PlatformRadioStop();
	BSP_SystemReset(SYSRESET_APROM);
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
	return OT_PLAT_RESET_REASON_POWER_ON;
}

void otPlatWakeHost(void)
{
	//This is the host - not applicable to posix & hardmac systems
}

otError PlatformSleep(uint32_t aSleepTime)
{
	struct ca821x_dev *pDeviceRef = PlatformGetDeviceRef();
	u8_t               framecounter[4];
	u8_t               fclen;
	u8_t               dsn[1];

	// Get frame counter from MAC
	MLME_GET_request_sync(macFrameCounter, 0, &fclen, framecounter, pDeviceRef);
	MLME_GET_request_sync(macDSN, 0, &fclen, dsn, pDeviceRef);

	EVBME_PowerDown(PDM_POWEROFF, aSleepTime, pDeviceRef);

	otLinkSyncExternalMac(OT_INSTANCE);
	MLME_SET_request_sync(macFrameCounter, 0, 4, framecounter, pDeviceRef);
	MLME_SET_request_sync(macDSN, 0, 1, dsn, pDeviceRef);

	return OT_ERROR_NONE;
}

bool PlatformCanSleep(otInstance *aInstance)
{
	otLinkModeConfig linkMode   = otThreadGetLinkMode(aInstance);
	otDeviceRole     deviceRole = otThreadGetDeviceRole(aInstance);

	if (otTaskletsArePending(OT_INSTANCE))
		return false;
	if (linkMode.mDeviceType != 0)
		return false;
	if (linkMode.mRxOnWhenIdle != 0)
		return false;
	if (otLinkIsInTransmitState(aInstance))
		return false;
	if (!(deviceRole == OT_DEVICE_ROLE_CHILD || deviceRole == OT_DEVICE_ROLE_DETACHED))
		return false;
	if (PlatformIsExpectingIndication())
		return false;

	return true;
}

struct join_status
{
	bool    complete;
	otError error;
};

static void HandleJoinerCallback(otError aError, void *aContext)
{
	struct join_status *join_status = aContext;
	join_status->complete           = true;
	join_status->error              = aError;
}

/**
 * Randomly fill the joiner credential with valid characters.
 * @param aPskd The buffer to fill with the joiner credential.
 * @param len Number of characters to output (which aPskd must be able to accommodate)
 */
static void genJoinerCred(char *aPskd, uint8_t len)
{
	ca_static_assert(sizeof(joiner_alphabet) == 32); //For the shift below

	otRandomCryptoFillBuffer(aPskd, len);
	for (uint8_t i = 0; i < len; i++)
	{
		/* Instead of just taking the modulo/masking we're shifting down the higher bits.
		 * This is because in certain RNGs, the lower bits are less random the higher bits,
		 * and so to be paranoid, we take the highest instead of lowest.
		 */
		uint8_t index = aPskd[i] >> 3;
		aPskd[i]      = joiner_alphabet[index];
	}
	aPskd[len] = '\0';
}

static bool isValidJoinerCredChar(char aChar)
{
	bool rval = false;
	for (int i = 0; i < sizeof(joiner_alphabet); i++)
	{
		if (aChar == joiner_alphabet[i])
		{
			rval = true;
			break;
		}
	}
	return rval;
}

static bool isValidJoinerCred(char *aPskd, uint8_t len)
{
	if (len > JOINER_CREDENTIAL_MAXLEN || len < JOINER_CREDENTIAL_MINLEN)
		return false;

	for (int i = 0; i < len; i++)
	{
		if (!isValidJoinerCredChar(aPskd[i]))
			return false;
	}

	if (aPskd[len] != '\0')
		return false;

	return true;
}

const char *PlatformGetJoinerCredential(otInstance *aInstance)
{
	uint16_t cred_len = JOINER_CREDENTIAL_MAXLEN;

	if (joiner_credential[0])
		return joiner_credential; //Already cached

#ifdef CASCODA_OT_JOINER_CRED
	return CASCODA_OT_JOINER_CRED;
#endif

	//Generate or get the joiner credential
	if (otPlatSettingsGet(aInstance, joiner_credential_key, 0, joiner_credential, &cred_len) != OT_ERROR_NONE ||
	    !isValidJoinerCred(joiner_credential, cred_len))
	{
		genJoinerCred(joiner_credential, JOINER_CREDENTIAL_LEN);
		otPlatSettingsSet(aInstance, joiner_credential_key, joiner_credential, JOINER_CREDENTIAL_LEN);
	}
	ca_log_info("Joiner Credential: %s", joiner_credential);

	return joiner_credential;
}

/**
 * DEVELOPMENT ONLY function to inject credentials into a baremetal platform at build time.
 * @param aInstance Openthread instance
 * @return ca_error
 * @retval CA_ERROR_SUCCESS Successfully injected credentials
 * @retval CA_ERROR_NOT_FOUND Credentials were not configured for injection
 */
static otError PlatformInjectCreds(otInstance *aInstance)
{
#ifdef INJECT_CREDS
	otMasterKey key = {INJECT_MASTERKEY};

	assert(otLinkSetPanId(aInstance, INJECT_PANID) == OT_ERROR_NONE);
	assert(otLinkSetChannel(aInstance, INJECT_CHANNEL) == OT_ERROR_NONE);
	assert(otThreadSetMasterKey(aInstance, &key) == OT_ERROR_NONE);

	ca_log_warn("Injecting openthread credentials! Not for production use!");

	return CA_ERROR_SUCCESS;
#else
	(void)aInstance;
	return CA_ERROR_NOT_FOUND;
#endif
}

otError PlatformTryJoin(struct ca821x_dev *pDeviceRef, otInstance *aInstance)
{
	struct join_status join_status = {0, 0};
	const char *       aPskd;

	otIp6SetEnabled(OT_INSTANCE, true);

	if (PlatformInjectCreds(aInstance) == CA_ERROR_SUCCESS)
	{
		return OT_ERROR_ALREADY;
	}

	if (otDatasetIsCommissioned(aInstance))
	{
		return OT_ERROR_ALREADY;
	}

	aPskd = PlatformGetJoinerCredential(aInstance);

	//Attempt to join
	otJoinerStart(
	    aInstance, aPskd, NULL, "Cascoda", NULL, ca821x_get_version(), NULL, &HandleJoinerCallback, &join_status);

	//Wait for completion
	while (!join_status.complete)
	{
		cascoda_io_handler(pDeviceRef);
		otTaskletsProcess(OT_INSTANCE);
	}

	if (join_status.error)
		otIp6SetEnabled(OT_INSTANCE, false);

	//Report status
	ca_log_info("Join complete with error %s", otThreadErrorToString(join_status.error));

	return join_status.error;
}

otError PlatformPrintJoinerCredentials(struct ca821x_dev *pDeviceRef, otInstance *aInstance, uint32_t aMaxWaitMs)
{
	otExtAddress extAddress;

	otLinkGetFactoryAssignedIeeeEui64(aInstance, &extAddress);
	printf("Thread Joining Credential: %s, EUI64: ", PlatformGetJoinerCredential(aInstance));
	for (int i = 0; i < sizeof(extAddress); i++) printf("%02x", extAddress.m8[i]);
	printf("\n");

#if defined(USE_USB)
	if (BSP_IsUSBPresent() && !otDatasetIsCommissioned(aInstance))
	{
		uint32_t starttime = TIME_ReadAbsoluteTime();

		while (TIME_ReadAbsoluteTime() - starttime < aMaxWaitMs)
		{
			cascoda_io_handler(pDeviceRef);
		}
	}
#endif
	return OT_ERROR_NONE;
}

ca_error EVBME_GET_OT_Attrib(enum evbme_attribute aAttrib, uint8_t *aOutBufLen, uint8_t *aOutBuf)
{
	ca_error       error = CA_ERROR_SUCCESS;
	otExtAddress   extAddress;
	uint8_t        maxLen  = *aOutBufLen;
	size_t         attrlen = 0;
	const uint8_t *attr    = NULL;

	switch (aAttrib)
	{
	case EVBME_OT_EUI64:
		otLinkGetFactoryAssignedIeeeEui64(OT_INSTANCE, &extAddress);
		attr    = extAddress.m8;
		attrlen = sizeof(extAddress);
		break;
	case EVBME_OT_JOINCRED:
		attr    = PlatformGetJoinerCredential(OT_INSTANCE);
		attrlen = strlen(attr) + 1;
		break;
	default:
		error = CA_ERROR_UNKNOWN;
		break;
	}

	if (attrlen > maxLen)
		error = CA_ERROR_NO_BUFFER;

	if (attr && !error)
	{
		memcpy(aOutBuf, attr, attrlen);
		*aOutBufLen = attrlen;
	}
	else
	{
		*aOutBufLen = 0;
	}

	return error;
}

otError PlatformGetQRString(char *aBufOut, size_t bufferSize, otInstance *aInstance)
{
	otError      error = OT_ERROR_NONE;
	otExtAddress eui64;
	size_t       bufferLen;

	bufferLen = strlen("v=1&&eui=") + sizeof(eui64.m8) * 2 + strlen("&&cc=") +
	            strlen(PlatformGetJoinerCredential(OT_INSTANCE)) + sizeof('\0');

	if (bufferSize < bufferLen)
	{
		error = OT_ERROR_NO_BUFS;
		return error;
	}

	otLinkGetFactoryAssignedIeeeEui64(aInstance, &eui64);
	strcpy(aBufOut, "v=1&&eui=");
	bufferLen = strlen(aBufOut);

	for (int i = 0; i < sizeof(eui64); i++)
	{
		sprintf(aBufOut + bufferLen, "%02x", eui64.m8[i]);
		bufferLen += 2;
	}

	strcat(aBufOut, "&&cc=");
	strcat(aBufOut, PlatformGetJoinerCredential(OT_INSTANCE));

	return error;
}

#ifdef __MINGW32__
//Mingw doesn't work with 'weak' attribute for some reason, so we provide copies of what openthread already defines weakly.
uint32_t otPlatRadioGetSupportedChannelMask(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return 0xffff << 11;
}

uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
	return otPlatRadioGetSupportedChannelMask(aInstance);
}

const char *otPlatRadioGetVersionString(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return otGetVersionString();
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return OT_RADIO_STATE_INVALID;
}
#endif
