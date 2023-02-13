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
#include "thermo3_click.h"
#include "thermo3_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static thermo3_t     thermo3;
static thermo3_cfg_t cfg;

/* flag if alarm / interrupt has been triggered */
static uint8_t thermo3_alarm = 0;

/* ISR for irq handling */
static int Thermo3_isr(void)
{
	/* set the motion_alarm to active actions further up */
	thermo3_alarm = 1;
	return 0;
}

/* enable interrupt function - this has to bypass mikrosdk driver and hal */
static uint8_t Thermo3_EnableInterrupt(uint8_t irq_pin)
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
		args.callback = Thermo3_isr;
		status        = BSP_ModuleRegisterGPIOInput(&args);
	}

	return (status);
}

/* driver initialisation */
static uint8_t MIKROSDK_THERMO3_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg.i2c_speed;
	i2c_cfg.scl   = cfg.scl;
	i2c_cfg.sda   = cfg.sda;

	thermo3.slave_address = cfg.i2c_address;

	if (i2c_master_open(&thermo3.i2c, &i2c_cfg) == I2C_MASTER_ERROR)
		return THERMO3_ST_FAIL;

	i2c_master_set_slave_address(&thermo3.i2c, thermo3.slave_address);
	i2c_master_set_speed(&thermo3.i2c, cfg.i2c_speed);

	/* Input pins */
	digital_in_init(&thermo3.al, cfg.al);

	return (THERMO3_ST_OK);
}

/* modified read */
/* original thermo3_generic_read() doesn't work as TMP102 read has to write 2 bytes first: */
/* I2C slave address followed by register address */
static uint8_t MIKROSDK_THERMO3_generic_read(uint8_t reg_addr, uint8_t *rx_buf)
{
	uint8_t wrbuf[TMP102_ADDLEN] = {0, 0};

	wrbuf[0] = thermo3.slave_address;
	wrbuf[1] = reg_addr;
	if (i2c_master_write_then_read(&thermo3.i2c, wrbuf, TMP102_ADDLEN, rx_buf, TMP102_REGLEN))
		return (THERMO3_ST_FAIL);

	return (THERMO3_ST_OK);
}

/* modified write */
static uint8_t MIKROSDK_THERMO3_generic_write(uint8_t reg_addr, uint8_t *tx_buf)
{
	uint8_t wrbuf[TMP102_ADDLEN + TMP102_REGLEN] = {0, 0, 0, 0};

	wrbuf[0] = thermo3.slave_address;
	wrbuf[1] = reg_addr;
	wrbuf[2] = tx_buf[0];
	wrbuf[3] = tx_buf[1];

	if (i2c_master_write(&thermo3.i2c, wrbuf, TMP102_ADDLEN + TMP102_REGLEN))
		return (THERMO3_ST_FAIL);

	return (THERMO3_ST_OK);
}

/* has has alarm/interrupt been triggered */
uint8_t MIKROSDK_THERMO3_alarm_triggered(void)
{
	uint8_t alarm = thermo3_alarm;
	thermo3_alarm = 0;
	return (alarm);
}

/* get alarm pin status */
uint8_t MIKROSDK_THERMO3_get_alarm(void)
{
	uint8_t alarm = 0;

	digital_in_t apin = thermo3.al;
	alarm             = digital_in_read(&apin);
	return alarm;
}

/* get configuration register (byte 2 is high byte) */
uint8_t MIKROSDK_THERMO3_get_config(uint16_t *configuration)
{
	char config[TMP102_REGLEN] = {0, 0};

	if (MIKROSDK_THERMO3_generic_read(MIKROSDK_THERMO3_REGADD_CONFIG, config))
		return (THERMO3_ST_FAIL);

	*configuration = (uint16_t)(config[1] << 8) + config[0];

	return (THERMO3_ST_OK);
}

/* set configuration register (byte 2 is high byte) */
uint8_t MIKROSDK_THERMO3_set_config(uint16_t config)
{
	char wdata[TMP102_REGLEN] = {0, 0};

	wdata[0] = 0xFF & (config);
	wdata[1] = 0xFF & (config >> 8);
	if (MIKROSDK_THERMO3_generic_write(MIKROSDK_THERMO3_REGADD_CONFIG, wdata))
		return (THERMO3_ST_FAIL);

	return (THERMO3_ST_OK);
}

/* get the temperature limits for alarm with unsigned integer values */
/* Note: T('C) = compl(uint16_t / 16) */
uint8_t MIKROSDK_THERMO3_get_temperature_limits(uint16_t *temp_limit_low, uint16_t *temp_limit_high)
{
	char     tmp_data[TMP102_REGLEN] = {0, 0};
	uint16_t temp;

	if (MIKROSDK_THERMO3_generic_read(MIKROSDK_THERMO3_REGADD_TLOW, tmp_data))
		return (THERMO3_ST_FAIL);

	temp = (tmp_data[0] << 4) | (tmp_data[1] >> 4) & 0x0F;
	/* sign extension from 12 to 16 bit */
	if (temp & 0x800)
		temp |= 0xF000;
	*temp_limit_low = temp;

	if (MIKROSDK_THERMO3_generic_read(MIKROSDK_THERMO3_REGADD_THIGH, tmp_data))
		return (THERMO3_ST_FAIL);

	temp = (tmp_data[0] << 4) | (tmp_data[1] >> 4) & 0x0F;
	/* sign extension from 12 to 16 bit */
	if (temp & 0x800)
		temp |= 0xF000;
	*temp_limit_high = temp;

	return (THERMO3_ST_OK);
}

/* set the temperature limits for alarm with unsigned integer values */
/* Note: uint16_t = compl(T('C) * 16) */
uint8_t MIKROSDK_THERMO3_set_temperature_limits(uint16_t temp_limit_low, uint16_t temp_limit_high)
{
	char wdata[TMP102_REGLEN] = {0, 0};

	wdata[0] = 0xFF & (temp_limit_low >> 4);
	wdata[1] = 0xFF & ((temp_limit_low & 0x0F) << 4);
	if (MIKROSDK_THERMO3_generic_write(MIKROSDK_THERMO3_REGADD_TLOW, wdata))
		return (THERMO3_ST_FAIL);

	wdata[0] = 0xFF & (temp_limit_high >> 4);
	wdata[1] = 0xFF & ((temp_limit_high & 0x0F) << 4);
	if (MIKROSDK_THERMO3_generic_write(MIKROSDK_THERMO3_REGADD_THIGH, wdata))
		return (THERMO3_ST_FAIL);

	return (THERMO3_ST_OK);
}

/* get temperature value - modified value aquisition function with unsigned integer return */
/* Note: T('C) = compl(uint16_t / 16) */
uint8_t MIKROSDK_THERMO3_get_temperature(uint16_t *temperature)
{
	char     tmp_data[TMP102_REGLEN] = {0, 0};
	uint16_t temp;
	uint16_t config = MIKROSDK_THERMO3_CONFIG_ONESHOT;

#if (!THERMO3_USE_INTERRUPT)
	/* one-shot mode */
	if (MIKROSDK_THERMO3_set_config(MIKROSDK_THERMO3_CONFIG))
		return (THERMO3_ST_FAIL);
	/* check config register if conversion is done (OS bit is 0) */
	while (config & MIKROSDK_THERMO3_CONFIG_ONESHOT)
	{
		if (MIKROSDK_THERMO3_get_config(&config))
			return (THERMO3_ST_FAIL);
	}
#endif

	if (MIKROSDK_THERMO3_generic_read(MIKROSDK_THERMO3_REGADD_TEMP, tmp_data))
		return (THERMO3_ST_FAIL);

	/* temp data to uint16_t */
	temp = (tmp_data[0] << 4) | (tmp_data[1] >> 4) & 0x0F;
	/* sign extension from 12 to 16 bit */
	if (temp & 0x800)
		temp |= 0xF000;

	*temperature = temp;

	return (THERMO3_ST_OK);
}

/* pin mapping function */
void MIKROSDK_THERMO3_pin_mapping(uint8_t alarm)
{
	cfg.al = alarm;
}

/* device initialisation */
uint8_t MIKROSDK_THERMO3_Initialise(void)
{
	/* don't call thermo3_cfg_setup() as this de-initialises pin mapping */
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = TMP102_I2C_ADDR;

	/* initialisation */
	if (MIKROSDK_THERMO3_init())
		return THERMO3_ST_FAIL;

	if (MIKROSDK_THERMO3_set_config(MIKROSDK_THERMO3_CONFIG))
		return THERMO3_ST_FAIL;

	/* set temperature limits */
	if (MIKROSDK_THERMO3_set_temperature_limits(MIKROSDK_THERMO3_TEMP_LIMIT_LOW, MIKROSDK_THERMO3_TEMP_LIMIT_HIGH))
		return THERMO3_ST_FAIL;

/* enable interrupt */
#if (THERMO3_USE_INTERRUPT)
	if (Thermo3_EnableInterrupt(cfg.al))
		return THERMO3_ST_FAIL;
#endif

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return THERMO3_ST_OK;
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_THERMO3_Reinitialise(void)
{
	WAIT_ms(THERMO3_T_POWERUP);

	SENSORIF_I2C_Init(); /* enable interface */

	if (MIKROSDK_THERMO3_set_config(MIKROSDK_THERMO3_CONFIG))
		return THERMO3_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return THERMO3_ST_OK;
}

/* data acquisition */
/* Note: temperature: T('C) = compl(uint16_t / 16) */
uint8_t MIKROSDK_THERMO3_Acquire(uint16_t *temperature)
{
	uint8_t status = THERMO3_ST_OK;
	uint8_t alarm  = 0;

/* data acquisition */
#if (THERMO3_USE_INTERRUPT)
	alarm = MIKROSDK_THERMO3_get_alarm();
#endif

	SENSORIF_I2C_Init();                               /* enable interface */
	if (MIKROSDK_THERMO3_get_temperature(temperature)) /* read TMP102 temperature */
		return THERMO3_ST_FAIL;
	SENSORIF_I2C_Deinit(); /* disable interface */

#if (THERMO3_USE_INTERRUPT)
	if (alarm == THERMO3_ALARM_TRIGGERED)
	{
		if (*temperature >= MIKROSDK_THERMO3_TEMP_LIMIT_HIGH)
			status = THERMO3_ST_ALARM_TRIGGERED;
		else
			status = THERMO3_ST_ALARM_CLEARED;
	}
#endif

	return status;
}
