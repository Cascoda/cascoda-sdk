/*
 * Copyright (c) 2021, Cascoda
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
/**
 * @file
 * Declarations of helper functions for the EVBME DFU commands that operate on the external flash.
 */

#ifndef CASCODA_EXTERNAL_FLASH_H
#define CASCODA_EXTERNAL_FLASH_H

#include "cascoda-bm/cascoda_ota_upgrade.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
/****** External Erase Type ******/
typedef enum
{
	ERASE_CHIP = 0, //!< Whole chip erase
	ERASE_64KB,     //!< 64kb block erase
	ERASE_32KB,     //!< 32kb block erase
	ERASE_4KB,      //!< 4kb sector erase
} ExternalEraseType;

/****** Information used for erase procedure ******/
typedef struct
{
	u32_t               startAddress;     //!< Start address of erase
	u32_t               eraseLength;      //!< How many bytes to erase
	u32_t               endAddress;       //!< End address of erase
	ExtFlashAPICallback otaCallback;      //!< Callback set internally for the OTA upgrade procedure
	ExtFlashAPICallback upstreamCallback; //!< User-defined callback used to specify an upstream response
} ExtFlashEraseInfo;

/****** Information used for writing procedure ******/
typedef struct
{
	u32_t               startAddress;     //!< Start address of write
	u32_t               writeLength;      //!< Number of bytes to write
	u32_t               writeLimit;       //!< Maximum number of bytes that can be written in a single instruction
	u16_t               pageSize;         //!< Size (in bytes) of a page in the external flash
	ExtFlashAPICallback otaCallback;      //!< Callback set internally for the OTA upgrade procedure
	ExtFlashAPICallback upstreamCallback; //!< User-defined callback used to specify an upstream response
	u8_t *              data;             //!< Data to write
} ExtFlashWriteInfo;

/****** Information used for reading and checking procedures ******/
typedef struct
{
	u32_t               startAddress;     //!< Start address of read
	u32_t               checklen;         //!< Checksum length
	u32_t               checksum;         //!< Checksum
	u32_t               readLimit;        //!< Maximum number of bytes that can be read in a single instruction
	ExtFlashAPICallback upstreamCallback; //!< User-defined callback used to specify an upstream response
} ExtFlashCheckInfo;

/****** Union of all external flash info structs ******/
union ExtFlashInfoStructs
{
	ExtFlashEraseInfo eraseInfo;
	ExtFlashWriteInfo writeInfo;
	ExtFlashCheckInfo checkInfo;
};

/**
 * Callback function which sends an EVBME upstream message.
 *
 * @param aContext unused.
 * @return CA_ERROR_SUCCESS
 */
ca_error external_flash_evbme_send_upstream(void *aContext);

/**
 * This function is responsible for the entire "erase" procedure that is triggered by a call to ota_handle_erase().
 * It is initially scheduled by BSP_ExternalFlashScheduleCallback and then it reschedules
 * itself as many times as necessary until the erase procedure is over.
 *
 * @param aContext context pointer containing useful information about how to perform the procedure.
 * 				   Should be of type ExtFlashEraseInfo *.
 *
 * @return This function always returns CA_ERROR_SUCCESS.
 * The status of the procedure itself is sent back as an upstream response via a callback.
*/
ca_error external_flash_erase_helper(void *aContext);

/**
 * This function is responsible for the entire "writing" procedure that is triggered by by a call to ota_handle_write().
 * It is initially scheduled by BSP_ExternalFlashScheduleCallback and then it reschedules
 * itself as many times as necessary until the writing procedure is over.
 *
 * @param aContext context pointer containing useful information about how to perform the procedure.
 * 				   Should be of type ExtFlashWriteInfo *.
 *
 * @return This function always returns CA_ERROR_SUCCESS.
 * The status of the procedure itself is sent back as an upstream response via a callback.
*/
ca_error external_flash_write_helper(void *aContext);

/**
 * This function is responsible for the entire "checking" procedure (which involves reading and verifying)
 * that is triggered by a call to ota_handle_check().
 * It is initially scheduled by BSP_ExternalFlashScheduleCallback and then it reschedules
 * itself as many times as necessary until the checking procedure is over.
 *
 * @param aContext Context pointer containing useful information about how to perform the procedure.
 * 				   Should be of type ExtFlashCheckInfo *.
 *
 * @return This function always returns CA_ERROR_SUCCESS.
 * The status of the procedure itself is sent back as an upstream response via a callback.
*/
ca_error external_flash_check_helper(void *aContext);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CASCODA_EXTERNAL_FLASH_H */
