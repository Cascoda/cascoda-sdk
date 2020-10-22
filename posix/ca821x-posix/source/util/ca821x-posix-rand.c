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

#if _WIN32
#define _CRT_RAND_S
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "ca821x-posix/ca821x-posix.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

ca_error RAND_GetCryptoBytes(uint16_t aNumBytes, void *aBytesOut)
{
	ca_error error  = CA_ERROR_SUCCESS;
	uint8_t *outptr = aBytesOut;

#if _WIN32
	while (!error && aNumBytes)
	{
		unsigned int newRand = 0;
		if (rand_s(&newRand) == 0)
		{
			uint16_t cpyBytes = MIN(aNumBytes, sizeof(newRand));
			memcpy(outptr, &newRand, cpyBytes);
			aNumBytes -= cpyBytes;
			outptr += cpyBytes;
		}
		else
		{
			error = CA_ERROR_FAIL;
		}
	}
#else
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd != -1)
	{
		while (!error && aNumBytes)
		{
			ssize_t cnt = read(fd, outptr, aNumBytes);

			if (cnt == -1)
			{
				error = CA_ERROR_FAIL;
			}
			else
			{
				aNumBytes -= cnt;
				outptr += cnt;
			}
		}

		close(fd);
	}
	else
	{
		error = CA_ERROR_FAIL;
	}
#endif

	return error;
}
