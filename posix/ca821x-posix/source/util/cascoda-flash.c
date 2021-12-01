/*
 *  Copyright (c) 2016, Nest Labs, Inc.
 *  Modifications copyright (c) 2020, Cascoda Ltd.
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix-settings.h"
#include "ca821x-posix/ca821x-types.h"
#include "cascoda-util/cascoda_flash.h"

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

// filename for openthread flash storage
// static const char flashFileName[] = "otConfig";

uint32_t sEraseAddress;

enum
{
	FLASH_SIZE      = 0x40000,
	FLASH_PAGE_SIZE = 0x800,
	FLASH_PAGE_NUM  = 128,
};

/**
 * Find the best place to place the data files, and initialise the directory if it doesn't
 * already exist.
 */
#ifdef _WIN32
char *posixGetDataDir(uint32_t aNodeId)
{
	const char *ddp         = NULL;
	char *      dataPath    = NULL;
	size_t      dataPathLen = 0;
	struct stat st;

	memset(&st, 0, sizeof(st));

	if ((ddp = getenv("APPDATA")))
	{
		size_t     homelen  = strlen(ddp);
		const char subdir[] = "/Cascoda/storage/";

		dataPathLen = homelen + sizeof(subdir) + 4;
		//This allocation only occurs once, and is used until program end, so we don't need to bother free-ing
		dataPath = malloc(dataPathLen);
		snprintf(dataPath, dataPathLen, "%s%s%04d", ddp, subdir, aNodeId);
	}
	else
	{
		ca_log_crit("Failed to get APPDATA environment variable.");
		abort();
	}

	if (stat(dataPath, &st) == -1)
	{
		//Recursively create directories as necessary.
		for (char *p = dataPath + 1; *p; p++)
		{
			if (*p == '/' || *p == '\\')
			{
				*p = '\0';
				mkdir(dataPath);
				*p = '/';
			}
		}
		mkdir(dataPath);
	}

	return dataPath;
}
#else
char *posixGetDataDir(uint32_t aNodeId)
{
	const char *ddp         = NULL;
	char *      dataPath    = NULL;
	size_t      dataPathLen = 0;
	struct stat st;

	memset(&st, 0, sizeof(st));

	if ((ddp = getenv("XDG_DATA_HOME")))
	{
		size_t     homelen  = strlen(ddp);
		const char subdir[] = "/cascoda/ot";

		dataPathLen = homelen + sizeof(subdir) + 4;
		//This allocation only occurs once, and is used until program end, so we don't need to bother free-ing
		dataPath = malloc(dataPathLen);
		snprintf(dataPath, dataPathLen, "%s%s%04d", ddp, subdir, aNodeId);
	}
	else if ((ddp = getenv("HOME")))
	{
		size_t     homelen  = strlen(ddp);
		const char subdir[] = "/.local/share/cascoda/ot";

		dataPathLen = homelen + sizeof(subdir) + 4;
		//This allocation only occurs once, and is used until program end, so we don't need to bother free-ing
		dataPath = malloc(dataPathLen);
		snprintf(dataPath, dataPathLen, "%s%s%04d", ddp, subdir, aNodeId);
	}
	else
	{
		//Fall back to system files
		ddp         = "/usr/local/etc/cascoda/ot";
		dataPathLen = strlen(ddp) + 5;
		//This allocation only occurs once, and is used until program end, so we don't need to bother free-ing
		dataPath = malloc(dataPathLen);
		snprintf(dataPath, dataPathLen, "%s%04d", ddp, aNodeId);
	}

	if (stat(dataPath, &st) == -1)
	{
		//Recursively create directories as necessary.
		for (char *p = dataPath + 1; *p; p++)
		{
			if (*p == '/')
			{
				*p = '\0';
				mkdir(dataPath, 0700);
				*p = '/';
			}
		}
		mkdir(dataPath, 0700);
	}

	return dataPath;
}
#endif

ca_error utilsFlashInit(struct ca821x_dev *aInstance, const char *aApplicationName, uint32_t aNodeId)
{
	ca_error error       = CA_ERROR_SUCCESS;
	bool     create      = false;
	char *   dataDir     = posixGetDataDir(aNodeId);
	size_t   fileNameLen = strlen(aApplicationName) + strlen(dataDir) + 2; //"datadir/filename"
	char     fileName[fileNameLen];

	snprintf(fileName, fileNameLen, "%s/%s", dataDir, aApplicationName);

	if (access(fileName, 0))
	{
		create = true;
	}

	int flash_fd = ((struct ca821x_exchange_base *)aInstance->exchange_context)->flash_fd =
	    open(fileName, O_RDWR | O_CREAT, 0666);
	lseek(flash_fd, 0, SEEK_SET);

	otEXPECT_ACTION(flash_fd >= 0, error = CA_ERROR_FAIL);

	if (create)
	{
		for (uint16_t index = 0; index < (uint16_t)FLASH_PAGE_NUM; index++)
		{
			otEXPECT(error = utilsFlashErasePage(aInstance, index * FLASH_PAGE_SIZE));
		}
	}

exit:
	free(dataDir);
	return error;
}

ca_error utilsFlashDeinit(struct ca821x_dev *aInstance)
{
	int error = close(((struct ca821x_exchange_base *)aInstance->exchange_context)->flash_fd);
	if (error)
		ca_log_crit("Failed to close flash file descriptor! Error %d", error);
	return error ? CA_ERROR_FAIL : CA_ERROR_SUCCESS;
}

uint32_t utilsFlashGetSize(struct ca821x_dev *instance)
{
	(void)instance;
	return FLASH_SIZE;
}

ca_error utilsFlashErasePage(struct ca821x_dev *aInstance, uint32_t aAddress)
{
	ca_error error = CA_ERROR_SUCCESS;
	//TODO: Improve this quick fix for slow startup due to looping single character file writes
	static uint8_t buf[FLASH_PAGE_SIZE] = {0};
	uint32_t       address;
	int            flash_fd = ((struct ca821x_exchange_base *)aInstance->exchange_context)->flash_fd;

	//TODO: improve Quick-fix
	if (buf[0] == 0)
	{
		memset(buf, 0xFF, FLASH_PAGE_SIZE);
	}

	otEXPECT_ACTION(flash_fd >= 0, error = CA_ERROR_FAIL);
	otEXPECT_ACTION(aAddress < FLASH_SIZE, error = CA_ERROR_INVALID_ARGS);

	// Get start address of the flash page that includes aAddress
	address = aAddress & (~(uint32_t)(FLASH_PAGE_SIZE - 1));

	/*
    for (uint16_t offset = 0; offset < FLASH_PAGE_SIZE; offset++)
    {
        otEXPECT_ACTION(pwrite(sFlashFd, &buf, 1, address + offset) == 1, error = kThreadError_Failed);
    }
    */
	lseek(flash_fd, address, SEEK_SET);
	otEXPECT_ACTION(write(flash_fd, buf, FLASH_PAGE_SIZE) == FLASH_PAGE_SIZE, error = CA_ERROR_FAIL);

exit:
	return error;
}

ca_error utilsFlashStatusWait(struct ca821x_dev *instance, uint32_t aTimeout)
{
	(void)instance;
	(void)aTimeout;
	return CA_ERROR_SUCCESS;
}

uint32_t utilsFlashWrite(struct ca821x_dev *aInstance, uint32_t aAddress, const uint8_t *aData, uint32_t aSize)
{
	uint32_t ret   = 0;
	uint32_t index = 0;
	uint8_t  byte;
	int      flash_fd = ((struct ca821x_exchange_base *)aInstance->exchange_context)->flash_fd;

	otEXPECT_ACTION(flash_fd >= 0 && aAddress < FLASH_SIZE, ;);

	for (index = 0; index < aSize; index++)
	{
		otEXPECT_ACTION((ret = utilsFlashRead(aInstance, aAddress + index, &byte, 1)) == 1, ;);
		// Use bitwise AND to emulate the behavior of flash memory
		byte &= aData[index];
		lseek(flash_fd, aAddress + index, SEEK_SET);
		otEXPECT_ACTION(write(flash_fd, &byte, 1) == 1, ;);
	}

exit:
	return index;
}

uint32_t utilsFlashRead(struct ca821x_dev *aInstance, uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
	uint32_t ret      = 0;
	int      flash_fd = ((struct ca821x_exchange_base *)aInstance->exchange_context)->flash_fd;

	otEXPECT_ACTION(flash_fd >= 0 && aAddress < FLASH_SIZE, ;);
	lseek(flash_fd, aAddress, SEEK_SET);
	ret = (uint32_t)read(flash_fd, aData, aSize);

exit:
	return ret;
}

void BSP_GetFlashInfo(struct ca_flash_info *aFlashInfoOut)
{
	aFlashInfoOut->dataFlashBaseAddr = 0;
	aFlashInfoOut->numPages          = FLASH_PAGE_NUM;
	aFlashInfoOut->pageSize          = FLASH_PAGE_SIZE;
}

uint32_t utilsFlashGetBaseAddress(struct ca821x_dev *aInstance)
{
	return ((struct ca821x_exchange_base *)aInstance->exchange_context)->base_address;
}

uint32_t utilsFlashGetUsedSize(struct ca821x_dev *aInstance)
{
	return ((struct ca821x_exchange_base *)aInstance->exchange_context)->used_size;
}

void utilsFlashSetBaseAddress(struct ca821x_dev *aInstance, uint32_t aAddress)
{
	((struct ca821x_exchange_base *)aInstance->exchange_context)->base_address = aAddress;
}

void utilsFlashSetUsedSize(struct ca821x_dev *aInstance, uint32_t aSize)
{
	((struct ca821x_exchange_base *)aInstance->exchange_context)->used_size = aSize;
}