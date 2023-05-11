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

#include <stdio.h>

/* include <device>_drv.h and <device>_click.h */
#include "fan_click.h"
#include "fan_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static fan_t     fan;
static fan_cfg_t cfg;

/* flag if alarm / interrupt has been triggered */
static uint8_t fan_alarm = 0;

/* static (local) functions */
static int      fan_isr(void);
static uint8_t  fan_enable_interrupt(uint8_t irq_pin);
static uint8_t  MIKROSDK_FAN_init(void);
static uint8_t  MIKROSDK_FAN_write_register(uint8_t reg_addr, uint8_t data);
static uint8_t  MIKROSDK_FAN_read_register(uint8_t reg_addr, uint8_t *data);
static uint8_t  MIKROSDK_FAN_convert_range_to_multiplier(enum fan_range range);
static uint8_t  MIKROSDK_FAN_convert_edge_cfg_to_edges(enum fan_edges edge_cfg);
static uint8_t  MIKROSDK_FAN_convert_edge_cfg_to_poles(enum fan_edges edge_cfg);
static uint16_t MIKROSDK_FAN_convert_rpm_count(uint16_t inval);
static uint8_t  MIKROSDK_FAN_get_alarm(void);
static uint8_t  MIKROSDK_FAN_software_lock(uint8_t lock);
static uint8_t  MIKROSDK_FAN_set_pwm_base_freq(enum fan_pwm_frequency freq);
static uint8_t  MIKROSDK_FAN_set_pwm_duty_cycle(uint8_t percentage);
static uint8_t  MIKROSDK_FAN_get_pwm_duty_cycle(uint8_t *percentage);
static uint8_t  MIKROSDK_FAN_set_tach_target_value(uint16_t rpm);
static uint8_t  MIKROSDK_FAN_get_tach_value(uint16_t *rpm);
static uint8_t  MIKROSDK_FAN_get_status(void);
static uint8_t  MIKROSDK_FAN_set_mode(uint8_t mode);
static uint8_t  MIKROSDK_FAN_config_basic(enum fan_range range, enum fan_edges edges);
static uint8_t  MIKROSDK_FAN_config_spin_up(enum fan_spin_up_level level, enum fan_spin_up_time time, uint8_t kick);
static uint8_t  MIKROSDK_FAN_config_algorithm(enum fan_update_time       updtime,
                                              enum fan_error_window      errwin,
                                              enum fan_derivative_option doption,
                                              enum fan_gain              gain_p,
                                              enum fan_gain              gain_i,
                                              enum fan_gain              gain_d);
static uint8_t  MIKROSDK_FAN_config_alerts(enum fan_drive_fail_count dfc,
                                           uint8_t                   percent_max_speed,
                                           uint16_t                  min_spin_up_rpm);
static uint8_t  MIKROSDK_FAN_config_ramping(uint8_t enable, uint8_t stepsize);

/* ISR for irq handling */
static int fan_isr(void)
{
	/* set the motion_alarm to active actions further up */
	fan_alarm = 1;
	return 0;
}

/* enable interrupt function - this has to bypass mikrosdk driver and hal */
static uint8_t fan_enable_interrupt(uint8_t irq_pin)
{
	uint8_t                status;
	struct gpio_input_args args;

	status = BSP_ModuleDeregisterGPIOPin(irq_pin);
	if (!status)
	{
		args.mpin     = irq_pin;
		args.pullup   = MODULE_PIN_PULLUP_OFF;
		args.debounce = MODULE_PIN_DEBOUNCE_ON;
		args.irq      = MODULE_PIN_IRQ_FALL;
		args.callback = fan_isr;
		status        = BSP_ModuleRegisterGPIOInput(&args);
	}

	return (status);
}

/* driver initialisation */
static uint8_t MIKROSDK_FAN_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg.i2c_speed;
	i2c_cfg.scl   = cfg.scl;
	i2c_cfg.sda   = cfg.sda;

	fan.slave_address = cfg.i2c_address;

	if (i2c_master_open(&fan.i2c, &i2c_cfg) == I2C_MASTER_ERROR)
		return FAN_ST_FAIL;

	i2c_master_set_slave_address(&fan.i2c, fan.slave_address);
	i2c_master_set_speed(&fan.i2c, cfg.i2c_speed);

	/* Input pins */
	digital_in_init(&fan.int_pin, cfg.int_pin);

	return (FAN_ST_OK);
}

/* register write */
static uint8_t MIKROSDK_FAN_write_register(uint8_t reg_addr, uint8_t data)
{
	uint8_t wrbuf[3];

	wrbuf[0] = fan.slave_address;
	wrbuf[1] = reg_addr;
	wrbuf[2] = data;

	if (i2c_master_write(&fan.i2c, wrbuf, 3))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* register read */
static uint8_t MIKROSDK_FAN_read_register(uint8_t reg_addr, uint8_t *data)
{
	uint8_t wrbuf[2];

	wrbuf[0] = fan.slave_address;
	wrbuf[1] = reg_addr;
	if (i2c_master_write_then_read(&fan.i2c, wrbuf, 2, data, 1))
		return (FAN_ST_FAIL);
	return (FAN_ST_OK);
}

/* conversion of range to tachometer count multiplier */
static uint8_t MIKROSDK_FAN_convert_range_to_multiplier(enum fan_range range)
{
	uint8_t mult;

	if (range == FAN_RANGE_RPM_MIN_4000)
		mult = 8;
	if (range == FAN_RANGE_RPM_MIN_2000)
		mult = 4;
	if (range == FAN_RANGE_RPM_MIN_1000)
		mult = 2;
	else /* range == FAN_RANGE_RPM_MIN_500 */
		mult = 1;

	return (mult);
}

/* conversion of edge configuration to number of edges */
static uint8_t MIKROSDK_FAN_convert_edge_cfg_to_edges(enum fan_edges edge_cfg)
{
	uint8_t edges;

	if (edge_cfg == FAN_EDGES_4_POLE)
		edges = 9;
	if (edge_cfg == FAN_EDGES_3_POLE)
		edges = 7;
	if (edge_cfg == FAN_EDGES_2_POLE)
		edges = 5;
	else /* range == FAN_EDGES_1_POLE */
		edges = 3;

	return (edges);
}

/* conversion of edge configuration to number of poles */
static uint8_t MIKROSDK_FAN_convert_edge_cfg_to_poles(enum fan_edges edge_cfg)
{
	uint8_t poles;

	if (edge_cfg == FAN_EDGES_4_POLE)
		poles = 4;
	if (edge_cfg == FAN_EDGES_3_POLE)
		poles = 3;
	if (edge_cfg == FAN_EDGES_2_POLE)
		poles = 2;
	else /* range == FAN_EDGES_1_POLE */
		poles = 1;

	return (poles);
}

/* conversion of rpm to count and reverse (same formula) */
static uint16_t MIKROSDK_FAN_convert_rpm_count(uint16_t inval)
{
	uint32_t outval;
	uint8_t  n, m, p;

	/* rpm to count */
	p = MIKROSDK_FAN_convert_edge_cfg_to_poles(FAN_EDGES);
	n = MIKROSDK_FAN_convert_edge_cfg_to_edges(FAN_EDGES);
	m = MIKROSDK_FAN_convert_range_to_multiplier(FAN_RANGE);
	if ((inval == 0) || (p == 0))
		outval = 0xFFFF;
	else
		outval = ((n - 1) * m * FAN_FREQUENCY * 60) / ((uint32_t)(p * inval));

	return ((uint16_t)(outval & 0xFFFF));
}

/* get alarm pin status */
static uint8_t MIKROSDK_FAN_get_alarm(void)
{
	uint8_t alarm = 0;

	digital_in_t apin = fan.int_pin;
	alarm             = digital_in_read(&apin);
	return alarm;
}

/* lock device registers (make read-only) */
static uint8_t MIKROSDK_FAN_software_lock(uint8_t lock)
{
	uint8_t lockreg;

	if (lock)
		lockreg = 0x01;
	else
		lockreg = 0x00;

	if (MIKROSDK_FAN_write_register(FAN_REG_SOFTWARE_LOCK, lockreg))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* set pwm base frequency */
static uint8_t MIKROSDK_FAN_set_pwm_base_freq(enum fan_pwm_frequency freq)
{
	if (MIKROSDK_FAN_write_register(FAN_REG_PWM_BASE_FREQ, (uint8_t)(freq)))
		return (FAN_ST_FAIL);
	return (FAN_ST_OK);
}

/* set fan speed (pwm) in [%] */
static uint8_t MIKROSDK_FAN_set_pwm_duty_cycle(uint8_t percentage)
{
	uint16_t pwm_setting;

	pwm_setting = (percentage * 255) / 100;

	if (MIKROSDK_FAN_write_register(FAN_REG_SETTING, (uint8_t)(pwm_setting & 0xFF)))
		return (FAN_ST_FAIL);
	return (FAN_ST_OK);
}

/* get fan speed (pwm) in [%] */
static uint8_t MIKROSDK_FAN_get_pwm_duty_cycle(uint8_t *percentage)
{
	uint8_t  dbyte;
	uint16_t pwm_setting_percent;

	if (MIKROSDK_FAN_read_register(FAN_REG_SETTING, &dbyte))
		return (FAN_ST_FAIL);

	pwm_setting_percent = (dbyte * 100) / 255;

	*percentage = (uint8_t)pwm_setting_percent;

	return (FAN_ST_OK);
}

/* set tachometer target value */
static uint8_t MIKROSDK_FAN_set_tach_target_value(uint16_t rpm)
{
	uint16_t count;
	uint8_t  dbyte;

	/* rpm to count */
	count = MIKROSDK_FAN_convert_rpm_count(rpm);

	/* limit to 13 bit and shift left 3 */
	count = (count & 0x1FFF) << 3;

	dbyte = (uint8_t)((count >> 0) & 0xFF);
	if (MIKROSDK_FAN_write_register(FAN_REG_TACH_TARGET_LOW, dbyte))
		return (FAN_ST_FAIL);

	dbyte = (uint8_t)((count >> 8) & 0xFF);
	if (MIKROSDK_FAN_write_register(FAN_REG_TACH_TARGET_HIGH, dbyte))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* get tachometer value */
static uint8_t MIKROSDK_FAN_get_tach_value(uint16_t *rpm)
{
	uint8_t  dbyte;
	uint16_t count;
	uint32_t rpm32;
	uint8_t  n, m, p;

	if (MIKROSDK_FAN_read_register(FAN_REG_TACH_READING_HIGH, &dbyte))
		return (FAN_ST_FAIL);

	count = (dbyte << 8);

	if (MIKROSDK_FAN_read_register(FAN_REG_TACH_READING_LOW, &dbyte))
		return (FAN_ST_FAIL);

	count += dbyte;
	count >>= 3;

	/* count to rpm */
	rpm32 = MIKROSDK_FAN_convert_rpm_count(count);

	*rpm = (uint16_t)rpm32;

	return (FAN_ST_OK);
}

/* get fan status */
static uint8_t MIKROSDK_FAN_get_status(void)
{
	uint8_t dbyte;
	uint8_t dummy;

	if (MIKROSDK_FAN_read_register(FAN_REG_STATUS, &dbyte))
		return (FAN_ST_FAIL);

	/* clear alarms */
	if (dbyte)
	{
		if (MIKROSDK_FAN_read_register(FAN_REG_STALL_STATUS, &dummy))
			return (FAN_ST_FAIL);
		if (MIKROSDK_FAN_read_register(FAN_REG_SPIN_STATUS, &dummy))
			return (FAN_ST_FAIL);
		if (MIKROSDK_FAN_read_register(FAN_REG_DRIVE_FAIL_STATUS, &dummy))
			return (FAN_ST_FAIL);
	}

	if (dbyte & FAN_STATUS_FNSTL)
	{
		return (FAN_ST_ALARM_FNSTL);
	}
	else if (dbyte & FAN_STATUS_FNSPIN)
	{
		return (FAN_ST_ALARM_FNSPIN);
	}
	else if (dbyte & FAN_STATUS_DVFAIL)
	{
		return (FAN_ST_ALARM_DVFAIL);
	}

	return (FAN_ST_OK);
}

/* set control mode: */
/* 0: open loop (direct pwm) */
/* 1: closed loop rpm control (fsg) */
static uint8_t MIKROSDK_FAN_set_mode(uint8_t mode)
{
	uint8_t regval;

	if (MIKROSDK_FAN_read_register(FAN_REG_CONFIG1, &regval))
		return (FAN_ST_FAIL);

	if (mode)
		regval |= FAN_ENAG_BIT;
	else
		regval &= ~FAN_ENAG_BIT;

	if (MIKROSDK_FAN_write_register(FAN_REG_CONFIG1, regval))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* configure fan basic configuration */
static uint8_t MIKROSDK_FAN_config_basic(enum fan_range range, enum fan_edges edges)
{
	uint8_t regval;

	if (MIKROSDK_FAN_read_register(FAN_REG_CONFIG1, &regval))
		return (FAN_ST_FAIL);

	regval = (regval & (~FAN_RNG_MASK)) | (uint8_t)(range << FAN_RNG_SHFT);
	regval = (regval & (~FAN_EDG_MASK)) | (uint8_t)(edges << FAN_EDG_SHFT);

	if (MIKROSDK_FAN_write_register(FAN_REG_CONFIG1, regval))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* configure spin up */
static uint8_t MIKROSDK_FAN_config_spin_up(enum fan_spin_up_level level, enum fan_spin_up_time time, uint8_t kick)
{
	uint8_t regval;

	if (MIKROSDK_FAN_read_register(FAN_REG_SPINUP, &regval))
		return (FAN_ST_FAIL);

	regval = (regval & (~FAN_SPLV_MASK)) | (uint8_t)(level << FAN_SPLV_SHFT);
	regval = (regval & (~FAN_SPT_MASK)) | (uint8_t)(time << FAN_SPT_SHFT);

	if (kick)
		regval &= ~FAN_NKCK_BIT;
	else
		regval |= FAN_NKCK_BIT;

	if (MIKROSDK_FAN_write_register(FAN_REG_SPINUP, regval))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* configure closed loop algorithm */
static uint8_t MIKROSDK_FAN_config_algorithm(enum fan_update_time       updtime,
                                             enum fan_error_window      errwin,
                                             enum fan_derivative_option doption,
                                             enum fan_gain              gain_p,
                                             enum fan_gain              gain_i,
                                             enum fan_gain              gain_d)
{
	uint8_t regval;

	/* config1 */

	if (MIKROSDK_FAN_read_register(FAN_REG_CONFIG1, &regval))
		return (FAN_ST_FAIL);

	regval = (regval & (~FAN_UDT_MASK)) | (uint8_t)(updtime << FAN_UDT_SHFT);

	if (MIKROSDK_FAN_write_register(FAN_REG_CONFIG1, regval))
		return (FAN_ST_FAIL);

	/* config2 */

	if (MIKROSDK_FAN_read_register(FAN_REG_CONFIG2, &regval))
		return (FAN_ST_FAIL);

	regval = (regval & (~FAN_ERG_MASK)) | (uint8_t)(errwin << FAN_ERG_SHFT);
	regval = (regval & (~FAN_DPT_MASK)) | (uint8_t)(doption << FAN_DPT_SHFT);

	if (MIKROSDK_FAN_write_register(FAN_REG_CONFIG2, regval))
		return (FAN_ST_FAIL);

	/* gain */

	if (MIKROSDK_FAN_read_register(FAN_REG_GAIN, &regval))
		return (FAN_ST_FAIL);

	regval = (regval & (~FAN_GPR_MASK)) | (uint8_t)(gain_p << FAN_GPR_SHFT);
	regval = (regval & (~FAN_GIN_MASK)) | (uint8_t)(gain_i << FAN_GIN_SHFT);
	regval = (regval & (~FAN_GDE_MASK)) | (uint8_t)(gain_d << FAN_GDE_SHFT);

	if (MIKROSDK_FAN_write_register(FAN_REG_GAIN, regval))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* configure alerts */
/* set drive fail value to n percent of max speed of fan */
/* set tachometer valid value (min. speed) for spin-up */
static uint8_t MIKROSDK_FAN_config_alerts(enum fan_drive_fail_count dfc,
                                          uint8_t                   percent_max_speed,
                                          uint16_t                  min_spin_up_rpm)
{
	uint32_t count;
	uint8_t  regval;

	/* drive fail count */
	if (MIKROSDK_FAN_read_register(FAN_REG_SPINUP, &regval))
		return (FAN_ST_FAIL);

	regval = (regval & (~FAN_DFC_MASK)) | (uint8_t)(dfc << FAN_DFC_SHFT);

	if (MIKROSDK_FAN_write_register(FAN_REG_SPINUP, regval))
		return (FAN_ST_FAIL);

	/* drive fail value in [%] of ma. speed */
	/* max. speed [rpm] to count */
	count = MIKROSDK_FAN_convert_rpm_count(FAN_MAX_SPEED);
	/* from percent */
	count = (count * percent_max_speed) / 100;
	/* limit to 13 bit and shift left 3 */
	count = (count & 0x1FFF) << 3;

	regval = (uint8_t)((count >> 0) & 0xFF);
	if (MIKROSDK_FAN_write_register(FAN_REG_FAIL_LOW, regval))
		return (FAN_ST_FAIL);

	regval = (uint8_t)((count >> 8) & 0xFF);
	if (MIKROSDK_FAN_write_register(FAN_REG_FAIL_HIGH, regval))
		return (FAN_ST_FAIL);

	/* set tachometer valid value for spin-up */
	/* rpm to count */
	count = MIKROSDK_FAN_convert_rpm_count(min_spin_up_rpm);
	/* shift right 5 */
	count = (count & 0xFFFF) >> 5;

	regval = (uint8_t)(count & 0xFF);
	if (MIKROSDK_FAN_write_register(FAN_REG_VALID_TACH, regval))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* configure ramp rate control */
static uint8_t MIKROSDK_FAN_config_ramping(uint8_t enable, uint8_t stepsize)
{
	uint8_t regval;

	if (MIKROSDK_FAN_read_register(FAN_REG_CONFIG2, &regval))
		return (FAN_ST_FAIL);

	if (enable)
		regval |= FAN_ENRC_BIT;
	else
		regval &= ~FAN_ENRC_BIT;

	if (MIKROSDK_FAN_write_register(FAN_REG_CONFIG2, regval))
		return (FAN_ST_FAIL);

	if (!enable)
		return (FAN_ST_OK);

	if (stepsize > 32)
		regval = 31;
	else if (stepsize < 1)
		regval = 1;
	else
		regval = stepsize;

	if (MIKROSDK_FAN_write_register(FAN_REG_MAX_STEP, regval))
		return (FAN_ST_FAIL);

	return (FAN_ST_OK);
}

/* has has alarm/interrupt been triggered */
uint8_t MIKROSDK_FAN_alarm_triggered(void)
{
	uint8_t alarm = fan_alarm;
	fan_alarm     = 0;
	return (alarm);
}

/* pin mapping function */
void MIKROSDK_FAN_pin_mapping(uint8_t alarm)
{
	cfg.int_pin = alarm;
}

/* device initialisation */
uint8_t MIKROSDK_FAN_Initialise(void)
{
	uint8_t dbyte;

	/* don't call fan_cfg_setup() as this de-initialises pin mapping */
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = FAN_DEV_ADDR;

	/* initialisation */
	if (MIKROSDK_FAN_init())
		return FAN_ST_FAIL;

	/* check product id */
	if (MIKROSDK_FAN_read_register(FAN_REG_PRODUCT_ID, &dbyte))
		return FAN_ST_FAIL;
	if (dbyte != FAN_EMC2301_PRODUCTID)
		return FAN_ST_FAIL;

	/* set pwm base frequency */
	if (MIKROSDK_FAN_set_pwm_base_freq(FAN_PWM_FREQ))
		return FAN_ST_FAIL;

	/* configure device */
	if (MIKROSDK_FAN_config_basic(FAN_RANGE, FAN_EDGES))
		return FAN_ST_FAIL;
	if (MIKROSDK_FAN_config_spin_up(FAN_SPINUP_LEVEL, FAN_SPINUP_TIME, FAN_KICK))
		return FAN_ST_FAIL;
	if (MIKROSDK_FAN_config_algorithm(
	        FAN_UPDATE_TIME, FAN_ERR_WINDOW, FAN_DERIVATIVE, FAN_GAIN_P, FAN_GAIN_I, FAN_GAIN_D))
		return FAN_ST_FAIL;
	if (MIKROSDK_FAN_config_alerts(FAN_DRIVE_FAIL_COUNT, FAN_DRIVE_FAIL_LIMIT_PERCENT, FAN_MIN_SPIN_UP_VALUE_RPM))
		return FAN_ST_FAIL;
	if (MIKROSDK_FAN_config_ramping(FAN_RAMP_ENABLE, FAN_RAMP_STEPSIZE))
		return FAN_ST_FAIL;

/* enable interrupt */
#if (FAN_USE_INTERRUPT)
	if (fan_enable_interrupt(cfg.int_pin))
		return FAN_ST_FAIL;
	if (MIKROSDK_FAN_write_register(FAN_REG_INTERRUPT_ENABLE, 0x01))
		return FAN_ST_FAIL;
#endif

/* enable closed loop mode (fsg) */
#if (FAN_MODE == FAN_MODE_CLOSED_LOOP)
	MIKROSDK_FAN_set_mode(FAN_MODE_CLOSED_LOOP);
#endif

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return FAN_ST_OK;
}

/* driver function */
uint8_t MIKROSDK_FAN_Driver(uint8_t *speed_pwm_percent, uint16_t *speed_tach_rpm)
{
	uint8_t status = FAN_ST_OK;
	uint8_t alarm  = 0;

#if (FAN_USE_INTERRUPT)
	alarm = MIKROSDK_FAN_get_alarm();
#endif

	SENSORIF_I2C_Init(); /* enable interface */

/* set speed */
#if (FAN_MODE == FAN_MODE_CLOSED_LOOP)
	if (MIKROSDK_FAN_set_tach_target_value(*speed_tach_rpm))
		return FAN_ST_FAIL;
#else
	if (MIKROSDK_FAN_set_pwm_duty_cycle(*speed_pwm_percent))
		return FAN_ST_FAIL;
#endif

	/* get readings */
	if (MIKROSDK_FAN_get_pwm_duty_cycle(speed_pwm_percent))
		return FAN_ST_FAIL;
	if (MIKROSDK_FAN_get_tach_value(speed_tach_rpm))
		return FAN_ST_FAIL;

#if (FAN_USE_INTERRUPT)
	if (alarm == FAN_ALARM_TRIGGERED)
		status = MIKROSDK_FAN_get_status();
#else
	status = MIKROSDK_FAN_get_status();
#endif

	SENSORIF_I2C_Deinit(); /* disable interface */

	return status;
}

/****** NOTE: Noctua NF-A4X20 5V Fan ******/
/* Black	GND  */
/* Yellow	+5V  */
/* Blue		PWM  */
/* Green	TACH */
