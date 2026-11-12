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
 * @brief
 *   This file includes platform abstraction for non-volatile storage of settings.
 */

#ifndef CASCODA_SETTINGS_H
#define CASCODA_SETTINGS_H

#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-settings
 *
 * @brief
 *   This module includes the platform abstraction for non-volatile storage of settings.
 *
 * @{
 *
 */

/** Data structure for vectored I/O using caUtilSettingsAddVector */
struct settingBuffer
{
	const uint8_t *value;  //!< Pointer to the data to write to settings
	uint16_t       length; //!< Length of the data to write to settings
};

/**
 * @brief Performs any initialization for the settings subsystem, if necessary.
 *
 * On baremetal platforms, only one instance of the storage is created, and
 * therefore the arguments are not important.
 *
 * On host platforms (e.g. POSIX) multiple instances of the MAC may be in use
 * at the same time, and it may be desirable to have persistent storage for each
 * individual instance.
 *
 * This function opens / creates a different settings context for every unique
 * combination of application name & node ID, and binds it to the CA821x device
 * instance. Subsequent settings API calls using the same device instance will
 * use that same settings context.
 *
 * The configurable application name is useful for distinguishing between
 * applications (e.g. you would not want your integration test suite to
 * overwrite your Thread credentials), while the node ID is useful for
 * distinguishing between multiple SDKs running on the same host.
 *
 * @param[in] aInstance The Cascoda instance structure.  @param[in]
 * aApplicationName Used to distinguish between applications. Unused on
 * baremetal.
 * @param[in] aNodeId Used to distinguish between different nodes on
 * the same host. Must be smaller than or equal to 9999. Unused on baremetal.
 */
void caUtilSettingsInit(struct ca821x_dev *aInstance, const char *aApplicationName, uint32_t aNodeId);

/**
 * @brief Performs any de-initialization for the settings subsystem, if necessary.
 *
 * @param[in]  aInstance The Cascoda instance structure.
 *
 */
void caUtilSettingsDeinit(struct ca821x_dev *aInstance);

/** This function fetches the value of the setting identified
 *  by aKey and write it to the memory pointed to by aValue.
 *  It then writes the length to the integer pointed to by
 *  aValueLength. The initial value of aValueLength is the
 *  maximum number of bytes to be written to aValue.
 *
 *  This function can be used to check for the existence of
 *  a key without fetching the value by setting aValue and
 *  aValueLength to NULL. You can also check the length of
 *  the setting without fetching it by setting only aValue
 *  to NULL.
 *
 *  Note that the underlying storage implementation is not
 *  required to maintain the order of settings with multiple
 *  values. The order of such values MAY change after ANY
 *  write operation to the store.
 *
 *  @param[in]     aInstance     The Cascoda instance structure.
 *  @param[in]     aKey          The key associated with the requested setting.
 *  @param[in]     aIndex        The index of the specific item to get.
 *  @param[out]    aValue        A pointer to where the value of the setting should be written. May be set to NULL if
 *                               just testing for the presence or length of a setting.
 *  @param[inout]  aValueLength  A pointer to the length of the value. When called, this pointer should point to an
 *                               integer containing the maximum value size that can be written to aValue. At return,
 *                               the actual length of the setting is written. This may be set to NULL if performing
 *                               a presence check.
 *
 *  @retval CA_ERROR_SUCCESS         The given setting was found and fetched successfully.
 *  @retval CA_ERROR_NOT_FOUND       The given setting was not found in the setting store.
 *  @retval CA_ERROR_NOT_IMPLEMENTED This function is not implemented on this platform.
 */
ca_error caUtilSettingsGet(struct ca821x_dev *aInstance,
                           uint16_t           aKey,
                           int                aIndex,
                           uint8_t           *aValue,
                           uint16_t          *aValueLength);

/** 
 *  @brief Sets or replaces the value of a setting
 * 
 *  This function sets or replaces the value of a setting
 *  identified by aKey. If there was more than one
 *  value previously associated with aKey, then they are
 *  all deleted and replaced with this single entry.
 *
 *  Calling this function successfully may cause unrelated
 *  settings with multiple values to be reordered.
 *
 *  @param[in]  aInstance     The Cascoda instance structure.
 *  @param[in]  aKey          The key associated with the setting to change.
 *  @param[in]  aValue        A pointer to where the new value of the setting should be read from. MUST NOT be NULL if
 *                            aValueLength is non-zero.
 *  @param[in]  aValueLength  The length of the data pointed to by aValue. May be zero.
 *
 *  @retval CA_ERROR_SUCCESS          The given setting was changed or staged.
 *  @retval CA_ERROR_NOT_IMPLEMENTED  This function is not implemented on this platform.
 *  @retval CA_ERROR_NO_BUFFER        No space remaining to store the given setting.
 */
ca_error caUtilSettingsSet(struct ca821x_dev *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

/** 
 *  @brief Adds a value to a setting
 *  This function adds the value to a setting
 *  identified by aKey, without replacing any existing
 *  values.
 *
 *  Note that the underlying implementation is not required
 *  to maintain the order of the items associated with a
 *  specific key. The added value may be added to the end,
 *  the beginning, or even somewhere in the middle. The order
 *  of any pre-existing values may also change.
 *
 *  Calling this function successfully may cause unrelated
 *  settings with multiple values to be reordered.
 *
 * @param[in]  aInstance     The Cascoda instance structure.
 * @param[in]  aKey          The key associated with the setting to change.
 * @param[in]  aValue        A pointer to where the new value of the setting should be read from. MUST NOT be NULL
 *                           if aValueLength is non-zero.
 * @param[in]  aValueLength  The length of the data pointed to by aValue. May be zero.
 *
 * @retval CA_ERROR_SUCCESS          The given setting was added or staged to be added.
 * @retval CA_ERROR_NOT_IMPLEMENTED  This function is not implemented on this platform.
 * @retval CA_ERROR_NO_BUFFER        No space remaining to store the given setting.
 */
ca_error caUtilSettingsAdd(struct ca821x_dev *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

/** 
 * @brief Removes a setting from the setting store
 * 
 *  This function deletes a specific value from the
 *  setting identified by aKey from the settings store.
 *
 *  Note that the underlying implementation is not required
 *  to maintain the order of the items associated with a
 *  specific key.
 *
 *  @param[in] aInstance  The Cascoda instance structure.
 *  @param[in] aKey       The key associated with the requested setting.
 *  @param[in] aIndex     The index of the value to be removed. If set to -1, all values for this aKey will be removed.
 *
 *  @retval CA_ERROR_SUCCESS          The given key and index was found and removed successfully.
 *  @retval CA_ERROR_NOT_FOUND        The given key or index was not found in the setting store.
 *  @retval CA_ERROR_NOT_IMPLEMENTED  This function is not implemented on this platform.
 */
ca_error caUtilSettingsDelete(struct ca821x_dev *aInstance, uint16_t aKey, int aIndex);

/** 
 *  @brief Removes all settings from the setting store
 *
 *  This function deletes all settings from the settings
 *  store, resetting it to its initial factory state.
 *
 *  @param[in] aInstance  The Cascoda instance structure.
 * @param[in] aApplicationName Application of the storage file, used to distinguish between storage applications. Unused on baremetal.
 * @param[in] aNodeId Used to distinguish between different nodes on the same host. 
 * Must be smaller than or equal to 9999. Unused on baremetal.
 */
void caUtilSettingsWipe(struct ca821x_dev *aInstance, const char *aApplicationName, uint32_t aNodeId);

/**
 * @brief Get the address at which a particular setting is stored
 * 
 * @param aInstance    API instance of storage to inspect. Unused on baremetal
 * @param aKey         The key associated with the requested setting.
 * @param aIndex       The index of the value to be removed. If set to -1, all values for this aKey will be removed.
 * @param aValue       A pointer to where the address of the setting should be written.
 * @param aValueLength The length of the setting
 * 
 *  @retval CA_ERROR_SUCCESS          The given setting was found and fetched successfully.
 *  @retval CA_ERROR_NOT_FOUND        The given setting was not found in the setting store.
 *  @retval CA_ERROR_NOT_IMPLEMENTED  This function is not implemented on this platform.
 */
ca_error caUtilSettingsGetAddress(struct ca821x_dev *aInstance,
                                  uint16_t           aKey,
                                  int                aIndex,
                                  void             **aValue,
                                  uint16_t          *aValueLength);

/**
 * @brief Add a vector of buffers to the storage. Stored in continuous flash within the setting
 * described by aKey
 * 
 * @param aInstance API instance. Unused on baremetal.
 * @param aKey      The key associated with the requested setting.
 * @param aVector   An array of settingBuffer structures that describes the buffers to copy into flash.
 * @param aCount    The number of buffers within the aVector array.
 * @retval CA_ERROR_SUCCESS          The given setting was added or staged to be added.
 * @retval CA_ERROR_NOT_IMPLEMENTED  This function is not implemented on this platform.
 * @retval CA_ERROR_NO_BUFFER        No space remaining to store the given setting.
 */
ca_error caUtilSettingsAddVector(struct ca821x_dev    *aInstance,
                                 uint16_t              aKey,
                                 struct settingBuffer *aVector,
                                 size_t                aCount);

/**
 * @brief Attempt to read a setting from a specific address in constant time.
 * 
 * This function is designed to be used alongside a cache in RAM which stores
 * addresses obtained using @ref caUtilSettingsGetLastWriteAddress. In return,
 * this function is very fast and its speed is independent of how full the
 * persistent storage is, unlike @ref caUtilSettingsGet.
 * 
 * @param aInstance Device instance
 * @param aKey Key we are expecting to find at aRelativeAddress
 * @param aRelativeAddress Address where the entry is expected
 * @param aValue Output buffer
 * @param aValueLength Input for the output buffer size. Outputs the written size
 * @retval CA_ERROR_SUCCESS Found a valid item with the matching key.
 * @retval CA_ERROR_NOT_FOUND Item at address is not valid.
 */
ca_error caUtilSettingsGetFromAddress(struct ca821x_dev *aInstance,
                                      uint16_t           aKey,
                                      uint16_t           aRelativeAddress,
                                      uint8_t          **aValue,
                                      uint16_t          *aValueLength);

/**
 * @brief Delete a setting stored at a specific address.

 * This function is designed to be used alongside a cache in RAM which stores
 * addresses obtained using @ref caUtilSettingsGetLastWriteAddress. In return,
 * this function is very fast and its speed is independent of how full the
 * persistent storage is, unlike @ref caUtilSettingsGet.
 * 
 * @param aInstance Device Instance
 * @param aKey Expected key of the entry
 * @param aRelativeAddress Address of the entry
 * @retval CA_ERROR_SUCCESS Deleted a valid item with the matching key.
 * @retval CA_ERROR_ALREADY Item is valid but already deleted.
 * @retval CA_ERROR_NOT_FOUND Item at address is not valid.
 */
ca_error caUtilSettingsDeleteAtAddress(struct ca821x_dev *aInstance, uint16_t aKey, uint16_t aRelativeAddress);

/**
 * @brief Get the address of the last write made by a caUtilSettings* API.
 * 
 * @return uint16_t Relative address of the previous write. Internally converted 
 * to an absolute address when calling utilsFlash* functions
 */
uint16_t caUtilSettingsGetLastWriteAddress(void);

/**
 * @brief Get the address of the last read made by a caUtilSettings* API.
 * 
 * @return uint16_t Relative address of the previous read. Internally converted 
 * to an absolute address when calling utilsFlash* functions
 */
uint32_t caUtilSettingsGetLastReadAddress(void);

typedef void (*setting_moved_cb_t)(void);

/**
 * @brief Registers the callback for when all settings have moved. Used for updating or
 * invalidating an address cache generated with @ref caUtilSettingsGetLastWriteAddress
 * 
 * @param aCb The function to be called. Will be called once per settings swap
 * (aka defragment / garbage collection).
 */
void caUtilSettingsSetSettingMovedCb(setting_moved_cb_t aCb);

/**
 * @brief Defragment the persistent storage by migrating all valid items to a fresh
 * flash region.
 * 
 * This function is called automatically when necessary by the caUtilSettingsSet/Add 
 * APIs. In general, calling this function is unnecessary unless you want to guarantee
 * that future writes will return quickly (without having to defragment).
 * 
 */
uint32_t caUtilSettingsSwapSettingsBlock(struct ca821x_dev *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CASCODA_SETTINGS_H
