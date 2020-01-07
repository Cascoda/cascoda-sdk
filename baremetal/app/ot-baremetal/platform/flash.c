/*
 *  Copyright (c) 2017, Cascoda Ltd.
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
 *   This file implements the flash interface from flash.h.
 *   for a Nuvoton Nano120 device
 */

#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"

#include "flash.h"

/******************************************************************************/
/****** Configuration Define                                             ******/
/******************************************************************************/
#define FLASH_BUFSIZE 4

/**
 * Perform any initialization for flash driver.
 *
 * @retval OT_ERROR_NONE    Initialize flash driver success.
 * @retval OT_ERROR_FAILED  Initialize flash driver fail.
 */
otError utilsFlashInit(void)
{
	return OT_ERROR_NONE;
}

/**
 * Get the size of flash that can be read/write by the caller.
 * The usable flash size is always the multiple of flash page size.
 *
 * @returns The size of the flash.
 */
uint32_t utilsFlashGetSize(void)
{
	struct FlashInfo flash_info = BSP_GetFlashInfo();
	return flash_info.numPages * flash_info.pageSize;
}

/**
 * Erase one flash page that include the input address.
 * This is a non-blocking function. It can work with utilsFlashStatusWait to check when erase is done.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for erasing.
 * 0 is always mapped to the beginning of one flash page.
 * The input address should never be mapped to the firmware space or any other protected flash space.
 *
 * @param[in]  aAddress  The start address of the flash to erase.
 *
 * @retval OT_ERROR_NONE           Erase flash operation is started.
 * @retval OT_ERROR_FAILED         Erase flash operation is not started.
 * @retval OT_ERROR_INVALID_ARGS    aAddress is out of range of flash or not aligned.
 */
otError utilsFlashErasePage(uint32_t aAddress)
{
	BSP_EraseDataFlashPage(aAddress);
	return OT_ERROR_NONE;
}

/**
  * Check whether flash is ready or busy.
  *
  * @param[in]  aTimeout  The interval in milliseconds waiting for the flash operation to be done and become ready again.
  *                       zero indicates that it is a polling function, and returns current status of flash immediately.
  *                       non-zero indicates that it is blocking there until the operation is done and become ready, or timeout expires.
  *
  * @retval OT_ERROR_NONE           Flash is ready for any operation.
  * @retval OT_ERROR_BUSY           Flash is busy.
  */
otError utilsFlashStatusWait(uint32_t aTimeout)
{
	return OT_ERROR_NONE;
}

/**
 * Write flash. The write operation only clears bits, but never set bits.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for writing.
 * 0 is always mapped to the beginning of one flash page.
 * The input address should never be mapped to the firmware space or any other protected flash space.
 *
 * @param[in]  aAddress  The start address of the flash to write.
 * @param[in]  aData     The pointer of the data to write.
 * @param[in]  aSize     The size of the data to write.
 *
 * @returns The actual size of octets write to flash.
 *          It is expected the same as aSize.
 *          0 indicates that something wrong happens when writing.
 */
uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
	uint32_t buffer[FLASH_BUFSIZE];
	uint32_t sizeLeft = aSize;

	while (sizeLeft)
	{
		//Calculate byte count to copy
		uint32_t byteLen = sizeLeft;
		if (byteLen > FLASH_BUFSIZE * 4)
			byteLen = FLASH_BUFSIZE * 4;

		//Copy into buffer
		memset(buffer, 0, FLASH_BUFSIZE * 4);
		memcpy(buffer, aData, byteLen);

		//Move to flash
		BSP_WriteDataFlashInitial(aAddress, buffer, (byteLen + 3) / 4);

		//Increment remainders
		sizeLeft -= byteLen;
		aAddress += byteLen;
		aData += byteLen;
	}

	return aSize;
}

/**
 * Read flash.
 *
 * The flash address starts from 0, and this function maps the input address to the physical address of flash for reading.
 * 0 is always mapped to the beginning of one flash page.
 * The input address should never be mapped to the firmware space or any other protected flash space.
 *
 * @param[in]   aAddress  The start address of the flash to read.
 * @param[out]  aData     The pointer of buffer for reading.
 * @param[in]   aSize     The size of the data to read.
 *
 * @returns The actual size of octets read to buffer.
 *          It is expected the same as aSize.
 *          0 indicates that something wrong happens when reading.
 */
uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
	uint32_t buffer[FLASH_BUFSIZE];
	uint32_t sizeLeft = aSize;

	while (sizeLeft)
	{
		//Calculate byte count to copy
		uint32_t byteLen = sizeLeft;
		if (byteLen > sizeof(buffer))
			byteLen = sizeof(buffer);

		//Move from flash
		memset(buffer, 0, sizeof(buffer));
		BSP_ReadDataFlash(aAddress, buffer, (byteLen + 3) / 4);

		//Copy from buffer
		memcpy(aData, buffer, byteLen);

		//Increment remainders
		sizeLeft -= byteLen;
		aAddress += byteLen;
		aData += byteLen;
	}

	return aSize;
}
