/*
 *  Copyright (c) 2019, Cascoda Ltd.
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
 * Sensor interface for Silicon Labs Si7021 temperature / humidity sensor
*/

#include <stdio.h>
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "sif_si7021.h"

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
		ca_log_warn("SIF_SI7021_ReadTemperature() Error; write status: %02X", status);
		return (0x00);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_SI7021_ReadTemperature() Error: bytes written: %02X", num);
		return (0x00);
	}

	/* wait for conversion time */
	if (SIF_SI7021_MODE == SIF_SI7021_MODE_TCONV_WAIT)
		WAIT_ms(SIF_SI7021_TCONV_MAX_TEMP);

	/* read data */
	tstart = TIME_ReadAbsoluteTime();
	do
	{
		if ((TIME_ReadAbsoluteTime() - tstart) > SIF_SI7021_TCONV_MAX_TEMP)
		{
			ca_log_warn("SIF_SI7021_ReadTemperature() Error; NACK timeout");
			return (0x00);
		}
		num = 2;
	} while ((status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num)) == SENSORIF_I2C_ST_RX_AD_NACK);

	if (status)
	{
		ca_log_warn("SIF_SI7021_ReadTemperature() Error; read status: %02X", status);
		return (0x00);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_SI7021_ReadTemperature() Error: bytes read: %02X", num);
		return (0x00);
	}

	/* 16-bit read value */
	tread = (rdata[0] << 8) + rdata[1];
	// ca_log_warn("TRAW=%u", tread);

	/* 32-bit converted value
	 * T = (175.7 * tread)/65536 - 46.85
	 *   = (176 * tread)/65536 - 46.85		error: 0.3 'C max.
	 *   = (176 * tread - 3070362)/65536	error: none
	 */
	tconv = (176 * (i32_t)tread - 3070362) / 65536;

	return (LS0_BYTE(tconv));
}

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
		ca_log_warn("SIF_SI7021_ReadHumidity() Error; write status: %02X", status);
		return (0x00);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_SI7021_ReadHumidity() Error: bytes written: %02X", num);
		return (0x00);
	}

	/* wait for conversion time */
	if (SIF_SI7021_MODE == SIF_SI7021_MODE_TCONV_WAIT)
		WAIT_ms(SIF_SI7021_TCONV_MAX_HUM);

	/* read data */
	tstart = TIME_ReadAbsoluteTime();
	do
	{
		if ((TIME_ReadAbsoluteTime() - tstart) > SIF_SI7021_TCONV_MAX_HUM)
		{
			ca_log_warn("SIF_SI7021_ReadHumidity() Error; NACK timeout");
			return (0x00);
		}
		num = 2;
	} while ((status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num)) == SENSORIF_I2C_ST_RX_AD_NACK);

	if (status)
	{
		ca_log_warn("SIF_SI7021_ReadHumidity() Error; read status: %02X", status);
		return (0x00);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_SI7021_ReadHumidity() Error: bytes read: %02X", num);
		return (0x00);
	}

	/* 16-bit read value */
	hread = (rdata[0] << 8) + rdata[1];
	// ca_log_warn("HRAW=%u", hread);

	/* 32-bit converted value
	 * H = (125 * hread)/65536 - 6
	 *   = (125 * hread - 393216)/65536
	 */
	hconv = (125 * (u32_t)hread - 393216) / 65536;

	return (LS0_BYTE(hconv));
}

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
		ca_log_warn("SIF_SI7021_Reset() Error; write status: %02X", status);
	}
	else if (num != 1)
	{
		ca_log_warn("SIF_SI7021_Reset() Error: bytes written: %02X", num);
	}
	WAIT_ms(20);
}

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
		ca_log_warn("SIF_SI7021_ReadID() Error; write status: %02X", status);
		return (0x00);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_SI7021_ReadID() Error: bytes written: %02X", num);
		return (0x00);
	}
	/* read data */
	num    = 8;
	status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num);
	if (status)
	{
		ca_log_warn("SIF_SI7021_ReadID() Error; read status: %02X", status);
		return (0x00);
	}
	if (num != 8)
	{
		ca_log_warn("SIF_SI7021_ReadID() Error: bytes read: %02X", num);
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
		ca_log_warn("SIF_SI7021_ReadID() Error; write status: %02X", status);
		return (0x00);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_SI7021_ReadID() Error: bytes written: %02X", num);
		return (0x00);
	}
	/* read data */
	num    = 6;
	status = SENSORIF_I2C_Read(SIF_SAD_SI7021, rdata, &num);
	if (status)
	{
		ca_log_warn("SIF_SI7021_ReadID() Error; read status: %02X", status);
		return (0x00);
	}
	if (num != 6)
	{
		ca_log_warn("SIF_SI7021_ReadID() Error: bytes read: %02X", num);
		return (0x00);
	}

	return (rdata[0]); /* 1st byte of 2nd access */
}
