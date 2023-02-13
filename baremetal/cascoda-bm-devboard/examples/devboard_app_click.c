/**
 * @file
 * @brief test15_4 main program loop and supporting functions
 */
/*
 *  Copyright (c) 2022, Cascoda Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Example application for external click sensor interfaces
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
#include "airquality4_click.h"
#include "devboard_btn.h"
#include "devboard_click.h"
#include "environment2_click.h"
#include "hvac_click.h"
#include "motion_click.h"
#include "relay_click.h"
#include "sht_click.h"
#include "thermo3_click.h"
#include "thermo_click.h"

/* change if using handlers other trhan default */
#include "devboard_click_handlers_default.h"

/* mikrobus click interface 1 configuration */
#define CLICK1_TYPE STYPE_THERMO3
#define CLICK1_HANDLER CLICK_Handler_Default_THERMO3
/* mikrobus click interface 2 configuration */
//#define CLICK2_TYPE     STYPE_THERMO
//#define CLICK2_HANDLER  MIKROSDK_Handler_THERMO
#define CLICK2_TYPE STYPE_NONE
#define CLICK2_HANDLER NULL

/* sleep (1) or stay awake (0) */
#define USE_SLEEP_MODE 0

/* use power control (crowbar) for isolating click boards during power-down (vdd33 off) */
/* note: jumper has to be populated when 1 */
#define USE_POWER_CONTROL_ON_POWERDOWN 0

/* use additional wakeup button (button SW3) */
#define USE_ADDITIONAL_WAKEUP_BUTTON 1

/* measurement period in [ms] */
#if ((((CLICK1_TYPE == STYPE_THERMO3) || (CLICK2_TYPE == STYPE_THERMO3)) && (THERMO3_USE_INTERRUPT)) || \
     (((CLICK1_TYPE == STYPE_SHT) || (CLICK2_TYPE == STYPE_SHT)) && (SHT_USE_INTERRUPT)) ||             \
     (((CLICK1_TYPE == STYPE_MOTION) || (CLICK2_TYPE == STYPE_MOTION)) && (MOTION_USE_INTERRUPT)))
#define MEASUREMENT_PERIOD DVBD_MAX_SLEEP_TIME
#else
#define MEASUREMENT_PERIOD 5000
#endif

extern const char *click_name_default[];

/* sensors have been handled after wakeup */
static bool g_sensors_handled = true;
/* hardware error */
static bool g_hw_error = false;
/* scheduling tasklet for wakeup */
static ca_tasklet g_sensor_wakeup_tasklet;

/* structure for click sensor callbacks */
static mikrosdk_callbacks click_callbacks[DVBD_NUM_MIKROBUS];

/* This has to be declared if sgp40 voc index algorithm is used (ENVIRONMENT2 click) */
u32_t sgp40_sampling_interval = MEASUREMENT_PERIOD;

/* isr for relay 1 (button 0) */
static void relay1_isr(void *context)
{
	(void)context;
	/* toggle state */
	g_relay_1_state   = (1 - g_relay_1_state);
	g_sensors_handled = false;
}

/* isr for relay 2 (button 1) */
static void relay2_isr(void *context)
{
	(void)context;
	/* toggle state */
	g_relay_2_state   = (1 - g_relay_2_state);
	g_sensors_handled = false;
}

/* isr for additional wakeup (button 2) */
static void wakeup_isr(void *context)
{
	(void)context;
	g_sensors_handled = false;
}

/* set hardware error */
static void set_hw_error(void)
{
	g_hw_error = true;
}

/* check sensor alarm status */
static void check_alarms(void)
{
	/* check alarm status for specific click boards */
	for (int i = 0; i < DVBD_NUM_MIKROBUS; ++i)
	{
		if (click_callbacks[i].click_alarm)
		{
			if (click_callbacks[i].click_alarm())
			{
				g_sensors_handled = false;
			}
		}
	}
}

// wakeup tasklet function - schedule next period
static ca_error sensor_wakeup(void *aContext)
{
	u8_t pinstate;

	if (!g_hw_error)
	{
		g_sensors_handled = false;
		TASKLET_ScheduleDelta(&g_sensor_wakeup_tasklet, MEASUREMENT_PERIOD, NULL);
	}
	else
	{
		DVBD_Sense(LED_BTN_3, &pinstate);
		DVBD_SetLED(LED_BTN_3, !pinstate);
		TASKLET_ScheduleDelta(&g_sensor_wakeup_tasklet, 400, NULL);
	}
	return CA_ERROR_SUCCESS;
}

// application initialisation
void hardware_init(void)
{
	/* click types */
	dvbd_click_type dev_types[DVBD_NUM_MIKROBUS] = {CLICK1_TYPE, CLICK2_TYPE};
	/* click handler functions */
	ca_error (*handlers[DVBD_NUM_MIKROBUS])(void) = {CLICK1_HANDLER, CLICK2_HANDLER};

	/* Application-Specific Button/LED Initialisation Routines */
	/* SW4 BTN/LED is LED output; static for power-on */
	DVBD_RegisterLEDOutput(LED_BTN_3, JUMPER_POS_1);
	DVBD_SetLED(LED_BTN_3, LED_ON);
	if ((dev_types[0] == STYPE_RELAY) || (dev_types[1] == STYPE_RELAY))
	{
		/* register buttons for RELAY */
		/* SW1 BTN/LED is button for toggling relay_1 */
		/* SW2 BTN/LED is button for toggling relay_2 */
		DVBD_RegisterButtonIRQInput(LED_BTN_0, JUMPER_POS_1);
		DVBD_RegisterButtonIRQInput(LED_BTN_1, JUMPER_POS_1);
		DVBD_SetButtonShortPressCallback(LED_BTN_0, &relay1_isr, NULL, BTN_SHORTPRESS_PRESSED);
		DVBD_SetButtonShortPressCallback(LED_BTN_1, &relay2_isr, NULL, BTN_SHORTPRESS_PRESSED);
	}
	else
	{
#if (USE_ADDITIONAL_WAKEUP_BUTTON == 1)
		/* register buttons for other sensors */
		/* SW3 BTN/LED for additional wake-up interrupt */
		DVBD_RegisterButtonIRQInput(LED_BTN_2, JUMPER_POS_1);
		DVBD_SetButtonShortPressCallback(LED_BTN_2, &wakeup_isr, NULL, BTN_SHORTPRESS_PRESSED);
#endif
	}

/* power up click interfaces if power control is used */
#if (USE_POWER_CONTROL_ON_POWERDOWN)
	if (DVBD_click_power_init())
		printf("No access to power control pin for mikrobus interfaces\n");
#endif

	printf("Initialising devices:\n");
	for (int i = 0; i < DVBD_NUM_MIKROBUS; ++i) printf("Interface %d: %s\n", i, click_name_default[dev_types[i]]);
	printf("\n");

	/* Application-Specific Click Device Initialisation */
	for (int i = 0; i < DVBD_NUM_MIKROBUS; ++i)
	{
		if (DVBD_select_click(&click_callbacks[i], dev_types[i], handlers[i]))
			printf("Invalid Device Type: %d\n", dev_types[i]);
		else
		{
			if (click_callbacks[i].click_initialise)
			{
				if (click_callbacks[i].click_initialise())
				{
					printf("Click Sensor Number %d (%s): Initialisation failed\n", i, click_name_default[dev_types[i]]);
					set_hw_error();
				}
			}
		}
	}

	/* init scheduling, and trigger first calls (after 100 ms) to initialise click sensor interface in handlers */
	TASKLET_Init(&g_sensor_wakeup_tasklet, &sensor_wakeup);
	TASKLET_ScheduleDelta(&g_sensor_wakeup_tasklet, 100, NULL);
}

// application polling function
void hardware_poll(void)
{
	/* check buttons */
	DVBD_PollButtons();

	/* click sensor handlers */
	check_alarms();

	if (!g_sensors_handled)
	{
		for (int i = 0; i < DVBD_NUM_MIKROBUS; ++i)
		{
			if (click_callbacks[i].click_handler)
			{
				if (click_callbacks[i].click_handler())
				{
					printf("Error in Handler %d (%s)\n", i, click_name_default[click_callbacks[i].dev_type]);
					set_hw_error();
				}
			}
		}
		g_sensors_handled = true;
	}
}

// application level check if device can go to sleep
bool hardware_can_sleep(void)
{
	if (!DVBD_CanSleep())
		return false;

	if (g_hw_error)
		return false;

	return true;
}

// application sleep (power-down) function
void hardware_sleep(struct ca821x_dev *pDeviceRef)
{
	uint32_t taskletTimeLeft = MEASUREMENT_PERIOD;

	/* schedule wakeup */
	TASKLET_GetTimeToNext(&taskletTimeLeft);

	/* check that it's worth going to sleep */
	if ((taskletTimeLeft > 100) && USE_SLEEP_MODE)
	{
		/* and sleep */
		DVBD_DevboardSleep(taskletTimeLeft, pDeviceRef);
	}
}

// application reinitialise after wakeup
void hardware_reinitialise(void)
{
}

// re-initialise after wakeup from sleep
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
	cascoda_reinitialise    = reinitialise_after_wakeup;

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
