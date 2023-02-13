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

#ifndef THERMO3_CLICK_H
#define THERMO3_CLICK_H

#include <stdint.h>

/* use alarm pin as interrupt (1) with continuous mode instead of polling (0) with one-shot mode */
#define THERMO3_USE_INTERRUPT 0

/* all tmp102 registers are 2 bytes */
#define TMP102_REGLEN 2
/* I2C slave address + register address */
#define TMP102_ADDLEN 2

/* tmp102 i2c address */
#define TMP102_I2C_ADDR 0x48

/* register addresses */
#define MIKROSDK_THERMO3_REGADD_TEMP 0x00
#define MIKROSDK_THERMO3_REGADD_CONFIG 0x01
#define MIKROSDK_THERMO3_REGADD_TLOW 0x02
#define MIKROSDK_THERMO3_REGADD_THIGH 0x03

/* configuration register bit mapping */
#define MIKROSDK_THERMO3_CONFIG_SHUTDOWN 0x01
#define MIKROSDK_THERMO3_CONFIG_TMMODE 0x02
#define MIKROSDK_THERMO3_CONFIG_ONESHOT 0x80

/* configuration register byte 1 default */
#define MIKROSDK_THERMO3_DEFAULT_CONFIG_0 0x60
/* configuration register byte 2 default */
#define MIKROSDK_THERMO3_DEFAULT_CONFIG_1 0xA0

#if (THERMO3_USE_INTERRUPT)
/* continuous mode */
#define MIKROSDK_THERMO3_CONFIG \
	((MIKROSDK_THERMO3_DEFAULT_CONFIG_1 << 8) + (MIKROSDK_THERMO3_DEFAULT_CONFIG_0 | MIKROSDK_THERMO3_CONFIG_TMMODE))
#else
/* one-shot mode */
#define MIKROSDK_THERMO3_CONFIG                                                                                        \
	((MIKROSDK_THERMO3_DEFAULT_CONFIG_1 << 8) + (MIKROSDK_THERMO3_DEFAULT_CONFIG_0 | MIKROSDK_THERMO3_CONFIG_ONESHOT | \
	                                             MIKROSDK_THERMO3_CONFIG_TMMODE | MIKROSDK_THERMO3_CONFIG_SHUTDOWN))
#endif

/* default temperature limits (in T['C] * 16) */
#define MIKROSDK_THERMO3_TEMP_LIMIT_LOW 0x01B0  /* +27.0000 'C */
#define MIKROSDK_THERMO3_TEMP_LIMIT_HIGH 0x01C8 /* +28.5000 'C */

/* timing parameters [ms] */
#define THERMO3_T_POWERUP 35 /* tmp102 35 ms power-up (conversion time max) */

/* alarm active low */
enum thermo3_alarm_state
{
	THERMO3_ALARM_TRIGGERED = 0,
	THERMO3_ALARM_CLEARED   = 1
};

/* data acquisition status */
enum thermo3_status
{
	THERMO3_ST_OK              = 0, /* read values ok */
	THERMO3_ST_ALARM_CLEARED   = 1, /* alarm has been cleared (values ok) */
	THERMO3_ST_ALARM_TRIGGERED = 2, /* alarm has been triggered (values ok) */
	THERMO3_ST_FAIL            = 3  /* acquisition failed */
};

/* new functions */
uint8_t MIKROSDK_THERMO3_get_config(uint16_t *configuration);
uint8_t MIKROSDK_THERMO3_set_config(uint16_t config);
uint8_t MIKROSDK_THERMO3_get_temperature_limits(uint16_t *temp_limit_low, uint16_t *temp_limit_high);
uint8_t MIKROSDK_THERMO3_set_temperature_limits(uint16_t temp_limit_low, uint16_t temp_limit_high);
uint8_t MIKROSDK_THERMO3_get_alarm(void);
uint8_t MIKROSDK_THERMO3_alarm_triggered(void);
uint8_t MIKROSDK_THERMO3_get_temperature(uint16_t *temperature);
void    MIKROSDK_THERMO3_pin_mapping(uint8_t alarm);
uint8_t MIKROSDK_THERMO3_Initialise(void);
uint8_t MIKROSDK_THERMO3_Reinitialise(void);
uint8_t MIKROSDK_THERMO3_Acquire(uint16_t *temperature);

#endif // THERMO3_CLICK_H
