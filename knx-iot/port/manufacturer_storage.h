/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright (c) 2023 Cascoda Limited
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 *    of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Cascoda Limited.
 *    integrated circuit in a product or a software update for such product, must
 *    reproduce the above copyright notice, this list of  conditions and the following
 *    disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Cascoda Limited nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * 4. This software, whether provided in binary or any other form must not be decompiled,
 *    disassembled, reverse engineered or otherwise modified.
 *
 *  5. This software, in whole or in part, must only be used with a Cascoda Limited circuit.
 *
 * THIS SOFTWARE IS PROVIDED BY CASCODA LIMITED "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CASCODA LIMITED OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/

#ifndef MANUFACTURER_STORAGE_H
#define MANUFACTURER_STORAGE_H

#include <stdint.h>
#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"

typedef struct dummy_mbedtls_mpi
{
	uint8_t data_offset[2]; //uint16_t
} dummy_mbedtls_mpi;

typedef struct dummy_mbedtls_ecp_point
{
	dummy_mbedtls_mpi X;
	dummy_mbedtls_mpi Y;
	dummy_mbedtls_mpi Z;
} dummy_mbedtls_ecp_point;

struct knx_manufacturer_storage
{
	/// @brief used for checking whether storage has been written to - should be CA 5C OD A0
	uint8_t magic_number[4];
	/// @brief KNX Serial Number, as defined by the Point API specification
	uint8_t knx_serial_number[6];
	/// @brief used for checking whether pre-programmed SPAKE2+ record exists
	/// should be "PASE"
	uint8_t pase_magic[4];
	/// @brief SPAKE2+ offline record
	struct spake_record_storage
	{
		uint8_t                 password[32];
		uint8_t                 rand[32];
		uint8_t                 salt[32];
		uint8_t                 iter[4]; //uint32_t
		dummy_mbedtls_mpi       w0;
		dummy_mbedtls_ecp_point L;
	} spake_record;
};

/**
 * @brief Get the KNX serial number stored within the manufacturer storage flash page
 * 
 * @param output_buffer 6-byte array to copy binary serial number into
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
 */
int knx_get_stored_serial_number(uint8_t output_buffer[6]);

/**
 * @brief Get the stored password used for authentication
 * 
 * @param output_buffer 16-byte array to copy password into
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
*/
int knx_get_stored_password(uint8_t output_buffer[16]);

/**
 * @brief Get the stored w0 value for SPAKE2+
 * 
 * @param out Destination multiple-precision-integer
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
*/
int knx_get_stored_w0(mbedtls_mpi *out);

/**
 * @brief Get the stored L value for SPAKE2+
 * 
 * @param out Destination ECP point
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
*/
int knx_get_stored_L(mbedtls_ecp_point *out);

/**
 * @brief Get the stored PBKDF salt value for SPAKE2+
 * 
 * @param salt Destination array to store 32 byte salt
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
*/
int knx_get_stored_salt(uint8_t salt[32]);

/**
 * @brief Get the stored PBKDF rand value for SPAKE2+
 * 
 * @param rand Destination array to store 32 byte salt
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
*/
int knx_get_stored_rand(uint8_t rand[32]);

/**
 * @brief Get the stored PBKDF no iterations value for SPAKE2+
 * 
 * @param it num iterations variable to store to
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
*/
int knx_get_stored_iter(uint32_t *it);

/**
 * @brief Get the all stored params for SPAKE2+
 * 
 * @param salt Destination array to store 32 byte salt
 * @param rand Destination array to store 32 byte salt
 * @param it num iterations variable to store to
 * @param w0 Destination multiple-precision-integer for w0
 * @param L Destination ECP point for L
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
*/
int knx_get_stored_spake(uint8_t salt[32], uint8_t rand[32], uint32_t *it, mbedtls_mpi *w0, mbedtls_ecp_point *L);

#endif // MANUFACTURER_STORAGE_H