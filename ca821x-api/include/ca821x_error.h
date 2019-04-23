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

#ifndef CA821X_API_INCLUDE_CA821X_ERROR_H_
#define CA821X_API_INCLUDE_CA821X_ERROR_H_

/** Cascoda error type */
typedef enum ca_error
{
	/*General Errors*/
	CA_ERROR_SUCCESS       = 0x00,
	CA_ERROR_FAIL          = 0x01,
	CA_ERROR_UNKNOWN       = 0x02,
	CA_ERROR_INVALID       = 0x03,
	CA_ERROR_NO_ACCESS     = 0x04,
	CA_ERROR_INVALID_STATE = 0x05,
	CA_ERROR_BUSY          = 0x06,
	CA_ERROR_INVALID_ARGS  = 0x07,
	CA_ERROR_NOT_HANDLED   = 0x08,
	CA_ERROR_NOT_FOUND     = 0x09,
	CA_ERROR_NO_BUFFER     = 0x0A,
	CA_ERROR_TIMEOUT       = 0x0B,
	/* SPI Errors*/
	CA_ERROR_SPI_WAIT_TIMEOUT       = 0xA0,
	CA_ERROR_SPI_NACK_TIMEOUT       = 0xA1,
	CA_ERROR_SPI_SCAN_IN_PROGRESS   = 0xA2,
	CA_ERROR_SPI_SEND_EXCHANGE_FAIL = 0xA3
} ca_error;

#endif /* CA821X_API_INCLUDE_CA821X_ERROR_H_ */
