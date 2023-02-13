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
#include "hvac_click.h"
#include "hvac_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static hvac_t     hvac;
static hvac_cfg_t cfg;

/* driver initialisation */
static uint8_t MIKROSDK_HVAC_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);

	i2c_cfg.scl = cfg.scl;
	i2c_cfg.sda = cfg.sda;

	hvac.slave_address = cfg.i2c_address;

	if (I2C_MASTER_ERROR == i2c_master_open(&hvac.i2c, &i2c_cfg))
		return HVAC_ST_FAIL;

	if (I2C_MASTER_ERROR == i2c_master_set_slave_address(&hvac.i2c, hvac.slave_address))
		return HVAC_ST_FAIL;

	if (I2C_MASTER_ERROR == i2c_master_set_speed(&hvac.i2c, cfg.i2c_speed))
		return HVAC_ST_FAIL;

	return HVAC_ST_OK;
}

/* scd41 modified i2c write addding i2c slave address to data and data structure specific to scd41 */
static uint8_t MIKROSDK_HVAC_scd41_generic_write(uint16_t cmd, uint8_t *tx_buf, size_t len)
{
	uint8_t data_buf[SCD41_ADDLEN + SCD41_MAXDLEN];
	uint8_t cnt;
	size_t  length = len + 1;

	data_buf[0] = hvac.slave_address;
	data_buf[1] = (cmd >> 8) & 0xFF;
	data_buf[2] = (cmd >> 0) & 0xFF;
	for (cnt = 0; cnt < len; ++cnt) data_buf[3 + cnt] = tx_buf[cnt];

	if (i2c_master_write(&hvac.i2c, data_buf, SCD41_ADDLEN + len))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 modified i2c read adding i2c slave address to data and data structure specific to scd41 */
static uint8_t MIKROSDK_HVAC_scd41_generic_write_then_read(uint16_t cmd, uint8_t *rx_buf, size_t len)
{
	uint8_t wrbuf[SCD41_ADDLEN] = {0, 0, 0};

	wrbuf[0] = hvac.slave_address;
	wrbuf[1] = (cmd >> 8) & 0xFF;
	wrbuf[2] = (cmd >> 0) & 0xFF;

	if (i2c_master_write_then_read(&hvac.i2c, wrbuf, SCD41_ADDLEN, rx_buf, len))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* calculate outgoing crc for writes */
static uint8_t MIKROSDK_HVAC_calc_crc(uint8_t data_0, uint8_t data_1)
{
	uint8_t i_cnt;
	uint8_t j_cnt;
	uint8_t crc_data[2];
	uint8_t crc = 0xFF;

	crc_data[0] = data_0;
	crc_data[1] = data_1;

	for (i_cnt = 0; i_cnt < 2; i_cnt++)
	{
		crc ^= crc_data[i_cnt];

		for (j_cnt = 8; j_cnt > 0; --j_cnt)
		{
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x31u;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}

/* convert scd41 2-byte temperature value to int16 */
/* value is returned in ['C * 100] (resolution = 0.01) */
/* Note: T('C)  = int16_t / 100 */
static int16_t MIKROSDK_HVAC_convert_temperature(uint8_t *raw_data)
{
	uint16_t regval;
	int16_t  result;

	regval = (raw_data[0] << 8) + raw_data[1];
	result = (int16_t)(((regval * 17500) / 65535) - 4500);

	return (result);
}

/* convert scd41 2-byte humidity value to int16 */
/* value is returned in [%RH * 100] (resolution = 0.01) */
/* Note: H(%RH) = int16_t / 100 */
static int16_t MIKROSDK_HVAC_convert_humidity(uint8_t *raw_data)
{
	uint16_t regval;
	int16_t  result;

	regval = (raw_data[0] << 8) + raw_data[1];
	result = (int16_t)(((regval * 10000) / 65535));

	return (result);
}

/* scd41 start periodic measurement command */
uint8_t MIKROSDK_HVAC_scd41_start_periodic_measurement(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_START_PERIODIC_MEASUREMENT, NULL, 0))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 start low power periodic measurement command */
uint8_t MIKROSDK_HVAC_scd41_start_low_power_periodic_measurement(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_START_LOW_POWER_PERIODIC_MEASUREMENT, NULL, 0))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 read measurement command */
uint8_t MIKROSDK_HVAC_scd41_read_measurement(uint16_t *co2content, int16_t *temperature, int16_t *humidity)
{
	uint8_t rx_buf[9];

	if (MIKROSDK_HVAC_scd41_generic_write_then_read(SCD41_CMD_READ_MEASUREMENT, rx_buf, 9))
		return (HVAC_ST_FAIL);

	*co2content  = (rx_buf[0] << 8) + rx_buf[1];
	*temperature = MIKROSDK_HVAC_convert_temperature(rx_buf + 3);
	*humidity    = MIKROSDK_HVAC_convert_humidity(rx_buf + 6);

	return (HVAC_ST_OK);
}

/* scd41 stop periodic measurement command */
uint8_t MIKROSDK_HVAC_scd41_stop_periodic_measurement(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_STOP_PERIODIC_MEASUREMENT, NULL, 0))
		return (HVAC_ST_FAIL);

	WAIT_ms(SCD41_T_STOP);

	return (HVAC_ST_OK);
}

/* scd41 get data ready status command */
uint8_t MIKROSDK_HVAC_scd41_get_data_ready_status(uint8_t *ready)
{
	uint8_t rx_buf[3];

	if (MIKROSDK_HVAC_scd41_generic_write_then_read(SCD41_CMD_GET_DATA_READY_STATUS, rx_buf, 3))
		return (HVAC_ST_FAIL);

	if (((rx_buf[0] << 8) + rx_buf[1]) & 0x07FF)
		*ready = HVAC_SCD41_NEW_DATA_IS_READY;
	else
		*ready = HVAC_SCD41_NEW_DATA_NOT_READY;

	return (HVAC_ST_OK);
}

/* scd41 set temperature offset command */
/* Note: int16_t temperature value in ['C * 100] */
uint8_t MIKROSDK_HVAC_scd41_set_temperature_offset(int16_t temp_offset)
{
	uint8_t  tx_buf[3];
	uint16_t write_val;

	/* calculate 16-bit register value */
	write_val = (uint16_t)((uint32_t)(temp_offset * 65535) / 17500);

	tx_buf[0] = (write_val >> 8) & 0xFF;
	tx_buf[1] = (write_val)&0xFF;
	tx_buf[2] = MIKROSDK_HVAC_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_SET_TEMPERATURE_OFFSET, tx_buf, 3))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 get temperature offset command */
/* Note: int16_t temperature value in ['C * 100] */
uint8_t MIKROSDK_HVAC_scd41_get_temperature_offset(int16_t *temp_offset)
{
	uint8_t  rx_buf[3];
	uint16_t regval;

	if (MIKROSDK_HVAC_scd41_generic_write_then_read(SCD41_CMD_GET_TEMPERATURE_OFFSET, rx_buf, 3))
		return (HVAC_ST_FAIL);

	regval = (rx_buf[0] << 8) + rx_buf[1];

	*temp_offset = (int16_t)((regval * 17500) / 65535);

	return (HVAC_ST_OK);
}

/* scd41 set sensor altitude command */
/* Note: uint16_t altitude value in [m] */
uint8_t MIKROSDK_HVAC_scd41_set_sensor_altitude(uint16_t altitude)
{
	uint8_t tx_buf[3];

	tx_buf[0] = (altitude >> 8) & 0xFF;
	tx_buf[1] = (altitude)&0xFF;
	tx_buf[2] = MIKROSDK_HVAC_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_SET_SENSOR_ALTITUDE, tx_buf, 3))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 get sensor altitude command */
/* Note: uint16_t altitude value in [m] */
uint8_t MIKROSDK_HVAC_scd41_get_sensor_altitude(uint16_t *altitude)
{
	uint8_t rx_buf[3];

	if (MIKROSDK_HVAC_scd41_generic_write_then_read(SCD41_CMD_GET_SENSOR_ALTITUDE, rx_buf, 3))
		return (HVAC_ST_FAIL);

	*altitude = (rx_buf[0] << 8) + rx_buf[1];

	return (HVAC_ST_OK);
}

/* scd41 set ambient pressure command */
/* Note: uint16_t pressure value in [P / 100] */
uint8_t MIKROSDK_HVAC_scd41_set_ambient_pressure(uint16_t pressure)
{
	uint8_t tx_buf[3];

	tx_buf[0] = (pressure >> 8) & 0xFF;
	tx_buf[1] = (pressure)&0xFF;
	tx_buf[2] = MIKROSDK_HVAC_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_SET_AMBIENT_PRESSURE, tx_buf, 3))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 perform forced calibration command */
/* Note: uint16_t pressure value in [P / 100] */
uint8_t MIKROSDK_HVAC_scd41_perform_forced_calibration(void)
{
	uint8_t  tx_buf[3];
	uint8_t  rx_buf[3];
	uint16_t result;

	/* stop_periodic_measurement command has to be issued before this funtion */
	tx_buf[0] = (SCD41_CO2_TARGET >> 8) & 0xFF;
	tx_buf[1] = (SCD41_CO2_TARGET)&0xFF;
	tx_buf[2] = MIKROSDK_HVAC_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_PERFORM_FORCED_RECALIBRATION, tx_buf, 3))
		return (HVAC_ST_FAIL);

	WAIT_ms(SCD41_T_CAL);

	if (i2c_master_read(&hvac.i2c, rx_buf, 3))
		return (HVAC_ST_FAIL);

	result = (rx_buf[0] << 8) + rx_buf[1];

	if (result == 0xFFFF)
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 set automatic self calibration enabled command */
/* Note: 1: enabled / 0: disabled */
uint8_t MIKROSDK_HVAC_scd41_set_automatic_self_calibration_enabled(uint8_t enable)
{
	uint8_t tx_buf[3];

	tx_buf[0] = 0x00;
	if (enable)
		tx_buf[1] = 0x01;
	else
		tx_buf[1] = 0x00;
	tx_buf[2] = MIKROSDK_HVAC_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, tx_buf, 3))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 get automatic self calibration enabled command */
/* Note: 1: enabled / 0: disabled */
uint8_t MIKROSDK_HVAC_scd41_get_automatic_self_calibration_enabled(uint8_t *enabled)
{
	uint8_t rx_buf[3];

	if (MIKROSDK_HVAC_scd41_generic_write_then_read(SCD41_CMD_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED, rx_buf, 3))
		return (HVAC_ST_FAIL);

	*enabled = rx_buf[1];

	return (HVAC_ST_OK);
}

/* scd41 persist settings command */
uint8_t MIKROSDK_HVAC_scd41_persist_settings(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_PERSIST_SETTINGS, NULL, 0))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 get serial number command */
/* serial number is 3 16-bit (uint16_t) words */
uint8_t MIKROSDK_HVAC_scd41_get_serial_number(uint16_t *serial_number)
{
	uint8_t rx_buf[9];

	if (MIKROSDK_HVAC_scd41_generic_write_then_read(SCD41_CMD_GET_SERIAL_NUMBER, rx_buf, 9))
		return (HVAC_ST_FAIL);

	serial_number[0] = (rx_buf[0] << 8) + rx_buf[1];
	serial_number[1] = (rx_buf[3] << 8) + rx_buf[4];
	serial_number[2] = (rx_buf[5] << 8) + rx_buf[7];

	return (HVAC_ST_OK);
}

/* scd41 perform self test command */
uint8_t MIKROSDK_HVAC_scd41_perform_self_test(void)
{
	uint8_t rx_buf[3];

	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_PERFORM_SELF_TEST, NULL, 0))
		return (HVAC_ST_FAIL);

	WAIT_ms(SCD41_T_SELF_TEST);

	if (i2c_master_read(&hvac.i2c, rx_buf, 3))
		return (HVAC_ST_FAIL);

	if (rx_buf[0] || rx_buf[1])
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 perform factory reset command */
uint8_t MIKROSDK_HVAC_scd41_perform_factory_reset(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_PERFORM_FACTORY_RESET, NULL, 0))
		return (HVAC_ST_FAIL);

	WAIT_ms(SCD41_T_FACTORY_RESET);

	return (HVAC_ST_OK);
}

/* scd41 reinit command */
uint8_t MIKROSDK_HVAC_scd41_reinit(void)
{
	/* stop_periodic_measurement command has to be issued before this funtion */
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_REINIT, NULL, 0))
		return (HVAC_ST_FAIL);

	WAIT_ms(SCD41_T_REINIT);

	return (HVAC_ST_OK);
}

/* scd41 measure single shot command */
/* only successful if periodic measurements have been stopped */
uint8_t MIKROSDK_HVAC_scd41_measure_single_shot(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_MEASURE_SINGLE_SHOT, NULL, 0))
		return (HVAC_ST_FAIL);

	/* wait / check data_ready_status 5 seconds after command until data will be ready */

	return (HVAC_ST_OK);
}

/* scd41 measure single shot rht only command */
/* only successful if periodic measurements have been stopped */
uint8_t MIKROSDK_HVAC_scd41_measure_single_shot_rht_only(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_MEASURE_SINGLE_SHOT_RHT_ONLY, NULL, 0))
		return (HVAC_ST_FAIL);

	WAIT_ms(SCD41_T_MEAS_SINGLE_RHT);
	/* data should be available now */

	return (HVAC_ST_OK);
}

/* scd41 power down command */
/* idle mode -> sleep mode, power_down to be used in combination with wakeup */
uint8_t MIKROSDK_HVAC_scd41_power_down(void)
{
	if (MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_POWER_DOWN, NULL, 0))
		return (HVAC_ST_FAIL);

	return (HVAC_ST_OK);
}

/* scd41 wake up command */
/* sleep mode -> idle mode, power_down to be used in combination with wakeup */
uint8_t MIKROSDK_HVAC_scd41_wake_up(void)
{
	/* note: causes i2c error 0x30 (transmit data NACKed), so don't check status */
	MIKROSDK_HVAC_scd41_generic_write(SCD41_CMD_WAKE_UP, NULL, 0);

	WAIT_ms(SCD41_T_WAKEUP);
	/* first reading after waking up the sensor should be discarded */

	return (HVAC_ST_OK);
}

/* interface device initialisation */
uint8_t MIKROSDK_HVAC_Initialise(void)
{
	/* don't call hvac_cfg_setup() as this de-initialises pin mapping */
	// hvac_cfg_setup(&cfg);
	cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address = HVAC_SCD40_SLAVE_ADDR;

	if (MIKROSDK_HVAC_init())
		return (HVAC_ST_FAIL);

#if (HVAC_FACTORY_RESET)
	if (MIKROSDK_HVAC_scd41_perform_factory_reset())
		return (HVAC_ST_FAIL);
#endif

#if (HVAC_MODE == HVAC_MODE_PERIODIC)
	if (MIKROSDK_HVAC_scd41_start_periodic_measurement())
		return (HVAC_ST_FAIL);
#endif

#if (HVAC_MODE == HVAC_MODE_PERIODIC_LP)
	if (MIKROSDK_HVAC_scd41_start_low_power_periodic_measurement())
		return (HVAC_ST_FAIL);
#endif

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return HVAC_ST_OK;
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_HVAC_Reinitialise(void)
{
	WAIT_ms(SCD41_T_WAKEUP);

	return HVAC_ST_OK;
}

/* data acquisition */
/* Note: co2content: [ppm] (0-40000 ppm) */
/* Note: temperature: T('C)  = int16_t / 100 */
/* Note: humidity: H(%RH) = int16_t / 100 */
uint8_t MIKROSDK_HVAC_Acquire(uint16_t *co2content, int16_t *humidity, int16_t *temperature)
{
	uint8_t status = HVAC_ST_OK;
	uint8_t data_ready;

#if ((HVAC_MODE == HVAC_MODE_SINGLE_WAIT) || (HVAC_MODE == HVAC_MODE_SINGLE_WAIT_SLEEP))
	uint32_t twait;
#endif
#if (HVAC_MODE == HVAC_MODE_SINGLE_NEXT)
	static uint8_t init = 1;
#endif

	SENSORIF_I2C_Init();

#if (HVAC_MODE == HVAC_MODE_SINGLE_WAIT_SLEEP)
	if (MIKROSDK_HVAC_scd41_wake_up())
		return (HVAC_ST_FAIL);
#endif

#if ((HVAC_MODE == HVAC_MODE_PERIODIC) || (HVAC_MODE == HVAC_MODE_PERIODIC_LP))
	if (MIKROSDK_HVAC_scd41_get_data_ready_status(&data_ready))
		return (HVAC_ST_FAIL);
	if (!data_ready)
	{
		status = HVAC_ST_WAITING;
		return status;
	}
	if (MIKROSDK_HVAC_scd41_read_measurement(co2content, temperature, humidity))
		return (HVAC_ST_FAIL);
#endif

#if ((HVAC_MODE == HVAC_MODE_SINGLE_WAIT) || (HVAC_MODE == HVAC_MODE_SINGLE_WAIT_SLEEP))
	if (MIKROSDK_HVAC_scd41_measure_single_shot())
		return (HVAC_ST_FAIL);
	twait = TIME_ReadAbsoluteTime();
	if (MIKROSDK_HVAC_scd41_get_data_ready_status(&data_ready))
		return (HVAC_ST_FAIL);
	while (!data_ready)
	{
		if ((TIME_ReadAbsoluteTime() - twait) > SCD41_T_TIMEOUT)
			return HVAC_ST_FAIL;
		WAIT_ms(SCD41_T_WAITPOLL);
		if (MIKROSDK_HVAC_scd41_get_data_ready_status(&data_ready))
			return (HVAC_ST_FAIL);
	}
	if (MIKROSDK_HVAC_scd41_read_measurement(co2content, temperature, humidity))
		return (HVAC_ST_FAIL);
#endif

#if (HVAC_MODE == HVAC_MODE_SINGLE_NEXT)
	if (MIKROSDK_HVAC_scd41_get_data_ready_status(&data_ready))
		return (HVAC_ST_FAIL);
	if (!data_ready)
	{
		status = HVAC_ST_WAITING;
		if (!init)
			return status;
	}
	if (!init)
	{
		if (MIKROSDK_HVAC_scd41_read_measurement(co2content, temperature, humidity))
			return (HVAC_ST_FAIL);
	}
	init = 0;
	if (MIKROSDK_HVAC_scd41_measure_single_shot())
		return (HVAC_ST_FAIL);
#endif

#if (HVAC_MODE == HVAC_MODE_SINGLE_WAIT_SLEEP)
	if (MIKROSDK_HVAC_scd41_power_down())
		return (HVAC_ST_FAIL);
#endif

	SENSORIF_I2C_Deinit();

	return status;
}
