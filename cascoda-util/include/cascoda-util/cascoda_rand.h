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

/**
 * @file
 * @brief  Random Number Generation functions
 */
/**
 * @ingroup cascoda-util
 * @defgroup ca-rand Random Number Generation functions
 * @brief  Utility functions for generating random data
 *
 * @{
 */

#ifndef CASCODA_RAND_H
#define CASCODA_RAND_H

#include <stddef.h>
#include <stdint.h>

#include "ca821x_api.h"
#include "ca821x_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get random data for non-cryptographic purposes.
 *
 * This function will never fail.
 *
 * Depending on the platform this may be from a PRNG. Will fill memory starting from
 * address aBytesOut with aNumBytes of pseudorandom random data. This data will be
 * uniformly distributed, but not suitable for cryptographic purposes as the sequence
 * can be predicted.
 *
 * @param aNumBytes The number of bytes to fill with random data
 * @param[out] aBytesOut The start address to fill with random data
 */
void RAND_GetBytes(uint16_t aNumBytes, void *aBytesOut);

/**
 * Seed the non-cryptographic Pseudo Random Number Generator
 *
 * It is required to call either this or @ref RAND_SeedFromDev before using @ref RAND_GetBytes
 * or the sequence will be the same for different devices. This is handled within the
 * initialisation functions for each platform.
 *
 * @param aSeed A seed for the PRNG. Should be unique, such as a random number or Unix time.
 */
void RAND_Seed(uint64_t aSeed);

/**
 * Seed the non-cryptographic Pseudo Random Number Generator using entropy from the ca821x
 *
 * It is required to call either this or @ref RAND_Seed before using @ref RAND_GetBytes
 * or the sequence will be the same for different devices. This is handled within the
 * initialisation functions for each platform.
 *
 * @param pDeviceRef The CA821x device to use to source the entropy
 */
void RAND_SeedFromDev(struct ca821x_dev *pDeviceRef);

/**
 * Get random data for cryptographic purposes.
 *
 * This function will use a CPRNG, and be suitable for cryptographic purposes. Will
 * fill memory starting from address aBytesOut with aNumBytes of random data. Upon
 * an error, the output memory will be left in an indeterminate state and should
 * not be relied upon to be random data, or be in its previous state.
 *
 * @param aNumBytes The number of bytes to fill with random data
 * @param[out] aBytesOut The start address to fill with random data
 * @return Cascoda Error code
 * @retval CA_ERROR_SUCCESS Success, aBytesOut is filled with aNumBytes of random data.
 * @retval CA_ERROR_FAIL Request failed, aBytesOut is indeterminate
 */
ca_error RAND_GetCryptoBytes(uint16_t aNumBytes, void *aBytesOut);

/**
 * @brief
 * Add the radio-based RNG as an entropy source used by mbedTLS
 *
 */
void RAND_AddRadioEntropySource(void);

#ifdef __cplusplus
}
#endif

#endif // CASCODA_RAND_H

/**
 * @}
 */
