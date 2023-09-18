/*
 *  Copyright (c) 2023, Cascoda Ltd.
 *  All rights reserved.
 *
 */
/*
 * Example application for onboard Buttons
*/
/*
 * SW1: Button 1	POS2
 * SW2: Button 2	POS2
 * SW3: Button 3	POS2
 * SW4: Led for On	POS2
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

#define DEF_SHORT_CB 1 /* short callback defined */
#define DEF_HOLD_CB 1  /* hold  callback defined */
#define DEF_LONG_CB 1  /* long  callback defined */

#define BTN_LONG_TIME_THRESHOLD 2000 /* time threshold to trigger button long function */
#define BTN_HOLD_TIME_INTERVAL 400   /* Time intervall for button hold time function */

#define WAKEUP_TIME 10000

#define USE_INTERRUPTS 0 /* power-down and interrupts when 1, polling when 0 */

#define BLINK_PERIOD_MS 500

static u32_t holdcount[3] = {0, 0, 0};
ca_tasklet   blink_tasklet;

struct blink_ctx
{
	ca_tasklet *tasklet;
	u8_t        led;
};

// button short press callbacks
static void short_press_cb(void *context)
{
	u8_t nr = *(u8_t *)context;
	printf("Button %d short pressed\n", nr);
	holdcount[nr - 1] = 0;
}

// button hold callbacks
static void hold_cb(void *context)
{
	u8_t nr = *(u8_t *)context;
	++holdcount[nr - 1];
	printf("Button %d held (%ld)\n", nr, holdcount[nr - 1]);
}

// button long press callbacks
static void long_press_cb(void *context)
{
	u8_t nr = *(u8_t *)context;
	printf("Button %d long pressed!\n", nr);
	holdcount[nr - 1] = 0;
}

// application initialisation
static void hardware_init(void)
{
	static u8_t nr[3] = {1, 2, 3};

	/* Register the SW4 as LED */
	DVBD_RegisterLEDOutput(DEV_SWITCH_4, JUMPER_POS_2);
	DVBD_SetLED(DEV_SWITCH_4, LED_ON);

	/* Register SW1/SW2/SW3 as Buttons */
#if USE_INTERRUPTS
	DVBD_RegisterSharedIRQButtonLED(DEV_SWITCH_1, JUMPER_POS_2);
	DVBD_RegisterSharedIRQButtonLED(DEV_SWITCH_2, JUMPER_POS_2);
	DVBD_RegisterSharedIRQButtonLED(DEV_SWITCH_3, JUMPER_POS_2);
#else
	DVBD_RegisterSharedButtonLED(DEV_SWITCH_1, JUMPER_POS_2);
	DVBD_RegisterSharedButtonLED(DEV_SWITCH_2, JUMPER_POS_2);
	DVBD_RegisterSharedButtonLED(DEV_SWITCH_3, JUMPER_POS_2);
#endif
	DVBD_SetLED(DEV_SWITCH_1, 1);
	DVBD_SetLED(DEV_SWITCH_2, 1);
	DVBD_SetLED(DEV_SWITCH_3, 0);
#if DEF_SHORT_CB
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_1, &short_press_cb, &nr[0], BTN_SHORTPRESS_RELEASED);
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_2, &short_press_cb, &nr[1], BTN_SHORTPRESS_RELEASED);
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_3, &short_press_cb, &nr[2], BTN_SHORTPRESS_RELEASED);
#endif
#if DEF_HOLD_CB
	DVBD_SetButtonHoldCallback(DEV_SWITCH_1, &hold_cb, &nr[0], BTN_HOLD_TIME_INTERVAL);
	DVBD_SetButtonHoldCallback(DEV_SWITCH_2, &hold_cb, &nr[1], BTN_HOLD_TIME_INTERVAL);
	DVBD_SetButtonHoldCallback(DEV_SWITCH_3, &hold_cb, &nr[2], BTN_HOLD_TIME_INTERVAL);
#endif
#if DEF_LONG_CB
	DVBD_SetButtonLongPressCallback(DEV_SWITCH_1, &long_press_cb, &nr[0], BTN_LONG_TIME_THRESHOLD);
	DVBD_SetButtonLongPressCallback(DEV_SWITCH_2, &long_press_cb, &nr[1], BTN_LONG_TIME_THRESHOLD);
	DVBD_SetButtonLongPressCallback(DEV_SWITCH_3, &long_press_cb, &nr[2], BTN_LONG_TIME_THRESHOLD);
#endif
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
	holdcount[0] = 0;
	holdcount[1] = 0;
	holdcount[2] = 0;
}

// application level check if device can go to sleep
bool hardware_can_sleep(void)
{
	if (!USE_INTERRUPTS) /* polling only */
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

ca_error blinkCB(void *_ctx)
{
	struct blink_ctx *ctx = (struct blink_ctx *)_ctx;
	u8_t              out;
	DVBD_SenseOutput(ctx->led, &out);
	out = !out;
	DVBD_SetLED(ctx->led, out);

	TASKLET_ScheduleDelta(ctx->tasklet, BLINK_PERIOD_MS, _ctx);
	return CA_ERROR_SUCCESS;
}

// main loop
int main(void)
{
	struct ca821x_dev dev;

	ca821x_api_init(&dev);
	cascoda_reinitialise = reinitialise_after_wakeup;

	EVBMEInitialise(CA_TARGET_NAME, &dev);

	hardware_init();

	struct blink_ctx ctx = {
	    &blink_tasklet,
	    DEV_SWITCH_1,
	};
	TASKLET_Init(&blink_tasklet, &blinkCB);
	TASKLET_ScheduleDelta(&blink_tasklet, BLINK_PERIOD_MS, &ctx);

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);
		hardware_poll();
		sleep_if_possible(&dev);
	} /* while(1) */

	return 0;
}
