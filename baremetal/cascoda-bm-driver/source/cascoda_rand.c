/*
 * Copyright (c) 2020, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Baremetal random function implementation
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/entropy_poll.h"

#include "cascoda-util/cascoda_rand.h"
#include "ca821x_api.h"
#include "ca821x_toolchain.h"
#include "cascoda_bm_internal.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static struct ca821x_dev *entropyDev = NULL;

static int getEntropy(void *data, unsigned char *output, size_t len, size_t *olen)
{
	size_t             outLen     = 0;
	struct ca821x_dev *pDeviceRef = data;

	while (outLen < len)
	{
		uint8_t       readlen = 0;
		uint8_t       buf[2];
		size_t        remlen = len - outLen;
		size_t        cpylen = 0;
		ca_mac_status status = HWME_GET_request_sync(HWME_RANDOMNUM, &readlen, buf, pDeviceRef);

		if (status != MAC_SUCCESS || readlen != 2)
			break;

		cpylen = MIN(remlen, readlen);
		memcpy(output, buf, cpylen);
		output += cpylen;
		outLen += cpylen;
	}

	*olen = outLen;
	if (outLen != len)
		return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
	return 0;
}

// Provide a weak definition of otRandomCryptoMbedTlsContextGet for the case where we don't link openthread
CA_TOOL_WEAK mbedtls_ctr_drbg_context *otRandomCryptoMbedTlsContextGet(void)
{
	static bool                     isInitialised = false;
	static mbedtls_ctr_drbg_context sContext;
	static mbedtls_entropy_context  sEntropy;

	assert(entropyDev);

	if (!isInitialised)
	{
		mbedtls_ctr_drbg_init(&sContext);
		mbedtls_entropy_init(&sEntropy);
		mbedtls_entropy_add_source(
		    &sEntropy, &getEntropy, entropyDev, MBEDTLS_ENTROPY_MIN_HARDWARE, MBEDTLS_ENTROPY_SOURCE_STRONG);
		mbedtls_ctr_drbg_seed(&sContext, mbedtls_entropy_func, &sEntropy, NULL, 0);
		isInitialised = true;
	}

	return &sContext;
}

void RAND_SetCryptoEntropyDev(struct ca821x_dev *pDeviceRef)
{
	entropyDev = pDeviceRef;
	RAND_SeedFromDev(pDeviceRef);
}

ca_error RAND_GetCryptoBytes(uint16_t aNumBytes, void *aBytesOut)
{
	int error = mbedtls_ctr_drbg_random(otRandomCryptoMbedTlsContextGet(), (unsigned char *)aBytesOut, aNumBytes);

	if (!error)
		return CA_ERROR_SUCCESS;
	if (error == MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG)
		return CA_ERROR_INVALID_ARGS;

	return CA_ERROR_FAIL;
}

void RAND_AddRadioEntropySource(void)
{
	static bool                    isInitialised = false;
	static mbedtls_entropy_context sEntropy;

	assert(entropyDev);

	if (!isInitialised)
	{
		mbedtls_entropy_init(&sEntropy);
		mbedtls_entropy_add_source(
		    &sEntropy, &getEntropy, entropyDev, MBEDTLS_ENTROPY_MIN_HARDWARE, MBEDTLS_ENTROPY_SOURCE_STRONG);
		isInitialised = true;
	}
}
