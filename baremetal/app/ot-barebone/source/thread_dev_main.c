/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2016, CA-821X Openthread Code                       ******/
/******************************************************************************/
/******************************************************************************/
/****** Application-Specific Main Program                                ******/
/******************************************************************************/
/******************************************************************************/
/****** Revision           Notes                                         ******/
/******************************************************************************/
/****** 1.0  31/10/16  PB  Release Baseline                              ******/
/******************************************************************************/
/******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "ca821x_api.h"
// Insert Application-Specific Includes here
#include "ot_api_headers.h"

#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

extern u8_t HandleMac;
otInstance *OT_INSTANCE;

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/

int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret;
	ret = otApi_Dispatch((struct SerialBuffer *)(buf));

	return ret;
}

int ot_reinitialise(struct ca821x_dev *pDeviceRef)
{
	otLinkSyncExternalMac(OT_INSTANCE);
}

void sleep_if_possible()
{
	if (PlatformCanSleep(OT_INSTANCE))
	{
		uint32_t idleTimeLeft = 600000;

		TASKLET_GetTimeToNext(&idleTimeLeft);

		if (idleTimeLeft > 100)
		{
			struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();
			BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_OFF);
			BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_OFF);
			PlatformSleep(idleTimeLeft);
			BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_ON);
		}
	}
}

/* TODO: Rename this? Or put something in BSP? */
/******************************************************************************/
/******************************************************************************/
/****** NANO120_Initialise()                                             ******/
/******************************************************************************/
/****** Brief:  NANO120 Initialisation routine                           ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: -                                                        ******/
/******************************************************************************/
/******************************************************************************/
void NANO120_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
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

	//NANO120_APP_SaveOrRestoreAddress();
} // End of NANO120_Initialise()

/******************************************************************************/
/******************************************************************************/
/****** NANO120_Handler()                                                ******/
/******************************************************************************/
/****** Brief:  NANO120 Event Handler in Main Polling Loop               ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: -                                                        ******/
/******************************************************************************/
/******************************************************************************/
void NANO120_Handler(struct ca821x_dev *pDeviceRef)
{
	i32_t t32val;

	if (HandleMac)
	{
		cascoda_io_handler(pDeviceRef);
		sleep_if_possible();
		otTaskletsProcess(OT_INSTANCE);
	}

} // End of NANO120_Handler()

void otTaskletsSignalPending(otInstance *aInstance)
{
	(void)aInstance;
}

/******************************************************************************/
/******************************************************************************/
/****** main()                                                           ******/
/******************************************************************************/
/****** Brief:  Main Program Endless Loop                                ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: Does not return                                          ******/
/******************************************************************************/
/******************************************************************************/
int main(void)
{
	u8_t              StartupStatus;
	struct ca821x_dev dev;
	ca821x_api_init(&dev);
	cascoda_serial_dispatch = ot_serial_dispatch;
	cascoda_reinitialise    = ot_reinitialise;

	// Initialisation of Chip and EVBME
	// Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action
	// in case there is no UpStream Communications Channel available
	StartupStatus = EVBMEInitialise(CA_TARGET_NAME, &dev);

	// Insert Application-Specific Initialisation Routines here
	NANO120_Initialise(StartupStatus, &dev);

	PlatformRadioInitWithDev(&dev);

	HandleMac   = 1;
	OT_INSTANCE = otInstanceInitSingle();
	otLinkSetPollPeriod(OT_INSTANCE, 10000);
	// Endless Polling Loop
	while (1)
	{
		cascoda_io_handler(&dev);
		// Insert Application-Specific Event Handlers here
		NANO120_Handler(&dev);
		//TEST15_4_PHY_Handler();

	} // while(1)
}
