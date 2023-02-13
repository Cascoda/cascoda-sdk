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
/**
 * @file
 *
 * @brief API implementation for the demo which uses PWM to dim an LED
 */

#include "pwm_led_dimming.h"

#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-util/cascoda_tasklet.h"

#define DEMO_NUM_OF_DUTY_CYCLES 4
#define DEMO_NUM_OF_PINS 5

#define TESTING_MODE 0

/* 
Struct to hold the current state of the demo.
*/
struct demo_state
{
	uint8_t pin_index;
	uint8_t duty_cycle_index;
};

/* 
How long to wait (in ms) between different events in the demo 
(hence determines the speed of the demo). 
*/
static uint16_t const g_demo_speed = 2000;

/*
Frequency of the PWM used in this demo.
*/
static uint32_t const g_freq_hz = 1000;

/*
Array of duty cycles that the demo will cycle through.
These values represent the duty cycle percentage, and as such should be between 0 and 100.
*/
static uint8_t const g_duty_cycles[DEMO_NUM_OF_DUTY_CYCLES] = {100, 60, 20, 0};

#if TESTING_MODE
/*
Array of pins that can support PWM.
*/
static uint8_t const g_pins[DEMO_NUM_OF_PINS] = {15, 31, 32, 33, 34};
#endif

/* 
Tasklets for scheduling the initialisation and the changing of duty cycles.
*/
static ca_tasklet g_init_tasklet;
static ca_tasklet g_change_duty_cycle_tasklet;
#if TESTING_MODE
static ca_tasklet g_change_pin_tasklet;
#endif

/*
Keeps track of current state of the demo.
*/
static struct demo_state g_cur_state = {.pin_index = 0, .duty_cycle_index = 0};

#if TESTING_MODE
static ca_error LED_Dimming_ChangePin(void *context)
{
	(void)context;

	// Progress the state: Move to the next pin in the demo.
	g_cur_state.pin_index = (g_cur_state.pin_index + 1) % DEMO_NUM_OF_PINS;

	// Deinitialise the PWM. This is necessary because the
	// PWM needs to be newly initialised when changing pins.
	SENSORIF_PWM_Deinit();

	// Schedule the initialisation of the PWM.
	TASKLET_ScheduleDelta(&g_init_tasklet, 0, NULL);

	return CA_ERROR_SUCCESS;
}
#endif

static ca_error LED_Dimming_SetDutyCycle(void *context)
{
	(void)context;

	ca_error status;
	u32_t    duty_cycle;

	// Progress the state: Move to the next duty cycle in the demo.
	g_cur_state.duty_cycle_index = (g_cur_state.duty_cycle_index + 1) % DEMO_NUM_OF_DUTY_CYCLES;

#if TESTING_MODE
	u8_t pin = g_pins[g_cur_state.pin_index];
#endif
	duty_cycle = g_duty_cycles[g_cur_state.duty_cycle_index];

	// Change the duty cycle of the PWM
	SENSORIF_PWM_SetDutyCycle(duty_cycle);

#if TESTING_MODE
	// Schedule the function which transitions to the next pin, once all duty cycles tested.
	// Otherwise, schedule this function again so that the next duty cycle gets tested.
	if (g_cur_state.duty_cycle_index == DEMO_NUM_OF_DUTY_CYCLES - 1)
	{
		status = TASKLET_ScheduleDelta(&g_change_pin_tasklet, g_demo_speed, NULL);
	}
	else
#endif
	{
		status = TASKLET_ScheduleDelta(&g_change_duty_cycle_tasklet, g_demo_speed, NULL);
	}

	return status;
}

static ca_error LED_Dimming_Init(void *context)
{
	(void)context;

	ca_error status;

#if TESTING_MODE
	u8_t pin = g_pins[g_cur_state.pin_index];
#else
	u8_t pin = 15;
#endif
	u32_t duty_cycle = g_duty_cycles[g_cur_state.duty_cycle_index];

	// Start the PWM with the initial conditions
	if ((status = SENSORIF_PWM_Init(pin, g_freq_hz, duty_cycle)))
		return status;

	// Schedule the change to the next duty cycle
	status = TASKLET_ScheduleDelta(&g_change_duty_cycle_tasklet, g_demo_speed, NULL);

	return status;
}

ca_error LED_Dimming_Initialise(void)
{
	ca_error status;

	// Initialise the tasklets for this demo
	TASKLET_Init(&g_init_tasklet, &LED_Dimming_Init);
	TASKLET_Init(&g_change_duty_cycle_tasklet, &LED_Dimming_SetDutyCycle);
#if TESTING_MODE
	TASKLET_Init(&g_change_pin_tasklet, &LED_Dimming_ChangePin);
#endif

	// Schedule the PWM initialisation
	status = TASKLET_ScheduleDelta(&g_init_tasklet, g_demo_speed, NULL);

	return status;
}
