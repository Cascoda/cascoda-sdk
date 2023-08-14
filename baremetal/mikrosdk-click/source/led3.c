/**
 * @file
 * @brief mikrosdk interface
 */
/*
 *  Copyright (c) 2023, Cascoda Ltd.
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
#include "led3_click.h"
#include "led3_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static led3_t     led3;
static led3_cfg_t cfg;

/* driver initialisation */
static uint8_t MIKROSDK_LED3_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);

	i2c_cfg.scl = cfg.scl;
	i2c_cfg.sda = cfg.sda;

	led3.slave_address = cfg.i2c_address;

	if (I2C_MASTER_ERROR == i2c_master_open(&led3.i2c, &i2c_cfg))
		return LED3_ST_FAIL;

	if (I2C_MASTER_ERROR == i2c_master_set_slave_address(&led3.i2c, led3.slave_address))
		return LED3_ST_FAIL;

	if (I2C_MASTER_ERROR == i2c_master_set_speed(&led3.i2c, cfg.i2c_speed))
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}

/* led3 modified i2c write addding i2c slave address to data and data structure specific to led3 */
static uint8_t MIKROSDK_LED3_generic_write(uint8_t cmd)
{
	uint8_t tx_buf[LED3_ADD_CMD_LEN];

	tx_buf[0] = led3.slave_address;
	tx_buf[1] = cmd;

	if (i2c_master_write(&led3.i2c, tx_buf, 2))
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}

uint8_t MIKROSDK_LED3_set_dimming_and_intensity(uint8_t cmd)
{
	if (MIKROSDK_LED3_generic_write(cmd))
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}

uint8_t MIKROSDK_LED3_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
	uint8_t tx_buf[3];

	if ((red < RED_MIN) || (red > RED_MAX))
		tx_buf[TX_BUF_RED] = RED_MIN;
	else
		tx_buf[TX_BUF_RED] = red;

	if ((green < GREEN_MIN) || (green > GREEN_MAX))
		tx_buf[TX_BUF_GREEN] = GREEN_MIN;
	else
		tx_buf[TX_BUF_GREEN] = green;

	if ((blue < BLUE_MIN) || (blue > BLUE_MAX))
		tx_buf[TX_BUF_BLUE] = BLUE_MIN;
	else
		tx_buf[TX_BUF_BLUE] = blue;

	if (MIKROSDK_LED3_generic_write(tx_buf[TX_BUF_RED]))
		return LED3_ST_FAIL;

	if (MIKROSDK_LED3_generic_write(tx_buf[TX_BUF_GREEN]))
		return LED3_ST_FAIL;

	if (MIKROSDK_LED3_generic_write(tx_buf[TX_BUF_BLUE]))
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}

uint8_t MIKROSDK_LED3_set_colour(uint32_t colour)
{
	uint8_t tx_buf[3];

	tx_buf[TX_BUF_RED]   = colour >> 16;
	tx_buf[TX_BUF_GREEN] = colour >> 8;
	tx_buf[TX_BUF_BLUE]  = colour;

	if (MIKROSDK_LED3_generic_write(tx_buf[TX_BUF_RED]))
		return LED3_ST_FAIL;

	if (MIKROSDK_LED3_generic_write(tx_buf[TX_BUF_GREEN]))
		return LED3_ST_FAIL;

	if (MIKROSDK_LED3_generic_write(tx_buf[TX_BUF_BLUE]))
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}

uint8_t MIKROSDK_LED3_shut_down(void)
{
	if (MIKROSDK_LED3_generic_write(LED3_CMD_SHUT_DOWN))
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}

uint8_t MIKROSDK_LED3_set_timer(uint8_t time)
{
	if (MIKROSDK_LED3_generic_write(time))
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}

uint8_t MIKROSDK_LED3_Initialise(void)
{
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = LED3_SLAVE_ADDR;

	SENSORIF_I2C_Config(I2C_PORTNUM);

	if (MIKROSDK_LED3_init())
		return LED3_ST_FAIL;

	return LED3_ST_OK;
}