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

#include <ca821x_api_helper.h>

struct SecSpec *MCPS_DATA_indication_get_secspec(const struct MCPS_DATA_indication_pset *aPset)
{
#if CASCODA_CA_VER >= 8212
	return (struct SecSpec *)(aPset->Data + aPset->HeaderIELength + aPset->PayloadIELength + aPset->MsduLength);
#else
	return (struct SecSpec *)(aPset->Msdu + aPset->MsduLength);
#endif // CASCODA_CA_VER >= 8212
}

static uint8_t *MLME_BEACON_NOTIFY_get_pendaddrspec(const struct MLME_BEACON_NOTIFY_indication_pset *aPset)
{
	if (aPset->PanDescriptor.Security.SecurityLevel == 0)
	{
		return (uint8_t *)&(aPset->PanDescriptor.Security.KeyIdMode);
	}
	else
	{
		return (uint8_t *)&(aPset->PanDescriptor.Security.KeyIndex) + 1;
	}
}

struct ShortAddr *MLME_BEACON_NOTIFY_indication_get_shortaddrs(uint8_t                                         *aLength,
                                                               const struct MLME_BEACON_NOTIFY_indication_pset *aPset)
{
	uint8_t *wrkPtr = MLME_BEACON_NOTIFY_get_pendaddrspec(aPset);

	if (aLength)
		*aLength = (*wrkPtr & 0x03);

	wrkPtr += 1;

	return (struct ShortAddr *)wrkPtr;
}

struct ExtAddr *MLME_BEACON_NOTIFY_indication_get_extaddrs(uint8_t                                         *aLength,
                                                           const struct MLME_BEACON_NOTIFY_indication_pset *aPset)
{
	uint8_t           pas    = *MLME_BEACON_NOTIFY_get_pendaddrspec(aPset);
	uint8_t           shLen  = 0;
	struct ShortAddr *shList = NULL;

	if (aLength)
		*aLength = (pas >> 4) & 0x03;

	shList = MLME_BEACON_NOTIFY_indication_get_shortaddrs(&shLen, aPset);
	shList += shLen;

	return (struct ExtAddr *)shList;
}

uint8_t *MLME_BEACON_NOTIFY_indication_get_sdu(uint8_t *aLength, const struct MLME_BEACON_NOTIFY_indication_pset *aPset)
{
	uint8_t         exLen  = 0;
	struct ExtAddr *exList = NULL;
	uint8_t        *rval   = NULL;

	exList = MLME_BEACON_NOTIFY_indication_get_extaddrs(&exLen, aPset);
	exList = exList + exLen;
	rval   = (uint8_t *)exList;

	if (aLength)
		*aLength = *rval;
	rval += 1;

	return rval;
}

struct PanDescriptor *MLME_SCAN_confirm_get_pandescriptor(uint8_t aIndex, const struct MLME_SCAN_confirm_pset *aPset)
{
	struct PanDescriptor *rval = NULL;

	if (aPset->ScanType != ACTIVE_SCAN && aPset->ScanType != PASSIVE_SCAN)
		goto exit;
	if (aIndex >= aPset->ResultListSize)
		goto exit;

	rval = (struct PanDescriptor *)(aPset->ResultList);
	for (int i = 0; i < aIndex; i++)
	{
		if (rval->Security.SecurityLevel == 0)
		{
			rval = (struct PanDescriptor *)&(rval->Security.KeyIdMode);
		}
		else
		{
			rval = rval + 1;
		}
	}

exit:
	return rval;
}

#if CASCODA_CA_VER <= 8211
struct M_KeyIdLookupDesc *KeyTableEntry_get_keyidlookupdescs(const struct M_KeyTableEntryFixed *aKte)
{
	return (struct M_KeyIdLookupDesc *)(aKte->Key + 16);
}

struct M_KeyDeviceDesc *KeyTableEntry_get_keydevicedescs(const struct M_KeyTableEntryFixed *aKte)
{
	struct M_KeyIdLookupDesc *working;

	working = KeyTableEntry_get_keyidlookupdescs(aKte);
	working += aKte->KeyIdLookupListEntries;

	return (struct M_KeyDeviceDesc *)working;
}

struct M_KeyUsageDesc *KeyTableEntry_get_keyusagedescs(const struct M_KeyTableEntryFixed *aKte)
{
	struct M_KeyDeviceDesc *working;

	working = KeyTableEntry_get_keydevicedescs(aKte);
	working += aKte->KeyDeviceListEntries;

	return (struct M_KeyUsageDesc *)working;
}
#endif // CASCODA_CA_VER <= 8211