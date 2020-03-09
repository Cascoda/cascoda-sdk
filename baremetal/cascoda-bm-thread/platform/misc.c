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
#include "ca821x_api.h"
#include "platform.h"

enum
{
	JOINER_CREDENTIAL_LEN = 8
};

static char       joiner_credential[JOINER_CREDENTIAL_LEN + 1];
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
}

const char *PlatformGetJoinerCredential(otInstance *aInstance)
{
	uint16_t cred_len = JOINER_CREDENTIAL_LEN;

	if (joiner_credential[0])
		return joiner_credential; //Already cached

	//Generate or get the joiner credential
	if (otPlatSettingsGet(aInstance, joiner_credential_key, 0, joiner_credential, &cred_len) != OT_ERROR_NONE ||
	    cred_len != JOINER_CREDENTIAL_LEN)
	{
		genJoinerCred(joiner_credential, JOINER_CREDENTIAL_LEN);
		otPlatSettingsSet(aInstance, joiner_credential_key, joiner_credential, JOINER_CREDENTIAL_LEN);
	}
	ca_log_info("Joiner Credential: %s", joiner_credential);

	return joiner_credential;
}

otError PlatformTryJoin(struct ca821x_dev *pDeviceRef, otInstance *aInstance)
{
	struct join_status join_status = {0, 0};
	const char *       aPskd;

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

	//Report status
	ca_log_info("Join complete with error %s", otThreadErrorToString(join_status.error));

	return join_status.error;
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
