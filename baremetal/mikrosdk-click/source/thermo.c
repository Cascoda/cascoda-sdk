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
#include "thermo.h"
#include <stdio.h>
#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

// ------------------------------------------------------------- PRIVATE MACROS

#define THERMO_DUMMY 0

// ---------------------------------------------- PRIVATE VARIABLES

static thermo_t thermo; // Click object

// ---------------------------------------------- PRIVATE FUNCTION DECLARATIONS

static void transfer_delay(void);

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

void thermo_cfg_setup(thermo_cfg_t *cfg)
{
	cfg->sck  = HAL_PIN_NC;
	cfg->miso = HAL_PIN_NC;
	cfg->mosi = HAL_PIN_NC;
	cfg->cs   = HAL_PIN_NC;

	cfg->spi_speed   = 100000;
	cfg->spi_mode    = SPI_MASTER_MODE_0;
	cfg->cs_polarity = SPI_MASTER_CHIP_SELECT_POLARITY_ACTIVE_LOW;
}

THERMO_RETVAL thermo_init(thermo_t *ctx, thermo_cfg_t *cfg)
{
	spi_master_config_t spi_cfg;

	spi_master_configure_default(&spi_cfg);
	spi_cfg.speed              = cfg->spi_speed;
	spi_cfg.sck                = cfg->sck;
	spi_cfg.miso               = cfg->miso;
	spi_cfg.mosi               = cfg->mosi;
	spi_cfg.default_write_data = THERMO_DUMMY;

	digital_out_init(&ctx->cs, cfg->cs);
	ctx->chip_select = cfg->cs;

	if (spi_master_open(&ctx->spi, &spi_cfg) == SPI_MASTER_ERROR)
	{
		return THERMO_INIT_ERROR;
	}

	spi_master_set_default_write_data(&ctx->spi, THERMO_DUMMY);
	spi_master_set_speed(&ctx->spi, cfg->spi_speed);
	spi_master_set_mode(&ctx->spi, cfg->spi_mode);
	spi_master_set_chip_select_polarity(cfg->cs_polarity);

	return THERMO_OK;
}

uint32_t thermo_read_data(thermo_t *ctx)
{
	uint8_t  buffer[4] = {0};
	uint32_t result;

	spi_master_select_device(ctx->chip_select);
	spi_master_read(&ctx->spi, buffer, 4);
	spi_master_deselect_device(ctx->chip_select);

	result = buffer[0];
	result <<= 8;
	result |= buffer[1];
	result <<= 8;
	result |= buffer[2];
	result <<= 8;
	result |= buffer[3];

	return result;
}

uint16_t thermo_get_temperature(void)
{
	uint8_t  buffer[4] = {0};
	uint16_t temp_data;
	spi_master_select_device(thermo.chip_select);
	spi_master_read(&thermo.spi, buffer, 4);
	spi_master_deselect_device(thermo.chip_select);

	temp_data = buffer[0];
	temp_data <<= 8;
	temp_data |= buffer[1];

	return (temp_data >> 2);
}

uint16_t thermo_get_junction_temperature(void)
{
	uint32_t temp_all_data;
	uint16_t temp_data;
	float    temperature;

	temp_all_data = thermo_read_data(&thermo);

	temp_data = (uint16_t)temp_all_data;

	return (temp_data >> 4);
}

uint8_t thermo_check_fault(thermo_t *ctx)
{
	uint32_t tmp;

	tmp = thermo_read_data(ctx);

	tmp >>= 16;
	tmp &= 0x01;

	return tmp;
}

uint8_t thermo_short_circuited_vcc(void)
{
	uint32_t tmp;

	tmp = thermo_read_data(&thermo);

	tmp >>= 2;
	tmp &= 0x01;

	return tmp;
}

uint8_t thermo_short_circuited_gnd(void)
{
	uint32_t tmp;

	tmp = thermo_read_data(&thermo);

	tmp >>= 1;
	tmp &= 0x01;

	return tmp;
}

uint8_t thermo_check_connections(void)
{
	uint32_t tmp;

	tmp = thermo_read_data(&thermo);

	tmp &= 0x01;

	return tmp;
}

uint8_t MIKROSDK_THERMO_Initialise(void)
{
	thermo_cfg_t cfg;
	thermo_cfg_setup(&cfg);
	THERMO_MAP_MIKROBUS(cfg);
	thermo_init(&thermo, &cfg);

	if (thermo_check_fault(&thermo))
	{
		ca_log_warn("Unable to initialise THERMO click");
	}
	else
	{
		printf("Status OK\n");
	}
	spi_master_close(&thermo.spi);
}

// ----------------------------------------------- PRIVATE FUNCTION DEFINITIONS

static void transfer_delay(void)
{
	BSP_WaitTicks(350);
}

// ------------------------------------------------------------------------- END
