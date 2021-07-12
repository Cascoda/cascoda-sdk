/*
 *  Copyright (c) 2019, Cascoda Ltd.
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
/**
 * @file
 * Declarations of internal functions for communication with CA-821x
 */
/**
 * @ingroup bm-core
 * @defgroup bm-dispatch Baremetal CA-821x Dispatch
 * @brief  Internal functions used by ca821x-api to handle messages going to and from the CA-821x
 *
 * @{
 */

#ifndef CASCODA_DISPATCH_H
#define CASCODA_DISPATCH_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/****** Function Declarations for cascoda_dispatch.c                     ******/

/**
 * \brief Send Request to CA-821x over SPI
 *
 * This function is the system-specific definition of ca821x_api_downstream. It
 * transmits a SAP command and if applicable, waits for a synchronous confirm and
 * stores it in the 'response' buffer.
 *
 * \param buf - Message to transmit, length is encoded in Cascoda TLV format
 * \param response - Buffer to fill with response
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return Status
 *
 */
ca_error DISPATCH_ToCA821x(const uint8_t *buf, u8_t *response, struct ca821x_dev *pDeviceRef);

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

#ifdef __cplusplus
}
#endif

#endif // CASCODA_SPI_H

/**
 * @}
 */
