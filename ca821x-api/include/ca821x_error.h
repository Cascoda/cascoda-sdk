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

#ifndef CA821X_API_INCLUDE_CA821X_ERROR_H_
#define CA821X_API_INCLUDE_CA821X_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Cascoda error type */
typedef enum ca_error
{
	/*General Errors*/
	CA_ERROR_SUCCESS       = 0x00, ///< Success
	CA_ERROR_FAIL          = 0x01, ///< Failed for miscellaneous reason
	CA_ERROR_UNKNOWN       = 0x02, ///< Request for unknown data
	CA_ERROR_INVALID       = 0x03, ///< Invalid request
	CA_ERROR_NO_ACCESS     = 0x04, ///< Cannot access requested resource
	CA_ERROR_INVALID_STATE = 0x05, ///< Request cannot be processed in current state
	CA_ERROR_BUSY          = 0x06, ///< Busy, try again
	CA_ERROR_INVALID_ARGS  = 0x07, ///< Invalid arguments
	CA_ERROR_NOT_HANDLED   = 0x08, ///< Request has not been handled
	CA_ERROR_NOT_FOUND     = 0x09, ///< Requested resource not found
	CA_ERROR_NO_BUFFER     = 0x0A, ///< No buffers currently available
	CA_ERROR_TIMEOUT       = 0x0B, ///< Operation timed out
	CA_ERROR_ALREADY       = 0x0C, ///< Operation already executed
	/* SPI Errors*/
	CA_ERROR_SPI_WAIT_TIMEOUT       = 0xA0, ///< SPI Wait timed out
	CA_ERROR_SPI_NACK_TIMEOUT       = 0xA1, ///< SPI NACKing for too long
	CA_ERROR_SPI_SCAN_IN_PROGRESS   = 0xA2, ///< CA-821x is scanning and cannot be used
	CA_ERROR_SPI_SEND_EXCHANGE_FAIL = 0xA3, ///< SPI Message failed to send
} ca_error;

#ifdef __cplusplus
}
#endif

#endif /* CA821X_API_INCLUDE_CA821X_ERROR_H_ */
