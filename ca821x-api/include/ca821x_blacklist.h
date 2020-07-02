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

#endif
