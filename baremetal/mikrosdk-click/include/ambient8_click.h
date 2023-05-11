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

#ifndef AMBIENT8_CLICK_H
#define AMBIENT8_CLICK_H

#include <stdint.h>

/* master measurement (read access) modes */
enum ltr303als_mode
{
	LTR303ALS_MODE_ONE_SHOT,   /* one-shot   conversion - sensor in standby mode otherwise */
	LTR303ALS_MODE_CONTINUOUS, /* continuous conversion - sensor always active */
};

/* measurement mode */
#define LTR303ALS_MODE LTR303ALS_MODE_ONE_SHOT

/* data acquisition status */
enum ambient8_status
{
	AMBIENT8_ST_OK      = 0, /* read values ok */
	AMBIENT8_ST_FAIL    = 1, /* acquisition failed */
	AMBIENT8_ST_INVALID = 2  /* data invalid / not ready */
};

/* i2c address */
#define AMBIENT8_DEVICE_ADDRESS 0x29

/* timing parameters [ms] */
#define LTR303ALS_T_ACTIVE 10   /* time [ms] before data after setting active mode */
#define LTR303ALS_T_POWERUP 100 /* time [ms] before data after power-up */

/* part id and manufacturer id */
#define LTR303ALS_PARTID 0xA0
#define LTR303ALS_MANFID 0x05

/* register addresses */
enum sif_ltr303als_reg_address
{
	LTR303ALS_REG_CONTROL     = 0x80,
	LTR303ALS_REG_MEAS_RATE   = 0x85,
	LTR303ALS_REG_PART_ID     = 0x86,
	LTR303ALS_REG_MANUFAC_ID  = 0x87,
	LTR303ALS_REG_DATA_CH1_0  = 0x88,
	LTR303ALS_REG_DATA_CH1_1  = 0x89,
	LTR303ALS_REG_DATA_CH0_0  = 0x8A,
	LTR303ALS_REG_DATA_CH0_1  = 0x8B,
	LTR303ALS_REG_STATUS      = 0x8C,
	LTR303ALS_REG_INTERRUPT   = 0x8F,
	LTR303ALS_REG_THRES_UP_0  = 0x97,
	LTR303ALS_REG_THRES_UP_1  = 0x98,
	LTR303ALS_REG_THRES_LOW_0 = 0x99,
	LTR303ALS_REG_THRES_LOW_1 = 0x9A,
	LTR303ALS_REG_INT_PERS    = 0x9E
};

/* ALS control register (already bitshifted) */
#define LTR303ALS_MODE_STANDBY 0x00
#define LTR303ALS_MODE_ACTIVE 0x01
#define LTR303ALS_SW_RESET 0x02
#define LTR303ALS_GAIN_1X 0x00
#define LTR303ALS_GAIN_2X 0x04
#define LTR303ALS_GAIN_4X 0x08
#define LTR303ALS_GAIN_8X 0x0C
#define LTR303ALS_GAIN_48X 0x18
#define LTR303ALS_GAIN_96X 0x1C

/* ALS status register (already bitshifted) */
#define LTR303ALS_DATA_VALID 0x80
#define LTR303ALS_DATA_STATUS 0x04

/* ALS measurement rate register (already bitshifted) */
/* measurement time (period) [ms] (default 500) */
#define LTR303ALS_TMEAS_50 0x00
#define LTR303ALS_TMEAS_100 0x01
#define LTR303ALS_TMEAS_200 0x02
#define LTR303ALS_TMEAS_500 0x03
#define LTR303ALS_TMEAS_1000 0x04
#define LTR303ALS_TMEAS_2000 0x05
/* integration time [ms] (default 100) */
#define LTR303ALS_TINT_50 0x08
#define LTR303ALS_TINT_100 0x00
#define LTR303ALS_TINT_150 0x20
#define LTR303ALS_TINT_200 0x10
#define LTR303ALS_TINT_250 0x28
#define LTR303ALS_TINT_300 0x30
#define LTR303ALS_TINT_350 0x38
#define LTR303ALS_TINT_400 0x18

/* default gain, measurement time and integration period */
#define LTR303ALS_GAIN 1    // gain, 1, 2, 4, 8, 48, 96
#define LTR303ALS_TINT 100  // integration time [ms]: 50, 100, 150, 200, 250, 300, 350, 400
#define LTR303ALS_TMEAS 200 // measurement time [ms]: 50, 100, 200, 500, 1000, 2000

/* new functions */
uint8_t MIKROSDK_AMBIENT8_Initialise(void);
uint8_t MIKROSDK_AMBIENT8_Reinitialise(void);
uint8_t MIKROSDK_AMBIENT8_Reconfigure(uint16_t meastime, uint16_t inttime, uint8_t gain);
uint8_t MIKROSDK_AMBIENT8_Acquire(uint32_t *illuminance_ch0, uint32_t *illuminance_ch1, uint32_t *illuminance_ambient);

#endif // AMBIENT8_CLICK_H
