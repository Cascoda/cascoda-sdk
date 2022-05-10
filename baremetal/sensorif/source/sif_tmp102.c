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
 * Sensor interface for Texas TMP102 human body temperature sensor
*/

#include <stdio.h>
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "sif_tmp102.h"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Write Configuration Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_TMP102_WriteConfig(u8_t config)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata[3] = {0, 0, 0};

	wdata[0] = 0x01;   /* configuration register address */
	wdata[1] = config; /* config first byte */
	wdata[2] = 0x00;   /* config second byte */
	num      = 3;
	status   = SENSORIF_I2C_Write(SIF_SAD_TMP102, wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_WriteConfig() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 3)
	{
		ca_log_warn("() Error: bytes written: %02X", num);
		return (0xFF);
	}
	return (0x00);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Write Temperature Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_TMP102_WriteTemp()
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata[3] = {0, 0, 0};

	/*Setup the value for the low-limit register*/
	wdata[0] = 0x02; /* TL register address */
	wdata[1] = 0x17; /* TL = 23'C */
	wdata[2] = 0x00;
	num      = 3;
	status   = SENSORIF_I2C_Write(SIF_SAD_TMP102, wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_WriteTemp_Low() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 3)
	{
		ca_log_warn("() Error: bytes written: %02X", num);
		return (0xFF);
	}

	/*Setup the value for the high-limit register*/
	wdata[0] = 0x03; /* TH register address */
	wdata[1] = 0x1a; /* TH = 26'C */
	wdata[2] = 0x00;
	num      = 3;
	status   = SENSORIF_I2C_Write(SIF_SAD_TMP102, wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_WriteTemp_High() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 3)
	{
		ca_log_warn("() Error: bytes written: %02X", num);
		return (0xFF);
	}
	return (0x00);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Read Configuration Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_TMP102_ReadConfig(u8_t *config)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata = 0;

	wdata  = 0x01; /* configuration register address */
	num    = 1;
	status = SENSORIF_I2C_Write(SIF_SAD_TMP102, &wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_MAX30205_ReadConfig() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_MAX30205_ReadConfig() Error: bytes written: %02X", num);
		return (0xFF);
	}

	num    = 2;
	status = SENSORIF_I2C_Read(SIF_SAD_TMP102, config, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP1023_ReadConfig() Error; read status: %02X", status);
		return (0xFF);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_TMP1024_ReadConfig() Error: bytes read: %02X", num);
		return (0xFF);
	}
	return (0x00);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Read Temperature Low Limit Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_TMP102_ReadTempLow()
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata    = 0;
	u8_t  rdata[2] = {0, 0};

	wdata  = 0x02; /* TL register address */
	num    = 1;
	status = SENSORIF_I2C_Write(SIF_SAD_TMP102, &wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_ReadTempLow() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_TMP102_ReadTempLow() Error: bytes written: %02X", num);
		return (0xFF);
	}

	/* read temperature register */
	num    = 2;
	status = SENSORIF_I2C_Read(SIF_SAD_TMP102, rdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_ReadTempLow() Error; read status: %02X", status);
		return (0);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_TMP102_ReadTempLow() Error: bytes read: %02X", num);
		return (0);
	}
	printf("\nTL: %02X, %02X\n", rdata[0], rdata[1]);
	return (0x00);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Read Temperature High Limit Register
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
static u8_t SIF_TMP102_ReadTempHigh()
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata    = 0;
	u8_t  rdata[2] = {0, 0};

	wdata  = 0x03; /* TL register address */
	num    = 1;
	status = SENSORIF_I2C_Write(SIF_SAD_TMP102, &wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_ReadTempHigh() Error; write status: %02X", status);
		return (0xFF);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_TMP102_ReadTempHigh() Error: bytes written: %02X", num);
		return (0xFF);
	}

	/* read temperature register */
	num    = 2;
	status = SENSORIF_I2C_Read(SIF_SAD_TMP102, rdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_ReadTempHigh() Error; read status: %02X", status);
		return (0);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_TMP102_ReadTempHigh() Error: bytes read: %02X", num);
		return (0);
	}
	printf("TH: %02X, %02X\n", rdata[0], rdata[1]);
	return (0x00);
}

u16_t SIF_TMP102_ReadTemperature(void)
{
	u8_t  config;
	u32_t num = 0;
	u8_t  status;
	u8_t  d7       = 0;
	u8_t  wdata    = 0;
	u8_t  rdata[2] = {0, 0};
	u16_t temp;

	/* read config first */
	status = SIF_TMP102_ReadConfig(&config);
	if (status)
		return (0);
	status = SIF_TMP102_ReadTempLow();
	if (status)
		return (0);
	status = SIF_TMP102_ReadTempHigh();
	if (status)
		return (0);

	/* always in shutdown, oneshot, and interrupt mode */
	config = (SIF_TMP102_CONFIG_ONESHOT | SIF_TMP102_CONFIG_INTERRUPT);
	status = SIF_TMP102_WriteConfig(config);

	if (status)
		return (0);

	/* wait for conversion time */
	if (SIF_TMP102_MODE == SIF_TMP102_MODE_TCONV_WAIT)
		WAIT_ms(SIF_TMP102_TCONV_MAX_TEMP);

	if (SIF_TMP102_MODE == SIF_TMP102_MODE_POLL_ONE_SHOT)
	{
		while (!d7) /* check if oneshot bit has been reset */
		{
			status = SIF_TMP102_ReadConfig(&config);

			if (status)
				return (0);
			d7 = config & SIF_TMP102_CONFIG_ONESHOT;
		}
	}

	/* write temperature register address */
	wdata  = 0x00; /* temperature register address */
	num    = 1;
	status = SENSORIF_I2C_Write(SIF_SAD_TMP102, &wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_ReadTemperature() Error; write status: %02X", status);
		return (0);
	}
	if (num != 1)
	{
		ca_log_warn("SIF_TMP102_ReadTemperature() Error: bytes written: %02X", num);
		return (0);
	}

	/* read temperature register */
	num    = 2;
	status = SENSORIF_I2C_Read(SIF_SAD_TMP102, rdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_ReadTemperature() Error; read status: %02X", status);
		return (0);
	}
	if (num != 2)
	{
		ca_log_warn("SIF_TMP102_ReadTemperature() Error: bytes read: %02X", num);
		return (0);
	}

	/* MSB is temperature in 1'C, LSB gives resolution of 0.0625'C */
	temp = (rdata[0] << 8) | rdata[1];
	printf("Current temp: %02X, %02X\n", rdata[0], rdata[1]);
	return (temp);
}

u8_t SIF_TMP102_Initialise(void)
{
	u8_t config;
	u8_t status;
	/* write config: one-shot mode and interrupt mode, default otherwise */
	config = SIF_TMP102_CONFIG_ONESHOT | SIF_TMP102_CONFIG_INTERRUPT;
	printf("%02X", config);
	status = SIF_TMP102_WriteConfig(config);
	if (status)
		return (status);

	status = SIF_TMP102_WriteTemp();
	if (status)
		return (status);

	/* Register pin 33 on Chili2 to be general input pin*/
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
	    SIF_TMP102_ALERT_PIN, MODULE_PIN_PULLUP_ON, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_RISE, NULL});
	return (0);
}

void SIF_TMP102_Reset(void)
{
	u32_t num = 0;
	u8_t  status;
	u8_t  wdata[2] = {0, 0};
	wdata[0]       = 0x00; //general call address
	wdata[1]       = 0x06;

	num    = 2;
	status = SENSORIF_I2C_Write(SIF_SAD_TMP102, wdata, &num);
	if (status)
	{
		ca_log_warn("SIF_TMP102_Reset() Error; write status: %02X", status);
	}
	else if (num != 2)
	{
		ca_log_warn("SIF_TMP102_Reset() Error: bytes written: %02X", num);
	}
	WAIT_ms(20);
}