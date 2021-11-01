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

#include "cascoda-util/cascoda_flash.h"

/******************************************************************************/
/****** Configuration Define                                             ******/
/******************************************************************************/
#define FLASH_BUFSIZE 4

ca_error utilsFlashInit(struct ca821x_dev *instance, const char *aApplicationName, uint32_t aNodeId)
{
	(void)instance;
	(void)aApplicationName;
	(void)aNodeId;
	return CA_ERROR_SUCCESS;
}

uint32_t utilsFlashGetSize(struct ca821x_dev *instance)
{
	(void)instance;
	struct ca_flash_info flash_info;
	BSP_GetFlashInfo(&flash_info);
	return flash_info.numPages * flash_info.pageSize;
}

ca_error utilsFlashErasePage(struct ca821x_dev *instance, uint32_t aAddress)
{
	(void)instance;
	struct ca_flash_info flash_info;

	BSP_GetFlashInfo(&flash_info);
	return BSP_FlashErase(flash_info.dataFlashBaseAddr + aAddress);
}

ca_error utilsFlashStatusWait(struct ca821x_dev *instance, uint32_t aTimeout)
{
	(void)instance;
	(void)aTimeout;
	return CA_ERROR_SUCCESS;
}

uint32_t utilsFlashWrite(struct ca821x_dev *instance, uint32_t aAddress, const uint8_t *aData, uint32_t aSize)
{
	(void)instance;
	uint32_t             buffer[FLASH_BUFSIZE];
	uint32_t             sizeLeft = aSize;
	struct ca_flash_info flash_info;

	BSP_GetFlashInfo(&flash_info);
	aAddress += flash_info.dataFlashBaseAddr;

	//TODO: Investigate whether this dance is necessary
	while (sizeLeft)
	{
		//Calculate byte count to copy
		uint32_t byteLen = sizeLeft;
		uint32_t alignLen;
		if (byteLen > FLASH_BUFSIZE * 4)
			byteLen = FLASH_BUFSIZE * 4;

		//Copy into buffer
		memset(buffer, 0xFF, sizeof(buffer));
		memcpy(buffer, aData, byteLen);

		//Move to flash
		alignLen = ((byteLen + sizeof(uint32_t) - 1) / sizeof(uint32_t)) * sizeof(uint32_t);
		BSP_FlashWriteInitial(aAddress, buffer, alignLen);

		//Increment remainders
		sizeLeft -= byteLen;
		aAddress += byteLen;
		aData += byteLen;
	}

	return aSize;
}

uint32_t utilsFlashRead(struct ca821x_dev *instance, uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
	(void)instance;
	uint32_t             buffer[FLASH_BUFSIZE];
	uint32_t             sizeLeft = aSize;
	struct ca_flash_info flash_info;

	BSP_GetFlashInfo(&flash_info);
	aAddress += flash_info.dataFlashBaseAddr;

	while (sizeLeft)
	{
		//Calculate byte count to copy
		uint32_t byteLen = sizeLeft;
		if (byteLen > sizeof(buffer))
			byteLen = sizeof(buffer);

		//Move from flash
		memset(buffer, 0, sizeof(buffer));
		BSP_FlashRead(aAddress, buffer, (byteLen + 3) / 4);

		//Copy from buffer
		memcpy(aData, buffer, byteLen);

		//Increment remainders
		sizeLeft -= byteLen;
		aAddress += byteLen;
		aData += byteLen;
	}

	return aSize;
}
