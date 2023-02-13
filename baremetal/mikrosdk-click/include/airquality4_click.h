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

#ifndef AIRQUALITY4_CLICK_H
#define AIRQUALITY4_CLICK_H

#include <stdint.h>

/* measure raw signals (H2 and Ethanol) when 1 instead of air quality values (when 0) */
#define AIRQUALITY4_MEASURE_RAW_SIGNALS 0

/* I2C slave address */
#define SGP30_I2C_ADDR 0x58

/* all sgp30 maximum data length */
#define SGP30_MAXDLEN 6
/* I2C slave address + memory address (command) */
#define SGP30_ADDLEN 3

/* timing parameters [ms] */
#define SGP30_T_INIT 17000      /* 17 seconds initialisation time after issuing init_air_quality command */
#define SGP30_T_CAL 43200000    /* 12 hours initial baseline calibration duration */
#define SGP30_T_TEST 250        /* 250 ms for measure_test self test */
#define SGP30_T_MEAS_RAW 25     /* 25 ms measurement time (between i2c write and read) for raw signals */
#define SGP30_T_MEAS_QUAL 12    /* 12 ms measurement time (between i2c write and read) for air quality signals */
#define SGP30_T_MEAS_POWERUP 10 /* 10 ms power-up time */

#define SGP30_T_SLEEP 0xFFFFFFFF /* dummy time to indicate device is in sleep mode */

/* memory address / command definitions */
#define SGP30_CMD_INIT_AIR_QUALITY 0x2003
#define SGP30_CMD_MEASURE_AIR_QUALITY 0x2008
#define SGP30_CMD_GET_BASELINE 0x2015
#define SGP30_CMD_SET_BASELINE 0x201E
#define SGP30_CMD_SET_HUMIDITY 0x2061
#define SGP30_CMD_MEASURE_TEST 0x2032
#define SGP30_CMD_GET_FEATURE_SET_VERSION 0x202F
#define SGP30_CMD_MEASURE_RAW_SIGNALS 0x2050
#define SGP30_CMD_GET_SERIAL_ID 0x3682

/* data acquisition status */
enum airquality4_status
{
	AIRQUALITY4_ST_OK    = 0, /* read values ok */
	AIRQUALITY4_ST_NCAL  = 1, /* baseline not yet calibrated */
	AIRQUALITY4_ST_INIT  = 2, /* initialisating */
	AIRQUALITY4_ST_SLEEP = 3, /* device is in sleep mode */
	AIRQUALITY4_ST_FAIL  = 4  /* acquisition failed */
};

/* new functions */
uint8_t MIKROSDK_AIRQUALITY4_dev_init(void);
uint8_t MIKROSDK_AIRQUALITY4_get_baseline(void);
uint8_t MIKROSDK_AIRQUALITY4_set_baseline(void);
uint8_t MIKROSDK_AIRQUALITY4_soft_reset(void);
uint8_t MIKROSDK_AIRQUALITY4_get_version(uint16_t *version);
uint8_t MIKROSDK_AIRQUALITY4_measure_test(void);
uint8_t MIKROSDK_AIRQUALITY4_measure_raw_signals(uint16_t *value);
uint8_t MIKROSDK_AIRQUALITY4_measure_air_quality(uint16_t *value);
uint8_t MIKROSDK_AIRQUALITY4_Initialise(void);
uint8_t MIKROSDK_AIRQUALITY4_Reinitialise(void);
uint8_t MIKROSDK_AIRQUALITY4_Acquire(uint16_t *co2_h2, uint16_t *tvoc_eth);
uint8_t MIKROSDK_AIRQUALITY4_Powerdown(void);
uint8_t MIKROSDK_AIRQUALITY4_Powerup(void);

#endif // AIRQUALITY4_CLICK_H
