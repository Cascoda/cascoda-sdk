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

#ifndef FAN_CLICK_H
#define FAN_CLICK_H

#include <stdint.h>

/* fan control mode */
#define FAN_MODE_OPEN_LOOP 0   /* open loop, direct setting of pwm value */
#define FAN_MODE_CLOSED_LOOP 1 /* closed loop, rpm setting */

#define FAN_MODE FAN_MODE_CLOSED_LOOP

/* use alarm pin as interrupt (1) with continuous mode instead of polling (0) with one-shot mode */
#define FAN_USE_INTERRUPT 0

/* fanspecific max. speed in [rpm] */
#define FAN_MAX_SPEED 5000

/* drive fail alarm limit value in [%] of max speed */
#define FAN_DRIVE_FAIL_LIMIT_PERCENT 10

/* spin-up fail limit (min. valid rpm value */
#define FAN_MIN_SPIN_UP_VALUE_RPM 500

/* i2c slave address */
#define FAN_DEV_ADDR 0x2F

/* product id for EMC2301 */
#define FAN_EMC2301_PRODUCTID 0x37

/* device clock frequency */
#define FAN_FREQUENCY 32768

/* alarm active low */
enum fan_alarm_state
{
	FAN_ALARM_TRIGGERED = 0,
	FAN_ALARM_CLEARED   = 1
};

/* data acquisition status */
enum fan_status
{
	FAN_ST_OK           = 0, /* success */
	FAN_ST_ALARM_DVFAIL = 1, /* alarm has been triggered - driver fail */
	FAN_ST_ALARM_FNSPIN = 2, /* alarm has been triggered - spin-up fail */
	FAN_ST_ALARM_FNSTL  = 3, /* alarm has been triggered - driver stall */
	FAN_ST_FAIL         = 4  /* command failed */
};

/* register addresses */
enum fan_reg_address
{
	FAN_REG_CONFIGURATION     = 0x20,
	FAN_REG_STATUS            = 0x24,
	FAN_REG_STALL_STATUS      = 0x25,
	FAN_REG_SPIN_STATUS       = 0x26,
	FAN_REG_DRIVE_FAIL_STATUS = 0x27,
	FAN_REG_INTERRUPT_ENABLE  = 0x29,
	FAN_REG_PWM_POLARITY      = 0x2A,
	FAN_REG_PWM_OUTPUT_CONFIG = 0x2B,
	FAN_REG_PWM_BASE_FREQ     = 0x2D,
	FAN_REG_SETTING           = 0x30,
	FAN_REG_DIVIDE            = 0x31,
	FAN_REG_CONFIG1           = 0x32,
	FAN_REG_CONFIG2           = 0x33,
	FAN_REG_GAIN              = 0x35,
	FAN_REG_SPINUP            = 0x36,
	FAN_REG_MAX_STEP          = 0x37,
	FAN_REG_MIN_DRIVE         = 0x38,
	FAN_REG_VALID_TACH        = 0x39,
	FAN_REG_FAIL_LOW          = 0x3A,
	FAN_REG_FAIL_HIGH         = 0x3B,
	FAN_REG_TACH_TARGET_LOW   = 0x3C,
	FAN_REG_TACH_TARGET_HIGH  = 0x3D,
	FAN_REG_TACH_READING_HIGH = 0x3E,
	FAN_REG_TACH_READING_LOW  = 0x3F,
	FAN_REG_SOFTWARE_LOCK     = 0xEF,
	FAN_REG_PRODUCT_ID        = 0xFD,
	FAN_REG_MANUFACTUERE_ID   = 0xFE,
	FAN_REG_REVISION          = 0xFF,
};

/* register content definitions */

/* status register bits */
#define FAN_STATUS_FNSTL 0x01
#define FAN_STATUS_FNSPIN 0x02
#define FAN_STATUS_DVFAIL 0x04

/* pwm base frequency */
enum fan_pwm_frequency
{
	FAN_PWM_FREQ_26000HZ = 0x00,
	FAN_PWM_FREQ_19531HZ = 0x01,
	FAN_PWM_FREQ_4882HZ  = 0x02,
	FAN_PWM_FREQ_2441HZ  = 0x03,
};

/* minimum fan speed range / tachometer count multiplier */
enum fan_range
{
	FAN_RANGE_RPM_MIN_500  = 0x00, //  500 rpm min., multiplier = 1
	FAN_RANGE_RPM_MIN_1000 = 0x01, // 1000 rpm min., multiplier = 2
	FAN_RANGE_RPM_MIN_2000 = 0x02, // 2000 rpm min., multiplier = 4
	FAN_RANGE_RPM_MIN_4000 = 0x03, // 4000 rpm min., multiplier = 8
};

/* number of edges to sample depending on poles */
enum fan_edges
{
	FAN_EDGES_1_POLE = 0x00, // 3 for 1 pole
	FAN_EDGES_2_POLE = 0x01, // 5 for 2 poles
	FAN_EDGES_3_POLE = 0x02, // 7 for 3 poles
	FAN_EDGES_4_POLE = 0x03, // 9 for 4 poles
};

/* closed loop algorithm update time */
enum fan_update_time
{
	FAN_UPDATE_TIME_100MS  = 0x00,
	FAN_UPDATE_TIME_200MS  = 0x01,
	FAN_UPDATE_TIME_300MS  = 0x02,
	FAN_UPDATE_TIME_400MS  = 0x03,
	FAN_UPDATE_TIME_500MS  = 0x04,
	FAN_UPDATE_TIME_800MS  = 0x05,
	FAN_UPDATE_TIME_1200MS = 0x06,
	FAN_UPDATE_TIME_1600MS = 0x07,
};

/* closed loop algorithm derivative option */
enum fan_derivative_option
{
	FAN_DERIVATIVE_NONE  = 0x00,
	FAN_DERIVATIVE_BASIC = 0x01,
	FAN_DERIVATIVE_STEP  = 0x02,
	FAN_DERIVATIVE_BOTH  = 0x03,
};

/* closed loop algorithm error window */
enum fan_error_window
{
	FAN_ERR_WINDOW_0RPM   = 0x00,
	FAN_ERR_WINDOW_50RPM  = 0x01,
	FAN_ERR_WINDOW_100RPM = 0x02,
	FAN_ERR_WINDOW_200RPM = 0x03,
};

/* closed loop algorithm pid gains */
enum fan_gain
{
	FAN_GAIN_1X = 0x00,
	FAN_GAIN_2X = 0x01,
	FAN_GAIN_4X = 0x02,
	FAN_GAIN_8X = 0x03,
};

/* spin up drive fail count */
enum fan_drive_fail_count
{
	FAN_DRIVE_FAIL_COUNT_DISABLED = 0x00,
	FAN_DRIVE_FAIL_COUNT_16       = 0x01,
	FAN_DRIVE_FAIL_COUNT_32       = 0x02,
	FAN_DRIVE_FAIL_COUNT_64       = 0x03,
};

/* spin up level */
enum fan_spin_up_level
{
	FAN_SPINUP_LEVEL_30PERCENT = 0x00,
	FAN_SPINUP_LEVEL_35PERCENT = 0x01,
	FAN_SPINUP_LEVEL_40PERCENT = 0x02,
	FAN_SPINUP_LEVEL_45PERCENT = 0x03,
	FAN_SPINUP_LEVEL_50PERCENT = 0x04,
	FAN_SPINUP_LEVEL_55PERCENT = 0x05,
	FAN_SPINUP_LEVEL_60PERCENT = 0x06,
	FAN_SPINUP_LEVEL_65PERCENT = 0x07,
};

/* spin up time */
enum fan_spin_up_time
{
	FAN_SPINUP_TIME_250MS = 0x00,
	FAN_SPINUP_TIME_500MS = 0x01,
	FAN_SPINUP_TIME_1S    = 0x02,
	FAN_SPINUP_TIME_2S    = 0x03,
};

/* default values */
#define FAN_PWM_FREQ FAN_PWM_FREQ_26000HZ            /* pwm base frequency */
#define FAN_RANGE FAN_RANGE_RPM_MIN_500              /* fan speed range */
#define FAN_EDGES FAN_EDGES_2_POLE                   /* number of edges for sampling */
#define FAN_SPINUP_LEVEL FAN_SPINUP_LEVEL_60PERCENT  /* spin-up level */
#define FAN_SPINUP_TIME FAN_SPINUP_TIME_1S           /* spin-up time */
#define FAN_KICK 1                                   /* kick */
#define FAN_UPDATE_TIME FAN_UPDATE_TIME_400MS        /* update time */
#define FAN_ERR_WINDOW FAN_ERR_WINDOW_0RPM           /* fan speed error window */
#define FAN_DERIVATIVE FAN_DERIVATIVE_BASIC          /* pid algorithm derivative option */
#define FAN_GAIN_P FAN_GAIN_4X                       /* pid p gain */
#define FAN_GAIN_I FAN_GAIN_4X                       /* pid i gain */
#define FAN_GAIN_D FAN_GAIN_4X                       /* pid d gain */
#define FAN_DRIVE_FAIL_COUNT FAN_DRIVE_FAIL_COUNT_16 /* drive fail count */
#define FAN_RAMP_ENABLE 0                            /* ramping enable */
#define FAN_RAMP_STEPSIZE 16                         /* ramping stepsize */

/* helpers for updating registers */
/* FAN_REG_CONFIG1 */
#define FAN_ENAG_BIT 0x80
#define FAN_RNG_MASK 0x60
#define FAN_RNG_SHFT 5
#define FAN_EDG_MASK 0x18
#define FAN_EDG_SHFT 3
#define FAN_UDT_MASK 0x07
#define FAN_UDT_SHFT 0
/* FAN_REG_CONFIG2 */
#define FAN_ENRC_BIT 0x40
#define FAN_GHEN_BIT 0x20
#define FAN_DPT_MASK 0x18
#define FAN_DPT_SHFT 3
#define FAN_ERG_MASK 0x06
#define FAN_ERG_SHFT 1
/* FAN_REG_SPINUP */
#define FAN_DFC_MASK 0xC0
#define FAN_DFC_SHFT 6
#define FAN_NKCK_BIT 0x20
#define FAN_SPLV_MASK 0x1C
#define FAN_SPLV_SHFT 2
#define FAN_SPT_MASK 0x03
#define FAN_SPT_SHFT 0
/* FAN_REG_GAIN */
#define FAN_GDE_MASK 0x30
#define FAN_GDE_SHFT 4
#define FAN_GIN_MASK 0x0C
#define FAN_GIN_SHFT 2
#define FAN_GPR_MASK 0x03
#define FAN_GPR_SHFT 0

/* new functions */
uint8_t MIKROSDK_FAN_Initialise(void);
void    MIKROSDK_FAN_pin_mapping(uint8_t alarm);
uint8_t MIKROSDK_FAN_alarm_triggered(void);
uint8_t MIKROSDK_FAN_Driver(uint8_t *speed_pwm_percent, uint16_t *speed_tach_rpm);

#endif // FAN_CLICK_H
