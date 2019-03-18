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
 * sensor/actuator I2C interface functions
*/

#ifndef CASCODA_SENSORIF_H
#define CASCODA_SENSORIF_H

#include "cascoda-bm/cascoda_types.h"

/* declarations */
#define SENSORIF_I2C_TIMEOUT 100      /* I2C bus access time-out time in [ms] */
#define SENSORIF_CLK_FREQUENCY 100000 /* I2C SCL frequency [Hz], 100kHz (full-speed) */
#define SENSORIF_INT_PULLUPS 0        /* I2C use internal pull-ups flag */

/* Note: most internal GPIO pull-ups have a value of around 50kOhms which is usually
 * too high for I2C pull-up values, depending  on number of peripherals connected and
 * overall load capacitance. The SCL clock frequency might have to be lowered to below
 * full-speed if no external pull-ups on SDA and SCL are available.
 */

/** I2C master status enumerations */
enum sensorif_i2c_status
{
	SENSORIF_I2C_ST_START         = 0x08, /**< start */
	SENSORIF_I2C_ST_RSTART        = 0x10, /**< repeat start */
	SENSORIF_I2C_ST_TX_AD_ACK     = 0x18, /**< transmit address ACKed */
	SENSORIF_I2C_ST_TX_AD_NACK    = 0x20, /**< transmit address NACKed */
	SENSORIF_I2C_ST_TX_DT_ACK     = 0x28, /**< transmit data    ACKed */
	SENSORIF_I2C_ST_TX_DT_NACK    = 0x30, /**< transmit data    NACKed */
	SENSORIF_I2C_ST_ARB_LOST      = 0x38, /**< arbitration lost */
	SENSORIF_I2C_ST_RX_AD_ACK     = 0x40, /**< receive  address ACKed */
	SENSORIF_I2C_ST_RX_AD_NACK    = 0x48, /**< receive  address NACKed */
	SENSORIF_I2C_ST_RX_DT_ACK     = 0x50, /**< receive  data    ACKed */
	SENSORIF_I2C_ST_RX_DT_NACK    = 0x58, /**< receive  data    NACKed */
	SENSORIF_I2C_ST_BUS_ERROR_RAW = 0x00, /**< bus error code transmitted over wire*/
	SENSORIF_I2C_ST_STOP          = 0xF0, /**< on reset or stop */
	SENSORIF_I2C_ST_RELEASED      = 0xF8, /**< bus released */

	SENSORIF_I2C_ST_NOT_IMPLEMENTED = 0xFE, /**< not implemented */
	SENSORIF_I2C_ST_SUCCESS         = 0x00, /**< successful transfer */
	SENSORIF_I2C_ST_TIMEOUT         = 0xFF, /**< bus access time-out */
	SENSORIF_I2C_ST_BUS_ERROR       = 0xF1, /**< bus error re-mapped from 0x00 for status return*/
};

/* functions for platform implementation */
void                     SENSORIF_I2C_Init(void);
void                     SENSORIF_I2C_Deinit(void);
enum sensorif_i2c_status SENSORIF_I2C_Write(u8_t slaveaddr, u8_t *data, u32_t *len);
enum sensorif_i2c_status SENSORIF_I2C_Read(u8_t slaveaddr, u8_t *pdata, u32_t *plen);

#endif // CASCODA_SENSORIF_H
