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
 * Sensor interface for Maxim MAX30205 human body temperature sensor
*/

#include <stdio.h>
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "sif_max30205.h"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MAX30205: Write Configuration Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_MAX30205_WriteConfig(u8_t config)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata[2] = {0, 0};

	wdata[0] = 0x01;   /* configuration register address */
	wdata[1] = config; /* config data */
	num      = 2;
	status   = SENSORIF_I2C_Write((0x00 + (SIF_SAD_MAX30205 >> 1)), wdata, &num);
	if (status)
	{
		printf("SIF_MAX30205_WriteConfig() Error; write status: %02X\n", status);
		return (0xFF);
	}
	if (num != 2)
	{
		printf("SIF_MAX30205_WriteConfig() Error: bytes written: %02X\n", num);
		return (0xFF);
	}

	return (0x00);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MAX30205: Read Configuration Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_MAX30205_ReadConfig(u8_t *config)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata = 0;

	wdata  = 0x01; /* configuration register address */
	num    = 1;
	status = SENSORIF_I2C_Write((0x00 + (SIF_SAD_MAX30205 >> 1)), &wdata, &num);
	if (status)
	{
		printf("SIF_MAX30205_ReadConfig() Error; write status: %02X\n", status);
		return (0xFF);
	}
	if (num != 1)
	{
		printf("SIF_MAX30205_ReadConfig() Error: bytes written: %02X\n", num);
		return (0xFF);
	}

	num    = 1;
	status = SENSORIF_I2C_Read((0x00 + (SIF_SAD_MAX30205 >> 1)), config, &num);
	if (status)
	{
		printf("SIF_MAX30205_ReadConfig() Error; read status: %02X\n", status);
		return (0xFF);
	}
	if (num != 1)
	{
		printf("SIF_MAX30205_ReadConfig() Error: bytes read: %02X\n", num);
		return (0xFF);
	}

	return (0x00);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MAX30205: Read Temperature
 *******************************************************************************
 * \return Temperature as defined in MAX30205 Datasheet, normal format
 *******************************************************************************
 ******************************************************************************/
u16_t SIF_MAX30205_ReadTemperature(void)
{
	u8_t  config;
	u32_t num = 0;
	u8_t  status;
	u8_t  d7       = 0;
	u8_t  wdata    = 0;
	u8_t  rdata[2] = {0, 0};
	u16_t temp;

	/* read config first */
	status = SIF_MAX30205_ReadConfig(&config);
	if (status)
		return (0);

	/* always in shutdown mode */
	config += (SIF_MAX30205_CONFIG_ONESHOT | SIF_MAX30205_CONFIG_SHUTDOWN);
	status = SIF_MAX30205_WriteConfig(config);
	if (status)
		return (0);

	/* wait for conversion time */
	if (SIF_MAX30205_MODE == SIF_MAX30205_MODE_TCONV_WAIT)
		TIME_WaitTicks(SIF_MAX30205_TCONV_MAX_TEMP);

	if (SIF_MAX30205_MODE == SIF_MAX30205_MODE_POLL_ONE_SHOT)
	{
		while (!d7) /* check if oneshot bit has been reset */
		{
			status = SIF_MAX30205_ReadConfig(&config);
			if (status)
				return (0);
			d7 = config & SIF_MAX30205_CONFIG_ONESHOT;
		}
	}

	/* write temperature register address */
	wdata  = 0x00; /* temperature register address */
	num    = 1;
	status = SENSORIF_I2C_Write((0x00 + (SIF_SAD_MAX30205 >> 1)), &wdata, &num);
	if (status)
	{
		printf("SIF_MAX30205_ReadTemperature() Error; write status: %02X\n", status);
		return (0);
	}
	if (num != 1)
	{
		printf("SIF_MAX30205_ReadTemperature() Error: bytes written: %02X\n", num);
		return (0);
	}

	/* write temperature register address */
	wdata  = 0x00; /* temperature register address */
	num    = 1;
	status = SENSORIF_I2C_Write((0x00 + (SIF_SAD_MAX30205 >> 1)), &wdata, &num);
	if (status)
	{
		printf("SIF_MAX30205_ReadTemperature() Error; write status: %02X\n", status);
		return (0);
	}
	if (num != 1)
	{
		printf("SIF_MAX30205_ReadTemperature() Error: bytes written: %02X\n", num);
		return (0);
	}

	/* read temperature register */
	num    = 2;
	status = SENSORIF_I2C_Read((0x00 + (SIF_SAD_MAX30205 >> 1)), rdata, &num);
	if (status)
	{
		printf("SIF_MAX30205_ReadTemperature() Error; read status: %02X\n", status);
		return (0);
	}
	if (num != 2)
	{
		printf("SIF_MAX30205_ReadTemperature() Error: bytes read: %02X\n", num);
		return (0);
	}

	/* MSB is temperature in 1'C, LSB gives resolution of 0.003'C */
	temp = (rdata[0] << 8) + rdata[1];

	return (temp);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MAX30205: Initialise Sensor
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_MAX30205_Initialise(void)
{
	u8_t config;
	u8_t status;

	/* write config: one-shot mode, default otherwise */
	config = SIF_MAX30205_CONFIG_ONESHOT;
	status = SIF_MAX30205_WriteConfig(config);
	if (status)
		return (status);

	return (0);
}
