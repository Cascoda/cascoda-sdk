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
 * Sensor interface for Maxim MAX30205 human body temperature sensor
*/

#ifndef SIF_MAX30205_H
#define SIF_MAX30205_H

/* slave addresses */
/* Note that the slave address is hardware progammable by 3 bits (A2,A1,A0)
 * according to table 1 in the datasheet
 * SIF_SAD_MAX30205 has to match table 1 (bits 7:1 in address+r/w transfer byte,
 * not address(6:0)).
 */
#define SIF_SAD_MAX30205 0x90 /* A2/A1/A0 = 0/0/0 */

/* master measurement (read access) modes */
enum sif_max30205_mode
{
	SIF_MAX30205_MODE_POLL_ONE_SHOT, /* poll D7 in configuration register */
	SIF_MAX30205_MODE_TCONV_WAIT,    /* wait for maximum conversion time */
};

/* measurement mode */
#define SIF_MAX30205_MODE SIF_MAX30205_MODE_TCONV_WAIT

/* max. conversion times for measurement [ms] */
#define SIF_MAX30205_TCONV_MAX_TEMP 60 /* temperature */

/* configuration register bit mapping */
#define SIF_MAX30205_CONFIG_ONESHOT 0x80
#define SIF_MAX30205_CONFIG_SHUTDOWN 0x01

/* functions */
u16_t SIF_MAX30205_ReadTemperature(void); /* measure temperature */
u8_t  SIF_MAX30205_Initialise(void);      /* initialise sensor, shutdown mode */

#endif // SIF_MAX30205_H
