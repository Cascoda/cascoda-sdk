/*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ca821x_api_helper.h>

struct SecSpec *MCPS_DATA_indication_get_secspec(struct MCPS_DATA_indication_pset *aPset)
{
	return (struct SecSpec *)(aPset->Msdu + aPset->MsduLength);
}

static uint8_t *MLME_BEACON_NOTIFY_get_pendaddrspec(struct MLME_BEACON_NOTIFY_indication_pset *aPset)
{
	if (aPset->PanDescriptor.Security.SecurityLevel == 0)
	{
		return &(aPset->PanDescriptor.Security.KeyIdMode);
	}
	else
	{
		return &(aPset->PanDescriptor.Security.KeyIndex) + 1;
	}
}

struct ShortAddr *MLME_BEACON_NOTIFY_indication_get_shortaddrs(uint8_t *                                  aLength,
                                                               struct MLME_BEACON_NOTIFY_indication_pset *aPset)
{
	uint8_t *wrkPtr = MLME_BEACON_NOTIFY_get_pendaddrspec(aPset);

	if (aLength)
		*aLength = (*wrkPtr & 0x03);

	wrkPtr += 1;

	return (struct ShortAddr *)wrkPtr;
}

struct ExtAddr *MLME_BEACON_NOTIFY_indication_get_extaddrs(uint8_t *                                  aLength,
                                                           struct MLME_BEACON_NOTIFY_indication_pset *aPset)
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

uint8_t *MLME_BEACON_NOTIFY_indication_get_sdu(uint8_t *aLength, struct MLME_BEACON_NOTIFY_indication_pset *aPset)
{
	uint8_t         exLen  = 0;
	struct ExtAddr *exList = NULL;
	uint8_t *       rval   = NULL;

	exList = MLME_BEACON_NOTIFY_indication_get_extaddrs(&exLen, aPset);
	exList = exList + exLen;
	rval   = (uint8_t *)exList;

	if (aLength)
		*aLength = *rval;
	rval += 1;

	return rval;
}

struct PanDescriptor *MLME_SCAN_confirm_get_pandescriptor(uint8_t aIndex, struct MLME_SCAN_confirm_pset *aPset)
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

struct M_KeyIdLookupDesc *KeyTableEntry_get_keyidlookupdescs(struct M_KeyTableEntryFixed *aKte)
{
	return (struct M_KeyIdLookupDesc *)(aKte->Key + 16);
}

struct M_KeyDeviceDesc *KeyTableEntry_get_keydevicedescs(struct M_KeyTableEntryFixed *aKte)
{
	struct M_KeyIdLookupDesc *working;

	working = KeyTableEntry_get_keyidlookupdescs(aKte);
	working += aKte->KeyIdLookupListEntries;

	return (struct M_KeyDeviceDesc *)working;
}

struct M_KeyUsageDesc *KeyTableEntry_get_keyusagedescs(struct M_KeyTableEntryFixed *aKte)
{
	struct M_KeyDeviceDesc *working;

	working = KeyTableEntry_get_keydevicedescs(aKte);
	working += aKte->KeyDeviceListEntries;

	return (struct M_KeyUsageDesc *)working;
}
