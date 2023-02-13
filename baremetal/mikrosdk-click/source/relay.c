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
#include "relay_click.h"
#include "relay_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_interface.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static relay_t     relay;
static relay_cfg_t cfg;

/* driver initialisation */
static uint8_t MIKROSDK_RELAY_init(void)
{
	// Output pins

	digital_out_init(&relay.rel2, cfg.rel2);
	digital_out_init(&relay.rel1, cfg.rel1);

	return RELAY_ST_OK;
}

/* modifed set state function for upper levels */
void MIKROSDK_RELAY_set_state(uint8_t num, uint8_t state)
{
	switch (num)
	{
	case RELAY_NUM_1:
	{
		digital_out_write(&relay.rel1, state);
		break;
	}
	case RELAY_NUM_2:
	{
		digital_out_write(&relay.rel2, state);
		break;
	}
	}
}

/* pin mapping function */
void MIKROSDK_RELAY_pin_mapping(uint8_t r1, uint8_t r2)
{
	cfg.rel1 = r1;
	cfg.rel2 = r2;
}

/* device initialisation */
uint8_t MIKROSDK_RELAY_Initialise(void)
{
	if (MIKROSDK_RELAY_init())
		return RELAY_ST_FAIL;

	/* switch off by default */
	digital_out_low(&relay.rel1);
	digital_out_low(&relay.rel2);

	/* for sleepy devices */
	if (BSP_ModuleSetGPIOOutputPermanent(cfg.rel1))
		return RELAY_ST_FAIL;
	if (BSP_ModuleSetGPIOOutputPermanent(cfg.rel2))
		return RELAY_ST_FAIL;

	return RELAY_ST_OK;
}

/* driver function
/* Note: relay_1_state: RELAY_OFF / RELAY_ON */
/* Note: relay_2_state: RELAY_OFF / RELAY_ON */
uint8_t MIKROSDK_RELAY_Driver(uint8_t relay_1_state, uint8_t relay_2_state)
{
	uint8_t status = RELAY_ST_OK;

	MIKROSDK_RELAY_set_state(1, relay_1_state);
	MIKROSDK_RELAY_set_state(2, relay_2_state);

	return status;
}
