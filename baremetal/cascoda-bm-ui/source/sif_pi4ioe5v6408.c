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
 * Sensor interface for Diodes/Pericom PI4IOE5V6408 8-bit gpio expander
*/

#include <stdio.h>
#include <stdlib.h>

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "sif_pi4ioe5v6408.h"

uint8_t SIF_PI4IOE5V6408_HasInterrupt = 0;

/* ISR for irq handling */
static int SIF_PI4IOE5V6408_Isr(void)
{
	SIF_PI4IOE5V6408_HasInterrupt = 1;

	/* wakeup only */
	return 0;
}

/* check interrupt status */
bool SIF_PI4IOE5V6408_IsInterruptActive(void)
{
	uint8_t val;

	BSP_ModuleSenseGPIOPin(SIF_PI4IOE5V6408_INT_PIN, &val);

	if (SIF_PI4IOE5V6408_HasInterrupt == 1)
	{
		SIF_PI4IOE5V6408_HasInterrupt = 0;
		return true;
	}

	if (val == 1)
		return false;

	return true;
}

/* enable interrupt function */
static uint8_t SIF_PI4IOE5V6408_EnableInterrupt(void)
{
	struct gpio_input_args args;

	if (BSP_ModuleDeregisterGPIOPin(SIF_PI4IOE5V6408_INT_PIN))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	args.mpin     = SIF_PI4IOE5V6408_INT_PIN;
	args.pullup   = MODULE_PIN_PULLUP_ON;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_FALL;
	args.callback = SIF_PI4IOE5V6408_Isr;

	SIF_PI4IOE5V6408_HasInterrupt = 0;

	if (BSP_ModuleRegisterGPIOInput(&args))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* generic i2c write; reg is register address, data length fixed to 1 */
static uint8_t SIF_PI4IOE5V6408_GenericWrite(uint8_t reg, uint8_t txdat, uint8_t init)
{
	uint8_t  tx_buf[SIF_PI4IOE5V6408_ADDLEN + SIF_PI4IOE5V6408_DATALEN];
	uint32_t len32;
	uint8_t  status;
	uint8_t  cnt;

	SENSORIF_I2C_Init();

	/* write register address and data */
	tx_buf[0] = reg;
	tx_buf[1] = txdat;
	len32     = SIF_PI4IOE5V6408_ADDLEN + SIF_PI4IOE5V6408_DATALEN;

	status = SENSORIF_I2C_Write(SIF_PI4IOE5V6408_I2C_ADDR, tx_buf, &len32);

	SENSORIF_I2C_Deinit();

	if (status)
	{
		/* check - display board might not be connected */
		if ((!init) || (status != SENSORIF_I2C_ST_TX_AD_NACK))
		{
			ca_log_warn("PI4IOE5V6408_GenericWrite Write Fail: %02X", status);
			return (SIF_PI4IOE5V6408_ST_FAIL);
		}
		else
		{
			return (SIF_PI4IOE5V6408_ST_UNAVAILABLE);
		}
	}

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* generic i2c read; reg is register start address */
static uint8_t SIF_PI4IOE5V6408_GenericRead(uint8_t reg, uint8_t *rxdat)
{
	uint8_t  tx_buf[SIF_PI4IOE5V6408_ADDLEN];
	uint32_t len32;
	uint8_t  status;

	SENSORIF_I2C_Init();

	/* write register address */
	tx_buf[0] = reg;
	len32     = (uint32_t)SIF_PI4IOE5V6408_ADDLEN;
	status    = SENSORIF_I2C_Write(SIF_PI4IOE5V6408_I2C_ADDR, tx_buf, &len32);
	if (status)
	{
		ca_log_warn("PI4IOE5V6408_GenericRead Write Fail: %02X", status);
		return (SIF_PI4IOE5V6408_ST_FAIL);
	}

	/* read data */
	len32  = (uint32_t)SIF_PI4IOE5V6408_DATALEN;
	status = SENSORIF_I2C_Read(SIF_PI4IOE5V6408_I2C_ADDR, rxdat, &len32);

	SENSORIF_I2C_Deinit();

	if (status)
	{
		ca_log_warn("PI4IOE5V6408_GenericRead Read Fail: %02X", status);
		return (SIF_PI4IOE5V6408_ST_FAIL);
	}

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* perform a software reset */
static uint8_t SIF_PI4IOE5V6408_SoftReset(void)
{
	uint8_t status;

	/* reset chip */
	status = SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_ID_CTRL, SIF_PI4IOE5V6408_CTRL_RESET, 1);
	if (status)
		return (status);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* set inputs default value */
static uint8_t SIF_PI4IOE5V6408_SetInputDefaults(uint8_t val)
{
	uint8_t data;

	/* all pull-ups (no pull-downs supported) */
	data = 0xFF;
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_PUP_DOWNB, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* set input defaults, all bits the same default value */
	if (val)
		data = 0xFF;
	else
		data = 0x00;
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_INP_DEFAULT, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* diable all interrrupts */
	data = 0xFF;
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_IRQ_MASK, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* read interrupt status to clear irq */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_IRQ_STATUS, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* device initialisation */
uint8_t SIF_PI4IOE5V6408_Initialise(void)
{
	uint8_t chip_id = 0;
	uint8_t rev_id  = 0;
	uint8_t status;

	/* perform a software reset in case system has been re-initialised */
	status = SIF_PI4IOE5V6408_SoftReset();
	if (status)
		return (status);

	WAIT_ms(SIF_PI4IOE5V6408_T_RESET);

	/* read chip id and revision id to check comms with sensor */
	if (SIF_PI4IOE5V6408_ReadDeviceId(&chip_id, &rev_id))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* all i/o by default input, set to default high */
	if (SIF_PI4IOE5V6408_SetInputDefaults(SIF_PI4IOE5V6408_STATE_HI))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* enable interrupt */
	if (SIF_PI4IOE5V6408_EnableInterrupt())
		return (SIF_PI4IOE5V6408_ST_FAIL);

	ca_log_info("PI4IOE5V6408 initialised, CHIP_ID: %02X; REV: %02X", chip_id, rev_id);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* read device id and firmware revision */
uint8_t SIF_PI4IOE5V6408_ReadDeviceId(uint8_t *chip_id, uint8_t *rev_id)
{
	uint8_t rx_buf;

	/* read serial number data */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_ID_CTRL, &rx_buf))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* conversion */
	*chip_id = (rx_buf & SIF_PI4IOE5V6408_ID_DEV_ID) >> SIF_PI4IOE5V6408_ID_DEV_ID_SHR;
	*rev_id  = (rx_buf & SIF_PI4IOE5V6408_ID_FW_REV) >> SIF_PI4IOE5V6408_ID_FW_REV_SHR;

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* sense input */
uint8_t SIF_PI4IOE5V6408_SenseInput(uint8_t io, uint8_t *val)
{
	uint8_t data;

	if (io > 7)
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* read inputs */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_INP_STATUS, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* extract value */
	*val = (data >> io) & 0x01;

	/* read interrupt status to clear irq */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_IRQ_STATUS, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* sense output */
uint8_t SIF_PI4IOE5V6408_SenseOutput(uint8_t io, uint8_t *val)
{
	uint8_t data;

	if (io > 7)
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* read inputs */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_OUT_STATE, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* extract value */
	*val = (data >> io) & 0x01;

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* sense all inputs */
uint8_t SIF_PI4IOE5V6408_SenseAllInputs(uint8_t *port)
{
	uint8_t data;

	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_INP_STATUS, port))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* read interrupt status to clear irq */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_IRQ_STATUS, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* set output */
uint8_t SIF_PI4IOE5V6408_SetOutput(uint8_t io, uint8_t val)
{
	uint8_t data;

	if (io > 7)
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* set output state */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_OUT_STATE, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);
	data = data & ~(1 << io) | (val << io);
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_OUT_STATE, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* register input pin */
uint8_t SIF_PI4IOE5V6408_ConfigureInput(uint8_t io, uint8_t irq_on, uint8_t pullup_on)
{
	uint8_t data;

	if (io > 7)
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* i/o direction */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_IO_DIR, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);
	data &= ~(1 << io);
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_IO_DIR, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* pull-up enable */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_PUP_ENABLE, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);
	data = data & ~(1 << io) | (pullup_on << io);
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_PUP_ENABLE, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* interrupt mask */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_IRQ_MASK, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);
	data = data & ~(1 << io) | ((1 - irq_on) << io); /* 1 is irq masked/off */
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_IRQ_MASK, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	return (SIF_PI4IOE5V6408_ST_OK);
}

/* register output pin */
uint8_t SIF_PI4IOE5V6408_ConfigureOutput(uint8_t io)
{
	uint8_t data;

	if (io > 7)
		return (SIF_PI4IOE5V6408_ST_FAIL);

	/* i/o direction */
	if (SIF_PI4IOE5V6408_GenericRead(SIF_PI4IOE5V6408_REG_IO_DIR, &data))
		return (SIF_PI4IOE5V6408_ST_FAIL);
	data |= (1 << io);
	if (SIF_PI4IOE5V6408_GenericWrite(SIF_PI4IOE5V6408_REG_IO_DIR, data, 0))
		return (SIF_PI4IOE5V6408_ST_FAIL);

	return (SIF_PI4IOE5V6408_ST_OK);
}
