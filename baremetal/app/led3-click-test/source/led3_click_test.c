/*
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
/**
 * @file
 *
 * @brief Test for led3 click
 */

#include "led3_click_test.h"
#include "led3_click.h"

#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"

#define SCHEDULE_NOW 0
#define RGB_CYCLE_MS 30

static led3_rgb g_led3;

static ca_tasklet g_rgb_cycle_tasklet;

static ca_error LED3_rgb_cycle(void *context)
{
	(void)context;

	if (g_led3.red == 0x5F && g_led3.green < 0x7F && g_led3.blue == 0x80)
		g_led3.green++;
	else if (g_led3.red > 0x40 && g_led3.green == 0x7F && g_led3.blue == 0x80)
		g_led3.red--;
	else if (g_led3.red == 0x40 && g_led3.green == 0x7F && g_led3.blue < 0x9F)
		g_led3.blue++;
	else if (g_led3.red == 0x40 && g_led3.green > 0x60 && g_led3.blue == 0x9F)
		g_led3.green--;
	else if (g_led3.red < 0x5F && g_led3.green == 0x60 && g_led3.blue == 0x9F)
		g_led3.red++;
	else if (g_led3.red == 0x5F && g_led3.green == 0x60 && g_led3.blue > 0x80)
		g_led3.blue--;

	MIKROSDK_LED3_set_rgb(g_led3.red, g_led3.green, g_led3.blue);

	TASKLET_ScheduleDelta(&g_rgb_cycle_tasklet, RGB_CYCLE_MS, NULL);
}

ca_error LED3_DEMO_Initialise(void)
{
	MIKROSDK_LED3_Initialise();

	g_led3.red   = 0x5F;
	g_led3.green = 0x60;
	g_led3.blue  = 0x80;

	MIKROSDK_LED3_set_dimming_and_intensity(LED3_CONSTANT | LED3_INTENSITY_MAX);
	MIKROSDK_LED3_set_rgb(g_led3.red, g_led3.green, g_led3.blue);

	TASKLET_Init(&g_rgb_cycle_tasklet, &LED3_rgb_cycle);
	TASKLET_ScheduleDelta(&g_rgb_cycle_tasklet, RGB_CYCLE_MS, NULL);
}