/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file includes the platform-specific initializers.
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "ca821x-posix-thread/posix-platform.h"
#include "openthread/platform/alarm-milli.h"
#include "openthread/platform/uart.h"
#include "openthread/tasklet.h"
#include "selfpipe.h"

uint32_t NODE_ID           = 1;
uint32_t WELLKNOWN_NODE_ID = 34;

static fd_set      read_fds;
static fd_set      write_fds;
static int         max_fd  = -1;
static const char *dataDir = NULL;

int    gArgumentsCount = 0;
char **gArguments      = NULL;

void posixPlatformSetOrigArgs(int argc, char *argv[])
{
	gArgumentsCount = argc;
	gArguments      = argv;
}

int posixPlatformInit(void)
{
	posixPlatformAlarmInit();
	if (PlatformRadioInit() < 0)
	{
		return -1;
	}
	posixPlatformRandomInit();

	return 0;
}

void otTaskletsSignalPending(otInstance *aInstance)
{
	selfpipe_push();
}

void posixPlatformGetTimeout(otInstance *aInstance, struct timeval *timeout)
{
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	platformUartUpdateFdSet(&read_fds, &write_fds, &max_fd);
	selfpipe_UpdateFdSet(&read_fds, &write_fds, &max_fd);
	posixPlatformAlarmUpdateTimeout(timeout);
}

void posixPlatformSleep(otInstance *aInstance, struct timeval *timeout)
{
	int rval;

	if (!otTaskletsArePending(aInstance))
	{
		rval = select(max_fd + 1, &read_fds, &write_fds, NULL, timeout);
		selfpipe_pop();
	}
}

void posixPlatformProcessDriversQuick(otInstance *aInstance)
{
	platformUartProcess();
	PlatformRadioProcess();
	posixPlatformAlarmProcess(aInstance);
}

void posixPlatformProcessDrivers(otInstance *aInstance)
{
	struct timeval timeout;
	posixPlatformProcessDriversQuick(aInstance);
	posixPlatformGetTimeout(aInstance, &timeout);
	posixPlatformSleep(aInstance, &timeout);
}

/**
 * Find the best place to place the data files, and initialise the directory if it doesn't
 * already exist.
 */
static const char *getDataDirPath()
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
		snprintf(dataPath, dataPathLen, "%s%s%04d", ddp, subdir, NODE_ID);
	}
	else if ((ddp = getenv("HOME")))
	{
		size_t     homelen  = strlen(ddp);
		const char subdir[] = "/.local/share/cascoda/ot";

		dataPathLen = homelen + sizeof(subdir) + 4;
		//This allocation only occurs once, and is used until program end, so we don't need to bother free-ing
		dataPath = malloc(dataPathLen);
		snprintf(dataPath, dataPathLen, "%s%s%04d", ddp, subdir, NODE_ID);
	}
	else
	{
		//Fall back to system files
		ddp         = "/usr/local/etc/cascoda/ot";
		dataPathLen = strlen(ddp) + 5;
		//This allocation only occurs once, and is used until program end, so we don't need to bother free-ing
		dataPath = malloc(dataPathLen);
		snprintf(dataPath, dataPathLen, "%s%04d", ddp, NODE_ID);
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

const char *posixGetDataDir()
{
	if (!dataDir)
		dataDir = getDataDirPath();

	return dataDir;
}
