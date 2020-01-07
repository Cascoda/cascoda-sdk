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

#ifndef CA821X_API_INCLUDE_CA821X_API_HELPER_H_
#define CA821X_API_INCLUDE_CA821X_API_HELPER_H_

#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * \param[out] aLength - Length of the output list (Must not be NULL)
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
 * \param[out] aLength - Length of the output list (Must not be NULL)
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
 * \param[out] aLength - Length of the output list (Must not be NULL)
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
 * \param aKte - Pointer to the fixed part of the key table entry
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
 * \param aKte - Pointer to the fixed part of the key table entry
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
 * \param aKte - Pointer to the fixed part of the key table entry
 * \return Pointer to the start of the KeyUsageDescriptor list (even if length=0)
 *
 */
struct M_KeyUsageDesc *KeyTableEntry_get_keyusagedescs(struct M_KeyTableEntryFixed *aKte);

#ifdef __cplusplus
}
#endif

#endif /* CA821X_API_INCLUDE_CA821X_API_HELPER_H_ */
