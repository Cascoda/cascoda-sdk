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
/*
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * Dataflash Functions
*/
/* System */
#include <stdio.h>
#include <string.h>

/* Platform */
#include "M2351.h"
#include "fmc.h"
#include "sys.h"
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili.h"

/******************************************************************************/
/****** Configuration Defines                                            ******/
/******************************************************************************/
#ifndef CASCODA_CHILI_FLASH_PAGES
#define CASCODA_CHILI_FLASH_PAGES 2
#endif

#define DATA_FLASH_BASE \
	(FMC_APROM_END - (CASCODA_CHILI_FLASH_PAGES * FMC_FLASH_PAGE_SIZE)) /* Dataflash: on top of APROM */

const struct ca_flash_info BSP_FlashInfo = {DATA_FLASH_BASE, FMC_FLASH_PAGE_SIZE, CASCODA_CHILI_FLASH_PAGES};

__NONSECURE_ENTRY void BSP_GetFlashInfo(struct ca_flash_info *aFlashInfoOut)
{
	memcpy(aFlashInfoOut, &BSP_FlashInfo, sizeof(BSP_FlashInfo));
}

static u8_t isValidFlashAddr(u32_t flashaddr)
{
	u8_t rval = 1;
	/* Check word alignment and range */
	if ((flashaddr % 4))
		rval = 0;

	if (flashaddr < DATA_FLASH_BASE || flashaddr >= FMC_APROM_END)
		if (flashaddr < FMC_LDROM_BASE || flashaddr >= FMC_LDROM_END)
			rval = 0;

	if (!rval)
	{
		ca_log_crit("Invalid Flash Address 0x%x", flashaddr);
	}
	return rval;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY ca_error BSP_FlashWriteInitial(u32_t startaddr, void *data, u32_t datasize)
{
	ca_log_debg("BSP_FlashWriteInitial addr: 0x%x, len: %d", startaddr, datasize);
	if (!isValidFlashAddr(startaddr) || !isValidFlashAddr(startaddr + datasize))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();
	FMC_ENABLE_LD_UPDATE();

	/* Write Dataflash */
	for (uint32_t i = 0; i < datasize; i += sizeof(uint32_t))
	{
		uint32_t u32data;
		memcpy(&u32data, data + i, sizeof(u32data));
		FMC_Write(startaddr + i, u32data);
	}

	FMC_DISABLE_LD_UPDATE();
	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY ca_error BSP_FlashErase(u32_t startaddr)
{
	int32_t  error;
	uint32_t page;

	page = startaddr / FMC_FLASH_PAGE_SIZE;
	if (!isValidFlashAddr(startaddr))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();
	FMC_ENABLE_LD_UPDATE();

	/* Align address to start of page and then erase whole page */
	error = FMC_Erase(FMC_FLASH_PAGE_SIZE * page);

	if (error)
		ca_log_crit("FMC_Erase Dataflash Error.");

	FMC_DISABLE_LD_UPDATE();
	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();

	if (error)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY ca_error BSP_FlashRead(u32_t startaddr, u32_t *data, u32_t datasize)
{
	uint32_t *addr = (uint32_t *)(startaddr);

	if (!isValidFlashAddr(startaddr) || !isValidFlashAddr(startaddr + datasize * sizeof(uint32_t)))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	memcpy(data, addr, datasize * sizeof(uint32_t));
	return CA_ERROR_SUCCESS;
}

__NONSECURE_ENTRY ca_error BSP_FlashCheck(u32_t startaddr, u32_t checklen, u32_t crc32)
{
	uint32_t crc_flash;

	// Subtraction of single word is because that is the final word we are actually operating on.
	// Otherwise we fail when we try to check the final page.
	if (!isValidFlashAddr(startaddr) || !isValidFlashAddr(startaddr + checklen - sizeof(uint32_t)))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	SYS_UnlockReg();
	FMC_Open();
	crc_flash = FMC_GetChkSum(startaddr, checklen);
	FMC_Close();
	SYS_LockReg();

	if (crc_flash != crc32)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

__NONSECURE_ENTRY void CHILI_SetLDROMBoot(void)
{
	SYS_UnlockReg();
	FMC_Open();
	FMC_SetVectorPageAddr(FMC_LDROM_BASE);
	FMC->ISPCTL |= FMC_ISPCTL_BS_Msk;
	FMC_Close();
	SYS_LockReg();
}

__NONSECURE_ENTRY void CHILI_SetAPROMBoot(void)
{
	SYS_UnlockReg();
	FMC_Open();
	FMC_SetVectorPageAddr(FMC_APROM_BASE);
	FMC->ISPCTL |= FMC_ISPCTL_BS_Msk; //We set to LDROM boot mode so vector mapping takes effect.
	FMC_Close();
	SYS_LockReg();
}
