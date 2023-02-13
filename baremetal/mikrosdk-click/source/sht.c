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
#include "sht_click.h"
#include "sht_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static sht_t     sht;
static sht_cfg_t cfg;

/* flag if alarm / interrupt has been triggered */
static uint8_t sht_alarm       = 0;
static uint8_t sht_alarm_state = SHT_ALARM_NOALARM;

/* ISR for irq handling */
static int SHT_isr(void)
{
	/* set the motion_alarm to active actions further up */
	sht_alarm = 1;

	if (digital_in_read(&sht.int_pin))
		sht_alarm_state = SHT_ALARM_TRIGGERED;
	else
		sht_alarm_state = SHT_ALARM_CLEARED;

	return 0;
}

/* enable interrupt function - this has to bypass mikrosdk driver and hal */
static uint8_t MIKROSDK_SHT_EnableInterrupt(uint8_t irq_pin)
{
	uint8_t                status;
	struct gpio_input_args args;

	status = BSP_ModuleDeregisterGPIOPin(irq_pin);
	if (!status)
	{
		args.mpin     = irq_pin;
		args.pullup   = MODULE_PIN_PULLUP_OFF;
		args.debounce = MODULE_PIN_DEBOUNCE_ON;
		args.irq      = MODULE_PIN_IRQ_BOTH;
		args.callback = SHT_isr;
		status        = BSP_ModuleRegisterGPIOInput(&args);
	}

	return (status);
}

/* driver initialisation */
static uint8_t MIKROSDK_SHT_init(void)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg.i2c_speed;
	i2c_cfg.scl   = cfg.scl;
	i2c_cfg.sda   = cfg.sda;

	sht.slave_address = cfg.i2c_address;

	if (i2c_master_open(&sht.i2c, &i2c_cfg) == I2C_MASTER_ERROR)
		return SHT_ST_FAIL;

	i2c_master_set_slave_address(&sht.i2c, sht.slave_address);
	i2c_master_set_speed(&sht.i2c, cfg.i2c_speed);

	// Output pins
	digital_out_init(&sht.rst, cfg.rst);
	// Input pins
	digital_in_init(&sht.int_pin, cfg.int_pin);

	return SHT_ST_OK;
}

/* modified i2c write addding i2c slave address to data and data structure specific to sht3x */
static uint8_t MIKROSDK_SHT_generic_write(uint16_t cmd, uint8_t *tx_buf, size_t len)
{
	uint8_t data_buf[SHT3X_ADDLEN + SHT3X_MAXDLEN];
	uint8_t cnt;
	size_t  length = len + 1;

	data_buf[0] = sht.slave_address;
	data_buf[1] = (cmd >> 8) & 0xFF;
	data_buf[2] = (cmd >> 0) & 0xFF;
	for (cnt = 0; cnt < len; ++cnt) data_buf[3 + cnt] = tx_buf[cnt];

	if (i2c_master_write(&sht.i2c, data_buf, SHT3X_ADDLEN + len))
		return (SHT_ST_FAIL);

	return (SHT_ST_OK);
}

/* modified i2c read addding i2c slave address to data and data structure specific to sht3x */
static uint8_t MIKROSDK_SHT_generic_read(uint16_t cmd, uint8_t *rx_buf, size_t len)
{
	uint8_t wrbuf[SHT3X_ADDLEN] = {0, 0, 0};

	wrbuf[0] = sht.slave_address;
	wrbuf[1] = (cmd >> 8) & 0xFF;
	wrbuf[2] = (cmd >> 0) & 0xFF;

	if (i2c_master_write_then_read(&sht.i2c, wrbuf, SHT3X_ADDLEN, rx_buf, len))
		return (SHT_ST_FAIL);

	return (SHT_ST_OK);
}

/* calculate outgoing crc for writes */
static uint8_t MIKROSDK_SHT_calc_crc(uint8_t data_0, uint8_t data_1)
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

/* convert sht3x 3-byte values (with crc) to int16 temperature and humidity */
/* values are returned in ['C * 100] and [%RH * 100] (resolution = 0.01) */
/* Note: T('C)  = int16_t / 100 */
/* Note: H(%RH) = int16_t / 100 */
static void MIKROSDK_SHT_convert_temp_hum(uint8_t *raw_data, int16_t *humidity, int16_t *temperature)
{
	uint16_t st;
	uint16_t srh;

	st  = (raw_data[0] << 8) + raw_data[1];
	srh = (raw_data[3] << 8) + raw_data[4];

	*temperature = (int16_t)(((st * 17500) / 65535) - 4500);
	*humidity    = (int16_t)(((srh * 10000) / 65535));
}

/* returns the 16-bit alert limit word */
static uint16_t MIKROSDK_SHT_calc_limit(int16_t humidity, int16_t temperature)
{
	uint16_t hval, tval;
	uint16_t result;

	/* calculate 16-bit values */
	hval = (uint16_t)((uint32_t)((humidity)*65535) / 10000);
	tval = (uint16_t)((uint32_t)((temperature + 4500) * 65535) / 17500);

	/* take 7 MSBs from humidity and 9 MSBs from temperature and concatenate */
	result = (hval & 0xFE00) + (((tval & 0xFF80) >> 7) & 0x01FF);

	return (result);
}

/* converts 3-byte alert limits data to 6-byte raw measurement data */
static void MIKROSDK_SHT_convert_limit_data_to_raw_data(uint8_t *limit_data, uint8_t *raw_data)
{
	uint16_t lval;
	uint16_t hval, tval;

	lval        = (limit_data[0] << 8) + limit_data[1];
	hval        = (lval & 0xFE00);
	tval        = (lval & 0x01FF) << 7;
	raw_data[0] = (tval >> 8) & 0xFF;
	raw_data[1] = (tval)&0xFF;
	raw_data[2] = 0; // MIKROSDK_SHT_calc_crc (raw_data[0], raw_data[1]);
	raw_data[3] = (hval >> 8) & 0xFF;
	raw_data[4] = (hval)&0xFF;
	raw_data[5] = 0; // MIKROSDK_SHT_calc_crc (raw_data[3], raw_data[4]);
}

/* returns read command depending on settings - single shot */
static uint16_t MIKROSDK_SHT_cmd_ss(void)
{
	uint8_t tmp_cmd[2];

	if (sht.vars.clk_stretching) /* clock stretching enabled */
	{
		tmp_cmd[0] = SHT_STR_ENABLE;
		if (sht.vars.repeatability == SHT_RPT_HIGH)
			tmp_cmd[1] = 0x06;
		else if (sht.vars.repeatability == SHT_RPT_MEDIUM)
			tmp_cmd[1] = 0x0D;
		else /* SHT_RPT_LOW */
			tmp_cmd[1] = 0x10;
	}
	else /* clock stretching disabled */
	{
		tmp_cmd[0] = SHT_STR_DISABLE;
		if (sht.vars.repeatability == SHT_RPT_HIGH)
			tmp_cmd[1] = 0x00;
		else if (sht.vars.repeatability == SHT_RPT_MEDIUM)
			tmp_cmd[1] = 0x0B;
		else /* SHT_RPT_LOW */
			tmp_cmd[1] = 0x16;
	}
	return ((tmp_cmd[0] << 8) + tmp_cmd[1]);
}

/* returns read command depending on settings - periodic */
static uint16_t MIKROSDK_SHT_cmd_pm(void)
{
	uint8_t tmp_cmd[2];

	tmp_cmd[0] = sht.vars.mps;

	if (sht.vars.mps == SHT_MPS_05) /* 0.5 mps */
	{
		if (sht.vars.repeatability == SHT_RPT_HIGH)
			tmp_cmd[1] = 0x32;
		else if (sht.vars.repeatability == SHT_RPT_MEDIUM)
			tmp_cmd[1] = 0x24;
		else /* SHT_RPT_LOW */
			tmp_cmd[1] = 0x2F;
	}
	else if (sht.vars.mps == SHT_MPS_1) /* 1 mps */
	{
		if (sht.vars.repeatability == SHT_RPT_HIGH)
			tmp_cmd[1] = 0x30;
		else if (sht.vars.repeatability == SHT_RPT_MEDIUM)
			tmp_cmd[1] = 0x26;
		else /* SHT_RPT_LOW */
			tmp_cmd[1] = 0x2D;
	}
	else if (sht.vars.mps == SHT_MPS_2) /* 2 mps */
	{
		if (sht.vars.repeatability == SHT_RPT_HIGH)
			tmp_cmd[1] = 0x36;
		else if (sht.vars.repeatability == SHT_RPT_MEDIUM)
			tmp_cmd[1] = 0x20;
		else /* SHT_RPT_LOW */
			tmp_cmd[1] = 0x2B;
	}
	else if (sht.vars.mps == SHT_MPS_4) /* 4 mps */
	{
		if (sht.vars.repeatability == SHT_RPT_HIGH)
			tmp_cmd[1] = 0x34;
		else if (sht.vars.repeatability == SHT_RPT_MEDIUM)
			tmp_cmd[1] = 0x22;
		else /* SHT_RPT_LOW */
			tmp_cmd[1] = 0x29;
	}
	else /* 10 mps */
	{
		if (sht.vars.repeatability == SHT_RPT_HIGH)
			tmp_cmd[1] = 0x37;
		else if (sht.vars.repeatability == SHT_RPT_MEDIUM)
			tmp_cmd[1] = 0x21;
		else /* SHT_RPT_LOW */
			tmp_cmd[1] = 0x2A;
	}
	return ((tmp_cmd[0] << 8) + tmp_cmd[1]);
}

/* start periodic mode */
uint8_t MIKROSDK_SHT_start_pm(void)
{
	if (MIKROSDK_SHT_generic_write(MIKROSDK_SHT_cmd_pm(), NULL, 0))
		return (SHT_ST_FAIL);

	WAIT_ms(SHT_T_COMMS);

	return (SHT_ST_OK);
}

/* stop periodic mode */
uint8_t MIKROSDK_SHT_stop_pm(void)
{
	if (MIKROSDK_SHT_generic_write(SHT_BREAK, NULL, 0))
		return (SHT_ST_FAIL);

	WAIT_ms(SHT_T_COMMS);

	return (SHT_ST_OK);
}

/* get temperature and humidity values - periodic mode */
/* Note: T('C)  = int16_t / 100 */
/* Note: H(%RH) = int16_t / 100 */
uint8_t MIKROSDK_SHT_get_temp_hum_pm(int16_t *humidity, int16_t *temperature)
{
	uint8_t raw_data[6];

	if (MIKROSDK_SHT_generic_read(SHT_FETCH_DATA, raw_data, 6))
		return (SHT_ST_FAIL);

	MIKROSDK_SHT_convert_temp_hum(raw_data, humidity, temperature);

	return (SHT_ST_OK);
}

/* get temperature and humidity values - single shot */
/* Note: T('C)  = int16_t / 100 */
/* Note: H(%RH) = int16_t / 100 */
uint8_t MIKROSDK_SHT_get_temp_hum_ss(int16_t *humidity, int16_t *temperature)
{
	uint8_t raw_data[6];

	if (MIKROSDK_SHT_generic_read(MIKROSDK_SHT_cmd_ss(), raw_data, 6))
		return (SHT_ST_FAIL);

	MIKROSDK_SHT_convert_temp_hum(raw_data, humidity, temperature);

	return (SHT_ST_OK);
}

/* set heater control on/off */
uint8_t MIKROSDK_SHT_heater_control(uint8_t state)
{
	uint8_t cmd[2];

	cmd[0] = SHT_HEATER;

	if (state == MIKROSDK_SHT_ON)
		cmd[1] = 0x6D; /* heater on */
	else
		cmd[1] = 0x66; /* heater off */

	if (MIKROSDK_SHT_generic_write(((cmd[0] << 8) + cmd[1]), NULL, 0))
		return (SHT_ST_FAIL);

	return (SHT_ST_OK);
}

/* read status register (16-bit) */
uint8_t MIKROSDK_SHT_read_status(uint16_t *statusregister)
{
	uint8_t raw_data[3];

	if (MIKROSDK_SHT_generic_read(SHT_READ_STATUS, raw_data, 3))
		return (SHT_ST_FAIL);

	*statusregister = (raw_data[0] << 8) + raw_data[1];

	return (SHT_ST_OK);
}

/* clear status register */
uint8_t MIKROSDK_SHT_clear_status(void)
{
	if (MIKROSDK_SHT_generic_write(SHT_CLEAR_STATUS1, NULL, 0))
		return (SHT_ST_FAIL);

	WAIT_ms(SHT_T_COMMS);

	return (SHT_ST_OK);
}

/* hard (pin) reset */
void MIKROSDK_SHT_hard_reset(void)
{
	digital_out_write(&sht.rst, 0);
	WAIT_ms(SHT_T_COMMS);
	digital_out_write(&sht.rst, 1);
	WAIT_ms(SHT_T_COMMS);
}

/* soft (register) reset */
uint8_t MIKROSDK_SHT_soft_reset(void)
{
	if (MIKROSDK_SHT_generic_write(SHT_SOFT_RESET, NULL, 0))
		return (SHT_ST_FAIL);

	return (SHT_ST_OK);
}

/* configure additional parameters */
void MIKROSDK_SHT_config(void)
{
	sht.vars.clk_stretching = cfg.vars_cfg.clk_stretching;
	sht.vars.repeatability  = cfg.vars_cfg.repeatability;
	sht.vars.mps            = cfg.vars_cfg.mps;
}

/* set temperature and humidity high alert limits */
/* Note: set temperature values in ['C * 100] */
/*       set humidity    values in [%RH * 100] */
uint8_t MIKROSDK_SHT_set_hi_alert_limits(int16_t h_set, int16_t h_clr, int16_t t_set, int16_t t_clr)
{
	uint16_t write_val;
	uint8_t  tx_buf[3];

	/* set limits */
	write_val = MIKROSDK_SHT_calc_limit(h_set, t_set);
	tx_buf[0] = (write_val >> 8) & 0xFF;
	tx_buf[1] = (write_val)&0xFF;
	tx_buf[2] = MIKROSDK_SHT_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_SHT_generic_write(SHT_CMD_WR_HI_SET, tx_buf, 3))
		return (SHT_ST_FAIL);

	WAIT_ms(SHT_T_COMMS);

	/* clear limits */
	write_val = MIKROSDK_SHT_calc_limit(h_clr, t_clr);
	tx_buf[0] = (write_val >> 8) & 0xFF;
	tx_buf[1] = (write_val)&0xFF;
	tx_buf[2] = MIKROSDK_SHT_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_SHT_generic_write(SHT_CMD_WR_HI_CLR, tx_buf, 3))
		return (SHT_ST_FAIL);

	WAIT_ms(SHT_T_COMMS);

	return (SHT_ST_OK);
}

/* set temperature and humidity low alert limits */
/* Note: set temperature values in ['C * 100] */
/*       set humidity    values in [%RH * 100] */
uint8_t MIKROSDK_SHT_set_lo_alert_limits(int16_t h_set, int16_t h_clr, int16_t t_set, int16_t t_clr)
{
	uint16_t write_val;
	uint8_t  tx_buf[3];

	/* set limits */
	write_val = MIKROSDK_SHT_calc_limit(h_set, t_set);
	tx_buf[0] = (write_val >> 8) & 0xFF;
	tx_buf[1] = (write_val)&0xFF;
	tx_buf[2] = MIKROSDK_SHT_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_SHT_generic_write(SHT_CMD_WR_LO_SET, tx_buf, 3))
		return (SHT_ST_FAIL);

	WAIT_ms(SHT_T_COMMS);

	/* clear limits */
	write_val = MIKROSDK_SHT_calc_limit(h_clr, t_clr);
	tx_buf[0] = (write_val >> 8) & 0xFF;
	tx_buf[1] = (write_val)&0xFF;
	tx_buf[2] = MIKROSDK_SHT_calc_crc(tx_buf[0], tx_buf[1]);

	if (MIKROSDK_SHT_generic_write(SHT_CMD_WR_LO_CLR, tx_buf, 3))
		return (SHT_ST_FAIL);

	WAIT_ms(SHT_T_COMMS);

	return (SHT_ST_OK);
}

/* get temperature and humidity high alert limits */
/* Note: T('C)  = int16_t / 100 */
/* Note: H(%RH) = int16_t / 100 */
uint8_t MIKROSDK_SHT_get_hi_alert_limits(int16_t *h_set, int16_t *h_clr, int16_t *t_set, int16_t *t_clr)
{
	uint8_t rx_buf[3];
	uint8_t raw_data[6];

	/* set limits */
	if (MIKROSDK_SHT_generic_read(SHT_CMD_RD_HI_SET, rx_buf, 3))
		return (SHT_ST_FAIL);

	MIKROSDK_SHT_convert_limit_data_to_raw_data(rx_buf, raw_data);
	MIKROSDK_SHT_convert_temp_hum(raw_data, h_set, t_set);

	/* clear limits */
	if (MIKROSDK_SHT_generic_read(SHT_CMD_RD_HI_CLR, rx_buf, 3))
		return (SHT_ST_FAIL);

	MIKROSDK_SHT_convert_limit_data_to_raw_data(rx_buf, raw_data);
	MIKROSDK_SHT_convert_temp_hum(raw_data, h_clr, t_clr);

	return (SHT_ST_OK);
}

/* get temperature and humidity low alert limits */
/* Note: T('C)  = int16_t / 100 */
/* Note: H(%RH) = int16_t / 100 */
uint8_t MIKROSDK_SHT_get_lo_alert_limits(int16_t *h_set, int16_t *h_clr, int16_t *t_set, int16_t *t_clr)
{
	uint8_t rx_buf[3];
	uint8_t raw_data[6];

	/* set limits */
	if (MIKROSDK_SHT_generic_read(SHT_CMD_RD_LO_SET, rx_buf, 3))
		return (SHT_ST_FAIL);

	MIKROSDK_SHT_convert_limit_data_to_raw_data(rx_buf, raw_data);
	MIKROSDK_SHT_convert_temp_hum(raw_data, h_set, t_set);

	/* clear limits */
	if (MIKROSDK_SHT_generic_read(SHT_CMD_RD_LO_CLR, rx_buf, 3))
		return (SHT_ST_FAIL);

	MIKROSDK_SHT_convert_limit_data_to_raw_data(rx_buf, raw_data);
	MIKROSDK_SHT_convert_temp_hum(raw_data, h_clr, t_clr);

	return (SHT_ST_OK);
}

/* has has alarm/interrupt been triggered */
uint8_t MIKROSDK_SHT_alarm_triggered(void)
{
	uint8_t alarm = sht_alarm;
	sht_alarm     = 0;
	return (alarm);
}

/* get alarm state */
uint8_t MIKROSDK_SHT_get_alarm(void)
{
	uint8_t state = SHT_ALARM_NOALARM;

	state           = sht_alarm_state;
	sht_alarm_state = SHT_ALARM_NOALARM;

	return state;
}

/* pin mapping function */
void MIKROSDK_SHT_pin_mapping(uint8_t reset, uint8_t alarm)
{
	cfg.rst     = reset;
	cfg.int_pin = alarm;
}

/* device initialisation */
uint8_t MIKROSDK_SHT_Initialise(void)
{
	/* don't call sht_cfg_setup() as this de-initialises pin mapping */
	// sht_cfg_setup(&cfg);
	cfg.i2c_speed               = I2C_MASTER_SPEED_STANDARD;
	cfg.i2c_address             = SHT_I2C_ADDR0;
	cfg.vars_cfg.clk_stretching = 1;
	cfg.vars_cfg.repeatability  = SHT_RPT_MEDIUM;
	cfg.vars_cfg.mps            = SHT_MPS_1;

	if (MIKROSDK_SHT_init())
		return (SHT_ST_FAIL);

	MIKROSDK_SHT_config();

#if (SHT_USE_INTERRUPT)
	/* start periodic mode */
	if (MIKROSDK_SHT_start_pm())
		return (SHT_ST_FAIL);
#else
	if (MIKROSDK_SHT_stop_pm())
		return (SHT_ST_FAIL);
#endif

	MIKROSDK_SHT_clear_status();

#if (SHT_USE_INTERRUPT)
	if (MIKROSDK_SHT_EnableInterrupt(cfg.int_pin))
		return (SHT_ST_FAIL);

	if (MIKROSDK_SHT_set_hi_alert_limits(
	        (SHT_LIMIT_H_HI), (SHT_LIMIT_H_HI - SHT_LIMIT_H_HYS), (SHT_LIMIT_T_HI), (SHT_LIMIT_T_HI - SHT_LIMIT_T_HYS)))
		return (SHT_ST_FAIL);

	if (MIKROSDK_SHT_set_lo_alert_limits(
	        (SHT_LIMIT_H_LO), (SHT_LIMIT_H_LO + SHT_LIMIT_H_HYS), (SHT_LIMIT_T_LO), (SHT_LIMIT_T_LO + SHT_LIMIT_T_HYS)))
		return (SHT_ST_FAIL);

#endif

	SENSORIF_I2C_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return (SHT_ST_OK);
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_SHT_Reinitialise(void)
{
	WAIT_ms(SHT_T_COMMS);

	return (SHT_ST_OK);
}

/* data acquisition */
/* Note: temperature: T('C)  = int16_t / 100 */
/* Note: humidity: H(%RH) = int16_t / 100 */
uint8_t MIKROSDK_SHT_Acquire(int16_t *humidity, int16_t *temperature)
{
	uint8_t status = SHT_ST_OK;
	uint8_t alarm  = SHT_ALARM_NOALARM;

	/* data acquisition */
	SENSORIF_I2C_Init();
#if (SHT_USE_INTERRUPT)
	alarm = MIKROSDK_SHT_get_alarm();
	if (MIKROSDK_SHT_get_temp_hum_pm(humidity, temperature))
		return (SHT_ST_FAIL);
#else
	if (MIKROSDK_SHT_get_temp_hum_ss(humidity, temperature))
		return (SHT_ST_FAIL);
#endif
	SENSORIF_I2C_Deinit();

#if (SHT_USE_INTERRUPT)
	if (alarm == SHT_ALARM_TRIGGERED)
		status = SHT_ST_ALARM_TRIGGERED;
	else if (alarm == SHT_ALARM_CLEARED)
		status = SHT_ST_ALARM_CLEARED;
#endif

	return status;
}
