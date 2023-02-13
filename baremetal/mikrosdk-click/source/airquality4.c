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
#include "airquality4_click.h"
#include "airquality4_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static airquality4_t     airquality4;
static airquality4_cfg_t cfg;

/* calibration baseline storage (with crc) */
static uint8_t sgp30_baseline[6];

/* timing values */
static uint32_t sgp30_t_init = 0; /* initialisation time */

/* driver initialisation */
static uint8_t MIKROSDK_AIRQUALITY4_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg.i2c_speed;
	i2c_cfg.scl   = cfg.scl;
	i2c_cfg.sda   = cfg.sda;

	airquality4.slave_address = cfg.i2c_address;

	if (i2c_master_open(&airquality4.i2c, &i2c_cfg) == I2C_MASTER_ERROR)
		return AIRQUALITY4_ST_FAIL;

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	i2c_master_set_speed(&airquality4.i2c, cfg.i2c_speed);

	return AIRQUALITY4_ST_OK;
}

/* modified i2c write addding i2c slave address to data and data structure specific to sgp30 */
static uint8_t MIKROSDK_AIRQUALITY4_write(uint16_t cmd, uint8_t *tx_buf, size_t len)
{
	uint8_t data_buf[SGP30_ADDLEN + SGP30_MAXDLEN];
	uint8_t cnt;
	size_t  length = len + 1;

	data_buf[0] = airquality4.slave_address;
	data_buf[1] = (cmd >> 8) & 0xFF;
	data_buf[2] = (cmd >> 0) & 0xFF;
	for (cnt = 0; cnt < len; ++cnt) data_buf[3 + cnt] = tx_buf[cnt];

	if (i2c_master_write(&airquality4.i2c, data_buf, SGP30_ADDLEN + len))
		return (AIRQUALITY4_ST_FAIL);

	return (AIRQUALITY4_ST_OK);
}

/* sensor initialisation command */
uint8_t MIKROSDK_AIRQUALITY4_dev_init(void)
{
	if (MIKROSDK_AIRQUALITY4_write(SGP30_CMD_INIT_AIR_QUALITY, NULL, 0))
		return (AIRQUALITY4_ST_FAIL);

	WAIT_ms(SGP30_T_MEAS_QUAL);
	sgp30_t_init = TIME_ReadAbsoluteTime();

	return (AIRQUALITY4_ST_OK);
}

/* get calibration baseline values command */
uint8_t MIKROSDK_AIRQUALITY4_get_baseline(void)
{
	if (MIKROSDK_AIRQUALITY4_write(SGP30_CMD_GET_BASELINE, NULL, 0))
		return (AIRQUALITY4_ST_FAIL);

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	if (i2c_master_read(&airquality4.i2c, sgp30_baseline, 6))
		return (AIRQUALITY4_ST_FAIL);

	return (AIRQUALITY4_ST_OK);
}

/* set calibration baseline values command */
uint8_t MIKROSDK_AIRQUALITY4_set_baseline(void)
{
	if (MIKROSDK_AIRQUALITY4_write(SGP30_CMD_SET_BASELINE, sgp30_baseline, 6))
		return (AIRQUALITY4_ST_FAIL);

	return (AIRQUALITY4_ST_OK);
}

/* soft reset command (sleep mode) */
uint8_t MIKROSDK_AIRQUALITY4_soft_reset(void)
{
	uint8_t tx_buf[2];

	/* uses i2c general call address (0x00), so i2c_master_write used directly */
	tx_buf[0] = 0x00;
	tx_buf[1] = 0x06;

	if (i2c_master_write(&airquality4.i2c, tx_buf, 2))
		return (AIRQUALITY4_ST_FAIL);

	sgp30_t_init = SGP30_T_SLEEP;

	return (AIRQUALITY4_ST_OK);
}

/* get feature set version command */
uint8_t MIKROSDK_AIRQUALITY4_get_version(uint16_t *version)
{
	uint8_t rx_buf[3];

	MIKROSDK_AIRQUALITY4_write(SGP30_CMD_GET_FEATURE_SET_VERSION, NULL, 0);

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	if (i2c_master_read(&airquality4.i2c, rx_buf, 3))
		return (AIRQUALITY4_ST_FAIL);

	*version = (rx_buf[0] << 8) + rx_buf[1];

	return (AIRQUALITY4_ST_OK);
}

/* measure test (self-test) command */
uint8_t MIKROSDK_AIRQUALITY4_measure_test(void)
{
	uint8_t  rx_buf[3];
	uint16_t value;

	if (MIKROSDK_AIRQUALITY4_write(SGP30_CMD_MEASURE_TEST, NULL, 0))
		return (AIRQUALITY4_ST_FAIL);

	WAIT_ms(SGP30_T_TEST);

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	if (i2c_master_read(&airquality4.i2c, rx_buf, 3))
		return (AIRQUALITY4_ST_FAIL);

	value = (rx_buf[0] << 8) + rx_buf[1];
	if (value != 0xD400)
		return (AIRQUALITY4_ST_FAIL);

	return (AIRQUALITY4_ST_OK);
}

/* measure raw signals command */
uint8_t MIKROSDK_AIRQUALITY4_measure_raw_signals(uint16_t *value)
{
	uint8_t rx_buf[6] = {0};

	if (MIKROSDK_AIRQUALITY4_write(SGP30_CMD_MEASURE_RAW_SIGNALS, NULL, 0))
		return (AIRQUALITY4_ST_FAIL);

	WAIT_ms(SGP30_T_MEAS_RAW);

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	if (i2c_master_read(&airquality4.i2c, rx_buf, 6))
		return (AIRQUALITY4_ST_FAIL);

	value[0] = (rx_buf[0] << 8) + rx_buf[1];
	value[1] = (rx_buf[3] << 8) + rx_buf[4];

	return (AIRQUALITY4_ST_OK);
}

/* measure air quality values command */
uint8_t MIKROSDK_AIRQUALITY4_measure_air_quality(uint16_t *value)
{
	uint8_t  rx_buf[6] = {0};
	uint8_t  status;
	uint32_t tmeas;

	tmeas = TIME_ReadAbsoluteTime();

	MIKROSDK_AIRQUALITY4_write(SGP30_CMD_MEASURE_AIR_QUALITY, NULL, 0);

	WAIT_ms(SGP30_T_MEAS_QUAL);

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	if (i2c_master_read(&airquality4.i2c, rx_buf, 6))
		return (AIRQUALITY4_ST_FAIL);

	value[0] = (rx_buf[0] << 8) + rx_buf[1];
	value[1] = (rx_buf[3] << 8) + rx_buf[4];

	if (sgp30_t_init == SGP30_T_SLEEP)
		status = AIRQUALITY4_ST_SLEEP;
	else if ((tmeas - sgp30_t_init) < SGP30_T_INIT)
		status = AIRQUALITY4_ST_INIT;
	else if (tmeas < SGP30_T_CAL)
		status = AIRQUALITY4_ST_NCAL;
	else
		status = AIRQUALITY4_ST_OK;

	return (status);
}

/* device initialisation */
uint8_t MIKROSDK_AIRQUALITY4_Initialise(void)
{
	/* don't call airquality4_cfg_setup() as this de-initialises pin mapping */
	//airquality4_cfg_setup(&cfg);
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = SGP30_I2C_ADDR;

	if (MIKROSDK_AIRQUALITY4_init())
		return (AIRQUALITY4_ST_FAIL);

	if (MIKROSDK_AIRQUALITY4_dev_init())
		return (AIRQUALITY4_ST_FAIL);

	/* Note: baseline should only be stored after 12 hours of continuous operation */
	/*       and should not be restored with set_baseline before that */
	if (MIKROSDK_AIRQUALITY4_get_baseline())
		return (AIRQUALITY4_ST_FAIL);

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return (AIRQUALITY4_ST_OK);
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_AIRQUALITY4_Reinitialise(void)
{
	WAIT_ms(SGP30_T_MEAS_POWERUP);

	SENSORIF_I2C_Init();

	if (MIKROSDK_AIRQUALITY4_dev_init())
		return (AIRQUALITY4_ST_FAIL);

	/* Note: baseline should only be stored after 12 hours of continuous operation */
	/*       and should not be restored with set_baseline before that */
	if (MIKROSDK_AIRQUALITY4_get_baseline())
		return (AIRQUALITY4_ST_FAIL);

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return (AIRQUALITY4_ST_OK);
}

/* data acquisition */
/* AIRQUALITY4_MEASURE_RAW_SIGNALS = 0: */
/* co2_h2   = CO2  [ppm] */
/* tvoc_eth = TVOC [ppb] */
/* AIRQUALITY4_MEASURE_RAW_SIGNALS = 1: */
/* co2_h2   = H2  (raw value) */
/* tvoc_eth = ETH (raw value) */
uint8_t MIKROSDK_AIRQUALITY4_Acquire(uint16_t *co2_h2, uint16_t *tvoc_eth)
{
	uint8_t  status = AIRQUALITY4_ST_OK;
	uint16_t data_buffer[2];

	/* data acquisition */
	SENSORIF_I2C_Init();
#if (AIRQUALITY4_MEASURE_RAW_SIGNALS)
	if (MIKROSDK_AIRQUALITY4_measure_raw_signals(data_buffer))
		return (AIRQUALITY4_ST_FAIL);
#else
	status = MIKROSDK_AIRQUALITY4_measure_air_quality(data_buffer);
#endif
	SENSORIF_I2C_Deinit();

	*co2_h2   = data_buffer[0];
	*tvoc_eth = data_buffer[1];

	return status;
}

/* macro function for power-down */
uint8_t MIKROSDK_AIRQUALITY4_Powerdown(void)
{
	/* save baseline if device has been calibrated */
	if (TIME_ReadAbsoluteTime() > SGP30_T_CAL)
	{
		if (MIKROSDK_AIRQUALITY4_get_baseline())
			return (AIRQUALITY4_ST_FAIL);
	}
	/* soft reset to power down */
	if (MIKROSDK_AIRQUALITY4_soft_reset())
		return (AIRQUALITY4_ST_FAIL);

	return (AIRQUALITY4_ST_OK);
}

/* macro function for power-up */
uint8_t MIKROSDK_AIRQUALITY4_Powerup(void)
{
	/* device initialisation to wake up */
	if (MIKROSDK_AIRQUALITY4_dev_init())
		return (AIRQUALITY4_ST_FAIL);

	/* restore baseline if device has been calibrated */
	if (TIME_ReadAbsoluteTime() > SGP30_T_CAL)
	{
		if (MIKROSDK_AIRQUALITY4_set_baseline())
			return (AIRQUALITY4_ST_FAIL);
	}

	return (AIRQUALITY4_ST_OK);
}
