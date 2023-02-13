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
 *   This file defines the flash interface used by settings.cpp.
 */

#ifndef UTILS_FLASH_H
#define UTILS_FLASH_H

#include <stdint.h>
#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Description of the internal flash */
struct ca_flash_info
{
	uint32_t apromFlashBaseAddr;            //!< Base address of the APROM flash
	uint32_t dataFlashBaseAddr;             //!< Base address of the dataflash
	uint32_t manufacturerDataFlashBaseAddr; //!< Base address of flash page reserved for constant manufacturer data
	uint16_t pageSize;                      //!< Size of each flash page (in bytes)
	uint8_t  numPages;                      //!< Number of flash pages that make up the user flash region
};

/**
 * Perform any initialization for flash driver.
 *
 * @param[in] aInstance API instance to initialize/whose storage to access. Unused on baremetal.
 * @param[in] aApplicationName Filename of the storage file, used to distinguish between storage applications. Unused on baremetal.
 * @param[in] aNodeId Used to distinguish between different nodes on the same host. Unused on baremetal.
 * 
 * @retval CA_ERROR_SUCCESS    Initialize flash driver success.
 * @retval CA_ERROR_FAIL  Initialize flash driver fail.
 */
ca_error utilsFlashInit(struct ca821x_dev *aInstance, const char *aApplicationName, uint32_t aNodeId);

/**
 * @brief Deinitialize the flash driver
 * 
 * @param aInstance Instance to deinitialize. Unused on baremetal.
 * 
 * @retval CA_ERROR_SUCCESS    Initialize flash driver success.
 * @retval CA_ERROR_FAIL  Initialize flash driver fail.
 */
ca_error utilsFlashDeinit(struct ca821x_dev *aInstance);

/**
 * Get the size of flash that can be read/write by the caller.
 * The usable flash size is always the multiple of flash page size.
 *
 * @param[in] aInstance API instance to initialize/whose storage to access. Unused on baremetal.
 * @returns The size of the flash.
 */
uint32_t utilsFlashGetSize(struct ca821x_dev *aInstance);

/**
 * Erase one flash page that include the input address.
 * This is a non-blocking function. It can work with utilsFlashStatusWait to check when erase is done.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for erasing.
 * 0 is always mapped to the beginning of one flash page.
 * The input address should never be mapped to the firmware space or any other protected flash space.
 *
 * @param[in] aInstance API instance to initialize/whose storage to access. Unused on baremetal.
 * @param[in]  aAddress  The start address of the flash to erase.
 *
 * @retval CA_ERROR_SUCCESS           Erase flash operation is started.
 * @retval CA_ERROR_FAIL         Erase flash operation is not started.
 * @retval CA_ERROR_INVALID_ARGS    aAddress is out of range of flash or not aligend.
 */
ca_error utilsFlashErasePage(struct ca821x_dev *aInstance, uint32_t aAddress);

/**
  * Check whether flash is ready or busy.
  *
  * @param[in] aInstance API instance to initialize/whose storage to access. Unused on baremetal.
  * @param[in]  aTimeout  The interval in milliseconds waiting for the flash operation to be done and become ready again.
  *                       zero indicates that it is a polling function, and returns current status of flash immediately.
  *                       non-zero indicates that it is blocking there until the operation is done and become ready, or timeout expires.
  *
  * @retval CA_ERROR_SUCCESS           Flash is ready for any operation.
  * @retval CA_ERROR_BUSY           Flash is busy.
  */
ca_error utilsFlashStatusWait(struct ca821x_dev *aInstance, uint32_t aTimeout);

/**
 * Write flash. The write operation only clears bits, but never set bits.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for writing.
 * 0 is always mapped to the beginning of one flash page.
 * The input address should never be mapped to the firmware space or any other protected flash space.
 *
 * @param[in] aInstance API instance to initialize/whose storage to access. Unused on baremetal.
 * @param[in]  aAddress  The start address of the flash to write.
 * @param[in]  aData     The pointer of the data to write.
 * @param[in]  aSize     The size of the data to write.
 *
 * @returns The actual size of octets write to flash.
 *          It is expected the same as aSize, and may be less than aSize.
 *          0 indicates that something wrong happens when writing.
 */
uint32_t utilsFlashWrite(struct ca821x_dev *aInstance, uint32_t aAddress, const uint8_t *aData, uint32_t aSize);

/**
 * Read flash.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for reading.
 * 0 is always mapped to the beginning of one flash page.
 * The input address should never be mapped to the firmware space or any other protected flash space.
 *
 * @param[in] aInstance API instance to initialize/whose storage to access. Unused on baremetal.
 * @param[in]   aAddress  The start address of the flash to read.
 * @param[out]  aData     The pointer of buffer for reading.
 * @param[in]   aSize     The size of the data to read.
 *
 * @returns The actual size of octets read to buffer.
 *          It is expected the same as aSize, and may be less than aSize.
 *          0 indicates that something wrong happens when reading.
 */
uint32_t utilsFlashRead(struct ca821x_dev *aInstance, uint32_t aAddress, uint8_t *aData, uint32_t aSize);

/**
 * @brief Internal function for getting the base address
 * 
 * @param aInstance The device to access
 * @return uint32_t The base address
 */
uint32_t utilsFlashGetBaseAddress(struct ca821x_dev *aInstance);

/**
 * @brief Internal function for returning the amount of flash used
 * 
 * @param aInstance The device to access
 * @return uint32_t How much flash is used starting from the base address
 */
uint32_t utilsFlashGetUsedSize(struct ca821x_dev *aInstance);

/**
 * @brief Internal function for setting the base address
 * 
 * @param aInstance The device to access
 * @param aAddress The address
 */
void utilsFlashSetBaseAddress(struct ca821x_dev *aInstance, uint32_t aAddress);

/**
 * @brief Internal function for setting the amount of flash used
 * 
 * @param aInstance The device to access
 * @param aSize The size
 */
void utilsFlashSetUsedSize(struct ca821x_dev *aInstance, uint32_t aSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTILS_FLASH_H
