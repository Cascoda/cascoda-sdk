/*
 *  Copyright (c) 2020, Cascoda Ltd.
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

#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#include "openthread/cli.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/ncp.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "lwip/timeouts.h"
#include "lwipdemo.h"

/******************************************************************************/
/****** Application name                                                 ******/
/******************************************************************************/
#define APP_NAME "OT_LWIP"

otInstance *OT_INSTANCE;

static otCliCommand sCliCommand;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handle application specific commands.
 *******************************************************************************
 ******************************************************************************/
static int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret = 0;

	if (buf[0] == OT_SERIAL_DOWNLINK)
	{
		PlatformUartReceive(buf + 2, buf[1]);
		ret = 1;
	}

	// switch clock otherwise chip is locking up as it loses external clock
	if (((buf[0] == EVBME_SET_REQUEST) && (buf[2] == EVBME_RESETRF)) || (buf[0] == EVBME_HOST_CONNECTED))
	{
		EVBME_SwitchClock(pDeviceRef, 0);
	}
	return ret;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Main endless loop.
 *******************************************************************************
 ******************************************************************************/
int main(void)
{
	u8_t              StartupStatus;
	struct ca821x_dev dev;
	ca821x_api_init(&dev);
	cascoda_serial_dispatch = ot_serial_dispatch;

	// Initialisation
	StartupStatus = EVBMEInitialise(APP_NAME, &dev);
	PlatformRadioInitWithDev(&dev);
	OT_INSTANCE = otInstanceInitSingle();

	otCliUartInit(OT_INSTANCE);

	sCliCommand.mCommand = handle_cli_lwipdemo;
	sCliCommand.mName    = "lwip";
	otCliSetUserCommands(&sCliCommand, 1);

	init_lwipdemo(OT_INSTANCE, &dev);

	// Endless Polling Loop
	while (1)
	{
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
		sys_check_timeouts(); //TODO: This could probably be merged with TASKLETs
	}
}
