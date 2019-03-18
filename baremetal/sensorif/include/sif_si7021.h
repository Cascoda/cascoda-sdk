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

#ifndef SIF_SI7021_H
#define SIF_SI7021_H

/* slave addresses */
#define SIF_SAD_SI7021 0x40 /* Si7021 temperature / humidity sensor */

/* master measurement (read access) modes */
enum sif_si7021_mode
{
	SIF_SI7021_MODE_HOLD_MASTER, /* master hold mode (clock streching, clock suspend) */
	SIF_SI7021_MODE_NACK_WAIT,   /* no-hold master hold mode (wait while NACKs) */
	SIF_SI7021_MODE_TCONV_WAIT,  /* wait for maximum conversion time */
};

/* measurement mode */
#define SIF_SI7021_MODE SIF_SI7021_MODE_HOLD_MASTER

/* max. conversion times for measurement [ms] */
#define SIF_SI7021_TCONV_MAX_TEMP 10 /* temperature */
#define SIF_SI7021_TCONV_MAX_HUM 20  /* humidity */

/* functions */
u8_t SIF_SI7021_ReadTemperature(void); /* measure temperature, -128 to +127 'C */
u8_t SIF_SI7021_ReadHumidity(void);    /* measure humidity, 0 to 100 % */
void SIF_SI7021_Reset(void);           /* device soft-reset */
u8_t SIF_SI7021_ReadID(void);          /* device read device identification */

#endif // SIF_SI7021_H
