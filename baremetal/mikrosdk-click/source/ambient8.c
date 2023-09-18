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
#include "ambient8_click.h"
#include "ambient8_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static ambient8_t     ambient8;
static ambient8_cfg_t cfg;

/* static variables for ambient calculation */
static u16_t ambient8_meastime = 200;
static u16_t ambient8_inttime  = 100;
static u8_t  ambient8_gain     = 1;

/* register write */
static uint8_t MIKROSDK_AMBIENT8_write_register(uint8_t reg_addr, uint8_t data)
{
	uint8_t wrbuf[3];

	wrbuf[0] = ambient8.slave_address;
	wrbuf[1] = reg_addr;
	wrbuf[2] = data;

	if (i2c_master_write(&ambient8.i2c, wrbuf, 3))
		return (AMBIENT8_ST_FAIL);

	return (AMBIENT8_ST_OK);
}

/* register read */
static uint8_t MIKROSDK_AMBIENT8_read_register(uint8_t reg_addr, uint8_t *data)
{
	uint8_t wrbuf[2];

	wrbuf[0] = ambient8.slave_address;
	wrbuf[1] = reg_addr;
	if (i2c_master_write_then_read(&ambient8.i2c, wrbuf, 2, data, 1))
		return (AMBIENT8_ST_FAIL);
	return (AMBIENT8_ST_OK);
}

/* driver initialisation */
static uint8_t MIKROSDK_AMBIENT8_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg.i2c_speed;
	i2c_cfg.scl   = cfg.scl;
	i2c_cfg.sda   = cfg.sda;

	ambient8.slave_address = cfg.i2c_address;

	if (i2c_master_open(&ambient8.i2c, &i2c_cfg) == I2C_MASTER_ERROR)
		return AMBIENT8_ST_FAIL;

	i2c_master_set_slave_address(&ambient8.i2c, ambient8.slave_address);
	i2c_master_set_speed(&ambient8.i2c, cfg.i2c_speed);

	return (AMBIENT8_ST_OK);
}

/* read data for both channels */
static uint8_t MIKROSDK_AMBIENT8_read_data(uint16_t *data_ch0, uint16_t *data_ch1)
{
	uint8_t creg;
	uint8_t dbyte;
	uint8_t countdown = 255;

	/* oneshot: standby to active mode */
	if (LTR303ALS_MODE == LTR303ALS_MODE_ONE_SHOT)
	{
		/* read-modify-write control register as not to alter gain setting */
		if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_CONTROL, &creg))
			return AMBIENT8_ST_FAIL;
		creg |= LTR303ALS_MODE_ACTIVE; /* set active mode */
		if (MIKROSDK_AMBIENT8_write_register(LTR303ALS_REG_CONTROL, creg))
			return AMBIENT8_ST_FAIL;
		WAIT_ms(LTR303ALS_T_ACTIVE);
	}

	/* check status */
	/* wait for new data */
	dbyte = 0x00;
	while (!(dbyte & LTR303ALS_DATA_STATUS))
	{
		if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_STATUS, &dbyte))
			return AMBIENT8_ST_FAIL;
		if (!(--countdown))
			return AMBIENT8_ST_FAIL;
	}
	/* check data valid */
	if (dbyte & LTR303ALS_DATA_VALID)
		return AMBIENT8_ST_INVALID;

	/* read data */
	if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_DATA_CH1_0, &dbyte))
		return AMBIENT8_ST_FAIL;
	*data_ch1 = dbyte;
	if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_DATA_CH1_1, &dbyte))
		return AMBIENT8_ST_FAIL;
	*data_ch1 += (dbyte << 8);
	if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_DATA_CH0_0, &dbyte))
		return AMBIENT8_ST_FAIL;
	*data_ch0 = dbyte;
	if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_DATA_CH0_1, &dbyte))
		return AMBIENT8_ST_FAIL;
	*data_ch0 += (dbyte << 8);

	/* oneshot: active to standby mode */
	if (LTR303ALS_MODE == LTR303ALS_MODE_ONE_SHOT)
	{
		creg &= !LTR303ALS_MODE_ACTIVE; /* set standby mode */
		if (MIKROSDK_AMBIENT8_write_register(LTR303ALS_REG_CONTROL, creg))
			return AMBIENT8_ST_FAIL;
	}

	return (AMBIENT8_ST_OK);
}

/* device configuration */
static uint8_t MIKROSDK_AMBIENT8_configure(uint16_t meastime, uint16_t inttime, uint8_t gain)
{
	uint8_t dbyte;

	switch (meastime)
	{
	case 50:
		dbyte = LTR303ALS_TMEAS_50;
		break;
	case 100:
		dbyte = LTR303ALS_TMEAS_100;
		break;
	case 200:
		dbyte = LTR303ALS_TMEAS_200;
		break;
	case 500:
		dbyte = LTR303ALS_TMEAS_500;
		break;
	case 1000:
		dbyte = LTR303ALS_TMEAS_1000;
		break;
	case 2000:
		dbyte = LTR303ALS_TMEAS_2000;
		break;
	default:
		return AMBIENT8_ST_FAIL;
	}

	ambient8_meastime = meastime;

	switch (inttime)
	{
	case 50:
		dbyte += LTR303ALS_TINT_50;
		break;
	case 100:
		dbyte += LTR303ALS_TINT_100;
		break;
	case 150:
		dbyte += LTR303ALS_TINT_150;
		break;
	case 200:
		dbyte += LTR303ALS_TINT_200;
		break;
	case 250:
		dbyte += LTR303ALS_TINT_250;
		break;
	case 300:
		dbyte += LTR303ALS_TINT_300;
		break;
	case 350:
		dbyte += LTR303ALS_TINT_350;
		break;
	case 400:
		dbyte += LTR303ALS_TINT_400;
		break;
	default:
		return AMBIENT8_ST_FAIL;
	}

	ambient8_inttime = inttime;

	/* write integration time and measurement period */
	if (MIKROSDK_AMBIENT8_write_register(LTR303ALS_REG_MEAS_RATE, dbyte))
		return AMBIENT8_ST_FAIL;

	switch (gain)
	{
	case 1:
		dbyte = LTR303ALS_GAIN_1X;
		break;
	case 2:
		dbyte = LTR303ALS_GAIN_2X;
		break;
	case 4:
		dbyte = LTR303ALS_GAIN_4X;
		break;
	case 8:
		dbyte = LTR303ALS_GAIN_8X;
		break;
	case 48:
		dbyte = LTR303ALS_GAIN_48X;
		break;
	case 96:
		dbyte = LTR303ALS_GAIN_96X;
		break;
	default:
		return AMBIENT8_ST_FAIL;
	}

	ambient8_gain = gain;

	/* set control register */
	if (LTR303ALS_MODE == LTR303ALS_MODE_CONTINUOUS)
		dbyte += LTR303ALS_MODE_ACTIVE; /* active mode */
	if (MIKROSDK_AMBIENT8_write_register(LTR303ALS_REG_CONTROL, dbyte))
		return AMBIENT8_ST_FAIL;

	return AMBIENT8_ST_OK;
}

/* device initialisation */
uint8_t MIKROSDK_AMBIENT8_Initialise(void)
{
	uint32_t tnow;
	uint8_t  dbyte;

	/* check startup time after power-up */
	tnow = TIME_ReadAbsoluteTime();
	if (tnow < LTR303ALS_T_POWERUP)
		WAIT_ms(LTR303ALS_T_POWERUP - tnow);

	/* don't call ambient8_cfg_setup() as this de-initialises pin mapping */
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = AMBIENT8_DEVICE_ADDRESS;

	/* initialisation */
	if (MIKROSDK_AMBIENT8_init())
		return AMBIENT8_ST_FAIL;

	/* check part id and manufacturer id to see if correct device is connected */
	if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_PART_ID, &dbyte))
		return AMBIENT8_ST_FAIL;
	if (dbyte != LTR303ALS_PARTID)
		return AMBIENT8_ST_FAIL;
	if (MIKROSDK_AMBIENT8_read_register(LTR303ALS_REG_MANUFAC_ID, &dbyte))
		return AMBIENT8_ST_FAIL;
	if (dbyte != LTR303ALS_MANFID)
		return AMBIENT8_ST_FAIL;

	/* device configuration */
	if (MIKROSDK_AMBIENT8_configure(LTR303ALS_TMEAS, LTR303ALS_TINT, LTR303ALS_GAIN))
		return AMBIENT8_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return AMBIENT8_ST_OK;
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_AMBIENT8_Reinitialise(void)
{
	WAIT_ms(LTR303ALS_T_POWERUP);

	SENSORIF_I2C_Init(); /* enable interface */

	/* re-configure device */
	if (MIKROSDK_AMBIENT8_configure(ambient8_meastime, ambient8_inttime, ambient8_gain))
		return AMBIENT8_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return AMBIENT8_ST_OK;
}

/* reconfigure device */
uint8_t MIKROSDK_AMBIENT8_Reconfigure(uint16_t meastime, uint16_t inttime, uint8_t gain)
{
	SENSORIF_I2C_Init(); /* enable interface */

	if (MIKROSDK_AMBIENT8_configure(meastime, inttime, gain))
		return AMBIENT8_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return AMBIENT8_ST_OK;
}

/* data acquisition */
/* Note: Ev(lux) = uint32_t / 100 */
uint8_t MIKROSDK_AMBIENT8_Acquire(uint32_t *illuminance_ch0, uint32_t *illuminance_ch1, uint32_t *illuminance_ambient)
{
	uint16_t data_ch0;
	uint16_t data_ch1;
	uint32_t ratio;

	SENSORIF_I2C_Init(); /* enable interface */

	/* read illuminance */
	if (MIKROSDK_AMBIENT8_read_data(&data_ch0, &data_ch1))
		return AMBIENT8_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* disable interface */

	/* calculate illuminance values for channels */
	*illuminance_ch0 = (10000 * (uint32_t)data_ch0) / (uint32_t)(ambient8_gain * ambient8_inttime);
	*illuminance_ch1 = (10000 * (uint32_t)data_ch1) / (uint32_t)(ambient8_gain * ambient8_inttime);

	/* calculate ambient illuminance */
	ratio = (100 * *illuminance_ch1) / (*illuminance_ch0 + *illuminance_ch1);
	if (ratio < 45)
		*illuminance_ambient = (177 * *illuminance_ch0 + 111 * *illuminance_ch1) / 100;
	else if ((ratio < 64) && (ratio >= 45))
		*illuminance_ambient = (428 * *illuminance_ch0 - 195 * *illuminance_ch1) / 100;
	else if ((ratio < 85) && (ratio >= 64))
		*illuminance_ambient = (59 * *illuminance_ch0 + 12 * *illuminance_ch1) / 100;
	else
		*illuminance_ambient = 0;

	return AMBIENT8_ST_OK;
}
