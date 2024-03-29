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

#include <unistd.h>

#include "ca821x-posix-thread/posix-platform.h"
#include "openthread/platform/misc.h"

extern int    gArgumentsCount;
extern char **gArguments;

void otPlatReset(otInstance *aInstance)
{
	char *argv[gArgumentsCount + 1];

	for (int i = 0; i < gArgumentsCount; ++i)
	{
		argv[i] = gArguments[i];
	}

	argv[gArgumentsCount] = NULL;

	PlatformRadioStop();
	posixPlatformRestoreTerminal();

	execvp(argv[0], argv);
	perror("reset failed");
	exit(EXIT_FAILURE);
	(void)aInstance;
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
	return OT_PLAT_RESET_REASON_POWER_ON;
}

void otPlatWakeHost(void)
{
	//This is the host - not applicable to posix & hardmac systems
}

uint32_t otPlatRadioGetSupportedChannelMask(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	return 0xffff << 11;
}

uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
	return otPlatRadioGetSupportedChannelMask(aInstance);
}
