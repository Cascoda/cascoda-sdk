/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  Modifications Copyright (c) 2021 Cascoda Ltd.
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
 *   This file implements the Cascoda platform abstraction for non-volatile storage of settings.
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-util/cascoda_flash.h"
#include "cascoda-util/cascoda_settings.h"
#include "ca821x_api.h"

/**
 *  This checks for the specified condition, which is expected to
 *  commonly be true, and branches to the local label 'exit' if the
 *  condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 */
#define otEXPECT(aCondition) \
	do                       \
	{                        \
		if (!(aCondition))   \
		{                    \
			goto exit;       \
		}                    \
	} while (0)

/**
 *  This checks for the specified condition, which is expected to
 *  commonly be true, and both executes @p anAction and branches to
 *  the local label 'exit' if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aAction     An expression or block to execute when the
 *                          assertion fails.
 *
 */
#define otEXPECT_ACTION(aCondition, aAction) \
	do                                       \
	{                                        \
		if (!(aCondition))                   \
		{                                    \
			aAction;                         \
			goto exit;                       \
		}                                    \
	} while (0)

/**
 *  This unconditionally executes aAction and branches to the local
 *  label 'exit'.
 *
 *  @note The use of this interface implies neither success nor
 *        failure for the overall exit status of the enclosing
 *        function body.
 *
 *  @param[in]  aAction     An expression or block to execute.
 *
 */
#define otEXIT_NOW(aAction) \
	do                      \
	{                       \
		aAction;            \
		goto exit;          \
	} while (0)

enum
{
	kBlockAddBeginFlag    = 0x1,
	kBlockAddCompleteFlag = 0x02,
	kBlockDeleteFlag      = 0x04,
	kBlockIndex0Flag      = 0x08,
};

enum
{
	// Should this not be sizeof(settingsBlock)?
	kSettingsFlagSize      = 4,
	kSettingsBlockDataSize = 511,

	kSettingsInSwap = 0xbe5cc5ef,
	kSettingsInUse  = 0xbe5cc5ee,
	kSettingsNotUse = 0xbe5cc5ec,
};

struct settingsBlock
{
	uint16_t key;
	uint16_t flag;
	uint16_t length;
	uint16_t reserved;
} __attribute__((packed));

/**
 * @def SETTINGS_CONFIG_BASE_ADDRESS
 *
 * The base address of settings.
 *
 */
#ifndef SETTINGS_CONFIG_BASE_ADDRESS
#define SETTINGS_CONFIG_BASE_ADDRESS 0
#endif // SETTINGS_CONFIG_BASE_ADDRESS

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static uint32_t sSettingsBaseAddress;
static uint32_t sSettingsUsedSize;

void BSP_GetFlashInfo(struct ca_flash_info *aFlashInfoOut);

static uint16_t getAlignLength(uint16_t length)
{
	return (length + 3) & 0xfffc;
}

static void setSettingsFlag(struct ca821x_dev *aInstance, uint32_t aBase, uint32_t aFlag)
{
	utilsFlashWrite(aInstance, aBase, (uint8_t *)(&aFlag), sizeof(aFlag));
}

static void initSettings(struct ca821x_dev *aInstance, uint32_t aBase, uint32_t aFlag)
{
	uint32_t             address = aBase;
	struct ca_flash_info flash_info;
	BSP_GetFlashInfo(&flash_info);
	uint32_t settingsSize =
	    flash_info.numPages > 1 ? (flash_info.pageSize * flash_info.numPages) / 2 : flash_info.pageSize;

	while (address < (aBase + settingsSize))
	{
		utilsFlashErasePage(aInstance, address);
		utilsFlashStatusWait(aInstance, 1000);
		address += flash_info.pageSize;
	}

	setSettingsFlag(aInstance, aBase, aFlag);
}

static uint32_t swapSettingsBlock(struct ca821x_dev *aInstance)
{
	uint32_t             oldBase     = sSettingsBaseAddress;
	uint32_t             swapAddress = oldBase;
	uint32_t             usedSize    = sSettingsUsedSize;
	struct ca_flash_info flash_info;
	BSP_GetFlashInfo(&flash_info);
	uint8_t  pageNum      = flash_info.numPages;
	uint32_t settingsSize = pageNum > 1 ? flash_info.pageSize * pageNum / 2 : flash_info.pageSize;

	otEXPECT_ACTION(pageNum > 1, ;);

	sSettingsBaseAddress =
	    (swapAddress == SETTINGS_CONFIG_BASE_ADDRESS) ? (swapAddress + settingsSize) : SETTINGS_CONFIG_BASE_ADDRESS;

	initSettings(aInstance, sSettingsBaseAddress, (uint32_t)(kSettingsInSwap));
	sSettingsUsedSize = kSettingsFlagSize;
	swapAddress += kSettingsFlagSize;

	while (swapAddress < (oldBase + usedSize))
	{
		struct settingsBlock new_block;
		bool                 valid = true;

		// Get just the settings block, put it in the addSettingsBlock structure
		utilsFlashRead(aInstance, swapAddress, (uint8_t *)(&new_block), sizeof(struct settingsBlock));
		swapAddress += sizeof(struct settingsBlock);

		// If the block is complete and not deleted
		if (!(new_block.flag & kBlockAddCompleteFlag) && (new_block.flag & kBlockDeleteFlag))
		{
			// Address points to the end of the current file
			uint32_t address = swapAddress + getAlignLength(new_block.length);

			while (address < (oldBase + usedSize))
			{
				struct settingsBlock block;

				// Read the settings at address
				utilsFlashRead(aInstance, address, (uint8_t *)(&block), sizeof(block));

				// Block is complete and not set to be deleted and is index0 and the correct key
				if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag) &&
				    !(block.flag & kBlockIndex0Flag) && (block.key == new_block.key))
				{
					// The block is invalid - go to the next one
					valid = false;
					break;
				}

				// The block is valid - continue
				address += (getAlignLength(block.length) + sizeof(struct settingsBlock));
			}

			if (valid)
			{
				const int kCopyBufferSize = 256;
				uint8_t   copy_buffer[kCopyBufferSize];
				// swapAddress points to just after the settingsBlock of the setting to be copied,
				// but we want to copy the settingsBlock as well. Therefore, we reference from
				// the start of the settings block
				uint32_t start_address       = swapAddress - sizeof(struct settingsBlock);
				uint16_t bytes_copied_so_far = 0;
				// Copy the settings block. Flash data may be corrupted if power is lost past this point!
				utilsFlashWrite(aInstance,
				                sSettingsBaseAddress + sSettingsUsedSize,
				                (uint8_t *)(&new_block),
				                sizeof(struct settingsBlock));

				bytes_copied_so_far += sizeof(struct settingsBlock);

				// Stop when the number of written bytes is equal to the length of the block at SwapAddress;
				while (bytes_copied_so_far < getAlignLength(new_block.length))
				{
					// read many bytes from swapAddress
					uint16_t remaining_bytes =
					    (getAlignLength(new_block.length) - bytes_copied_so_far + sizeof(struct settingsBlock));
					uint16_t amount_to_copy = (kCopyBufferSize > remaining_bytes) ? remaining_bytes : kCopyBufferSize;

					// copy the setting block and the data
					utilsFlashRead(aInstance, start_address + bytes_copied_so_far, copy_buffer, amount_to_copy);

					// write many bytes to sSettingsBaseAddress
					utilsFlashWrite(aInstance,
					                sSettingsBaseAddress + sSettingsUsedSize + bytes_copied_so_far,
					                copy_buffer,
					                amount_to_copy);

					bytes_copied_so_far += amount_to_copy;
				}
				// Finished writing - update the settings used size
				sSettingsUsedSize += (sizeof(struct settingsBlock) + getAlignLength(new_block.length));
			}
		}
		// flag == 0xff => this block is not in use, so you are at the end!
		else if (new_block.flag == 0xff)
		{
			break;
		}

		// Get the address of the next block to potentially swap and loop
		swapAddress += getAlignLength(new_block.length);
	}

	setSettingsFlag(aInstance, sSettingsBaseAddress, (uint32_t)(kSettingsInUse));
	setSettingsFlag(aInstance, oldBase, (uint32_t)(kSettingsNotUse));

exit:
	// Returns the amount of space left
	return settingsSize - sSettingsUsedSize;
}

static ca_error addSettingVector(struct ca821x_dev *   aInstance,
                                 uint16_t              aKey,
                                 bool                  aIndex0,
                                 struct settingBuffer *aSetting,
                                 size_t                aSettingCnt)
{
	ca_error             error = CA_ERROR_SUCCESS;
	struct settingsBlock block;
	struct ca_flash_info flash_info;
	uint16_t             total_length = 0;

	for (size_t i = 0; i < aSettingCnt; ++i) total_length += aSetting[i].length;

	BSP_GetFlashInfo(&flash_info);
	uint32_t settingsSize =
	    flash_info.numPages > 1 ? flash_info.pageSize * flash_info.numPages / 2 : flash_info.pageSize;

	block.flag = 0xff;
	block.key  = aKey;

	//If this is the zeroth element, set the flag that it is!
	if (aIndex0)
	{
		block.flag &= (~kBlockIndex0Flag);
	}

	//Mark this block as in progress and set the length
	block.flag &= (~kBlockAddBeginFlag);
	block.length = total_length;

	// Is there room for the new block?
	if ((sSettingsUsedSize + getAlignLength(block.length) + sizeof(struct settingsBlock)) >= settingsSize)
	{
		// There is no room - get a new block free of deleted entries.
		otEXPECT_ACTION(swapSettingsBlock(aInstance) >= (getAlignLength(block.length) + sizeof(struct settingsBlock)),
		                error = CA_ERROR_NO_BUFFER);
	}

	utilsFlashWrite(
	    aInstance, sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&block), sizeof(struct settingsBlock));

	uint32_t write_address = sSettingsBaseAddress + sSettingsUsedSize + sizeof(struct settingsBlock);
	for (size_t i = 0; i < aSettingCnt; ++i)
	{
		// buffers for holding padding bytes
		// initialised to FF because writing FF to a byte in flash does not modify the data stored there
		uint8_t headPad[sizeof(uint32_t)] = {0xff, 0xff, 0xff, 0xff};

		// the amount of _padding_ bytes within the first word-aligned word of the write data
		// FF FF 13 37 ...
		// ~~ ~~       <- these ones are headPadCount
		//       ~~ ~~ <- these ones are headDataCount
		uint8_t headPadCount = write_address % sizeof(uint32_t);

		uint8_t headDataCount = MIN(sizeof(uint32_t) - headPadCount, aSetting[i].length);
		memcpy(headPad + headPadCount, aSetting[i].value, headDataCount);

		// write ahead of your data, in case the start is misaligned & thus word-aligning write_address
		write_address -= headPadCount;
		utilsFlashWrite(aInstance, write_address, headPad, sizeof(headPad));
		write_address += sizeof(headPad);
		// write what you know is aligned
		utilsFlashWrite(
		    aInstance, write_address, aSetting[i].value + headDataCount, aSetting[i].length - headDataCount);
		write_address += aSetting[i].length - headDataCount;
	}

	block.flag &= (~kBlockAddCompleteFlag);
	utilsFlashWrite(
	    aInstance, sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&block), sizeof(struct settingsBlock));
	sSettingsUsedSize += (sizeof(struct settingsBlock) + getAlignLength(block.length));

exit:
	return error;
}

// settings API
void caUtilSettingsInit(struct ca821x_dev *aInstance, const char *aApplicationName, uint32_t aNodeId)
{
	uint8_t              index;
	struct ca_flash_info flash_info;
	BSP_GetFlashInfo(&flash_info);
	uint32_t settingsSize =
	    flash_info.numPages > 1 ? flash_info.pageSize * flash_info.numPages / 2 : flash_info.pageSize;

	sSettingsBaseAddress = SETTINGS_CONFIG_BASE_ADDRESS;

	utilsFlashInit(aInstance, aApplicationName, aNodeId);

	for (index = 0; index < 2; index++)
	{
		uint32_t blockFlag;

		sSettingsBaseAddress += settingsSize * index;
		utilsFlashRead(aInstance, sSettingsBaseAddress, (uint8_t *)(&blockFlag), sizeof(blockFlag));

		if (blockFlag == kSettingsInUse)
		{
			break;
		}
	}

	if (index == 2)
	{
		initSettings(aInstance, sSettingsBaseAddress, (uint32_t)(kSettingsInUse));
	}

	sSettingsUsedSize = kSettingsFlagSize;

	while (sSettingsUsedSize < settingsSize)
	{
		struct settingsBlock block;

		utilsFlashRead(aInstance, sSettingsBaseAddress + sSettingsUsedSize, (uint8_t *)(&block), sizeof(block));

		if (!(block.flag & kBlockAddBeginFlag))
		{
			sSettingsUsedSize += (getAlignLength(block.length) + sizeof(struct settingsBlock));
		}
		else
		{
			break;
		}
	}
}

ca_error caUtilSettingsBeginChange(struct ca821x_dev *aInstance)
{
	(void)aInstance;
	return CA_ERROR_SUCCESS;
}

ca_error caUtilSettingsCommitChange(struct ca821x_dev *aInstance)
{
	(void)aInstance;
	return CA_ERROR_SUCCESS;
}

ca_error caUtilSettingsAbandonChange(struct ca821x_dev *aInstance)
{
	(void)aInstance;
	return CA_ERROR_SUCCESS;
}

void caUtilSettingsDeinit(struct ca821x_dev *aInstance)
{
	(void)aInstance;
}

static ca_error getRelativeAddress(struct ca821x_dev *aInstance,
                                   uint16_t           aKey,
                                   int                aIndex,
                                   uint32_t *         aAddress,
                                   uint16_t *         aValueLength)
{
	ca_error error   = CA_ERROR_NOT_FOUND;
	uint32_t address = sSettingsBaseAddress + kSettingsFlagSize;
	int      index   = 0;

	while (address < (sSettingsBaseAddress + sSettingsUsedSize))
	{
		struct settingsBlock block;

		utilsFlashRead(aInstance, address, (uint8_t *)(&block), sizeof(block));

		if (block.key == aKey)
		{
			if (!(block.flag & kBlockIndex0Flag))
			{
				index = 0;
				error = CA_ERROR_NOT_FOUND;
			}

			if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag))
			{
				if (index == aIndex)
				{
					*aAddress     = address + sizeof(struct settingsBlock);
					*aValueLength = block.length;
					error         = CA_ERROR_SUCCESS;
				}

				index++;
			}
		}
		address += (getAlignLength(block.length) + sizeof(struct settingsBlock));
	}
	return error;
}

ca_error caUtilSettingsGet(struct ca821x_dev *aInstance,
                           uint16_t           aKey,
                           int                aIndex,
                           uint8_t *          aValue,
                           uint16_t *         aValueLength)
{
	ca_error error;
	uint32_t address;
	uint16_t readLength;

	error = getRelativeAddress(aInstance, aKey, aIndex, &address, &readLength);
	if (error)
		goto exit;

	// only copy data if an input buffer was passed in
	if (aValue != NULL && aValueLength != NULL)
	{
		// adjust read length if input buffer length is smaller
		if (readLength > *aValueLength)
		{
			readLength = *aValueLength;
		}

		utilsFlashRead(aInstance, address, aValue, readLength);
	}

	if (aValueLength != NULL)
	{
		*aValueLength = readLength;
	}

exit:
	return error;
}

ca_error caUtilSettingsGetAddress(struct ca821x_dev *aInstance,
                                  uint16_t           aKey,
                                  int                aIndex,
                                  void **            aValue,
                                  uint16_t *         aValueLength)
{
	uint32_t             relative_address;
	struct ca_flash_info flash_info;
	ca_error             error;

	BSP_GetFlashInfo(&flash_info);

	error = getRelativeAddress(aInstance, aKey, aIndex, &relative_address, aValueLength);
	if (error)
		goto exit;

	*aValue = (void *)(intptr_t)(relative_address + flash_info.dataFlashBaseAddr);
exit:
	return error;
}

ca_error caUtilSettingsSet(struct ca821x_dev *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
	//Add a setting, setting Index0 flag as we want this to overwrite all others.
	struct settingBuffer setting = {aValue, aValueLength};
	return addSettingVector(aInstance, aKey, true, &setting, 1);
}

ca_error caUtilSettingsAdd(struct ca821x_dev *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
	uint16_t             length;
	bool                 index0;
	struct settingBuffer setting;

	//Set index0 flag if we don't have any other elements with same key
	index0         = (caUtilSettingsGet(aInstance, aKey, 0, NULL, &length) == CA_ERROR_NOT_FOUND ? true : false);
	setting.value  = aValue;
	setting.length = aValueLength;
	return addSettingVector(aInstance, aKey, index0, &setting, 1);
}

ca_error caUtilSettingsAddVector(struct ca821x_dev *   aInstance,
                                 uint16_t              aKey,
                                 struct settingBuffer *aVector,
                                 size_t                aCount)
{
	uint16_t length;
	bool     index0;

	index0 = (caUtilSettingsGet(aInstance, aKey, 0, NULL, &length) == CA_ERROR_NOT_FOUND ? true : false);
	return addSettingVector(aInstance, aKey, index0, aVector, aCount);
}

ca_error caUtilSettingsDelete(struct ca821x_dev *aInstance, uint16_t aKey, int aIndex)
{
	ca_error error   = CA_ERROR_NOT_FOUND;
	uint32_t address = sSettingsBaseAddress + kSettingsFlagSize;
	int      index   = 0;

	while (address < (sSettingsBaseAddress + sSettingsUsedSize))
	{
		struct settingsBlock block;

		utilsFlashRead(aInstance, address, (uint8_t *)(&block), sizeof(block));

		//If the key matches
		if (block.key == aKey)
		{
			//If this block is marked as 0, set the index tracker to 0
			if (!(block.flag & kBlockIndex0Flag))
			{
				index = 0;
				error = CA_ERROR_NOT_FOUND;
			}

			//If this block is both completed and not deleted, investigate
			if (!(block.flag & kBlockAddCompleteFlag) && (block.flag & kBlockDeleteFlag))
			{
				//If this device is the index we are looking for, or we have provided -1 as the index arg, delete it
				if (aIndex == index || aIndex == -1)
				{
					error = CA_ERROR_SUCCESS;
					block.flag &= (~kBlockDeleteFlag);
					utilsFlashWrite(aInstance, address, (uint8_t *)(&block), sizeof(block));
				}

				//If the index is 1, and we wanted to delete 0, set the old 1 to the new 0 with the zero flag!
				if (index == 1 && aIndex == 0)
				{
					block.flag &= (~kBlockIndex0Flag);
					utilsFlashWrite(aInstance, address, (uint8_t *)(&block), sizeof(block));
				}

				index++;
			}
		}

		//Iterate to the next block
		address += (getAlignLength(block.length) + sizeof(struct settingsBlock));
	}

	return error;
}

void caUtilSettingsWipe(struct ca821x_dev *aInstance, const char *aApplicationName, uint32_t aNodeId)
{
	initSettings(aInstance, sSettingsBaseAddress, (uint32_t)(kSettingsInUse));
	caUtilSettingsInit(aInstance, aApplicationName, aNodeId);
}
