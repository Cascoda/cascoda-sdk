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
 * Sensor interface for Silicon Labs Si7021 temperature / humidity sensor
*/

#include <stdio.h>
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "sif_si7021.h"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Read Temperature
 *******************************************************************************
 * \return Temperature in 'C, 1s complement (-128 to +127 'C)
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_SI7021_ReadTemperature(void)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata    = 0;
	u8_t  rdata[2] = {0, 0};
	u16_t tread;
	i32_t tconv;
	u32_t tstart;

	/* write start measurement command */
	if (SIF_SI7021_MODE == SIF_SI7021_MODE_HOLD_MASTER)
		wdata = 0xE3;
	else
		wdata = 0xF3;
	num    = 1;
	status = SENSORIF_I2C_Write(SIF_SAD_SI7021, &wdata, &num);
	if (status)
	{
		printf("SIF_SI7021_ReadTemperature() Error; write status: %02X\n", status);
		return (0x00);
	}
	if (num != 1)
	{
		printf("SIF_SI7021_ReadTemperature() Error: bytes written: %02X\n", num);
		return (0x00);
	}

	/* wait for conversion time */
	if (SIF_SI7021_MODE == SIF_SI7021_MODE_TCONV_WAIT)
		TIME_WaitTicks(SIF_SI7021_TCONV_MAX_TEMP);

	/* read data */
	tstart = TIME_ReadAbsoluteTime();
	do
	{
		if ((TIME_ReadAbsoluteTime() - tstart) > SIF_SI7021_TCONV_MAX_TEMP)
		{
			printf("SIF_SI7021_ReadTemperature() Error; NACK timeout\n");
			return (0x00);
		}
		num = 2;
	} while ((status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num)) == SENSORIF_I2C_ST_RX_AD_NACK);

	if (status)
	{
		printf("SIF_SI7021_ReadTemperature() Error; read status: %02X\n", status);
		return (0x00);
	}
	if (num != 2)
	{
		printf("SIF_SI7021_ReadTemperature() Error: bytes read: %02X\n", num);
		return (0x00);
	}

	/* 16-bit read value */
	tread = (rdata[0] << 8) + rdata[1];
	// printf("TRAW=%u\n", tread);

	/* 32-bit converted value
	 * T = (175.7 * tread)/65536 - 46.85
	 *   = (176 * tread)/65536 - 46.85		error: 0.3 'C max.
	 *   = (176 * tread - 3070362)/65536	error: none
	 */
	tconv = (176 * (u32_t)tread - 3070362) / 65536;

	return (LS0_BYTE(tconv));
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Read Humidity
 *******************************************************************************
 * \return Humidity in % (0 to 100 %)
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_SI7021_ReadHumidity(void)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata    = 0;
	u8_t  rdata[2] = {0, 0};
	u16_t hread;
	i32_t hconv;
	u32_t tstart;

	/* write start measurement command */
	if (SIF_SI7021_MODE == SIF_SI7021_MODE_HOLD_MASTER)
		wdata = 0xE5;
	else
		wdata = 0xF5;
	num    = 1;
	status = SENSORIF_I2C_Write(SIF_SAD_SI7021, &wdata, &num);
	if (status)
	{
		printf("SIF_SI7021_ReadHumidity() Error; write status: %02X\n", status);
		return (0x00);
	}
	if (num != 1)
	{
		printf("SIF_SI7021_ReadHumidity() Error: bytes written: %02X\n", num);
		return (0x00);
	}

	/* wait for conversion time */
	if (SIF_SI7021_MODE == SIF_SI7021_MODE_TCONV_WAIT)
		TIME_WaitTicks(SIF_SI7021_TCONV_MAX_HUM);

	/* read data */
	tstart = TIME_ReadAbsoluteTime();
	do
	{
		if ((TIME_ReadAbsoluteTime() - tstart) > SIF_SI7021_TCONV_MAX_HUM)
		{
			printf("SIF_SI7021_ReadHumidity() Error; NACK timeout\n");
			return (0x00);
		}
		num = 2;
	} while ((status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num)) == SENSORIF_I2C_ST_RX_AD_NACK);

	if (status)
	{
		printf("SIF_SI7021_ReadHumidity() Error; read status: %02X\n", status);
		return (0x00);
	}
	if (num != 2)
	{
		printf("SIF_SI7021_ReadHumidity() Error: bytes read: %02X\n", num);
		return (0x00);
	}

	/* 16-bit read value */
	hread = (rdata[0] << 8) + rdata[1];
	// printf("HRAW=%u\n", hread);

	/* 32-bit converted value
	 * H = (125 * hread)/65536 - 6
	 *   = (125 * hread - 393216)/65536
	 */
	hconv = (125 * (u32_t)hread - 393216) / 65536;

	return (LS0_BYTE(hconv));
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Soft Reset Command
 *******************************************************************************
 ******************************************************************************/
void SIF_SI7021_Reset(void)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata = 0;

	wdata  = 0xFE;
	num    = 1;
	status = SENSORIF_I2C_Write(SIF_SAD_SI7021, &wdata, &num);
	if (status)
	{
		printf("SIF_SI7021_Reset() Error; write status: %02X\n", status);
	}
	else if (num != 1)
	{
		printf("SIF_SI7021_Reset() Error: bytes written: %02X\n", num);
	}
	TIME_WaitTicks(20);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Read ID byte of Electronic Serial Number
 *******************************************************************************
 * \return Sensr ID
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_SI7021_ReadID(void)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata[2] = {0, 0};
	u8_t  rdata[8] = {0, 0};

	/* reads electronic serial number and returns SNB3 byte:
	 * 0x0D: Si7013
	 * 0x14: Si7020
	 * 0x15: Si7021
	*/

	/* first access: */
	/* write command */
	wdata[0] = 0xFA;
	wdata[1] = 0x0F;
	num      = 2;
	status   = SENSORIF_I2C_Write(SIF_SAD_SI7021, wdata, &num);
	if (status)
	{
		printf("SIF_SI7021_ReadID() Error; write status: %02X\n", status);
		return (0x00);
	}
	if (num != 2)
	{
		printf("SIF_SI7021_ReadID() Error: bytes written: %02X\n", num);
		return (0x00);
	}
	/* read data */
	num    = 8;
	status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num);
	if (status)
	{
		printf("SIF_SI7021_ReadID() Error; read status: %02X\n", status);
		return (0x00);
	}
	if (num != 8)
	{
		printf("SIF_SI7021_ReadID() Error: bytes read: %02X\n", num);
		return (0x00);
	}

	/* second access: */
	/* write command */
	wdata[0] = 0xFC;
	wdata[1] = 0xC9;
	num      = 2;
	status   = SENSORIF_I2C_Write(SIF_SAD_SI7021, wdata, &num);
	if (status)
	{
		printf("SIF_SI7021_ReadID() Error; write status: %02X\n", status);
		return (0x00);
	}
	if (num != 2)
	{
		printf("SIF_SI7021_ReadID() Error: bytes written: %02X\n", num);
		return (0x00);
	}
	/* read data */
	num    = 6;
	status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num);
	if (status)
	{
		printf("SIF_SI7021_ReadID() Error; read status: %02X\n", status);
		return (0x00);
	}
	if (num != 6)
	{
		printf("SIF_SI7021_ReadID() Error: bytes read: %02X\n", num);
		return (0x00);
	}

	return (rdata[0]); /* 1st byte of 2nd access */
}
