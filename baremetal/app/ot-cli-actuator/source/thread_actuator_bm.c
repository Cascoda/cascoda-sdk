#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "ca821x_api.h"

#include "openthread/cli.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/ncp.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "actuatordemo.h"

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
	StartupStatus = EVBMEInitialise(CA_TARGET_NAME, &dev);
	PlatformRadioInitWithDev(&dev);
	OT_INSTANCE = otInstanceInitSingle();

	otAppCliInit(OT_INSTANCE);

	sCliCommand.mCommand = &handle_cli_actuatordemo;
	sCliCommand.mName    = "actuatordemo";
	otCliSetUserCommands(&sCliCommand, 1, OT_INSTANCE);

	init_actuatordemo(OT_INSTANCE, &dev);

	// Endless Polling Loop
	while (1)
	{
		handle_actuatordemo(&dev);
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
	}
}
