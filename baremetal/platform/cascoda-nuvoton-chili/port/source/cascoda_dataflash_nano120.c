/*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
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
		printf("FMC_WriteConfig Error.\n");

	FMC_Close();
	SYS_LockReg();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
static u8_t isValidFlashAddr(u32_t flashaddr)
{
	/* Check word alignment and range */
	if ((flashaddr % 4) || (flashaddr >= CASCODA_CHILI_FLASH_PAGES * FMC_FLASH_PAGE_SIZE))
		return 0;

	return 1;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_WriteDataFlashInitial(u32_t startaddr, u32_t *data, u32_t datasize)
{
	uint32_t addr;

	if (!isValidFlashAddr(startaddr))
	{
		printf("BSP_WriteDataFlashInitial: Start Address invalid\n");
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_EraseDataFlashPage(u32_t startaddr)
{
	int32_t  error;
	uint32_t page;

	page = startaddr / FMC_FLASH_PAGE_SIZE;
	if (page >= CASCODA_CHILI_FLASH_PAGES)
	{
		printf("BSP_EraseDataFlashPage address out of range\n");
		return;
	}

	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();

	/* Align address to start of page and then erase whole page */
	error = FMC_Erase(DATA_FLASH_BASE + (FMC_FLASH_PAGE_SIZE * page));

	if (error)
		printf("FMC_Erase Dataflash Error.\n");

	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_ReadDataFlash(u32_t startaddr, u32_t *data, u32_t datasize)
{
	uint32_t addr;

	if (!isValidFlashAddr(startaddr))
	{
		printf("BSP_WriteDataFlashReprog: Start Address invalid\n");
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_ClearDataFlash(void)
{
	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();

	for (int32_t i = 0; i < CASCODA_CHILI_FLASH_PAGES; i++)
	{
		int32_t error = FMC_Erase(DATA_FLASH_BASE + (FMC_FLASH_PAGE_SIZE * i));

		if (error)
			printf("FMC_Erase Dataflash Page %d Error.\n", i);
	}

	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();
}
