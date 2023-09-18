/*
 * Copyright (c) 2021, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Helper functions for the EVBME DFU commands that operate on the external flash.
 */

#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_hash.h"
#include "cascoda_external_flash.h"

#define MAX_ADDRESS 0xFFFFF
#define PAGE_SIZE 0x100

#define HEX_64KB 0x10000
#define HEX_32KB 0x8000
#define HEX_4KB 0x1000

#define MIN(a, b) (a < b ? a : b)

enum
{
	mask_64kb = 0x1F0000,
	mask_32kb = 0x1F8000,
	mask_4kb  = 0x1FF000
};

/***** Variable passed as an argument to upstream callbacks, providing status information *****/
static ca_error upstream_status;

/**
 * This function determines whether to erase the entire chip or not, based
 * on the start and end addresses.
 * If startAddress is in the first 4kb sector of the first 64kb block AND
 * endAddress is in the last 4kb sector of the last 64kb block, then the entire
 * chip should be erased.
 *
 * @param aStartAddress The memory address of the first byte that should be erased.
 * @param aEndAddress   The memory address of the final byte that should be erased.
 *
 * @return true if the entire chip should be erased, false otherwise.
*/
static bool should_erase_chip(u32_t aStartAddress, u32_t aEndAddress)
{
	return (aStartAddress < HEX_4KB) && (aEndAddress > MAX_ADDRESS - HEX_4KB + 1);
}

/**
 * This function determines whether to erase a 64kb block or not, based
 * on the start and end addresses.
 * If startAddress is in the first 4kb sector of a 64kb block AND
 * endAddress is higher than the start of the last 4kb sector of the same 64kb block,
 * then a 64kb block should be erased.
 *
 * @param aStartAddress The memory address of the first byte that should be erased.
 * @param aEndAddress   The memory address of the final byte that should be erased.
 *
 * @return true if a 64kb block should be erased, false otherwise.
*/
static bool should_erase_64kb(u32_t aStartAddress, u32_t aEndAddress)
{
	return (aStartAddress % HEX_64KB < HEX_4KB) && (aEndAddress > (aStartAddress & mask_32kb) + HEX_64KB - HEX_4KB);
}

/**
 * This function determines whether to erase a 32kb block or not, based
 * on the start and end addresses.
 * If startAddress is in the first 4kb sector of a 32kb block AND
 * endAddress is higher than the start of the last 4kb sector of the same 32kb block,
 * then a 32kb block should be erased.
 *
 * @param aStartAddress The memory address of the first byte that should be erased.
 * @param aEndAddress   The memory address of the final byte that should be erased.
 *
 * @return true if a 32kb block should be erased, false otherwise.
*/
static bool should_erase_32kb(u32_t aStartAddress, u32_t aEndAddress)
{
	return (aStartAddress % HEX_32KB < HEX_4KB) && (aEndAddress > (aStartAddress & mask_32kb) + HEX_32KB - HEX_4KB);
}

/**
 * Function used by 'update_erase_info' to obtain an updated start address for the
 * next scheduled erase instruction.
 *
 * @param aT            The type of erase that was performed.
 * @param aStartAddress The memory address of the first byte that was erased.
 * @param aEndAddress   The memory address of the final byte that was erased.
 *
 * @return The next startAddress.
*/
static u32_t get_new_start(ExternalEraseType aT, u32_t aStartAddress, u32_t aEndAddress)
{
	uint32_t nextBoundary;

	if (aT == ERASE_CHIP)
		return aEndAddress;
	else if (aT == ERASE_64KB)
		nextBoundary = (aStartAddress + HEX_64KB) & mask_64kb;
	else if (aT == ERASE_32KB)
		nextBoundary = (aStartAddress + HEX_32KB) & mask_32kb;
	else
		nextBoundary = (aStartAddress + HEX_4KB) & mask_4kb;

	return MIN(aEndAddress, nextBoundary);
}

/**
 * Function used by 'erase_helper' to updated its info pointer.
 *
 * @param aT    The type of erase that was performed
 * @param aInfo Pointer to struct containing information necessary for the external flash erase instruction
 */
static void update_erase_info(ExternalEraseType aT, ExtFlashEraseInfo *aInfo)
{
	aInfo->startAddress = get_new_start(aT, aInfo->startAddress, aInfo->endAddress);
	aInfo->eraseLength  = aInfo->endAddress - aInfo->startAddress;
}

/**
 * Function that calls the appropriate External Flash Erase instruction based on
 * the arguments. It then schedules 'external_flash_evbme_erase_helper' again.
 *
 * @param aT    The type of erase to perform
 * @param aInfo Pointer to struct containing information necessary for the external flash erase instruction
 *
 * @return Status
*/
static ca_error erase_helper(ExternalEraseType aT, ExtFlashEraseInfo *aInfo)
{
	ca_error                      status = CA_ERROR_SUCCESS;
	ExternalFlashPartialEraseType eraseType;

	if (aT == ERASE_CHIP)
	{
		status = BSP_ExternalFlashChipErase();
	}
	else
	{
		if (aT == ERASE_64KB)
			eraseType = BLOCK_64KB;
		else if (aT == ERASE_32KB)
			eraseType = BLOCK_32KB;
		else
			eraseType = SECTOR_4KB;

		//Erase the block or sector that startAddress is in.
		status = BSP_ExternalFlashPartialErase(eraseType, aInfo->startAddress);
	}

	if (status)
		return status;

	//Update the eraseInfo structure for the next erase
	update_erase_info(aT, aInfo);

	//Schedule the next instruction
	status = BSP_ExternalFlashScheduleCallback(&external_flash_erase_helper, aInfo);

	return status;
}

/**
 * Wrapper function that determines which type of erase needs to be performed
 * and calls the erase_helper with the appropriate arguments.
 *
 * @param aInfo Pointer to struct containing information necessary for the external flash erase instruction.
 *
 * @return Status
*/
static ca_error erase_and_reschedule(ExtFlashEraseInfo *aInfo)
{
	ca_error status = CA_ERROR_SUCCESS;

	if (should_erase_chip(aInfo->startAddress, aInfo->endAddress))
		status = erase_helper(ERASE_CHIP, aInfo);
	else if (should_erase_64kb(aInfo->startAddress, aInfo->endAddress))
		status = erase_helper(ERASE_64KB, aInfo);
	else if (should_erase_32kb(aInfo->startAddress, aInfo->endAddress))
		status = erase_helper(ERASE_32KB, aInfo);
	else
		status = erase_helper(ERASE_4KB, aInfo);

	return status;
}

/**
 * Function that calls the External Flash Program instruction based on
 * the arguments. It then schedules 'external_flash_write_helper' again
 * with updated information.
 *
 * @param aInfo             Pointer to struct containing information necessary for the external flash program instruction.
 * @param aExtFlashWriteLen The number of bytes that should be written to the external flash.
 *
 * @return Status
*/
static ca_error write_and_reschedule(ExtFlashWriteInfo *aInfo, u8_t aExtFlashWriteLen)
{
	ca_error status = CA_ERROR_SUCCESS;

	//Write
	status = BSP_ExternalFlashProgram(aInfo->startAddress, aExtFlashWriteLen, aInfo->data);
	if (status)
		return status;

	//Update info
	aInfo->startAddress += aExtFlashWriteLen;
	aInfo->data += aExtFlashWriteLen;
	aInfo->writeLength -= aExtFlashWriteLen;

	//Reschedule
	status = BSP_ExternalFlashScheduleCallback(&external_flash_write_helper, aInfo);

	return status;
}

/**
 * Function that calls the External Flash Read instruction based on
 * the arguments, and partially computes the crc32 of the data read.
 * It then schedules 'external_flash_check_helper' again
 * with updated information.
 *
 * @param aInfo            Pointer to struct containing information necessary for the external flash read instruction.
 * @param aExtFlashReadLen The number of bytes that should be read from the external flash.
 * @param aCrc             Output variable into which the result of the partial crc32 calculation is stored.
 *
 * @return Status
*/
static ca_error read_and_reschedule(ExtFlashCheckInfo *aInfo, u8_t aExtFlashReadLen, u32_t *aCrc)
{
	ca_error status = CA_ERROR_SUCCESS;
	//Maximum read limit for the external flash
	static uint8_t rxBuf[128];
	memset(rxBuf, 0xFF, sizeof(rxBuf));

	status = BSP_ExternalFlashReadData(aInfo->startAddress, aExtFlashReadLen, rxBuf);
	if (status)
		return status;

	//Compute the crc
	HASH_CRC32_stream(rxBuf, aExtFlashReadLen, aCrc);

	//Update info
	aInfo->startAddress += aExtFlashReadLen;
	aInfo->checklen -= aExtFlashReadLen;

	//Reschedule
	status = BSP_ExternalFlashScheduleCallback(&external_flash_check_helper, aInfo);

	return status;
}

ca_error external_flash_evbme_send_upstream(void *aContext)
{
	ca_error             cmd_status = *((ca_error *)aContext);
	struct EVBME_DFU_cmd dfuRsp;

	dfuRsp.mDfuSubCmdId              = DFU_STATUS;
	dfuRsp.mSubCmd.status_cmd.status = (uint8_t)cmd_status;
	MAC_Message(EVBME_DFU_CMD, 2, (u8_t *)&dfuRsp);

	return CA_ERROR_SUCCESS;
}

ca_error external_flash_erase_helper(void *aContext)
{
	ca_log_debg("external_flash_erase_helper()");
	ca_error           cmd_status = CA_ERROR_SUCCESS;
	ExtFlashEraseInfo *info       = ((ExtFlashEraseInfo *)aContext);

	if (info->eraseLength != 0)
	{
		cmd_status = erase_and_reschedule(info);

		//As long as eraseLength != 0, the erase procedure is still in progress.
		if (cmd_status == CA_ERROR_SUCCESS)
			cmd_status = CA_ERROR_BUSY;
	}

	//Send upstream response if the erase procedure either failed or succeeded.
	if (cmd_status != CA_ERROR_BUSY)
	{
		if (info->upstreamCallback)
		{
			upstream_status = cmd_status;
			info->upstreamCallback(&upstream_status);
		}

		if (cmd_status == CA_ERROR_SUCCESS && info->otaCallback)
			info->otaCallback(NULL);
	}

	return CA_ERROR_SUCCESS;
}

ca_error external_flash_write_helper(void *aContext)
{
	ca_error           cmd_status = CA_ERROR_SUCCESS;
	ExtFlashWriteInfo *info       = ((ExtFlashWriteInfo *)aContext);

	if (info->writeLength > 0)
	{
		//Determine how many bytes to write to the external flash
		uint32_t bytesLeftInPage  = info->pageSize - (info->startAddress % info->pageSize);
		uint8_t  extFlashWriteLen = MIN(MIN(info->writeLength, info->writeLimit), bytesLeftInPage);

		cmd_status = write_and_reschedule(info, extFlashWriteLen);

		//As long as writeLength > 0, the writing procedure is still in progress.
		if (cmd_status == CA_ERROR_SUCCESS)
			cmd_status = CA_ERROR_BUSY;
	}

	//Send upstream response if the erase procedure either failed or succeeded.
	if (cmd_status != CA_ERROR_BUSY)
	{
		if (info->upstreamCallback)
		{
			upstream_status = cmd_status;
			ca_log_debg("executing upstream callback()");
			info->upstreamCallback(&upstream_status);
		}

		if (cmd_status == CA_ERROR_SUCCESS && info->otaCallback)
		{
			ca_log_debg("executing ota callback()");
			info->otaCallback(NULL);
		}
	}

	return CA_ERROR_SUCCESS;
}

ca_error external_flash_check_helper(void *aContext)
{
	static uint32_t    crc        = 0xFFFFFFFF;
	ca_error           cmd_status = CA_ERROR_SUCCESS;
	ExtFlashCheckInfo *info       = ((ExtFlashCheckInfo *)aContext);

	if (info->checklen > PAGE_SIZE)
	{
		cmd_status = CA_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (info->checklen > 0)
	{
		uint8_t extFlashReadLen = MIN(info->checklen, info->readLimit);

		cmd_status = read_and_reschedule(info, extFlashReadLen, &crc);
		if (cmd_status == CA_ERROR_SUCCESS)
			cmd_status = CA_ERROR_BUSY;
	}
	else
	{
		if (~crc == info->checksum)
			cmd_status = CA_ERROR_SUCCESS;
		else
			cmd_status = CA_ERROR_FAIL;

		crc = 0xFFFFFFFF;
	}

exit:
	//Send upstream response if the erase procedure either failed or succeeded.
	if (cmd_status != CA_ERROR_BUSY)
	{
		if (info->upstreamCallback)
		{
			upstream_status = cmd_status;
			info->upstreamCallback(&upstream_status);
		}
	}

	return CA_ERROR_SUCCESS;
}
