/*
 *  Copyright (c) 2023, Cascoda Ltd.
 *  All rights reserved.
 *
 */
/*
 * Example application for onboard monitoring of battery status
*/

#include <stdio.h>
#include <stdlib.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#include "devboard_batt.h"
#include "devboard_btn.h"

#define GET_CHARGE_STAT 1
#define GET_VOLTS 1
#define GET_USB_PRESENT 1

/* reporting period in [ms] */
#define REPORT_TIME 5000

/* sleep (1) or stay awake (0) */
/* 0: never go to sleep  */
/* 1: go to sleep when running on battery (vbus disconnected) */
/* 2: always go to sleep */
#define USE_SLEEP_MODE 1

/* scheduling tasklet for reporting battery status */
static ca_tasklet g_report_batt_tasklet;

static u8_t g_go_sleep = 0;

// Reporting Battery Status
static ca_error ReportBattStatus(void *aContext)
{
	(void)aContext;
	u16_t vbatt         = 0;
	u8_t  charging      = 0;
	u8_t  vbusconnected = 0;

	if (GET_CHARGE_STAT)
		charging = DVBD_BattGetChargeStat();
	if (GET_VOLTS)
		vbatt = DVBD_BattGetVolts();
	if (GET_USB_PRESENT)
		vbusconnected = DVBD_BattGetUSBPresent();

	printf("%4us BattStatus: ", (TIME_ReadAbsoluteTime() / 1000));
	if (GET_VOLTS)
		printf("Vbatt: %2d.%02u V;", (vbatt / 100), abs(vbatt % 100));
	if (GET_CHARGE_STAT)
		printf(" %s;", ((charging == CHARGING) ? "charging" : "not charging"));
	if (GET_USB_PRESENT)
		printf(" %s;", ((vbusconnected == CONNECTED) ? "+5V connected" : "+5V not connected"));
	printf("\n");

	TASKLET_ScheduleDelta(&g_report_batt_tasklet, REPORT_TIME, NULL);

	if (vbusconnected)
		DVBD_SetLED(DEV_SWITCH_2, LED_ON);
	else
		DVBD_SetLED(DEV_SWITCH_2, LED_OFF);

	if (charging)
		DVBD_SetLED(DEV_SWITCH_3, LED_ON);
	else
		DVBD_SetLED(DEV_SWITCH_3, LED_OFF);

#if defined(USE_USB)
	if (vbusconnected)
		g_go_sleep = 0;
	else
		g_go_sleep = 1;
#else
	g_go_sleep = 1;
#endif // USE_USB

	return CA_ERROR_SUCCESS;
}

// application initialisation
static void hardware_init(void)
{
	/* Register LED/Button pairs */
	if (DVBD_RegisterLEDOutput(DEV_SWITCH_1, JUMPER_POS_2))
		printf("Failed to register SW1\n");
	if (DVBD_RegisterLEDOutput(DEV_SWITCH_2, JUMPER_POS_2))
		printf("Failed to register SW2\n");
	if (DVBD_RegisterLEDOutput(DEV_SWITCH_3, JUMPER_POS_2))
		printf("Failed to register SW3\n");
	if (DVBD_RegisterLEDOutput(DEV_SWITCH_4, JUMPER_POS_2))
		printf("Failed to register SW4\n");

	DVBD_SetLED(DEV_SWITCH_1, LED_ON);
	DVBD_SetLED(DEV_SWITCH_2, LED_OFF);
	DVBD_SetLED(DEV_SWITCH_3, LED_OFF);
	DVBD_SetLED(DEV_SWITCH_4, LED_OFF);

	if (GET_CHARGE_STAT)
	{
		/* initialise CHARGE_STAT */
		if (DVBD_BattInitChargeStat())
			printf("Failed to initialise CHARGE_STAT\n");
	}
	if (GET_VOLTS)
	{
		/* initialise VOLTS and VOLTS_TEST */
		if (DVBD_BattInitVolts())
			printf("Failed to initialise VOLTS/VOLTS_TEST\n");
	}
	if (GET_USB_PRESENT)
	{
		/* initialise USB_PRESENT */
		if (DVBD_BattInitUSBPresent())
			printf("Failed to initialise USB_PRESENT\n");
	}

	TASKLET_Init(&g_report_batt_tasklet, &ReportBattStatus);
	TASKLET_ScheduleDelta(&g_report_batt_tasklet, REPORT_TIME, NULL);
}

// application polling function
void hardware_poll(void)
{
	/* check buttons */
	DVBD_PollButtons();
}

// application reinitialise after wakeup
void hardware_reinitialise(void)
{
}

// application level check if device can go to sleep
bool hardware_can_sleep(void)
{
	if ((USE_SLEEP_MODE == 0) || ((USE_SLEEP_MODE == 1) && (!g_go_sleep)))
		return false;

	if (!DVBD_CanSleep())
		return false;

	return true;
}

// application sleep (power-down) function
void hardware_sleep(struct ca821x_dev *pDeviceRef)
{
	uint32_t taskletTimeLeft = 60000;

	/* schedule wakeup */
	TASKLET_GetTimeToNext(&taskletTimeLeft);

	/* check that it's worth going to sleep */
	if (taskletTimeLeft > 100)
	{
		/* and sleep */
		DVBD_DevboardSleep(taskletTimeLeft, pDeviceRef);
	}
}

// re-initialisation function for wake-up after power-down
static int reinitialise_after_wakeup(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;

	/* for OpenThread: */
	//otLinkSyncExternalMac(OT_INSTANCE);

	/* reinitialise hardware (application-specific) */
	hardware_reinitialise();

	return 0;
}

// sleep if possible
static void sleep_if_possible(struct ca821x_dev *pDeviceRef)
{
	/* for OpenThread: */
	//if (!PlatformCanSleep(OT_INSTANCE))
	//    return;

	/* check for hardware (application-specific) */
	if (!hardware_can_sleep())
		return;

	hardware_sleep(pDeviceRef);
}

// main loop
int main(void)
{
	struct ca821x_dev dev;

	ca821x_api_init(&dev);
	cascoda_reinitialise = reinitialise_after_wakeup;

	EVBMEInitialise(CA_TARGET_NAME, &dev);

	hardware_init();

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);
		hardware_poll();
		sleep_if_possible(&dev);
	} /* while(1) */

	return 0;
}
