/*
 *  Copyright (c) 2016, Nest Labs, Inc.
 *  Modifications copyright (c) 2019, Cascoda Ltd.
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "openthread/instance.h"
#include "openthread/ip6.h"
#include "openthread/ncp.h"
#include "openthread/tasklet.h"

#include "ca821x-posix-thread/posix-platform.h"

static int isRunning;

static void quit(int sig)
{
	isRunning = 0;
}

int main(int argc, char *argv[])
{
	otInstance *OT_INSTANCE;

	if (argc > 1)
		NODE_ID = atoi(argv[1]);

	posixPlatformSetOrigArgs(argc, argv);
	while (posixPlatformInit() < 0) sleep(1);
	OT_INSTANCE = otInstanceInitSingle();
	otNcpInit(OT_INSTANCE);

	isRunning = 1;
	signal(SIGINT, quit);

	//Set Multicast promiscuous mode, and leave filtering up to linux netstack
	otIp6SetMulticastPromiscuousEnabled(OT_INSTANCE, true);

	while (isRunning)
	{
		otTaskletsProcess(OT_INSTANCE);
		posixPlatformProcessDrivers(OT_INSTANCE);
	}

	return 0;
}
