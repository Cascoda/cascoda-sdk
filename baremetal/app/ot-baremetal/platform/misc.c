/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include "openthread/platform/misc.h"
#include "openthread/link.h"
#include "openthread/platform/radio.h"

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "ca821x_api.h"
#include "platform.h"

bool SleepEnabled = 1;

void otPlatReset(otInstance *aInstance)
{
	// This function does nothing on the Posix platform.
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

	BSP_LEDSigMode(LED_M_CLRALL);
	EVBME_PowerDown(PDM_POWEROFF, aSleepTime, pDeviceRef);
	BSP_LEDSigMode(LED_M_ALLON);

	otLinkSyncExternalMac(OT_INSTANCE);
	MLME_SET_request_sync(macFrameCounter, 0, 4, framecounter, pDeviceRef);
	MLME_SET_request_sync(macDSN, 0, 1, dsn, pDeviceRef);

	return OT_ERROR_NONE;
}
