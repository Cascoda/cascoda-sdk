/**
 * @file
 * @brief mikrosdk interface
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
 * Example click interface driver
*/

/* include <device>_drv.h and <device>_click.h */
#include "motion_click.h"
#include "motion_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* flag if alarm / interrupt has been triggered */
static uint8_t motion_alarm = 0;

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static motion_t     motion;
static motion_cfg_t cfg;

/* declare static/global variables */
static motion_detect_state_changes_t motion_state = MOTION_CH_NO_DETECT;

/* ISR for irq handling */
static int Motion_isr(void)
{
	/* set the motion_alarm to active actions further up */
	motion_alarm = 1;
	return 0;
}

/* enable interrupt function - this has to bypass mikrosdk driver and hal */
static uint8_t MIKROSDK_MOTION_EnableInterrupt(uint8_t irq_pin)
{
	uint8_t                status;
	struct gpio_input_args args;

	status = BSP_ModuleDeregisterGPIOPin(irq_pin);
	if (!status)
	{
		args.mpin     = irq_pin;
		args.pullup   = MODULE_PIN_PULLUP_OFF;
		args.debounce = MODULE_PIN_DEBOUNCE_ON;
		args.irq      = MODULE_PIN_IRQ_BOTH;
		args.callback = Motion_isr;
		status        = BSP_ModuleRegisterGPIOInput(&args);
	}

	return (status);
}

/* driver initialisation */
static uint8_t MIKROSDK_MOTION_init()
{
	// Output pins
	digital_out_init(&motion.en, cfg.en);
	// Input pins
	digital_in_init(&motion.out, cfg.out);

	return MOTION_ST_OK;
}

/* has has alarm/interrupt been triggered */
uint8_t MIKROSDK_MOTION_alarm_triggered(void)
{
	uint8_t alarm = motion_alarm;
	motion_alarm  = 0;
	return (alarm);
}

/* new function expanding original detection state from motion_detect_state_t to motion_detect_state_changes_t */
motion_detect_state_changes_t MIKROSDK_MOTION_get_detected(void)
{
	static motion_detect_state_changes_t motion_old_state = MOTION_CH_DETECTED;
	uint8_t                              out_state;

	out_state = (motion_pin_state_t)digital_in_read(&motion.out);

	if (out_state == MOTION_PIN_STATE_LOW)
	{
		motion_state = MOTION_CH_NO_DETECT;
	}
	else if (out_state == MOTION_PIN_STATE_HIGH)
	{
		motion_state = MOTION_CH_DETECTED;
	}

	if (motion_state == MOTION_CH_DETECTED && motion_old_state == MOTION_CH_NO_DETECT)
	{
		motion_old_state = MOTION_CH_DETECTED;
		return MOTION_CH_DETECTED;
	}

	if (motion_old_state == MOTION_CH_DETECTED & motion_state == MOTION_CH_NO_DETECT)
	{
		motion_old_state = MOTION_CH_NO_DETECT;
		return MOTION_CH_NO_DETECT;
	}

	if (motion_state == MOTION_CH_DETECTED)
	{
		return MOTION_CH_PRESENT;
	}

	if (motion_state == MOTION_CH_NO_DETECT)
	{
		return MOTION_CH_NOT_PRESENT;
	}

	return MOTION_CH_ERROR;
}

/* additional enable function for sleep */
void MIKROSDK_MOTION_Enable(void)
{
	digital_out_write(&motion.en, MOTION_PIN_STATE_HIGH);
}

/* additional disable function for sleep */
void MIKROSDK_MOTION_Disable(void)
{
	digital_out_write(&motion.en, MOTION_PIN_STATE_LOW);
}

/* pin mapping function */
void MIKROSDK_MOTION_pin_mapping(uint8_t enable, uint8_t output)
{
	cfg.en  = enable;
	cfg.out = output;
}

/* device initialisation */
uint8_t MIKROSDK_MOTION_Initialise(void)
{
	if (MIKROSDK_MOTION_init())
		return MOTION_ST_FAIL;

	MIKROSDK_MOTION_Enable();

	/* for sleepy devices */
	if (BSP_ModuleSetGPIOOutputPermanent(cfg.en))
		return MOTION_ST_FAIL;

/* enable interrupt */
#if (MOTION_USE_INTERRUPT)
	if (MIKROSDK_MOTION_EnableInterrupt(cfg.out))
		return MOTION_ST_FAIL;
#endif

	return MOTION_ST_OK;
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_MOTION_Reinitialise(void)
{
	MIKROSDK_MOTION_Enable();

	WAIT_ms(MOTION_T_TONMIN);

	return MOTION_ST_OK;
}

/* data acquisition */
/* Note: detection_state: MOTION_CH_NO_DETECT / MOTION_CH_DETECTED / MOTION_CH_NOT_PRESENT / MOTION_CH_PRESENT */
/* Note: detection_time: [ms] only used if interrupt driven, otherwise 0 */
uint8_t MIKROSDK_MOTION_Acquire(uint8_t *detection_state, uint32_t *detection_time)
{
	motion_detect_state_changes_t state;
#if (MOTION_USE_INTERRUPT)
	static u32_t det_time = 0;
#endif

#if (!MOTION_USE_INTERRUPT)
	MIKROSDK_MOTION_Enable();
#endif

	state = MIKROSDK_MOTION_get_detected();
	if (state == MOTION_CH_ERROR)
		return MOTION_ST_FAIL;

#if (!MOTION_USE_INTERRUPT)
	MIKROSDK_MOTION_Disable();
#else
	if (state == MOTION_CH_DETECTED)
		det_time = TIME_ReadAbsoluteTime();
	else if ((state == MOTION_CH_NO_DETECT) && (det_time > 0))
		det_time = TIME_ReadAbsoluteTime() - det_time;
#endif

	*detection_state = state;
#if (MOTION_USE_INTERRUPT)
	*detection_time = det_time;
#else
	*detection_time = 0;
#endif

	return MOTION_ST_OK;
}
