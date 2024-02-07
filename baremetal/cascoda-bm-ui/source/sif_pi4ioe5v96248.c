/*
 *  Copyright (c) 2023, Cascoda Ltd.
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
 * Sensor interface for Diodes/Pericom PI4IOE5V96248 48-bit gpio expander
 * (Currently limited to 8 (port 0)
*/

#include "sif_pi4ioe5v96248.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* flag if interrupt has been triggered */
static uint8_t SIF_PI4IOE5V96248_irq = 0;

/* ISR for irq handling */
static int SIF_PI4IOE5V96248_Isr(void)
{
	/* set the interupt flag to active actions further up */
	SIF_PI4IOE5V96248_irq = 1;
	return 0;
}

/* enable interrupt function - this has to bypass mikrosdk driver and hal */
static uint8_t SIF_PI4IOE5V96248_EnableInterrupt(void)
{
	uint8_t                status;
	struct gpio_input_args args;

	status = BSP_ModuleDeregisterGPIOPin(SIF_PI4IOE5V96248_INT_PIN);
	if (!status)
	{
		args.mpin     = SIF_PI4IOE5V96248_INT_PIN;
		args.pullup   = MODULE_PIN_PULLUP_ON;
		args.debounce = MODULE_PIN_DEBOUNCE_ON;
		args.irq      = MODULE_PIN_IRQ_FALL;
		args.callback = SIF_PI4IOE5V96248_Isr;
		status        = BSP_ModuleRegisterGPIOInput(&args);
	}

	return (status);
}

/* modified read */
/* I2C slave address followed by read of register */
static uint8_t SIF_PI4IOE5V96248_generic_read(uint8_t *rx_buf, size_t len)
{
	uint32_t len32;
	uint8_t  status;

	SENSORIF_I2C_Init();

	/* read data */
	len32  = (uint32_t)len;
	status = SENSORIF_I2C_Read(SIF_PI4IOE5V96248_I2C_ADDR, rx_buf, &len32);

	SENSORIF_I2C_Deinit();

	if (status)
	{
		ca_log_warn("PI4IOE5V96248_GenericRead Read Fail: %02X", status);
		return (SIF_PI4IOE5V96248_ST_FAIL);
	}

	return (SIF_PI4IOE5V96248_ST_OK);
}

/* modified write */
static uint8_t SIF_PI4IOE5V96248_generic_write(uint8_t *tx_buf, size_t len)
{
	uint32_t len32;
	uint8_t  status;

	if (len > 6)
		return (SIF_PI4IOE5V96248_ST_FAIL);

	SENSORIF_I2C_Init();

	/* write register address and data */
	len32  = (uint32_t)len;
	status = SENSORIF_I2C_Write(SIF_PI4IOE5V96248_I2C_ADDR, tx_buf, &len32);

	SENSORIF_I2C_Deinit();

	if (status)
	{
		ca_log_warn("PI4IOE5V6408_GenericWrite Write Fail: %02X", status);
		return (SIF_PI4IOE5V96248_ST_FAIL);
	}

	return (SIF_PI4IOE5V96248_ST_OK);
}

/* has alarm/interrupt been triggered */
uint8_t SIF_PI4IOE5V96248_alarm_triggered(void)
{
	uint8_t alarm         = SIF_PI4IOE5V96248_irq;
	SIF_PI4IOE5V96248_irq = 0;
	return (alarm);
}

/* device initialisation */
uint8_t SIF_PI4IOE5V96248_Initialise(void)
{
	SENSORIF_I2C_Config(I2C_PORTNUM);

	/* reset expander chip */
	BSP_ModuleRegisterGPIOOutput(SIF_PI4IOE5V96248_RST_PIN, MODULE_PIN_TYPE_GENERIC);
	BSP_ModuleSetGPIOOutputPermanent(SIF_PI4IOE5V96248_RST_PIN);
	BSP_ModuleSetGPIOPin(SIF_PI4IOE5V96248_RST_PIN, 0);
	WAIT_ms(SIF_PI4IOE5V96248_T_RESET);
	BSP_ModuleSetGPIOPin(SIF_PI4IOE5V96248_RST_PIN, 1);

	if (SIF_PI4IOE5V96248_EnableInterrupt())
		return SIF_PI4IOE5V96248_ST_FAIL;

	return SIF_PI4IOE5V96248_ST_OK;
}

/* data acquisition */
/* Note: only port 0 used */
uint8_t SIF_PI4IOE5V96248_Acquire(uint8_t *port0)
{
	if (SIF_PI4IOE5V96248_generic_read(port0, 1))
		return SIF_PI4IOE5V96248_ST_FAIL;

	return SIF_PI4IOE5V96248_ST_OK;
}

/* set output */
uint8_t SIF_PI4IOE5V96248_SetOutput(uint8_t io, uint8_t val)
{
	uint8_t data;

	if (io > 7)
		return SIF_PI4IOE5V96248_ST_FAIL;

	/* read */
	if (SIF_PI4IOE5V96248_generic_read(&data, 1))
		return SIF_PI4IOE5V96248_ST_FAIL;

	/* modify */
	data = data & ~(1 << io) | (val << io);

	if (SIF_PI4IOE5V96248_generic_write(&data, 1))
		return SIF_PI4IOE5V96248_ST_FAIL;

	return SIF_PI4IOE5V96248_ST_OK;
}

/* sense input */
uint8_t SIF_PI4IOE5V96248_Sense(uint8_t io, uint8_t *val)
{
	uint8_t data;

	if (io > 7)
		return SIF_PI4IOE5V96248_ST_FAIL;

	/* read */
	if (SIF_PI4IOE5V96248_generic_read(&data, 1))
		return SIF_PI4IOE5V96248_ST_FAIL;

	/* modify */
	*val = (data >> io) & 0x01;

	return SIF_PI4IOE5V96248_ST_OK;
}
