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

#ifndef ENVIRONMENT2_CLICK_H
#define ENVIRONMENT2_CLICK_H

#include <stdint.h>

/* use powerdown/sleep/idle mode between measurements when (1) */
#define ENVIRONMENT2_USE_POWERDOWN 0

/* timing parameters [ms] */
#define SGP40_T_TEST 250    /* sgp40 250 ms for measure_test self test */
#define SGP40_T_MEAS_RAW 30 /* sgp40 30 ms measurement time (between i2c write and read) for raw signals */
#define SHT40_T_MEAS 10     /* sht40 10 ms measurement time (between i2c write and read) for temperature and humidity */
#define SGP40_T_INIT 60000  /* sgp40 60 seconds until reliably detecting voc events */
#define SGP40_T_SETTLING 3600000 /* sgp40 1 hour until specification is met */
#define SGP40_T_POWERUP 170      /* sgp40 power-up (wait between dummy read and read to warm up  heating plate */
/* SGP40_T_POWERUP taken from: https://github.com/Sensirion/arduino-gas-index-algorithm */

#define SGP40_T_SLEEP 0xFFFFFFFF /* dummy time to indicate device is in sleep mode */

/* sgp40 maximum data length */
#define SGP40_MAXDLEN 6
/* sgp40 2C slave address + memory address (command) */
#define SGP40_ADDLEN 3
/* sht40 maximum data length */
#define SHT40_MAXDLEN 6
/* sht40 2C slave address + memory address (command) */
#define SHT40_ADDLEN 2

/* data acquisition status */
enum environment2_status
{
	ENVIRONMENT2_ST_OK       = 0, /* read values ok */
	ENVIRONMENT2_ST_SETTLING = 1, /* settling */
	ENVIRONMENT2_ST_INIT     = 2, /* initialisating */
	ENVIRONMENT2_ST_SLEEP    = 3, /* device is in sleep mode */
	ENVIRONMENT2_ST_FAIL     = 4  /* acquisition failed */
};

/* new functions */
uint8_t MIKROSDK_ENVIRONMENT2_get_temp_hum(int16_t *humidity, int16_t *temperature);
uint8_t MIKROSDK_ENVIRONMENT2_get_air_quality(uint16_t *air_quality_raw);
uint8_t MIKROSDK_ENVIRONMENT2_get_voc_index(int32_t *voc_index);
uint8_t MIKROSDK_ENVIRONMENT2_get_voc_index_with_rh_t(int32_t  *voc_index,
                                                      uint16_t *air_quality_raw,
                                                      int16_t  *relative_humidity,
                                                      int16_t  *temperature);
uint8_t MIKROSDK_ENVIRONMENT2_sgp40_measure_test(void);
uint8_t MIKROSDK_ENVIRONMENT2_sgp40_heater_off(void);
uint8_t MIKROSDK_ENVIRONMENT2_soft_reset(void);
uint8_t MIKROSDK_ENVIRONMENT2_Initialise(void);
uint8_t MIKROSDK_ENVIRONMENT2_Reinitialise(void);
uint8_t MIKROSDK_ENVIRONMENT2_Acquire(int32_t  *voc_index,
                                      uint16_t *air_quality,
                                      int16_t  *humidity,
                                      int16_t  *temperature);
uint8_t MIKROSDK_ENVIRONMENT2_Powerdown(void);
uint8_t MIKROSDK_ENVIRONMENT2_Powerup(void);

#endif // ENVIRONMENT2_CLICK_H
