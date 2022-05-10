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
#include "airquality4.h"
#include <stdio.h>
#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-util/cascoda_time.h"

// ------------------------------------------------ PRIVATE VARIABLES
static airquality4_t airquality4;
// ------------------------------------------------ PRIVATE FUNCTION DEFINITIONS

static void transfer_delay(void);
static void long_transfer_delay(void);

/**
 * @brief Write data to slave
 * @details This function append the slave address to the first byte of data buffer.
 * @param[in] ctx : Click context object.
 * @param[in] slave_addr : Slave address.
 * @param[in] tx_buf : data buffer .
 * @param[in] len : data buffer length.
 *
 *
 */

void write_data(airquality4_t *ctx, uint8_t slave_addr, uint8_t *tx_buf, size_t *len);

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

void airquality4_cfg_setup(airquality4_cfg_t *cfg)
{
	// Communication gpio pins

	cfg->scl = HAL_PIN_NC;
	cfg->sda = HAL_PIN_NC;

	cfg->i2c_speed   = I2C_MASTER_SPEED_STANDARD;
	cfg->i2c_address = AIRQUALITY4_SLAVE_ADDR;
}

AIRQUALITY4_RETVAL airquality4_init(airquality4_t *ctx, airquality4_cfg_t *cfg)
{
	i2c_master_config_t i2c_cfg;

	i2c_master_configure_default(&i2c_cfg);
	i2c_cfg.speed = cfg->i2c_speed;
	i2c_cfg.scl   = cfg->scl;
	i2c_cfg.sda   = cfg->sda;

	ctx->slave_address = cfg->i2c_address;

	if (i2c_master_open(&ctx->i2c, &i2c_cfg) == I2C_MASTER_ERROR)
	{
		return AIRQUALITY4_INIT_ERROR;
	}

	i2c_master_set_slave_address(&ctx->i2c, ctx->slave_address);
	i2c_master_set_speed(&ctx->i2c, cfg->i2c_speed);

	return AIRQUALITY4_OK;
}

void air_quality4_get_id(airquality4_t *ctx, uint8_t *data_buf)
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x36;
	tx_buf[1] = 0x82;

	write_data(ctx, ctx->slave_address, tx_buf, &num_w);

	transfer_delay();

	i2c_master_set_slave_address(&ctx->i2c, ctx->slave_address);

	i2c_master_read(&ctx->i2c, data_buf, 6);
}

void airquality4_default_cfg(airquality4_t *ctx)
{
	air_quality4_dev_init(ctx);
}

void air_quality4_dev_init(airquality4_t *ctx)
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x20;
	tx_buf[1] = 0x03;

	write_data(ctx, ctx->slave_address, tx_buf, &num_w);

	long_transfer_delay();
}

void air_quality4_measure_quality(airquality4_t *ctx, uint8_t *read_air)
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x20;
	tx_buf[1] = 0x08;

	write_data(ctx, ctx->slave_address, tx_buf, &num_w);

	transfer_delay();

	i2c_master_set_slave_address(&ctx->i2c, ctx->slave_address);

	i2c_master_read(&ctx->i2c, read_air, 6);
}

void air_quality4_get_co2_and_tvoc(uint16_t *value)
{
	uint8_t read_air[6] = {0};

	air_quality4_measure_quality(&airquality4, read_air);

	value[0] = read_air[0];
	value[0] <<= 8;
	value[0] |= read_air[1];

	value[1] = read_air[3];
	value[1] <<= 8;
	value[1] |= read_air[4];
}

void air_quality4_measure_signal(airquality4_t *ctx, uint8_t *read_air)
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x20;
	tx_buf[1] = 0x50;

	write_data(ctx, ctx->slave_address, tx_buf, &num_w);

	long_transfer_delay();
	transfer_delay();

	i2c_master_set_slave_address(&ctx->i2c, ctx->slave_address);
	i2c_master_read(&ctx->i2c, read_air, 6);
	transfer_delay();
}

void air_quality4_get_h2_and_ethon(uint16_t *value)
{
	uint8_t read_air[6] = {0};

	air_quality4_measure_signal(&airquality4, read_air);

	value[0] = read_air[0];
	value[0] <<= 8;
	value[0] |= read_air[1];

	value[1] = read_air[3];
	value[1] <<= 8;
	value[1] |= read_air[4];

	printf("\n H2: ");
	for (int i = 0; i < 6; i++)
	{
		printf("%02X", read_air[i]);
		if (i == 2)
		{
			printf("\n Ethanol: ");
		}
	}
	printf("\n");
}

void air_quality4_get_baseline(airquality4_t *ctx, uint8_t *read_air)
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x20;
	tx_buf[1] = 0x15;

	write_data(ctx, ctx->slave_address, tx_buf, &num_w);

	transfer_delay();

	i2c_master_set_slave_address(&ctx->i2c, airquality4.slave_address);
	i2c_master_read(&ctx->i2c, read_air, 6);
	transfer_delay();
}

void air_quality4_set_baseline()
{
	size_t  num_w = 8;
	uint8_t tx_buf[num_w];

	air_quality4_get_baseline(&airquality4, tx_buf + 2);
	tx_buf[0] = 0x20;
	tx_buf[1] = 0x1E;

	write_data(&airquality4, airquality4.slave_address, tx_buf, &num_w);

	transfer_delay();
}

void air_quality4_soft_reset()
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x00;
	tx_buf[1] = 0x06;

	i2c_master_write(&airquality4.i2c, tx_buf, num_w);

	transfer_delay();

	air_quality4_dev_init(&airquality4);
}

void air_quality4_get_version(uint8_t *version)
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x20;
	tx_buf[1] = 0x2F;

	write_data(&airquality4, airquality4.slave_address, tx_buf, &num_w);

	transfer_delay();

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	i2c_master_read(&airquality4.i2c, version, 3);

	transfer_delay();
}

void air_quality4_measure_test(uint8_t *value)
{
	size_t  num_w = 2;
	uint8_t tx_buf[num_w];

	tx_buf[0] = 0x20;
	tx_buf[1] = 0x32;

	write_data(&airquality4, airquality4.slave_address, tx_buf, &num_w);

	transfer_delay();

	i2c_master_set_slave_address(&airquality4.i2c, airquality4.slave_address);
	i2c_master_read(&airquality4.i2c, value, 3);
}

uint8_t MIKROSDK_AIRQUALITY4_Initialise(void)
{
	airquality4_cfg_t cfg;
	airquality4_cfg_setup(&cfg);
	AIRQUALITY4_MAP_MIKROBUS(cfg);
	airquality4_init(&airquality4, &cfg);
	airquality4_default_cfg(&airquality4);

	i2c_master_close(&airquality4.i2c);
}

// ------------------------------------------------------------------------- PRIVATE FUNCTIONS
static void transfer_delay(void)
{
	BSP_WaitTicks(10);
}

static void long_transfer_delay(void)
{
	BSP_WaitTicks(100);
}

void write_data(airquality4_t *ctx, uint8_t slave_addr, uint8_t *tx_buf, size_t *len)
{
	uint8_t data_buf[256];
	uint8_t cnt;
	size_t  length     = ++*len;
	err_t   hal_status = HAL_I2C_MASTER_SUCCESS;

	data_buf[0] = slave_addr;

	for (cnt = 1; cnt <= *len; cnt++)
	{
		data_buf[cnt] = tx_buf[cnt - 1];
	}

	i2c_master_write(&ctx->i2c, data_buf, length);
}
// ------------------------------------------------------------------------- END
