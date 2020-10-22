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
/*
 * Example application to test own subsystems
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_rand.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "selftest_app.h"

static ca_tasklet selftest_handler;

static bool is_all_zero(size_t aSize, uint8_t *buf)
{
	for (size_t i = 0; i < aSize; i++)
	{
		if (buf[i])
		{
			return false;
		}
	}
	return true;
}

static void print_buf(char *prefix, uint8_t *buf, size_t len)
{
	printf("%s:", prefix);
	for (size_t i = 0; i < len; i++)
	{
		printf(" %02x", buf[i]);
	}
	printf("\r\n");
}

static void validate(bool aMustBeTrue, char *aFailString)
{
	if (aMustBeTrue)
	{
		return;
	}
	printf("Test Failed: %s\r\n", aFailString);
}

static ca_error SELFTEST_Handler(void *context)
{
	ca_error error      = CA_ERROR_SUCCESS;
	uint8_t  buffer[20] = {};
	(void)context;

	validate(is_all_zero(sizeof(buffer), buffer), "Initializing buffer");
	RAND_GetBytes(sizeof(buffer), buffer);
	print_buf("RAND_GetBytes", buffer, sizeof(buffer));
	validate(!is_all_zero(sizeof(buffer), buffer), "RAND_GetBytes data");
	memset(buffer, 0, sizeof(buffer));
	validate(is_all_zero(sizeof(buffer), buffer), "Re-initializing buffer");
	RAND_GetBytes(sizeof(buffer), buffer);
	print_buf("RAND_GetBytes", buffer, sizeof(buffer));
	validate(!is_all_zero(sizeof(buffer), buffer), "RAND_GetBytes data 2");
	memset(buffer, 0, sizeof(buffer));
	validate(is_all_zero(sizeof(buffer), buffer), "Re-initializing buffer");

	error = RAND_GetCryptoBytes(sizeof(buffer), buffer);
	print_buf("RAND_GetCryptoBytes", buffer, sizeof(buffer));
	validate(!error, "Rand Crypto error");
	validate(!is_all_zero(sizeof(buffer), buffer), "RAND_GetCryptoBytes data");
	memset(buffer, 0, sizeof(buffer));
	validate(is_all_zero(sizeof(buffer), buffer), "Re-initializing buffer");
	error = RAND_GetCryptoBytes(sizeof(buffer), buffer);
	print_buf("RAND_GetCryptoBytes", buffer, sizeof(buffer));
	validate(!error, "Rand Crypto error 2");
	validate(!is_all_zero(sizeof(buffer), buffer), "RAND_GetCryptoBytes data 2");

	TASKLET_ScheduleDelta(&selftest_handler, 1000, NULL);

	return CA_ERROR_SUCCESS;
}

u8_t SELFTEST_Initialise(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	TASKLET_Init(&selftest_handler, &SELFTEST_Handler);
	TASKLET_ScheduleDelta(&selftest_handler, 1000, NULL);

	return 0;
}
