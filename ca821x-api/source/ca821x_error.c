/*
 *  Copyright (c) 2020, Cascoda Ltd.
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

#include <ca821x_error.h>

const char *ca_error_str(ca_error aError)
{
	const char *rval = "???";

	switch (aError)
	{
	case CA_ERROR_SUCCESS:
		rval = "SUCCESS";
		break;
	case CA_ERROR_FAIL:
		rval = "FAIL";
		break;
	case CA_ERROR_UNKNOWN:
		rval = "UNKNOWN";
		break;
	case CA_ERROR_INVALID:
		rval = "INVALID";
		break;
	case CA_ERROR_NO_ACCESS:
		rval = "NO_ACCESS";
		break;
	case CA_ERROR_INVALID_STATE:
		rval = "INV_STATE";
		break;
	case CA_ERROR_BUSY:
		rval = "BUSY";
		break;
	case CA_ERROR_INVALID_ARGS:
		rval = "INV_ARGS";
		break;
	case CA_ERROR_NOT_HANDLED:
		rval = "NOT_HANDLED";
		break;
	case CA_ERROR_NOT_FOUND:
		rval = "NOT_FOUND";
		break;
	case CA_ERROR_NO_BUFFER:
		rval = "NO_BUF";
		break;
	case CA_ERROR_TIMEOUT:
		rval = "TIMEOUT";
		break;
	case CA_ERROR_ALREADY:
		rval = "ALREADY";
		break;

	case CA_ERROR_SPI_WAIT_TIMEOUT:
		rval = "SPI_WAIT";
		break;
	case CA_ERROR_SPI_NACK_TIMEOUT:
		rval = "SPI_NACK";
		break;
	case CA_ERROR_SPI_SCAN_IN_PROGRESS:
		rval = "SPI_SCAN";
		break;
	case CA_ERROR_SPI_SEND_EXCHANGE_FAIL:
		rval = "SPI_SEND";
		break;
	} // Intentionally no default so that compiler warns us if we miss one

	return rval;
}
