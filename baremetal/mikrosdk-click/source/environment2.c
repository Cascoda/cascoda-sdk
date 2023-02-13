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
#include "environment2_click.h"
#include "environment2_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static environment2_t     environment2;
static environment2_cfg_t cfg;

/* timing values */
static uint32_t sgp40_t_init = 0; /* initialisation time */

err_t MIKROSDK_ENVIRONMENT2_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);

	i2c_cfg.scl = cfg.scl;
	i2c_cfg.sda = cfg.sda;

	environment2.slave_address = cfg.i2c_address;

	if (i2c_master_open(&environment2.i2c, &i2c_cfg) == I2C_MASTER_ERROR)
	{
		return I2C_MASTER_ERROR;
	}

	if (i2c_master_set_slave_address(&environment2.i2c, environment2.slave_address) == I2C_MASTER_ERROR)
	{
		return I2C_MASTER_ERROR;
	}

	if (i2c_master_set_speed(&environment2.i2c, cfg.i2c_speed) == I2C_MASTER_ERROR)
	{
		return I2C_MASTER_ERROR;
	}

	return I2C_MASTER_SUCCESS;
}

/* determine sgp40 read status */
static uint8_t MIKROSDK_ENVIRONMENT2_status(void)
{
	uint8_t  status;
	uint32_t tmeas;

	tmeas = TIME_ReadAbsoluteTime();

	if (sgp40_t_init == SGP40_T_SLEEP)
		status = ENVIRONMENT2_ST_SLEEP;
	else if ((tmeas - sgp40_t_init) < SGP40_T_INIT)
		status = ENVIRONMENT2_ST_INIT;
	else if ((tmeas - sgp40_t_init) < SGP40_T_SETTLING)
		status = ENVIRONMENT2_ST_SETTLING;
	else
		status = ENVIRONMENT2_ST_OK;

	return (status);
}

/* internal switching i2c slave address between sht40 and sgp40 */
static void MIKROSDK_ENVIRONMENT2_set_device_slave_address(uint8_t select_device)
{
	if (select_device == ENVIRONMENT2_SEL_SGP40)
	{
		environment2.slave_address = ENVIRONMENT2_SGP40_SET_DEV_ADDR;
		i2c_master_set_slave_address(&environment2.i2c, environment2.slave_address);
	}
	else /* ENVIRONMENT2_SEL_SHT40 */
	{
		environment2.slave_address = ENVIRONMENT2_SHT40_SET_DEV_ADDR;
		i2c_master_set_slave_address(&environment2.i2c, environment2.slave_address);
	}
}

/* modified i2c write addding i2c slave address to data */
static uint8_t MIKROSDK_ENVIRONMENT2_write(uint8_t *tx_buf, size_t len)
{
	uint8_t data_buf[SGP40_ADDLEN + SGP40_MAXDLEN];
	uint8_t cnt;

	data_buf[0] = environment2.slave_address;

	for (cnt = 1; cnt <= len; cnt++)
	{
		data_buf[cnt] = tx_buf[cnt - 1];
	}

	if (i2c_master_write(&environment2.i2c, data_buf, ++len))
		return ENVIRONMENT2_ST_FAIL;

	return (ENVIRONMENT2_ST_OK);
}

/* measure sht40 values for temperature and humidity */
/* returns sht40 3-byte values (with crc) so they can be used directly for sgp40 calibration */
static uint8_t MIKROSDK_ENVIRONMENT2_measure_temp_hum(uint8_t *hum, uint8_t *tmp)
{
	uint8_t tx_buf[1];
	uint8_t rx_buf[6];

	tx_buf[0] = ENVIRONMENT2_SHT40_CMD_MEASURE_T_RH_HIGH_PRECISION;

	MIKROSDK_ENVIRONMENT2_set_device_slave_address(ENVIRONMENT2_SEL_SHT40);

	if (MIKROSDK_ENVIRONMENT2_write(tx_buf, 1))
		return ENVIRONMENT2_ST_FAIL;

	WAIT_ms(SHT40_T_MEAS);
	if (i2c_master_read(&environment2.i2c, rx_buf, 6))
		return ENVIRONMENT2_ST_FAIL;

	tmp[0] = rx_buf[0]; /* msb */
	tmp[1] = rx_buf[1]; /* lsb */
	tmp[2] = rx_buf[2]; /* crc */

	hum[0] = rx_buf[3]; /* msb */
	hum[1] = rx_buf[4]; /* lsb */
	hum[2] = rx_buf[5]; /* crc */

	return (ENVIRONMENT2_ST_OK);
}

/* measure sgp40 air quality (raw value) */
static uint8_t MIKROSDK_ENVIRONMENT2_measure_air_quality(uint16_t *air_quality, uint8_t *hum, uint8_t *tmp)
{
	uint8_t tx_buf[8];
	uint8_t rx_buf[3];

	tx_buf[0] = (uint8_t)(ENVIRONMENT2_SGP40_CMD_MEASURE_RAW >> 8);
	tx_buf[1] = (uint8_t)(ENVIRONMENT2_SGP40_CMD_MEASURE_RAW);

	tx_buf[2] = hum[0];
	tx_buf[3] = hum[1];
	tx_buf[4] = hum[2];

	tx_buf[5] = tmp[0];
	tx_buf[6] = tmp[1];
	tx_buf[7] = tmp[2];

	MIKROSDK_ENVIRONMENT2_set_device_slave_address(ENVIRONMENT2_SEL_SGP40);

	if (MIKROSDK_ENVIRONMENT2_write(tx_buf, 8))
		return ENVIRONMENT2_ST_FAIL;

	WAIT_ms(SGP40_T_MEAS_RAW);

	if (i2c_master_read(&environment2.i2c, rx_buf, 3))
		return ENVIRONMENT2_ST_FAIL;

	if (sgp40_t_init == SGP40_T_SLEEP)
		sgp40_t_init = TIME_ReadAbsoluteTime();

	*air_quality = (rx_buf[0] << 8) + rx_buf[1];

	return (ENVIRONMENT2_ST_OK);
}

/* convert sht40 3-byte values (with crc) to int16 temperature and humidity */
/* values are returned in ['C * 100] and [%RH * 100] (resolution = 0.01) */
/* Note: T('C)  = i16_t / 100 */
/* Note: H(%RH) = i16_t / 100 */
static void MIKROSDK_ENVIRONMENT2_convert_temp_hum(uint8_t *hum, uint8_t *tmp, int16_t *humidity, int16_t *temperature)
{
	uint16_t st;
	uint16_t srh;

	st  = (tmp[0] << 8) + tmp[1];
	srh = (hum[0] << 8) + hum[1];

	*temperature = (int16_t)(((st * 17500) / 65535) - 4500);
	*humidity    = (int16_t)(((srh * 12500) / 65535) - 600);
}

/* read temperature and humidity */
/* values are returned in ['C * 100] and [%RH * 100] (resolution = 0.01) */
/* Note: T('C)  = i16_t / 100 */
/* Note: H(%RH) = i16_t / 100 */
uint8_t MIKROSDK_ENVIRONMENT2_get_temp_hum(int16_t *humidity, int16_t *temperature)
{
	uint8_t  tmp[3];
	uint8_t  hum[3];
	uint16_t st;
	uint16_t srh;

	if (MIKROSDK_ENVIRONMENT2_measure_temp_hum(hum, tmp))
		return ENVIRONMENT2_ST_FAIL;

	MIKROSDK_ENVIRONMENT2_convert_temp_hum(hum, tmp, humidity, temperature);

	return (ENVIRONMENT2_ST_OK);
}

/* read air quality (raw value) */
uint8_t MIKROSDK_ENVIRONMENT2_get_air_quality(uint16_t *air_quality_raw)
{
	uint8_t  tx_buf[8];
	uint8_t  rx_buf[3];
	uint8_t  tmp[3];
	uint8_t  hum[3];
	uint16_t result;

	if (MIKROSDK_ENVIRONMENT2_measure_temp_hum(hum, tmp))
		return ENVIRONMENT2_ST_FAIL;

	if (MIKROSDK_ENVIRONMENT2_measure_air_quality(air_quality_raw, hum, tmp))
		return ENVIRONMENT2_ST_FAIL;

	return (MIKROSDK_ENVIRONMENT2_status());
}

/* read voc index algorithm output */
uint8_t MIKROSDK_ENVIRONMENT2_get_voc_index(int32_t *voc_index)
{
	uint16_t air_quality;
	uint8_t  tmp[3];
	uint8_t  hum[3];

	if (MIKROSDK_ENVIRONMENT2_measure_temp_hum(hum, tmp))
		return ENVIRONMENT2_ST_FAIL;

	if (MIKROSDK_ENVIRONMENT2_measure_air_quality(&air_quality, hum, tmp))
		return ENVIRONMENT2_ST_FAIL;

	environment2_voc_algorithm(air_quality, voc_index);

	return (MIKROSDK_ENVIRONMENT2_status());
}

/* read all measured values */
/* voc index algorithm output [0-500] */
/* air quality raw value [0-65535] */
/* relative_humidity [% RH / 100] */
/* temperature ['C / 100] */
uint8_t MIKROSDK_ENVIRONMENT2_get_voc_index_with_rh_t(int32_t  *voc_index,
                                                      uint16_t *air_quality_raw,
                                                      int16_t  *relative_humidity,
                                                      int16_t  *temperature)
{
	uint16_t air_quality;
	uint8_t  tmp[3];
	uint8_t  hum[3];

	if (MIKROSDK_ENVIRONMENT2_measure_temp_hum(hum, tmp))
		return ENVIRONMENT2_ST_FAIL;

	if (MIKROSDK_ENVIRONMENT2_measure_air_quality(&air_quality, hum, tmp))
		return ENVIRONMENT2_ST_FAIL;

	environment2_voc_algorithm(air_quality, voc_index);

	MIKROSDK_ENVIRONMENT2_convert_temp_hum(hum, tmp, relative_humidity, temperature);
	*air_quality_raw = air_quality;

	return (MIKROSDK_ENVIRONMENT2_status());
}

/* measure test (self-test) command */
uint8_t MIKROSDK_ENVIRONMENT2_sgp40_measure_test(void)
{
	uint8_t  tx_buf[2];
	uint8_t  rx_buf[3];
	uint16_t result;

	tx_buf[0] = (uint8_t)(ENVIRONMENT2_SGP40_CMD_MEASURE_TEST >> 8);
	tx_buf[1] = (uint8_t)(ENVIRONMENT2_SGP40_CMD_MEASURE_TEST);

	MIKROSDK_ENVIRONMENT2_set_device_slave_address(ENVIRONMENT2_SEL_SGP40);

	if (MIKROSDK_ENVIRONMENT2_write(tx_buf, 2))
		return ENVIRONMENT2_ST_FAIL;

	WAIT_ms(SGP40_T_TEST);

	if (i2c_master_read(&environment2.i2c, rx_buf, 3))
		return ENVIRONMENT2_ST_FAIL;

	result = (rx_buf[0] >> 8) + rx_buf[1];

	if (result != ENVIRONMENT2_SGP40_TEST_PASSED)
		return ENVIRONMENT2_ST_FAIL;

	return (ENVIRONMENT2_ST_OK);
}

/* turn sgp40 heater off command */
uint8_t MIKROSDK_ENVIRONMENT2_sgp40_heater_off(void)
{
	uint8_t tx_buf[2];

	tx_buf[0] = (uint8_t)(ENVIRONMENT2_SGP40_CMD_HEATER_OFF >> 8);
	tx_buf[1] = (uint8_t)(ENVIRONMENT2_SGP40_CMD_HEATER_OFF);

	MIKROSDK_ENVIRONMENT2_set_device_slave_address(ENVIRONMENT2_SEL_SGP40);

	sgp40_t_init = SGP40_T_SLEEP;

	if (MIKROSDK_ENVIRONMENT2_write(tx_buf, 2))
		return ENVIRONMENT2_ST_FAIL;

	return (ENVIRONMENT2_ST_OK);
}

/* soft reset command (sleep mode) */
/* soft reset command (i2c general call) applies to both devices */
uint8_t MIKROSDK_ENVIRONMENT2_soft_reset(void)
{
	uint8_t tx_buf[2];

	/* uses i2c general call address (0x00), so i2c_master_write used directly */
	tx_buf[0] = 0x00;
	tx_buf[1] = 0x06;

	sgp40_t_init = SGP40_T_SLEEP;

	if (i2c_master_write(&environment2.i2c, tx_buf, 2))
		return ENVIRONMENT2_ST_FAIL;

	return (ENVIRONMENT2_ST_OK);
}

/* device initialisation */
uint8_t MIKROSDK_ENVIRONMENT2_Initialise(void)
{
	/* don't call environment2_cfg_setup() as this de-initialises pin mapping */
	//environment2_cfg_setup(&cfg);
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = ENVIRONMENT2_SGP40_SET_DEV_ADDR;

	if (MIKROSDK_ENVIRONMENT2_init())
		return ENVIRONMENT2_ST_FAIL;

	environment2_voc_config();

	if (MIKROSDK_ENVIRONMENT2_sgp40_heater_off())
		return ENVIRONMENT2_ST_FAIL;

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return ENVIRONMENT2_ST_OK;
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_ENVIRONMENT2_Reinitialise(void)
{
	uint16_t airq;
	uint8_t  status;

	WAIT_ms(SGP40_T_POWERUP);

	SENSORIF_I2C_Init();

	/* dummy read */
	if ((status = MIKROSDK_ENVIRONMENT2_get_air_quality(&airq)) == ENVIRONMENT2_ST_FAIL)
		return ENVIRONMENT2_ST_FAIL;

	SENSORIF_I2C_Deinit();

	/* wait time after sleep as specified in: */
	/* https://github.com/Sensirion/arduino-gas-index-algorithm/blob/master/examples/exampleLowPowerUsage/exampleLowPowerUsage.ino */
	WAIT_ms(SGP40_T_POWERUP - SGP40_T_MEAS_RAW);

	return ENVIRONMENT2_ST_OK;
}

/* data acquisition */
/* Note: voc_index: [0-500] */
/* Note: air_quality: [0-65535] (raw value) */
/* Note: humidity: [% RH / 100] */
/* Note: temperature: ['C / 100] */
uint8_t MIKROSDK_ENVIRONMENT2_Acquire(int32_t  *voc_index,
                                      uint16_t *air_quality,
                                      int16_t  *humidity,
                                      int16_t  *temperature)
{
	uint8_t status = ENVIRONMENT2_ST_OK;

	/* data acquisition */

	SENSORIF_I2C_Init();

#if (ENVIRONMENT2_USE_POWERDOWN)
	if (MIKROSDK_ENVIRONMENT2_Powerup())
		return ENVIRONMENT2_ST_FAIL;
#endif

	status = MIKROSDK_ENVIRONMENT2_get_voc_index_with_rh_t(voc_index, air_quality, humidity, temperature);

#if (ENVIRONMENT2_USE_POWERDOWN)
	if (MIKROSDK_ENVIRONMENT2_Powerdown())
		return ENVIRONMENT2_ST_FAIL;
#endif

	SENSORIF_I2C_Deinit();

	return status;
}

/* macro function for power-down */
uint8_t MIKROSDK_ENVIRONMENT2_Powerdown(void)
{
	/* soft reset to power down both devices */
	if (MIKROSDK_ENVIRONMENT2_soft_reset())
		return ENVIRONMENT2_ST_FAIL;

	return (ENVIRONMENT2_ST_OK);
}

/* macro function for power-up */
uint8_t MIKROSDK_ENVIRONMENT2_Powerup(void)
{
	uint16_t airq;
	uint8_t  status;

	/* dummy read */
	if ((status = MIKROSDK_ENVIRONMENT2_get_air_quality(&airq)) == ENVIRONMENT2_ST_FAIL)
		return ENVIRONMENT2_ST_FAIL;

	/* wait time after sleep as specified in: */
	/* https://github.com/Sensirion/arduino-gas-index-algorithm/blob/master/examples/exampleLowPowerUsage/exampleLowPowerUsage.ino */
	WAIT_ms(SGP40_T_POWERUP - SGP40_T_MEAS_RAW);

	return (ENVIRONMENT2_ST_OK);
}
