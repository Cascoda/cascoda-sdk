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

#include "actuatordemo.h"

/******************************************************************************/
/****** Application name                                                 ******/
/******************************************************************************/
#define APP_NAME "OT_ACTDEMO"

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
	if (((buf[0] == EVBME_SET_REQUEST) && (buf[2] == EVBME_RESETRF)) || (buf[0] == EVBME_GUI_CONNECTED))
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

	/* Setup Thread stack with hard coded demo parameters */
	otLinkModeConfig linkMode = {true, true, true, true};
	otMasterKey key = {0xca, 0x5c, 0x0d, 0xa5, 0x01, 0x07, 0xca, 0x5c, 0x0d, 0xaa, 0xca, 0xfe, 0xbe, 0xef, 0xde, 0xad};
	otIp6SetEnabled(OT_INSTANCE, true);
	otLinkSetPanId(OT_INSTANCE, 0xc0da);
	otThreadSetLinkMode(OT_INSTANCE, linkMode);
	otThreadSetMasterKey(OT_INSTANCE, &key);
	otLinkSetChannel(OT_INSTANCE, 22);
	otThreadSetEnabled(OT_INSTANCE, true);

	otCliUartInit(OT_INSTANCE);

	sCliCommand.mCommand = handle_cli_actuatordemo;
	sCliCommand.mName    = "actuatordemo";
	otCliSetUserCommands(&sCliCommand, 1);

	init_actuatordemo(OT_INSTANCE, &dev);

	// Endless Polling Loop
	while (1)
	{
		handle_actuatordemo(&dev);
		cascoda_io_handler(&dev);
		PlatformAlarmProcess(OT_INSTANCE);
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
	}
}
