/*
 * Copyright (c) 2019, Cascoda
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

#include "cascoda-util/cascoda_hash.h"
#include "ca821x_log.h"

//Constants and alogrithm from http://www.isthe.com/chongo/tech/comp/fnv
static const uint32_t prime32 = 16777619;
static const uint32_t basis32 = 2166136261;

/*
 * CRC32 algorithm taken from the zlib source, which is
 * Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
 */
static const unsigned int tinf_crc32tab[16] = {0x00000000,
                                               0x1db71064,
                                               0x3b6e20c8,
                                               0x26d930ac,
                                               0x76dc4190,
                                               0x6b6b51f4,
                                               0x4db26158,
                                               0x5005713c,
                                               0xedb88320,
                                               0xf00f9344,
                                               0xd6d6a3e8,
                                               0xcb61b38c,
                                               0x9b64c2b0,
                                               0x86d3d2d4,
                                               0xa00ae278,
                                               0xbdbdf21c};

uint32_t HASH_fnv1a_32(const void *data_in, size_t num_bytes)
{
	uint32_t       hash = basis32;
	const uint8_t *data = data_in;

	for (size_t i = 0; i < num_bytes; i++)
	{
		hash = (data[i] ^ hash) * prime32;
	}
	return hash;
}

uint64_t HASH_fnv1a_64(const void *data_in, size_t num_bytes)
{
	uint64_t hash = basis64;

	HASH_fnv1a_64_stream(data_in, num_bytes, &hash);

	return hash;
}

void HASH_fnv1a_64_stream(const void *data_in, size_t num_bytes, uint64_t *hash)
{
	const uint8_t *data = data_in;

	for (size_t i = 0; i < num_bytes; i++)
	{
		*hash = (data[i] ^ *hash) * prime64;
	}
}

uint32_t HASH_CRC32(uint8_t *data, uint32_t dataLen)
{
	uint32_t crc = 0xFFFFFFFF;

	HASH_CRC32_stream(data, dataLen, &crc);

	crc = ~crc;
	return crc;
}

void HASH_CRC32_stream(uint8_t *data, uint32_t dataLen, uint32_t *crc)
{
	// CRC algorithm adapted from zlib source, which is
	// Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
	for (size_t i = 0; i < dataLen; ++i)
	{
		uint8_t newByte = data[i];
		*crc ^= newByte;
		*crc = tinf_crc32tab[*crc & 0x0f] ^ (*crc >> 4);
		*crc = tinf_crc32tab[*crc & 0x0f] ^ (*crc >> 4);
	}
}
