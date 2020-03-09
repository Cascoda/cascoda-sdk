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

#include "cascoda-bm/cascoda_hash.h"

//Constants and alogrithm from http://www.isthe.com/chongo/tech/comp/fnv
static const uint32_t prime32 = 16777619;
static const uint32_t basis32 = 2166136261;
static const uint64_t prime64 = 1099511628211ULL;
static const uint64_t basis64 = 14695981039346656037ULL;

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
	uint64_t       hash = basis64;
	const uint8_t *data = data_in;

	for (size_t i = 0; i < num_bytes; i++)
	{
		hash = (data[i] ^ hash) * prime64;
	}
	return hash;
}
