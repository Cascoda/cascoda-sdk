/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for non-volatile storage of settings.
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "openthread/platform/settings.h"
#include "openthread-core-config.h"

#include "cascoda-bm/cascoda_interface.h"

#include "code_utils.h"
#include "platform.h"

#include "cascoda-util/cascoda_settings.h"

static otError convertError(ca_error error)
{
	switch (error)
	{
	case CA_ERROR_SUCCESS:
		return OT_ERROR_NONE;
	case CA_ERROR_INVALID_ARGS:
		return OT_ERROR_INVALID_ARGS;
	case CA_ERROR_NOT_FOUND:
		return OT_ERROR_NOT_FOUND;
	case CA_ERROR_NOT_IMPLEMENTED:
		return OT_ERROR_NOT_IMPLEMENTED;
	case CA_ERROR_NO_BUFFER:
		return OT_ERROR_NO_BUFS;
	case CA_ERROR_FAIL:
		return OT_ERROR_FAILED;
	case CA_ERROR_INVALID:
		return OT_ERROR_INVALID_ARGS;
	case CA_ERROR_BUSY:
		return OT_ERROR_BUSY;
	default:
		return OT_ERROR_FAILED;
	}
}

// settings API
void otPlatSettingsInit(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	caUtilSettingsInit(pDevice, "otConfig", 1);
}

otError otPlatSettingsBeginChange(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return OT_ERROR_NONE;
}

otError otPlatSettingsCommitChange(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return OT_ERROR_NONE;
}

otError otPlatSettingsAbandonChange(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return OT_ERROR_NONE;
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	ca_error           err     = caUtilSettingsGet(pDevice, aKey, aIndex, aValue, aValueLength);
	return convertError(err);
}

otError otPlatSettingsGetAddress(uint16_t aKey, int aIndex, void **aValue, uint16_t *aValueLength)
{
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	ca_error           err     = caUtilSettingsGetAddress(pDevice, aKey, aIndex, aValue, aValueLength);
	return convertError(err);
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	ca_error           err     = caUtilSettingsSet(pDevice, aKey, aValue, aValueLength);
	return convertError(err);
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	ca_error           err     = caUtilSettingsAdd(pDevice, aKey, aValue, aValueLength);
	return convertError(err);
}

otError otPlatSettingsAddVector(otInstance *aInstance, uint16_t aKey, struct settingBuffer *aVector, size_t aCount)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	ca_error           err     = caUtilSettingsAddVector(pDevice, aKey, aVector, aCount);
	return convertError(err);
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	ca_error           err     = caUtilSettingsDelete(pDevice, aKey, aIndex);
	return convertError(err);
}

void otPlatSettingsWipe(otInstance *aInstance)
{
	struct ca821x_dev *pDevice = PlatformGetDeviceRef();
	//Wipe the settings, caching the joiner credential, which persists.
	const char *cred = PlatformGetJoinerCredential(aInstance);

	caUtilSettingsWipe(pDevice, "otConfig", 1);

	caUtilSettingsSet(pDevice, joiner_credential_key, cred, strlen(cred));
}
