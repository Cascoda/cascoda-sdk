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
 * MCU:    Nuvoton Nano120
 * MODULE: Chili 1 (1.2, 1.3)
 * Dataflash Functions
*/
/* System */
#include <stdio.h>
/* Platform */
#include "Nano100Series.h"
#include "fmc.h"
#include "sys.h"
/* Cascoda */
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili.h"

/******************************************************************************/
/****** Configuration Defines                                            ******/
/******************************************************************************/
#define DATA_FLASH_CONFIG0 0xFFFFFFFE /* mask for CONFIG0 register dataflash enable */
#ifndef CASCODA_CHILI_FLASH_PAGES
#define CASCODA_CHILI_FLASH_PAGES 2 /* Dataflash: Number of pages configured */
#endif

#define DATA_FLASH_BASE \
	(FMC_APROM_END - (CASCODA_CHILI_FLASH_PAGES * FMC_FLASH_PAGE_SIZE)) /* Dataflash: 122k - 123k (1k or 2 pages) */

const struct FlashInfo BSP_FlashInfo = {FMC_FLASH_PAGE_SIZE, CASCODA_CHILI_FLASH_PAGES};
struct FlashInfo       BSP_GetFlashInfo(void)
{
	return BSP_FlashInfo;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void CHILI_FlashInit()
{
	uint32_t cfg[2];
	int32_t  error;

	SYS_UnlockReg();
	FMC_Open();

	/* Erase and Write CONFIG0 and CONFIG1 (DFBADR) (Read-Modify-Write for CONFIG0) */
	cfg[0] = FMC_Read(FMC_CONFIG_BASE);
	cfg[0] &= DATA_FLASH_CONFIG0;
	cfg[1] = DATA_FLASH_BASE;
	error  = FMC_WriteConfig(cfg, 2);
	if (error)
		ca_log_crit("FMC_WriteConfig Error.");

	FMC_Close();
	SYS_LockReg();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
static u8_t isValidFlashAddr(u32_t flashaddr)
{
	/* Check word alignment and range */
	if ((flashaddr % 4) || (flashaddr >= CASCODA_CHILI_FLASH_PAGES * FMC_FLASH_PAGE_SIZE))
		return 0;

	return 1;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_WriteDataFlashInitial(u32_t startaddr, u32_t *data, u32_t datasize)
{
	uint32_t addr;

	if (!isValidFlashAddr(startaddr))
	{
		ca_log_crit("BSP_WriteDataFlashInitial: Start Address invalid");
		return;
	}

	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();

	/* Write Dataflash */
	addr = startaddr;
	for (uint32_t i = 0; i < datasize; ++i)
	{
		uint32_t u32data = data[i];
		FMC_Write(DATA_FLASH_BASE + addr, u32data);

		addr += 4;
	}

	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_EraseDataFlashPage(u32_t startaddr)
{
	int32_t  error;
	uint32_t page;

	page = startaddr / FMC_FLASH_PAGE_SIZE;
	if (page >= CASCODA_CHILI_FLASH_PAGES)
	{
		ca_log_crit("BSP_EraseDataFlashPage address out of range");
		return;
	}

	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();

	/* Align address to start of page and then erase whole page */
	error = FMC_Erase(DATA_FLASH_BASE + (FMC_FLASH_PAGE_SIZE * page));

	if (error)
		ca_log_crit("FMC_Erase Dataflash Error.");

	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_ReadDataFlash(u32_t startaddr, u32_t *data, u32_t datasize)
{
	uint32_t addr;

	if (!isValidFlashAddr(startaddr))
	{
		ca_log_crit("BSP_WriteDataFlashReprog: Start Address invalid");
		return;
	}

	SYS_UnlockReg();
	FMC_Open();

	/* Loop to read the data into the buffer */
	addr = startaddr;
	for (uint32_t i = 0; i < datasize; ++i)
	{
		data[i] = FMC_Read(DATA_FLASH_BASE + addr);
		addr += 4;
	}

	FMC_Close();
	SYS_LockReg();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_ClearDataFlash(void)
{
	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();

	for (int32_t i = 0; i < CASCODA_CHILI_FLASH_PAGES; i++)
	{
		int32_t error = FMC_Erase(DATA_FLASH_BASE + (FMC_FLASH_PAGE_SIZE * i));

		if (error)
			ca_log_crit("FMC_Erase Dataflash Page %d Error.", i);
	}

	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();
}
