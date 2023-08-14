/**
 * @file
 * @brief mikrosdk interface
 */
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
/*
 * Example click interface driver
*/

/* include <device>_drv.h and <device>_click.h */
#include "buzz2_click.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"

// Tasklet for turning off PWM when specified duration is up
static ca_tasklet g_stop_note_tasklet;

// Tasklet callback functions
static ca_error BUZZ2_stop_note(void *context)
{
	(void)context;

	SENSORIF_PWM_Deinit();
}

/* Buzz2 function to play note with specified frequency, volume and duration */
/* Volume[%] = 0-100 */
/* Duration[ms] = 0-65536 */
/* Mode[BLOCKING/NON_BLOCKING] = 
/* Blocking: Uses delay, can call this function successively, not recommended for KNX applications
/* Non-blocking: Uses tasklet, can only call this function once within the specified duration */
uint8_t MIKROSDK_BUZZ2_play_note(uint32_t freq, uint32_t volume, uint16_t duration, uint8_t mode)
{
	if (SENSORIF_PWM_Init(BUZZ2_PWM_PIN, freq, volume))
		return BUZZ2_ST_FAIL;

	if (mode == BLOCKING)
	{
		while (duration)
		{
			WAIT_ms(1);
			duration--;
		}
		SENSORIF_PWM_Deinit();
	}
	else if (mode == NON_BLOCKING)
	{
		if (TASKLET_ScheduleDelta(&g_stop_note_tasklet, duration, NULL))
			return BUZZ2_ST_FAIL;
	}

	return BUZZ2_ST_OK;
}

uint8_t MIKROSDK_BUZZ2_Initialise(void)
{
	if (TASKLET_Init(&g_stop_note_tasklet, &BUZZ2_stop_note))
		return BUZZ2_ST_FAIL;

	return BUZZ2_ST_OK;
}