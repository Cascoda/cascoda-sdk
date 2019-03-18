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
/****** Application name                                                 ******/
/******************************************************************************/
#define APP_NAME "OT MTD"

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/

int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret;
	ret = otApi_Dispatch((struct SerialBuffer *)(buf - 1));

	// switch clock otherwise chip is locking up as it looses external clock
	if (((buf[0] == EVBME_SET_REQUEST) && (buf[2] == EVBME_RESETRF)) || (buf[0] == EVBME_GUI_CONNECTED))
	{
		EVBME_SwitchClock(pDeviceRef, 0);
	}
	return ret;
}

void sleep_if_possible()
{
	if (!otTaskletsArePending(OT_INSTANCE))
	{
		otLinkModeConfig linkMode = otThreadGetLinkMode(OT_INSTANCE);

		if (linkMode.mDeviceType == 0 && linkMode.mRxOnWhenIdle == 0 &&
		    otThreadGetDeviceRole(OT_INSTANCE) == OT_DEVICE_ROLE_CHILD && !otLinkIsInTransmitState(OT_INSTANCE) &&
		    !PlatformIsExpectingIndication())
		{
			uint32_t idleTimeLeft = PlatformGetAlarmMilliTimeout();

			if (idleTimeLeft > 5)
			{
				PlatformSleep(idleTimeLeft);
			}
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
	if (status == CA_ERROR_FAIL)
	{
		BSP_LEDSigMode(LED_M_SETERROR);
		return;
	}

	BSP_LEDSigMode(LED_M_CLRALL);
	BSP_LEDSigMode(LED_M_CONNECTED_BAT_FULL);

	EVBME_SwitchClock(pDeviceRef, 1);

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

#if (0)
	if (USBPresent)
	{
		if (BSP_GetChargeStat())
			BSP_LEDSigMode(LED_M_CONNECTED_BAT_CHRG); // charging
		else
			BSP_LEDSigMode(LED_M_CONNECTED_BAT_FULL); // not charging
	}
#endif

#if (0)
	if (++HandlerCount > 20)
	{ // Read temperature every 20 calls
		HandlerCount = 0;
		t32val       = BSP_GetTemperature() / 10;

		Temperature = LS0_BYTE(t32val);
	}
#endif

	if (HandleMac)
	{
		PlatformAlarmProcess(OT_INSTANCE);
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

	// Initialisation of Chip and EVBME
	// Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action
	// in case there is no UpStream Communications Channel available
	StartupStatus = EVBMEInitialise(APP_NAME, &dev);

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
