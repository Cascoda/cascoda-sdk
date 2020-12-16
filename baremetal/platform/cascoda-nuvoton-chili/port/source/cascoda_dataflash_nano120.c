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
#include <string.h>
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

const struct FlashInfo BSP_FlashInfo = {DATA_FLASH_BASE, FMC_FLASH_PAGE_SIZE, CASCODA_CHILI_FLASH_PAGES};
void                   BSP_GetFlashInfo(struct FlashInfo *aFlashInfoOut)
{
	memcpy(aFlashInfoOut, &BSP_FlashInfo, sizeof(BSP_FlashInfo));
}

/*
 * CRC32 algorithm taken from the zlib source, which is
 * Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
 */
static const unsigned int tinf_crc32tab[16] = {0x00000000,
                                               0x1db71064,
                                               0x3b6e20c8,
                                               0x26d930ac,
                                               0x76dc4190,
                                               0x6b6b51f4,
                                               0x4db26158,
                                               0x5005713c,
                                               0xedb88320,
                                               0xf00f9344,
                                               0xd6d6a3e8,
                                               0xcb61b38c,
                                               0x9b64c2b0,
                                               0x86d3d2d4,
                                               0xa00ae278,
                                               0xbdbdf21c};

/* crc is previous value for incremental computation, 0xffffffff initially */
static uint32_t uzlib_crc32(const void *data, unsigned int length, uint32_t crc)
{
	const unsigned char *buf = (const unsigned char *)data;
	unsigned int         i;

	for (i = 0; i < length; ++i)
	{
		crc ^= buf[i];
		crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
		crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
	}

	// return value suitable for passing in next time, for final value invert it
	return crc /* ^ 0xffffffff*/;
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
	if ((flashaddr % 4))
		return 0;

	if (flashaddr < DATA_FLASH_BASE)
		return 0;

	if (flashaddr >= FMC_APROM_END)
		return 0;

	ca_log_crit("Flash Address invalid");

	return 1;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_FlashWriteInitial(u32_t startaddr, void *data, u32_t datasize)
{
	if (!isValidFlashAddr(startaddr))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();

	/* Write Dataflash */
	for (uint32_t i = 0; i < datasize; i += sizeof(uint32_t))
	{
		uint32_t u32data;
		memcpy(&u32data, data + i, sizeof(u32data));
		FMC_Write(startaddr + i, u32data);
	}

	FMC_DISABLE_AP_UPDATE();

	FMC_Close();
	SYS_LockReg();

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_FlashErase(u32_t startaddr)
{
	int32_t  error;
	uint32_t page;

	page = startaddr / FMC_FLASH_PAGE_SIZE;
	if (isValidFlashAddr(startaddr))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	SYS_UnlockReg();
	FMC_Open();

	FMC_ENABLE_AP_UPDATE();

	/* Align address to start of page and then erase whole page */
	error = FMC_Erase(FMC_FLASH_PAGE_SIZE * page);

	if (error)
		ca_log_crit("FMC_Erase Dataflash Error.");

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
ca_error BSP_FlashRead(u32_t startaddr, u32_t *data, u32_t datasize)
{
	uint32_t addr;

	if (!isValidFlashAddr(startaddr))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	SYS_UnlockReg();
	FMC_Open();

	/* Loop to read the data into the buffer */
	addr = startaddr;
	for (uint32_t i = 0; i < datasize; ++i)
	{
		data[i] = FMC_Read(addr);
		addr += 4;
	}

	FMC_Close();
	SYS_LockReg();
	return CA_ERROR_SUCCESS;
}

ca_error BSP_FlashCheck(u32_t startaddr, u32_t checklen, u32_t crc32)
{
	uint32_t crc_flash = 0xFFFFFFFF;

	if (!isValidFlashAddr(startaddr))
	{
		return CA_ERROR_INVALID_ARGS;
	}

	SYS_UnlockReg();
	FMC_Open();
	/* Loop to read the data into the buffer */
	for (uint32_t i = 0; i < checklen; ++i)
	{
		uint32_t data = FMC_Read(startaddr);
		crc_flash     = uzlib_crc32(&data, sizeof(data), crc_flash);
		startaddr += sizeof(data);
	}
	FMC_Close();
	SYS_LockReg();

	crc_flash = ~crc_flash;

	if (crc_flash != crc32)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}
