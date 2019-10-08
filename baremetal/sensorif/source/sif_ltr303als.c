/*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Sensor interface for LITEON LTR-303ALS-01 ambient light sensor
*/

#include <stdio.h>
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "sif_ltr303als.h"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief LTR303ALS: Write Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_LTR303ALS_write_register(u8_t add, u8_t data)
{
	u8_t  status;
	u8_t  wdata[2];
	u32_t num;

	wdata[0] = add;
	wdata[1] = data;
	num      = 2;
	/* write register address and data */
	status = SENSORIF_I2C_Write(SIF_SAD_LTR303ALS, wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_LTR303ALS_write_register() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_LTR303ALS_write_register() Error: bytes written: %02X", num);
		return (0xFF);
	}

	return (0x00);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief LTR303ALS: Read Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_LTR303ALS_read_register(u8_t add, u8_t *data)
{
	u8_t  status;
	u8_t  wdata;
	u32_t num;

	wdata = add;
	num   = 1;
	/* write register address */
	status = SENSORIF_I2C_Write(SIF_SAD_LTR303ALS, &wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_LTR303ALS_read_register() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_LTR303ALS_read_register() Error: bytes written: %02X", num);
		return (0xFF);
	}

	/* read register */
	num    = 1;
	status = SENSORIF_I2C_Read(SIF_SAD_LTR303ALS, data, &num);
	if (status)
	{
		ca_log_warn("SIF_LTR303ALS_read_register() Error; read status: %02X", status);
		return (0xFF);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_LTR303ALS_read_register() Error: bytes read: %02X", num);
		return (0xFF);
	}

	return (0x00);
}

u8_t SIF_LTR303ALS_Initialise(void)
{
	u32_t tnow;
	u8_t  dbyte;
	u8_t  status;

	/* check startup time after power-up */
	tnow = TIME_ReadAbsoluteTime();
	if (tnow < SIF_LTR303ALS_TSTUP_POWERUP_MS)
		TIME_WaitTicks(SIF_LTR303ALS_TSTUP_POWERUP_MS - tnow);

	/* check part id and manufacturer id to see if correct device is connected */
	if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_PART_ID, &dbyte)))
		return (status);
	if (dbyte != SIF_LTR303ALS_PARTID)
	{
		ca_log_warn("SIF_LTR303ALS_Initialise() Error: Invalid Part ID %02X", dbyte);
		return (0xFF);
	}
	if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_MANUFAC_ID, &dbyte)))
		return (status);
	if (dbyte != SIF_LTR303ALS_MANFID)
	{
		ca_log_warn("SIF_LTR303ALS_Initialise() Error: Invalid Manufacturer ID %02X", dbyte);
		return (0xFF);
	}

	/* set integration time and measurement period */
	dbyte = (SIF_LTR303ALS_TINT << 3) + SIF_LTR303ALS_TMEAS;
	if ((status = SIF_LTR303ALS_write_register(REG_LTR303ALS_MEAS_RATE, dbyte)))
		return (status);

	/* set control register */
	dbyte = (SIF_LTR303ALS_GAIN << 2); /* gain */
	if (SIF_LTR303ALS_MODE == SIF_LTR303ALS_MODE_CONTINUOUS)
		dbyte += 0x01; /* active mode */
	if ((status = SIF_LTR303ALS_write_register(REG_LTR303ALS_CONTR, dbyte)))
		return (status);

	return (0);
}

u8_t SIF_LTR303ALS_Configure(u8_t gain, u8_t tint, u8_t tmeas)
{
	u8_t dbyte;
	u8_t status;

	/* gain: read-modify-write control register */
	if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_CONTR, &dbyte)))
		return (status);
	dbyte = (dbyte & 0xE3) + (gain << 2);
	if ((status = SIF_LTR303ALS_write_register(REG_LTR303ALS_CONTR, dbyte)))
		return (status);

	/* tint and tmeas */
	dbyte = (tint << 3) + tmeas;
	if ((status = SIF_LTR303ALS_write_register(REG_LTR303ALS_MEAS_RATE, dbyte)))
		return (status);

	return (0);
}

u8_t SIF_LTR303ALS_ReadLight(u16_t *pch0, u16_t *pch1)
{
	u8_t creg;
	u8_t dbyte;
	u8_t status;
	u8_t countdown = 255;

	/* oneshot: standby to active mode */
	if (SIF_LTR303ALS_MODE == SIF_LTR303ALS_MODE_POLL_ONE_SHOT)
	{
		/* read-modify-write control register as not to alter gain setting */
		if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_CONTR, &creg)))
			return (status);
		creg |= 0x01; /* set active mode */
		if ((status = SIF_LTR303ALS_write_register(REG_LTR303ALS_CONTR, creg)))
			return (status);
	}

	TIME_WaitTicks(10);

	/* check status */
	/* wait for new data */
	dbyte = 0x00;
	while (!(dbyte & 0x04))
	{
		if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_STATUS, &dbyte)))
			return (status);
		if (!(--countdown))
			return (0xFF);
	}
	/* check data valid */
	if (dbyte & 0x80)
	{
		ca_log_warn("SIF_LTR303ALS_ReadLight(): measured Data Invalid");
		return (0xFF);
	}

	/* read data */
	if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_DATA_CH1_0, &dbyte)))
		return (status);
	*pch1 = dbyte;
	if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_DATA_CH1_1, &dbyte)))
		return (status);
	*pch1 += (dbyte << 8);
	if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_DATA_CH0_0, &dbyte)))
		return (status);
	*pch0 = dbyte;
	if ((status = SIF_LTR303ALS_read_register(REG_LTR303ALS_DATA_CH0_1, &dbyte)))
		return (status);
	*pch0 += (dbyte << 8);

	/* oneshot: active to standby mode */
	if (SIF_LTR303ALS_MODE == SIF_LTR303ALS_MODE_POLL_ONE_SHOT)
	{
		creg &= 0xFE; /* set standby mode */
		if ((status = SIF_LTR303ALS_write_register(REG_LTR303ALS_CONTR, creg)))
			return (status);
	}

	return (0);
}
