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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "cascoda-util/cascoda_rand.h"
#include "ca821x_api.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static uint64_t mcg_state = 130973517;

static uint32_t pcg32_fast(void)
{
	uint64_t x     = mcg_state;
	unsigned count = (unsigned)(x >> 61);

	mcg_state = x * 0xf13283ad;
	x ^= x >> 22;
	return (uint32_t)(x >> (22 + count));
}

static void pcg32_init(struct ca821x_dev *pDeviceRef)
{
	uint64_t seed = 0;
	uint8_t  data[sizeof(seed)];
	size_t   filledLen = 0;

	while (filledLen < sizeof(seed))
	{
		uint8_t readlen = 0;
		HWME_GET_request_sync(HWME_RANDOMNUM, &readlen, data + filledLen, pDeviceRef);
		filledLen += readlen;
	}
	memcpy(&seed, data, sizeof(seed));
	mcg_state = 2 * seed + 1;
	pcg32_fast();
}

void RAND_Seed(uint64_t aSeed)
{
	mcg_state = 2 * aSeed + 1;
	pcg32_fast();
}

void RAND_SeedFromDev(struct ca821x_dev *pDeviceRef)
{
	pcg32_init(pDeviceRef);
}

void RAND_GetBytes(uint16_t aNumBytes, void *aBytesOut)
{
	uint8_t *outptr = aBytesOut;
	while (aNumBytes)
	{
		uint32_t newRand  = pcg32_fast();
		uint16_t cpyBytes = MIN(aNumBytes, sizeof(newRand));
		memcpy(outptr, &newRand, cpyBytes);
		aNumBytes -= cpyBytes;
		outptr += cpyBytes;
	}
}
