/*
 *  Copyright (c) 2023, Cascoda Ltd.
 *  All rights reserved.
 *
 */
/*
 * Example application for onboard LEDs and Buttons
*/

#include <stdio.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#include "cascoda-bm/test15_4_evbme.h"
#include "devboard_btn.h"

#define LED_0_PERIOD 1000
#define LED_1_PERIOD 1000
#define LED_2_PERIOD 1000
#define LED_0_DELAY 0
#define LED_1_DELAY 333
#define LED_2_DELAY 666

#define DEF_SHORT_CB 1 /* short callback defined */
#define DEF_HOLD_CB 1  /* hold  callback defined */
#define DEF_LONG_CB 1  /* long  callback defined */

#define BTN_LONG_TIME_THRESHOLD 2000 /* time threshold to trigger button long function */
#define BTN_HOLD_TIME_INTERVAL 400   /* Time intervall for button hold time function */
#define BTN_LED_ONTIME 200           /* LED on time for button functions */

#define WAKEUP_TIME 10000

#define USE_INTERRUPTS 0 /* power-down and interrupts when 1, polling when 0 */

static ca_tasklet LEDsWakeupTasklet;

static u32_t holdcount               = 0;
static u32_t led_on_time[NUM_LEDBTN] = {0, 0, 0, 0};

// button short press callback
static void short_press_cb(void *context)
{
	(void)context;
	led_on_time[DEV_SWITCH_1] = TIME_ReadAbsoluteTime();
	printf("Button short pressed!\n");
	holdcount = 0;
}

// button hold callback
static void hold_cb(void *context)
{
	(void)context;
	led_on_time[DEV_SWITCH_2] = TIME_ReadAbsoluteTime();
	++holdcount;
	printf("Button held! (%ld)\n", holdcount);
}

// button long press callback
static void long_press_cb(void *context)
{
	(void)context;
	led_on_time[DEV_SWITCH_3] = TIME_ReadAbsoluteTime();
	printf("Button long pressed!\n");
	holdcount = 0;
}

// LEDs blinking, used when not compiled in interrupt/powerdown mode as messages are available
#if (!USE_INTERRUPTS)
static void LEDBlinkHandler(void)
{
	static u32_t lastcurTime = 0;
	u32_t        curTime     = TIME_ReadAbsoluteTime();
	u8_t         pinstate;

	/* turns the LEDs on and off for LED_X_PERIOD after LED_X_DELAY (to be able to stagger them) */
	if (((curTime % LED_0_PERIOD) == LED_0_DELAY) && ((lastcurTime % LED_0_PERIOD) != LED_0_DELAY) &&
	    (curTime >= LED_0_PERIOD))
	{
		DVBD_Sense(DEV_SWITCH_1, &pinstate);
		DVBD_SetLED(DEV_SWITCH_1, !pinstate);
	}
	if (((curTime % LED_1_PERIOD) == LED_1_DELAY) && ((lastcurTime % LED_1_PERIOD) != LED_1_DELAY) &&
	    (curTime >= LED_1_PERIOD))
	{
		DVBD_Sense(DEV_SWITCH_2, &pinstate);
		DVBD_SetLED(DEV_SWITCH_2, !pinstate);
	}
	if (((curTime % LED_2_PERIOD) == LED_2_DELAY) && ((lastcurTime % LED_2_PERIOD) != LED_2_DELAY) &&
	    (curTime >= LED_2_PERIOD))
	{
		DVBD_Sense(DEV_SWITCH_3, &pinstate);
		DVBD_SetLED(DEV_SWITCH_3, !pinstate);
	}

	lastcurTime = curTime;
}
#endif

// LEDs mirroring button presses, used when compiled in interrupt/powerdown mode
// led 0: short press
// led 1: hold
// led 2: long press
#if USE_INTERRUPTS
static void LEDBtnHandler(void)
{
	for (u8_t i = 0; i < NUM_LEDBTN; ++i)
	{
		if (led_on_time[i] != 0)
		{
			if (TIME_ReadAbsoluteTime() < (led_on_time[i] + BTN_LED_ONTIME))
			{
				DVBD_SetLED(i, LED_ON);
			}
			else
			{
				led_on_time[i] = 0;
				DVBD_SetLED(i, LED_OFF);
			}
		}
		else
		{
			DVBD_SetLED(i, LED_OFF);
		}
	}
}
#endif

// LEDs all on periodaically for wakeup
static ca_error LEDsWakeUp(void *aContext)
{
	led_on_time[DEV_SWITCH_1] = TIME_ReadAbsoluteTime();
	led_on_time[DEV_SWITCH_2] = TIME_ReadAbsoluteTime();
	led_on_time[DEV_SWITCH_3] = TIME_ReadAbsoluteTime();
	TASKLET_ScheduleDelta(&LEDsWakeupTasklet, WAKEUP_TIME, NULL);
	return CA_ERROR_SUCCESS;
}

// application initialisation
static void hardware_init(void)
{
	/* Register the first 3 LEDS as output */
	DVBD_RegisterLEDOutput(DEV_SWITCH_1, JUMPER_POS_2);
	DVBD_RegisterLEDOutput(DEV_SWITCH_2, JUMPER_POS_2);
	DVBD_RegisterLEDOutput(DEV_SWITCH_3, JUMPER_POS_2);

// Register the last button (3) as input
#if USE_INTERRUPTS
	DVBD_RegisterButtonIRQInput(DEV_SWITCH_4, JUMPER_POS_2);
#else
	DVBD_RegisterButtonInput(DEV_SWITCH_4, JUMPER_POS_2);
#endif
#if DEF_SHORT_CB
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_4, &short_press_cb, NULL, BTN_SHORTPRESS_RELEASED);
#endif
#if DEF_HOLD_CB
	DVBD_SetButtonHoldCallback(DEV_SWITCH_4, &hold_cb, NULL, BTN_HOLD_TIME_INTERVAL);
#endif
#if DEF_LONG_CB
	DVBD_SetButtonLongPressCallback(DEV_SWITCH_4, &long_press_cb, NULL, BTN_LONG_TIME_THRESHOLD);
#endif

	TASKLET_Init(&LEDsWakeupTasklet, &LEDsWakeUp);
	TASKLET_ScheduleDelta(&LEDsWakeupTasklet, WAKEUP_TIME, NULL);
}

// application polling function
void hardware_poll(void)
{
	/* check buttons */
	DVBD_PollButtons();

/* handle LEDs */
#if (USE_INTERRUPTS)
	LEDBtnHandler();
#else
	LEDBlinkHandler();
#endif

	/* check buttons */
	DVBD_PollButtons();
}

// application reinitialise after wakeup
void hardware_reinitialise(void)
{
	holdcount = 0;
}

// application level check if device can go to sleep
bool hardware_can_sleep(void)
{
	if (!USE_INTERRUPTS) /* polling only */
		return false;

	if (!DVBD_CanSleep())
		return false;

	for (u8_t i = 0; i < NUM_LEDBTN; ++i)
	{
		if (led_on_time[i] != 0)
			return false;
	}

	return true;
}

// application sleep (power-down) function
void hardware_sleep(struct ca821x_dev *pDeviceRef)
{
	uint32_t taskletTimeLeft = 60000;

	/* schedule wakeup */
	TASKLET_GetTimeToNext(&taskletTimeLeft);

	/* check that it's worth going to sleep */
	if (taskletTimeLeft > BTN_LED_ONTIME)
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
