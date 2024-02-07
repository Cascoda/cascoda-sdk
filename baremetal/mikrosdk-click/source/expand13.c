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
#include "expand13_click.h"
#include "expand13_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static expand13_t     expand13;
static expand13_cfg_t cfg;

/* flag if interrupt has been triggered */
static uint8_t expand13_irq = 0;

/* ISR for irq handling */
static int expand13_isr(void)
{
	/* set the interupt flag to active actions further up */
	expand13_irq = 1;
	return 0;
}

/* enable interrupt function - this has to bypass mikrosdk driver and hal */
static uint8_t MIKROSDK_EXPAND13_EnableInterrupt(uint8_t irq_pin)
{
	uint8_t                status;
	struct gpio_input_args args;

	status = BSP_ModuleDeregisterGPIOPin(irq_pin);
	if (!status)
	{
		args.mpin     = irq_pin;
		args.pullup   = MODULE_PIN_PULLUP_ON;
		args.debounce = MODULE_PIN_DEBOUNCE_ON;
		args.irq      = MODULE_PIN_IRQ_FALL;
		args.callback = expand13_isr;
		status        = BSP_ModuleRegisterGPIOInput(&args);
	}

	return (status);
}

/* driver initialisation */
static uint8_t MIKROSDK_EXPAND13_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg.i2c_speed;
	i2c_cfg.scl   = cfg.scl;
	i2c_cfg.sda   = cfg.sda;

	expand13.slave_address = cfg.i2c_address;

	if (i2c_master_open(&expand13.i2c, &i2c_cfg) == I2C_MASTER_ERROR)
		return EXPAND13_ST_FAIL;

	i2c_master_set_slave_address(&expand13.i2c, expand13.slave_address);
	i2c_master_set_speed(&expand13.i2c, cfg.i2c_speed);

	/* Input pins */
	digital_in_init(&expand13.int_pin, cfg.int_pin);
	/* Output pins */
	digital_out_init(&expand13.rst, cfg.rst);

	return (EXPAND13_ST_OK);
}

/* get interrupt pin status */
static uint8_t MIKROSDK_EXPAND13_get_interrupt(void)
{
	uint8_t irq = 0;

	digital_in_t apin = expand13.int_pin;
	irq               = digital_in_read(&apin);
	return irq;
}

/* modified read */
/* I2C slave address followed by read of register */
static uint8_t MIKROSDK_EXPAND13_generic_read(uint8_t *rx_buf, size_t len)
{
	i2c_master_set_slave_address(&expand13.i2c, expand13.slave_address);
	if (i2c_master_read(&expand13.i2c, rx_buf, len))
		return (EXPAND13_ST_FAIL);

	return (EXPAND13_ST_OK);
}

/* modified write */
static uint8_t MIKROSDK_EXPAND13_generic_write(uint8_t *tx_buf, size_t len)
{
	uint8_t wrbuf[7] = {0, 0, 0, 0, 0, 0, 0};

	if (len > 6)
		return (EXPAND13_ST_FAIL);

	wrbuf[0] = expand13.slave_address;
	for (uint8_t i = 0; i < len; ++i) wrbuf[i + 1] = tx_buf[i];

	if (i2c_master_write(&expand13.i2c, wrbuf, len + 1))
		return (EXPAND13_ST_FAIL);

	return (EXPAND13_ST_OK);
}

/* pin mapping function */
void MIKROSDK_EXPAND13_pin_mapping(uint8_t reset, uint8_t irq)
{
	cfg.rst     = reset;
	cfg.int_pin = irq;
}

/* has alarm/interrupt been triggered */
uint8_t MIKROSDK_EXPAND13_alarm_triggered(void)
{
	uint8_t alarm = expand13_irq;
	expand13_irq  = 0;
	return (alarm);
}

/* device initialisation */
uint8_t MIKROSDK_EXPAND13_Initialise(void)
{
	/* don't call expand13_cfg_setup() as this de-initialises pin mapping */
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = EXPAND13_I2C_ADDR;

	/* initialisation */
	if (MIKROSDK_EXPAND13_init())
		return EXPAND13_ST_FAIL;

	/* reset expander chip */
	digital_out_write(&expand13.rst, 0);
	WAIT_ms(EXPAND13_T_RESET);
	digital_out_write(&expand13.rst, 1);

/* enable interrupt */
#if (EXPAND13_USE_INTERRUPT)
	if (MIKROSDK_EXPAND13_EnableInterrupt(cfg.int_pin))
		return EXPAND13_ST_FAIL;
#endif

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return EXPAND13_ST_OK;
}

/* data acquisition */
/* Note: only port 0 used */
uint8_t MIKROSDK_EXPAND13_Acquire(uint8_t *port0)
{
	SENSORIF_I2C_Init(); /* enable interface */

	if (MIKROSDK_EXPAND13_generic_read(port0, 1))
		return EXPAND13_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* disable interface */

	return EXPAND13_ST_OK;
}

/* set output */
uint8_t MIKROSDK_EXPAND13_SetOutput(uint8_t io, uint8_t val)
{
	uint8_t data;

	if (io > 7)
		return EXPAND13_ST_FAIL;

	SENSORIF_I2C_Init(); /* enable interface */

	/* read */
	if (MIKROSDK_EXPAND13_generic_read(&data, 1))
		return EXPAND13_ST_FAIL;

	/* modify */
	data = data & ~(1 << io) | (val << io);

	if (MIKROSDK_EXPAND13_generic_write(&data, 1))
		return EXPAND13_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* disable interface */

	return EXPAND13_ST_OK;
}

/* sense input */
uint8_t MIKROSDK_EXPAND13_Sense(uint8_t io, uint8_t *val)
{
	uint8_t data;

	if (io > 7)
		return EXPAND13_ST_FAIL;

	SENSORIF_I2C_Init(); /* enable interface */

	/* read */
	if (MIKROSDK_EXPAND13_generic_read(&data, 1))
		return EXPAND13_ST_FAIL;

	/* modify */
	*val = (data >> io) & 0x01;

	SENSORIF_I2C_Deinit(); /* disable interface */

	return EXPAND13_ST_OK;
}
