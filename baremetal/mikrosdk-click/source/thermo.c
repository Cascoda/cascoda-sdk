/**
 * @file
 * @brief mikrosdk interface
 */
/*
 *  Copyright (c) 2022, Cascoda Ltd.
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
 * Example click interface driver
*/

/* include <device>_drv.h and <device>_click.h */
#include "thermo_click.h"
#include "thermo_drv.h"

/* include cascoda-bm code if required */
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"

/* declare <device>_t <device> and <device>_cfg_t cfg structures for click objects */
static thermo_t     thermo;
static thermo_cfg_t cfg;

/* driver initialisation */
uint8_t MIKROSDK_THERMO_init(void)
{
	spi_master_config_t spi_cfg;

	spi_master_configure_default(&spi_cfg);
	spi_cfg.speed              = cfg.spi_speed;
	spi_cfg.sck                = cfg.sck;
	spi_cfg.miso               = cfg.miso;
	spi_cfg.mosi               = cfg.mosi;
	spi_cfg.default_write_data = 0;

	digital_out_init(&thermo.cs, cfg.cs);
	thermo.chip_select = cfg.cs;

	if (spi_master_open(&thermo.spi, &spi_cfg) == SPI_MASTER_ERROR)
		return THERMO_ST_FAIL;

	spi_master_set_default_write_data(&thermo.spi, 0);
	spi_master_set_speed(&thermo.spi, cfg.spi_speed);
	spi_master_set_mode(&thermo.spi, cfg.spi_mode);
	spi_master_set_chip_select_polarity(cfg.cs_polarity);
	spi_master_deselect_device(thermo.chip_select);

	return THERMO_ST_OK;
}

/* modified read - no delays */
static uint8_t MIKROSDK_THERMO_read_data(uint32_t *data)
{
	uint8_t  status;
	uint8_t  buffer[4] = {0};
	uint32_t result;

	spi_master_select_device(thermo.chip_select);
	if (spi_master_read(&thermo.spi, buffer, 4))
		status = THERMO_ST_FAIL;
	else
		status = THERMO_ST_OK;
	spi_master_deselect_device(thermo.chip_select);

	result = buffer[0];
	result <<= 8;
	result |= buffer[1];
	result <<= 8;
	result |= buffer[2];
	result <<= 8;
	result |= buffer[3];

	*data = result;

	return status;
}

/* check fault condition in read data */
uint8_t MIKROSDK_THERMO_check_fault(void)
{
	uint32_t tmp;

	if (MIKROSDK_THERMO_read_data(&tmp))
		return THERMO_ST_FAIL;

	tmp = (tmp >> 16) & 0x01;

	if (tmp)
		return THERMO_ST_FAIL;

	return (THERMO_ST_OK);
}

/* get thermo couple temperature value - modified value aquisition function with unsigned integer return */
/* Note: T('C) = compl(uint16_t / 4) */
uint8_t MIKROSDK_THERMO_get_thermo_temperature(uint16_t *temperature)
{
	uint32_t all_data;
	uint16_t temp_data = 0;

	if (MIKROSDK_THERMO_read_data(&all_data))
		return THERMO_ST_FAIL;

	temp_data = (uint16_t)(0x3FFF & (all_data >> 18));
	/* sign extension from 14 to 16 bit */
	if (temp_data & 0x2000)
		temp_data |= 0xC000;

	*temperature = temp_data;

	return (THERMO_ST_OK);
}

/* get junction temperature value - modified value aquisition function with unsigned integer return */
/* Note: T('C) = compl(uint16_t / 16) */
uint8_t MIKROSDK_THERMO_get_junction_temperature(uint16_t *temperature)
{
	uint32_t all_data;
	uint16_t temp_data = 0;

	if (MIKROSDK_THERMO_read_data(&all_data))
		return THERMO_ST_FAIL;

	temp_data = (uint16_t)(0x0FFF & (all_data >> 4));
	/* sign extension from 12 to 16 bit */
	if (temp_data & 0x800)
		temp_data |= 0xF000;

	*temperature = temp_data;

	return (THERMO_ST_OK);
}

/* get both thermocouple and junction temperature values */
/* Note: thermo_temperature:   T('C) = compl(uint16_t /  4) */
/* Note: junction_temperature: T('C) = compl(uint16_t / 16) */
uint8_t MIKROSDK_THERMO_get_all_temperatures(uint16_t *thermo_temperature, uint16_t *junction_temperature)
{
	uint32_t all_data;
	uint16_t temp_data;

	if (MIKROSDK_THERMO_read_data(&all_data))
		return THERMO_ST_FAIL;

	temp_data = (uint16_t)(0x3FFF & (all_data >> 18));
	/* sign extension from 14 to 16 bit */
	if (temp_data & 0x2000)
		temp_data |= 0xC000;

	*thermo_temperature = temp_data;

	temp_data = (uint16_t)(0x0FFF & (all_data >> 4));
	/* sign extension from 12 to 16 bit */
	if (temp_data & 0x800)
		temp_data |= 0xF000;

	*junction_temperature = temp_data;

	return (THERMO_ST_OK);
}

/* device initialisation */
uint8_t MIKROSDK_THERMO_Initialise(void)
{
	/* chip select configuration */
	cfg.cs = SENSORIF_SPI_Chip_Select();

	/* driver initialisation */
	if (MIKROSDK_THERMO_init())
		return THERMO_ST_FAIL;

	if (MIKROSDK_THERMO_check_fault())
		return THERMO_ST_FAIL;

	SENSORIF_SPI_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return (THERMO_ST_OK);
}

/* device hardware reinitialisation for quick power-up */
uint8_t MIKROSDK_THERMO_Reinitialise(void)
{
	WAIT_ms(THERMO_T_POWERUP);

	SENSORIF_SPI_Init(false); /* enable interface */

	if (MIKROSDK_THERMO_check_fault())
		return THERMO_ST_FAIL;

	SENSORIF_SPI_Deinit(); /* only deinit, was initialised with i2c_master_open */

	return (THERMO_ST_OK);
}

/* data acquisition */
/* Note: thermo_temperature:   T('C) = compl(uint16_t /  4) */
/* Note: junction_temperature: T('C) = compl(uint16_t / 16) */
uint8_t MIKROSDK_THERMO_Acquire(uint16_t *thermo_temperature, uint16_t *junction_temperature)
{
	SENSORIF_SPI_Init(false); /* enable interface */
	if (MIKROSDK_THERMO_get_all_temperatures(thermo_temperature, junction_temperature))
		return THERMO_ST_FAIL;
	SENSORIF_SPI_Deinit(); /* disable interface */

	return THERMO_ST_OK;
}
