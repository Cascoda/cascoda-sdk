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

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	u32_t startAddress; //!< Start address of erase
	u32_t eraseLength;  //!< How many bytes to erase
	u32_t endAddress;   //!< End address of erase
} evbmeExtFlashEraseInfo;

/****** Information used for writing procedure ******/
typedef struct
{
	u32_t startAddress; //!< Start address of write
	u32_t writeLength;  //!< Number of bytes to write
	u32_t writeLimit;   //!< Maximum number of bytes that can be written in a single instruction
	u16_t pageSize;     //!< Size (in bytes) of a page in the external flash
	u8_t *data;         //!< Data to write
} evbmeExtFlashWriteInfo;

/****** Information used for reading and checking procedures ******/
typedef struct
{
	u32_t startAddress; //!< Start address of read
	u32_t checklen;     //!< Checksum length
	u32_t checksum;     //!< Checksum
	u32_t readLimit;    //!< Maximum number of bytes that can be read in a single instruction
} evbmeExtFlashCheckInfo;

/****** Union of all evbme external flash info structs ******/
union evbmeExtFlashInfoStructs
{
	evbmeExtFlashEraseInfo eraseInfo;
	evbmeExtFlashWriteInfo writeInfo;
	evbmeExtFlashCheckInfo checkInfo;
};

/**
 * This function is responsible for the entire "erase" procedure that is triggered by an EVBME_DFU_Erase command.
 * It is initially scheduled by BSP_ExternalFlashScheduleCallback in response to an EVBME_DFU_erase,
 * and then it reschedules itself as many times as necessary until the erase procedure is over.
 *
 * @param aContext context pointer containing useful information about how to perform the procedure.
 *
 * @return This function always returns CA_ERROR_SUCCESS.
 * The status of the procedure itself is sent back as an EVBME_DFU_STATUS_indication.
*/
ca_error external_flash_evbme_erase_helper(void *aContext);

/**
 * This function is responsible for the entire "writing" procedure that is triggered by an EVBME_DFU_Write command.
 * It is initially scheduled by BSP_ExternalFlashScheduleCallback in response to an EVBME_DFU_Write,
 * and then it reschedules itself as many times as necessary until the writing procedure is over.
 *
 * @param aContext context pointer containing useful information about how to perform the procedure.
 *
 * @return This function always returns CA_ERROR_SUCCESS.
 * The status of the procedure itself is sent back as an EVBME_DFU_STATUS_indication.
*/
ca_error external_flash_evbme_write_helper(void *aContext);

/**
 * This function is responsible for the entire "checking" procedure (which involves reading and verifying)
 * that is triggered by an EVBME_DFU_Check command.
 * It is initially scheduled by BSP_ExternalFlashScheduleCallback in response to an EVBME_DFU_Check,
 * and then it reschedules itself as many times as necessary until the checking procedure is over.
 *
 * @param aContext context pointer containing useful information about how to perform the procedure.
 *
 * @return This function always returns CA_ERROR_SUCCESS.
 * The status of the procedure itself is sent back as an EVBME_DFU_STATUS_indication.
*/
ca_error external_flash_evbme_check_helper(void *aContext);

#ifdef __cplusplus
}
#endif

#endif /* CASCODA_EXTERNAL_FLASH_H */
