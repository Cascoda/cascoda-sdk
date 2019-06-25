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

otInstance *OT_INSTANCE;

/**
 * Handle application specific commands.
 */
int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
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
void NCP_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	/* register LED_G */
	BSP_ModuleRegisterGPIOOutput(BSP_ModuleSpecialPins.LED_GREEN, MODULE_PIN_TYPE_LED);
	/* register LED_R */
	BSP_ModuleRegisterGPIOOutput(BSP_ModuleSpecialPins.LED_RED, MODULE_PIN_TYPE_LED);

	if (status == CA_ERROR_FAIL)
	{
		BSP_ModuleSetGPIOPin(BSP_ModuleSpecialPins.LED_RED, LED_ON);
		return;
	}

	BSP_ModuleSetGPIOPin(BSP_ModuleSpecialPins.LED_RED, LED_OFF);
	BSP_ModuleSetGPIOPin(BSP_ModuleSpecialPins.LED_GREEN, LED_ON);

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
#else
#error "Build system error, neither OT_NCP or OT_CLI defined."
#endif

	// Endless Polling Loop
	while (1)
	{
		cascoda_io_handler(&dev);
		PlatformAlarmProcess(OT_INSTANCE);
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
	}
}
