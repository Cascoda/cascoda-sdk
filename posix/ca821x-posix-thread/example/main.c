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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "openthread/cli.h"
#include "openthread/instance.h"
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
	isRunning = 1;
	signal(SIGINT, quit);

	posixPlatformSetOrigArgs(argc, argv);
	while (posixPlatformInit() < 0) sleep(1);
	OT_INSTANCE = otInstanceInitSingle();
	otCliUartInit(OT_INSTANCE);

	/* Test harness specific config */
#ifdef TESTHARNESS
	otSetNetworkName(OT_INSTANCE, "GRL");
	otLinkSetPanId(OT_INSTANCE, 0xface);
	uint8_t extPanId[] = {0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00};
	otSetExtendedPanId(OT_INSTANCE, extPanId);
	otSetChannel(OT_INSTANCE, 20);
#endif

	while (isRunning)
	{
		otTaskletsProcess(OT_INSTANCE);
		posixPlatformProcessDrivers(OT_INSTANCE);
		/* Simple application code can go here*/
	}

	/*
	 * The above program loop is simplistic, and doesn't allow for much control over
	 * the timing of the application code. Using the below model allows for more control.
	 * (The timeout value can be decreased by application code after GetTimeout is called
	 *  if necessary, but not increased)
	 *
	 *  There is a further example for multithreaded code in mainMultithreaded.c
	 *
	 * struct timeval timeout;
	 * while(1){
	 *     otTaskletsProcess(aInstance);
	 *     posixPlatformProcessDriversQuick(aInstance);
	 *
	 *     //Application code here!
	 *
	 *     posixPlatformGetTimeout(aInstance, &timeout);
	 *     posixPlatformSleep(aInstance, &timeout); //Must run very soon after posixPlatformProcessGetTimeout
	 * }
	 */

	return 0;
}
