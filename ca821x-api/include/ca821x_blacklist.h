/*
 *  Copyright (c) 2020, Cascoda Ltd.
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
 * Declarations of the blacklisting functions to filter out confirms and indications relating to certain devices.
 */
/**
 * @ingroup ca821x-api
 * @defgroup ca821x-api-bl Blacklisting functions
 * @brief Blacklisting functions for filtering out confirms and indications relating to blocked devices
 *
 * @{
 */

#ifndef CA821X_BLACKLIST
#define CA821X_BLACKLIST

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/****** Blacklisting Functions                                           ******/
/******************************************************************************/

/**
 * Add an address to the blacklist. Indications from this address.
 * will be filtered out by the API. 
 * 
 * The blacklist works with both short and extended addresses. 
 * CASCODA_MAC_BLACKLIST must be set within the CMake config for this 
 * to have any effect. If it is 0, this function will return CA_ERROR_FAIL
 * without blacklisting anything.
 * 
 * The filtering occurs above the MAC layer, and it does not affect ACK processing.
 * Consequently, requests sent by a blacklisted device will appear successful, which
 * is not what would happen if the devices were out of range of each other.
 * 
 * @param pAddress Pointer to the address that will be added to the blacklist.
 * AddressMode must be MAC_MODE_SHORT_ADDR or MAC_MODE_LONG_ADDR. The PanId 
 * field is ignored.
 * @param pDeviceRef Pointer to the device that will blacklist the given address
 * 
 * @return ca_error One of the following values of the Cascoda error type
 * @retval CA_ERROR_SUCCESS if the address was successfully added
 * @retval CA_ERROR_NO_BUFFER if there is no more room in the blacklist 
 * @retval CA_ERROR_INVALID_ARGS if AddressMode contains an invalid value 
 * @retval CA_ERROR_FAIL if blacklisting is disabled within the CMake configuration
 */
ca_error BLACKLIST_Add(struct MacAddr *pAddress, struct ca821x_dev *pDeviceRef);

/**
 * Clear the entire blacklist.
 * 
 * When called, this function deletes every entry within the blacklist. If
 * CASCODA_MAC_BLACKLIST is set to 0, this function will have no effect,
 * as the API is already not filtering any indications.
 * 
 * @param pDeviceRef Pointer to the device that will clear its blacklist.
 * 
 */
void BLACKLIST_Clear(struct ca821x_dev *pDeviceRef);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
