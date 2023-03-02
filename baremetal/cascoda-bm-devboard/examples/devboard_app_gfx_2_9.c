/**
 * @file
 *//*
 *  Copyright (c) 2023, Cascoda Ltd.
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
 * Application for E-ink display for usage of the graphics library.
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "cascoda-bm/test15_4_evbme.h"
#include "devboard_btn.h"
#include "sif_il3820.h"

#include "gfx_driver.h" // include  controller driver source code

#include "gfx_library.h" // include graphics library header

#define SPI_NUM 1                    // SPI interface used for display
#define NUM_SCREENS 6                // number of screens that can be shown
#define BTN_LONG_TIME_THRESHOLD 2000 /* time threshold to trigger button long function */
#define SLEEP_LONG_TIME 60000        // sleep time without button interrupt

ca_tasklet  screen_Tasklet;
static u8_t g_screen_nr = 0;

// load image
static void load_screen(u8_t nr)
{
	int  display_width  = display_getWidth();
	int  display_height = display_getHeight();
	char screen_str[20];

	// clear the frame buffer
	display_clear();
	// set the text color
	display_setTextColor(BLACK, WHITE);

	printf("Loading screen %u ..\n", (nr + 1));
	if (nr == 0)
	{
		display_drawRect(0, 0, display_width - 1, display_width - 1, BLACK);
		char qr[] = "KNX:S:00FA10010400;P:ABY8B77J50YXMUDW3DG4";
		SIF_IL3820_overlay_qr_code(qr, get_framebuffer(), 4, 2, 2);
		//display_setRotation(1);
		display_setCursor(1, 130);
		display_setTextColor(BLACK, WHITE);
		display_puts(qr);
	}
	if (nr > 0)
	{
		sprintf((char *)&screen_str, "%d", nr);
		// generic stuff
		display_drawRect(0, 0, display_width, display_width, BLACK);
		display_drawLine(0, 12, display_width, 12, BLACK);
		display_setCursor(3, 2);
		display_puts("BN1");
		display_setCursor(33, 2);
		display_puts("BN2");
		display_setCursor(63, 2);
		display_puts("BN3");
		display_setCursor(100, 2);
		snprintf(screen_str, 19, "%d/%d", nr + 1, NUM_SCREENS);
		display_puts(screen_str);
	}
	if (nr == 1)
	{
		display_setCursor(3, 15);
		display_puts("BN1 - next screen");
		display_setCursor(3, 27);
		display_puts("BN2 - something");
		display_setCursor(3, 39);
		display_puts("BN3 - something else");
	}
	if (nr == 2)
	{
		display_drawCircle(30, 30, 10, BLACK);
		display_fillCircle(70, 70, 20, BLACK);
	}
	if (nr == 3)
	{
		display_setCursor(3, 15);
		display_puts("Temperature");
		display_setCursor(3, 30);
		display_setTextSize(4);
		float temperature = 20.7;
		// snprintf does not work on embedded systems
		display_double(screen_str, 99, temperature, 1);
		display_puts(screen_str);
		display_setTextSize(1);
		display_setCursor(3, 75);
		display_puts("Humidity");
		display_setCursor(3, 90);
		display_setTextSize(4);
		float humidity = -5.3;
		display_double(screen_str, 99, humidity, 1);
		display_puts(screen_str);
	}
	if (nr == 4)
	{
		display_setCursor(3, 15);
		display_puts("percentage ");
		display_setCursor(3, 30);
		display_setTextSize(4);
		int percentage = 100;
		snprintf(screen_str, 19, "%d", percentage);
		display_puts(screen_str);
	}
	if (nr == 5)
	{
		display_setCursor(3, 15);
		display_puts("BN1 - next screen");
		display_setCursor(3, 27);
		display_puts("BN2 - reset");
		display_setCursor(3, 39);
		display_puts("BN3 - reboot");
	}
	// reset all the rendering constructs
	display_setCursor(0, 0);
	display_setTextSize(1);

	if (nr == 0 || nr == 1)
		display_render(FULL_UPDATE, WITH_CLEAR);
	else
		display_render(PARTIAL_UPDATE, WITHOUT_CLEAR);
}

// button callback
static void button_pressed(void *context)
{
	(void)context;

	if (g_screen_nr < (NUM_SCREENS - 1))
		++g_screen_nr;
	else
		g_screen_nr = 0;
	TASKLET_ScheduleDelta(&screen_Tasklet, 300, NULL);
}

// long button callback
static void long_button_pressed(void *context)
{
	(void)context;

	g_screen_nr = 1;
	TASKLET_ScheduleDelta(&screen_Tasklet, 300, NULL);
}

// sleep if possible
static void sleep_if_possible(struct ca821x_dev *pDeviceRef)
{
	if (DVBD_CanSleep())
	{
		DVBD_DevboardSleep(SLEEP_LONG_TIME, pDeviceRef);
	}
}

// application initialisation
static void app_initialise(void)
{
	/* Eink Initialisation */
	SENSORIF_SPI_Config(SPI_NUM);
	SIF_IL3820_Initialise(FULL_UPDATE);

	/* 3rd BTN/LED (2) is LED output; static for power-on */
	DVBD_RegisterLEDOutput(LED_BTN_2, JUMPER_POS_1);
	DVBD_SetLED(LED_BTN_2, LED_ON);
	/* 4th BTN/LED (3) is IRQ button to test button interrupts */
	DVBD_RegisterButtonIRQInput(LED_BTN_3, JUMPER_POS_1);
	DVBD_SetButtonShortPressCallback(LED_BTN_3, &button_pressed, NULL, BTN_SHORTPRESS_RELEASED);
	DVBD_SetButtonLongPressCallback(LED_BTN_3, &long_button_pressed, NULL, BTN_LONG_TIME_THRESHOLD);
}

ca_error initial_screen(void *args)
{
	load_screen(g_screen_nr);
}

// main loop
int main(void)
{
	struct ca821x_dev dev;

	ca821x_api_init(&dev);
	EVBMEInitialise(CA_TARGET_NAME, &dev);

	/* Application-Specific Initialisation Routines */
	app_initialise();

	TASKLET_Init(&screen_Tasklet, &initial_screen);
	TASKLET_ScheduleDelta(&screen_Tasklet, 0, NULL);

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);
		DVBD_PollButtons();
		sleep_if_possible(&dev);

	} /* while(1) */
}
