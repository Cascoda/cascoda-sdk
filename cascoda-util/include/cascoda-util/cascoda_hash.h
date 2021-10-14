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
 * @brief  Hashing helper functions
 */
/**
 * @ingroup cascoda-util
 * @defgroup ca-hash Hashing functions
 * @brief  General functions for hashing data
 *
 * @{
 */

#ifndef CASCODA_HASH_H
#define CASCODA_HASH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate the 32-bit fnv1a non-crypto hash of a block of data
 *
 * @param data_in  The data to hash
 * @param num_bytes The sizeof the data to be hashed (in bytes)
 *
 * @return 32-bit fnv1a non-crypto hash
 */
uint32_t HASH_fnv1a_32(const void *data_in, size_t num_bytes);

/**
 * Calculate the 64-bit fnv1a non-crypto hash of a block of data
 *
 * @param data_in  The data to hash
 * @param num_bytes The sizeof the data to be hashed (in bytes)
 *
 * @return 64-bit fnv1a non-crypto hash
 */
uint64_t HASH_fnv1a_64(const void *data_in, size_t num_bytes);

/**
 * Calculate the CRC32 hash of a block of data.
 *
 * @param data    The data to hash
 * @param dataLen The sizeof the data to be hashed (in bytes)
 *
 * @return 32-bit CRC hash
 */
uint32_t HASH_CRC32(uint8_t *data, uint32_t dataLen);

/**
 * Streaming version of HASH_CRC32.
 *
 * Usage notes: This function leaves out
 *              1. The initialisation of the CRC to 0xFFFFFFFF at the beginning, and
 *              2. the one's complement operation at the end.
 *              So these two steps have to be handled outside of this function, by the code
 *              that uses it. (For example, see HASH_CRC32, which uses HASH_CRC32_stream).
 *
 * @param data    The data to hash
 * @param dataLen The sizeof the data to be hashed (in bytes)
 * @param crc     Output variable into which the (partial) crc computation is stored
 */
void HASH_CRC32_stream(uint8_t *data, uint32_t dataLen, uint32_t *crc);

#ifdef __cplusplus
}
#endif

#endif // CASCODA_HASH_H

/**
 * @}
 */
