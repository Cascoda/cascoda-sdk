/*
 * MikroSDK - MikroE Software Development Kit
 * Copyright© 2020 MikroElektronika d.o.o.
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE. 
 */

/*!
 * \file
 *
 */

#include "motion.h"

static motion_t              motion; //Click object.
static motion_cfg_t          cfg;    //Click object.
static motion_detect_state_t motion_state     = MOTION_NO_DETECT;
static motion_detect_state_t motion_old_state = MOTION_DETECTED;

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

void motion_cfg_setup(motion_cfg_t *cfg)
{
	// Additional gpio pins

	cfg->en  = HAL_PIN_NC;
	cfg->out = HAL_PIN_NC;
}

MOTION_RETVAL motion_init(motion_t *ctx, motion_cfg_t *cfg)

{
	// Output pins
	digital_out_init(&ctx->en, cfg->en);

	// Input pins
	digital_in_init(&ctx->out, cfg->out);

	return MOTION_OK;
}

void motion_default_cfg(motion_t *ctx)
{
	// Click default configuration
	motion_set_en_pin(ctx, MOTION_PIN_STATE_HIGH);
}

void motion_set_en_pin(motion_t *ctx, motion_pin_state_t en_state)
{
	digital_out_write(&ctx->en, en_state);
}

motion_detect_state_t motion_get_detected()
{
	uint8_t out_state;

	out_state = (motion_pin_state_t)digital_in_read(&motion.out);

	if (out_state == MOTION_PIN_STATE_LOW)
	{
		motion_state = MOTION_NO_DETECT;
	}
	else if (out_state == MOTION_PIN_STATE_HIGH)
	{
		motion_state = MOTION_DETECTED;
	}

	if (motion_state == MOTION_DETECTED && motion_old_state == MOTION_NO_DETECT)
	{
		motion_old_state = MOTION_DETECTED;
		return MOTION_DETECTED;
	}

	if (motion_old_state == MOTION_DETECTED & motion_state == MOTION_NO_DETECT)
	{
		motion_old_state = MOTION_NO_DETECT;
		return MOTION_NO_DETECT;
	}

	if (motion_state == MOTION_DETECTED)
	{
		return MOTION_PRESENCE;
	}

	if (motion_state == MOTION_NO_DETECT)
	{
		return MOTION_NOT_PRESENCE;
	}

	return 0xFF;
}

void motion_pin_mapping(uint8_t enable, uint8_t output)
{
	cfg.en  = enable;
	cfg.out = output;
}

uint8_t MIKROSDK_MOTION_Initialise(void)
{
	motion_init(&motion, &cfg);
	motion_default_cfg(&motion);

	return 0;
}

// ------------------------------------------------------------------------- END
