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

#ifndef CA821X_API_INCLUDE_CA821X_API_HELPER_H_
#define CA821X_API_INCLUDE_CA821X_API_HELPER_H_

#include "ca821x_api.h"

/**
 * \brief Get pointer to the security spec of the MCPS Data Indication
 *
 * This helper function exists because the secspec exists after a variable length
 * portion of the data indication.
 *
 * \param aPset - Pointer to the MCPS_Data_indication to be analysed
 *
 * \return Pointer to the SecSpec
 *
 */
struct SecSpec *MCPS_DATA_indication_get_secspec(struct MCPS_DATA_indication_pset *aPset);

/**
 * \brief Get the short addresses in the beacon address list
 *
 * This helper function exists because the short address list exists after a
 * variable length portion of the beacon indication.
 *
 * \param aPset - Pointer to the MLME_BEACON_NOTIFY_indication to be analysed
 * \param aLength[out] - Length of the output list (Must not be NULL)
 *
 * \return Pointer to the start of the ShortAddr list (even if length=0)
 *
 */
struct ShortAddr *MLME_BEACON_NOTIFY_indication_get_shortaddrs(uint8_t *                                  aLength,
                                                               struct MLME_BEACON_NOTIFY_indication_pset *aPset);

/**
 * \brief Get the ext addresses in the beacon address list
 *
 * This helper function exists because the ext address list exists after a
 * variable length portion of the beacon indication.
 *
 * \param aPset - Pointer to the MLME_BEACON_NOTIFY_indication to be analysed
 * \param aLength[out] - Length of the output list (Must not be NULL)
 *
 * \return Pointer to the start of the ExtAddr list (even if length=0)
 *
 */
struct ExtAddr *MLME_BEACON_NOTIFY_indication_get_extaddrs(uint8_t *                                  aLength,
                                                           struct MLME_BEACON_NOTIFY_indication_pset *aPset);

/**
 * \brief Get the SDU from a beacon notify indication
 *
 * This helper function exists because the sdu exists after a variable length
 * portion of the beacon indication.
 *
 * \param aPset - Pointer to the MLME_BEACON_NOTIFY_indication to be analysed
 * \param aLength[out] - Length of the output list (Must not be NULL)
 *
 * \return Pointer to the start of the SDU (even if length=0)
 *
 */
uint8_t *MLME_BEACON_NOTIFY_indication_get_sdu(uint8_t *aLength, struct MLME_BEACON_NOTIFY_indication_pset *aPset);

/**
 * \brief Get an indexed pandescriptor from an MLME-SCAN-confirm of a passive or active scan
 *
 * This helper function exists because the pandescriptor list is not simple to parse,
 * as each pandescriptor has a variable length.
 *
 * \param aPset - Pointer to the MLME_SCAN_confirm to be analysed
 * \param aIndex - Index of the pandescriptor to access
 *
 * \return Pointer to the pandescriptor at that index (or NULL if out of range/invalid scan type)
 *
 */
struct PanDescriptor *MLME_SCAN_confirm_get_pandescriptor(uint8_t aIndex, struct MLME_SCAN_confirm_pset *aPset);

/**
 * \brief Get a pointer to the start of the Key ID Lookup Descriptor list
 *
 * If this function is being used to create a key table entry, the user MUST make
 * sure that the keytable entry being used has sufficient memory allocated to support
 * the descriptors being written. The M_KeyTableEntryFixed by itself does not have
 * any memory allocated for the variable lists.
 *
 * All Key..Entries fields MUST be filled out prior to filling the lists, so that
 * these helper functions can successfully parse the structure.
 *
 * \params aKte - Pointer to the fixed part of the key table entry
 * \return Pointer to the start of the KeyIdLookupDescriptor list (even if length=0)
 *
 */
struct M_KeyIdLookupDesc *KeyTableEntry_get_keyidlookupdescs(struct M_KeyTableEntryFixed *aKte);

/**
 * \brief Get a pointer to the start of the Key Device Descriptor list
 *
 * If this function is being used to create a key table entry, the user MUST make
 * sure that the keytable entry being used has sufficient memory allocated to support
 * the descriptors being written. The M_KeyTableEntryFixed by itself does not have
 * any memory allocated for the variable lists.
 *
 * All Key..Entries fields MUST be filled out prior to filling the lists, so that
 * these helper functions can successfully parse the structure.
 *
 * \params aKte - Pointer to the fixed part of the key table entry
 * \return Pointer to the start of the KeyDeviceDescriptor list (even if length=0)
 *
 */
struct M_KeyDeviceDesc *KeyTableEntry_get_keydevicedescs(struct M_KeyTableEntryFixed *aKte);

/**
 * \brief Get a pointer to the start of the Key Usage Descriptor list
 *
 * If this function is being used to create a key table entry, the user MUST make
 * sure that the keytable entry being used has sufficient memory allocated to support
 * the descriptors being written. The M_KeyTableEntryFixed by itself does not have
 * any memory allocated for the variable lists.
 *
 * All Key..Entries fields MUST be filled out prior to filling the lists, so that
 * these helper functions can successfully parse the structure.
 *
 * \params aKte - Pointer to the fixed part of the key table entry
 * \return Pointer to the start of the KeyUsageDescriptor list (even if length=0)
 *
 */
struct M_KeyUsageDesc *KeyTableEntry_get_keyusagedescs(struct M_KeyTableEntryFixed *aKte);

#endif /* CA821X_API_INCLUDE_CA821X_API_HELPER_H_ */
