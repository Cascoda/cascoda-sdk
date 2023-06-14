/*
 *  Copyright (c) 2023, Cascoda Ltd.
 *  All rights reserved.
 *
 */
/*
 * Example application for powerdown mechanisms
*/
#include <stdio.h>
#include <stdlib.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "cascoda-bm/test15_4_evbme.h"
#include "devboard_btn.h"

static u8_t  g_always_on   = 1;
static u8_t  g_go_sleep    = 0;
static u32_t g_wakeupcount = 0;

#define TIME_UP 10000   // 10 seconds power-up
#define TIME_DOWN 10000 // 10 seconds power-down

static ca_tasklet pd_tasklet;

// button 3 routine
// switches between:
// - permanently on (default)
// - power-down (sleeping)
// polled so can only be toggled when not sleeping.
static void button3_pressed(void *context)
{
	(void)context;
	g_always_on = 1 - g_always_on;
	DVBD_SetLED(DEV_SWITCH_2, LED_OFF);
}

// button 2 routine
// mapped to isr, wakes up device from power-down.
// wakeyp from button press is signalled on led 1
static void button2_pressed(void *context)
{
	(void)context;
	DVBD_SetLED(DEV_SWITCH_2, LED_ON);
}

// power down function
static ca_error pd_sleep(void *aContext)
{
	g_go_sleep = 1;
	return CA_ERROR_SUCCESS;
}

// monitor always_on on change
static void check_always_on(void)
{
	static u8_t always_on_del = 1;

	/* check if always_on has changed to 0 (powerdown mode) */
	if ((g_always_on == 0) && (always_on_del == 1))
	{
		TASKLET_ScheduleDelta(&pd_tasklet, TIME_UP, NULL);
		always_on_del = g_always_on;
		printf("Periodic Powerdown..\n");
	}
	/* check if always_on has changed to 1 (permanently powered) */
	if ((g_always_on == 1) && (always_on_del == 0))
	{
		TASKLET_Cancel(&pd_tasklet);
		always_on_del = g_always_on;
		printf("Permanently On..\n");
	}
}

// device initialisation, pin/led mapping
void hardware_init(void)
{
	/* SW1 is LED output; static for power-on */
	DVBD_RegisterLEDOutput(DEV_SWITCH_1, JUMPER_POS_2);
	DVBD_SetLED(DEV_SWITCH_1, LED_ON);
	/* SW2 is LED output for button interrupt; cleared when going to sleep again */
	DVBD_RegisterLEDOutput(DEV_SWITCH_2, JUMPER_POS_2);
	DVBD_SetLED(DEV_SWITCH_2, LED_OFF);
	/* W3 is IRQ button to test button interrupts */
	DVBD_RegisterButtonIRQInput(DEV_SWITCH_3, JUMPER_POS_2);
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_3, &button2_pressed, NULL, BTN_SHORTPRESS_PRESSED);
	/* 4th BTN/LED (3) is polled button to switch between always on and sleep mode */
	DVBD_RegisterButtonInput(DEV_SWITCH_4, JUMPER_POS_2);
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_4, &button3_pressed, NULL, BTN_SHORTPRESS_PRESSED);

	TASKLET_Init(&pd_tasklet, &pd_sleep);
}

// application polling function
void hardware_poll()
{
	/* check buttons */
	DVBD_PollButtons();

	check_always_on();
}

// application level check if device can go to sleep
bool hardware_can_sleep()
{
	if (!DVBD_CanSleep())
		return false;

	if (g_always_on)
		return false;

	if (!g_go_sleep)
		return false;

	return true;
}

// application sleep (power-down) function
void hardware_sleep(struct ca821x_dev *pDeviceRef)
{
	static u32_t sleepTime;

	g_go_sleep = 0;

	/* note: message is sent before next power-down (reporting previous power-down) */
	/*       to allow enough time to re-connect to comms interface (for usb) */
	if (g_wakeupcount > 0)
		printf("Wake-up %2d: time = %3d sec; sleep = %3d sec\n",
		       g_wakeupcount,
		       (TIME_ReadAbsoluteTime() / 1000),
		       (sleepTime / 1000));

	sleepTime = TIME_ReadAbsoluteTime();
	DVBD_DevboardSleep(TIME_DOWN, pDeviceRef);
	sleepTime = TIME_ReadAbsoluteTime() - sleepTime;

	/* schedule next power-down after power-up */
	TASKLET_ScheduleDelta(&pd_tasklet, TIME_UP, NULL);
}

// application reinitialise after wakeup
void hardware_reinitialise(void)
{
	++g_wakeupcount;
}

// re-initialisation function for wake-up after power-down
static int reinitialise_after_wakeup(struct ca821x_dev *pDeviceRef)
{
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
