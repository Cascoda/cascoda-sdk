/*
 * MikroSDK - MikroE Software Development Kit
 * Copyright© 2020 MikroElektronika d.o.o.
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE. 
 */

/*!
 * \file
 *
 */

#include "thermo3.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "ca821x_api.h"

static thermo3_t     thermo3; // Click object
static thermo3_cfg_t cfg;

void thermo3_cfg_setup(thermo3_cfg_t *cfg)
{
	// Communication gpio pins

	cfg->scl = HAL_PIN_NC;
	cfg->sda = HAL_PIN_NC;

	// Additional gpio pins

	cfg->al = HAL_PIN_NC;

	cfg->i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg->i2c_address = THERMO3_I2C_ADDR;
}

THERMO3_RETVAL thermo3_init(thermo3_t *ctx, thermo3_cfg_t *cfg)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg->i2c_speed;
	i2c_cfg.scl   = cfg->scl;
	i2c_cfg.sda   = cfg->sda;

	ctx->slave_address = cfg->i2c_address;

	if (i2c_master_open(&ctx->i2c, &i2c_cfg) == I2C_MASTER_ERROR)
	{
		return THERMO3_INIT_ERROR;
	}

	i2c_master_set_slave_address(&ctx->i2c, ctx->slave_address);
	i2c_master_set_speed(&ctx->i2c, cfg->i2c_speed);

	thermo3_default_cfg(ctx);

	return THERMO3_OK;
}

void thermo3_generic_write(thermo3_t *ctx, uint8_t reg, uint8_t *data_buf, uint8_t len)
{
	uint8_t tx_buf[256];
	uint8_t cnt;
	err_t   hal_status = HAL_I2C_MASTER_SUCCESS;

	tx_buf[0] = reg;

	for (cnt = 1; cnt <= len; cnt++)
	{
		tx_buf[cnt] = data_buf[cnt - 1];
	}

	hal_status = i2c_master_write(&ctx->i2c, tx_buf, len + 1);
	if (hal_status != HAL_I2C_MASTER_SUCCESS)
	{
		ca_log_warn("thermo3_generic_write() Error; write status: %02X", hal_status);
	}
}

void thermo3_generic_read(thermo3_t *ctx, uint8_t *tx_buf, uint8_t wlen, uint8_t *rx_buf, uint8_t rlen)
{
	i2c_master_write_then_read(&ctx->i2c, tx_buf, wlen, rx_buf, rlen);
}

void thermo3_default_cfg(thermo3_t *ctx)
{
	uint8_t config;
	uint8_t wdata[2] = {0, 0};
	config           = MIKROSDK_THERMO3_CONFIG_ONESHOT | MIKROSDK_THERMO3_CONFIG_SHUTDOWN;
	wdata[0]         = config;
	wdata[1]         = 0x00;
	thermo3_generic_write(ctx, ctx->slave_address, wdata, 2);
}

u16_t get_temperature()
{
	char  tmp_data[2];
	u8_t  wdata[2] = {0, 0};
	u16_t temp;

	tmp_data[0] = 0x00;
	wdata[0]    = THERMO3_I2C_ADDR;
	wdata[1]    = 0x00; //temperature register address

	thermo3_default_cfg(&thermo3);

	thermo3_generic_read(&thermo3, wdata, 2, tmp_data, 2);

	temp = (tmp_data[0] << 8) | tmp_data[1];

	return temp;
}

uint8_t MIKROSDK_THERMO3_Initialise(void)
{
	uint8_t status;

	//  Click initialization.
	thermo3_cfg_setup(&cfg);
	THERMO3_MAP_MIKROBUS(cfg);
	status = thermo3_init(&thermo3, &cfg);
	i2c_master_close(&thermo3.i2c);

	if (status)
		return (status);

	return (0);
}

// ------------------------------------------------------------------------- END
