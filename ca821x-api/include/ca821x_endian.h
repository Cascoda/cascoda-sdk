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
 *   Functions to help converting between system types and big/little endian octet representations.
 */

#ifndef CA821X_API_INCLUDE_CA821X_ENDIAN_H_
#define CA821X_API_INCLUDE_CA821X_ENDIAN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Extract the least significant octet of a 16-bit value
 */
static inline uint8_t LS_BYTE(uint16_t x)
{
	return ((uint8_t)((x)&0xFF));
}

/**
 * Extract the most significant octet of a 16-bit value
 */
static inline uint8_t MS_BYTE(uint16_t x)
{
	return ((uint8_t)(((x) >> 8) & 0xFF));
}

/**
 * Extract the first (little-endian) octet of a 32-bit value
 */
static inline uint8_t LS0_BYTE(uint16_t x)
{
	return ((uint8_t)((x)&0xFF));
}

/**
 * Extract the second (little-endian) octet of a 32-bit value
 */
static inline uint8_t LS1_BYTE(uint32_t x)
{
	return ((uint8_t)(((x) >> 8) & 0xFF));
}

/**
 * Extract the third (little-endian) octet of a 32-bit value
 */
static inline uint8_t LS2_BYTE(uint32_t x)
{
	return ((uint8_t)(((x) >> 16) & 0xFF));
}

/**
 * Extract the fourth (little-endian) octet of a 32-bit value
 */
static inline uint8_t LS3_BYTE(uint32_t x)
{
	return ((uint8_t)(((x) >> 24) & 0xFF));
}

/**
 * Extract a 16-bit value from a little-endian octet array
 */
static inline uint16_t GETLE16(const uint8_t *in)
{
	return ((in[1] << 8) & 0xff00) | (in[0] & 0x00ff);
}

/**
 * Extract a 32-bit value from a little-endian octet array
 */
static inline uint32_t GETLE32(const uint8_t *in)
{
	return (((uint32_t)in[3] << 24) + ((uint32_t)in[2] << 16) + ((uint32_t)in[1] << 8) + (uint32_t)in[0]);
}

/**
 * Put a 16-bit value into a little-endian octet array
 */
static inline void PUTLE16(uint16_t in, uint8_t *out)
{
	out[0] = in & 0xff;
	out[1] = (in >> 8) & 0xff;
}

/**
 * Put a 32-bit value into a little-endian octet array
 */
static inline void PUTLE32(uint32_t in, uint8_t *out)
{
	out[0] = in & 0xff;
	out[1] = (in >> 8) & 0xff;
	out[2] = (in >> 16) & 0xff;
	out[3] = (in >> 24) & 0xff;
}

/**
 * Extract a 16-bit value from a big-endian octet array
 */
static inline uint16_t GETBE16(const uint8_t *in)
{
	return ((in[0] << 8) & 0xff00) | (in[1] & 0x00ff);
}

/**
 * Put a 16-bit value into a big-endian octet array
 */
static inline void PUTBE16(uint16_t in, uint8_t *out)
{
	out[1] = in & 0xff;
	out[0] = (in >> 8) & 0xff;
}

/**
 * Extract a 32-bit value from a big-endian octet array
 */
static inline uint32_t GETBE32(const uint8_t *in)
{
	return (((uint32_t)in[0] << 24) + ((uint32_t)in[1] << 16) + ((uint32_t)in[2] << 8) + (uint32_t)in[3]);
}

/**
 * Put a 32-bit value into a big-endian octet array
 */
static inline void PUTBE32(uint32_t in, uint8_t *out)
{
	out[3] = in & 0xff;
	out[2] = (in >> 8) & 0xff;
	out[1] = (in >> 16) & 0xff;
	out[0] = (in >> 24) & 0xff;
}

#ifdef __cplusplus
}
#endif

#endif /* CA821X_API_INCLUDE_CA821X_ENDIAN_H_ */
