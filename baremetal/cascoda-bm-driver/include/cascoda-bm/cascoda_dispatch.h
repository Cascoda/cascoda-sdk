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
#include "cascoda-bm/cascoda_bm.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#ifndef CASCODA_DISPATCH_H
#define CASCODA_DISPATCH_H

/****** Function Declarations for cascoda_dispatch.c                     ******/

/**
 * \brief Send Request to CA-821x over SPI
 *
 * This function is the system-specific definition of ca821x_api_downstream. It
 * transmits a SAP command and if applicable, waits for a synchronous confirm and
 * stores it in the 'response' buffer.
 *
 * \param buf - Message to transmit
 * \param len - Length of message in buf
 * \param response - Buffer to fill with response
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return Status
 *
 */
ca_error DISPATCH_ToCA821x(const uint8_t *buf, size_t len, u8_t *response, struct ca821x_dev *pDeviceRef);

/**
 * \brief process and dispatch on any received SPI messages from the ca821x
 *
 * This function is not re-entrant - i.e. you cannot call DISPATCH_FromCA821x while
 * already in a callback that DISPATCH_FromCA821x has called. If you try to do this,
 * the function will fail the second call and return CA_ERROR_INVALID_STATE
 *
 * \retval CMB_SUCCESS, CA_ERROR_INVALID_STATE
 */
ca_error DISPATCH_FromCA821x(struct ca821x_dev *pDeviceRef);

/**
 * \brief Read SPI messages from the ca821x
 *
 * This function should be called by the BSP on the falling edge of the RFIRQ signal from the CA821x.
 *
 */
void DISPATCH_ReadCA821x(struct ca821x_dev *pDeviceRef);

#endif // CASCODA_SPI_H
