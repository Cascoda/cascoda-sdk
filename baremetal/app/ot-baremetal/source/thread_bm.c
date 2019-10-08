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

#include "sensordemo.h"

otInstance *OT_INSTANCE;

static otCliCommand sCliCommand;

/**
 * Handle application specific commands.
 */
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

/**
 * Initialise LEDs and clock
 */
static void NCP_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();
	/* register LED_G */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_GREEN, MODULE_PIN_TYPE_LED);
	/* register LED_R */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_RED, MODULE_PIN_TYPE_LED);

	if (status == CA_ERROR_FAIL)
	{
		BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_ON);
		return;
	}

	BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_OFF);
	BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_ON);

	EVBME_SwitchClock(pDeviceRef, 1);
}

/**
 * Stub
 */
void otTaskletsSignalPending(otInstance *aInstance)
{
	(void)aInstance;
}

/**
 * Main endless loop
 */
int main(void)
{
	u8_t              StartupStatus;
	struct ca821x_dev dev;
	ca821x_api_init(&dev);
	cascoda_serial_dispatch = ot_serial_dispatch;

	// Initialisation
	StartupStatus = EVBMEInitialise(APP_NAME, &dev);
	NCP_Initialise(StartupStatus, &dev);
	PlatformRadioInitWithDev(&dev);
	OT_INSTANCE = otInstanceInitSingle();

#if OT_NCP
	otNcpInit(OT_INSTANCE);
	otIp6SetMulticastPromiscuousEnabled(OT_INSTANCE, true);
#elif OT_CLI
	otCliUartInit(OT_INSTANCE);

	sCliCommand.mCommand = handle_cli_sensordemo;
	sCliCommand.mName    = "sensordemo";
	otCliSetUserCommands(&sCliCommand, 1);

	init_sensordemo(OT_INSTANCE, &dev);
#else
#error "Build system error, neither OT_NCP or OT_CLI defined."
#endif

	// Endless Polling Loop
	while (1)
	{
#if OT_CLI
		handle_sensordemo(&dev);
#endif
		cascoda_io_handler(&dev);
		PlatformAlarmProcess(OT_INSTANCE);
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
	}
}
