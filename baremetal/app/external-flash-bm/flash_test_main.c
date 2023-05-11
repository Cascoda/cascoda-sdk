/**
 * @file
 *//*
 *  Copyright (c) 2021, Cascoda Ltd.
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
 * Testing the Winbond external flash chip
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "cascoda-bm/test15_4_evbme.h"

#define TEST_MSDULENGTH (40)
#define TEST_MSDUHANDLE (0xAA)
#define TEST_MSDU                                                                                                     \
	0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, \
	    0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE,   \
	    0xAD, 0xBE, 0xEF
#define TEST_PANID 0x5C, 0xCA
#define TEST_DSTADDR 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88

#define NUMBER_OF_BYTES_IN_A_PAGE 256
#define NUMBER_OF_BYTES_IN_HALF_A_PAGE (NUMBER_OF_BYTES_IN_A_PAGE / 2)

#define NUMBER_OF_PAGES_TOTAL 4096
#define NUMBER_OF_HALF_PAGES_TOTAL (NUMBER_OF_PAGES_TOTAL * 2)

#define NUMBER_OF_PAGES_IN_A_SECTOR 16
#define NUMBER_OF_HALF_PAGES_IN_A_SECTOR (NUMBER_OF_PAGES_IN_A_SECTOR * 2)

#define NUMBER_OF_PAGES_IN_A_32KB_BLOCK 128
#define NUMBER_OF_HALF_PAGES_IN_A_32KB_BLOCK (NUMBER_OF_PAGES_IN_A_32KB_BLOCK * 2)

#define NUMBER_OF_PAGES_IN_A_64KB_BLOCK (NUMBER_OF_PAGES_IN_A_32KB_BLOCK * 2)
#define NUMBER_OF_HALF_PAGES_IN_A_64KB_BLOCK (NUMBER_OF_PAGES_IN_A_64KB_BLOCK * 2)

enum FLASH_nextState
{
	FLASH_GET_DEVICE_ID,
	FLASH_READ_STATUS_REG1,
	FLASH_WRITE_ENABLE_DISABLE,
	FLASH_CHIP_ERASE_AND_READ_DATA,
	FLASH_PAGE_PROGRAM_AND_ERASE_SECTOR,
	FLASH_PAGE_PROGRAM_AND_ERASE_BLOCK32K,
	FLASH_PAGE_PROGRAM_AND_ERASE_BLOCK64K,
	FLASH_POWERDOWN_AND_RELEASE,
	FLASH_PROGRAM_READ_AND_ERASE_UNDER_STRESS,
	FLASH_PROGRAM_AND_OVERWRITE,
	END
};
static enum FLASH_nextState test_nextstate = FLASH_GET_DEVICE_ID;

ca_tasklet        testTasklet;
struct ca821x_dev dev;

typedef struct read_data_helper_args
{
	int   numOfHalfPages;
	int   iterationNumber;
	u32_t startAddress;
	u8_t *dataReadFromFlash;
	u8_t  verificationByte;
	bool  stressed;
} read_data_helper_args;

typedef struct page_program_helper_args
{
	int   numOfHalfPages;
	int   iterationNumber;
	u32_t startAddress;
	u8_t *dataProgrammedToFlash;
	bool  stressed;
} page_program_helper_args;

/** Masks for individual bits in Status Register 1 */
typedef enum statusRegister1_masks
{
	BUSY_mask = 0x01, //!< S0
	WEL_mask  = 0x02, //!< S1
	BP0_mask  = 0x04, //!< S2
	BP1_mask  = 0x08, //!< S3
	BP2_mask  = 0x10, //!< S4
	TB_mask   = 0x20, //!< S5
	SEC_mask  = 0x40, //!< S6
	SRP0_mask = 0x80  //!< S7
} statusRegister1_masks;

/** Bit positions for Status Register 1 */
typedef enum statusRegister1_pos
{
	BUSY_pos = 0, //!< S0
	WEL_pos,      //!< S1
	BP0_pos,      //!< S2
	BP1_pos,      //!< S3
	BP2_pos,      //!< S4
	TB_pos,       //!< S5
	SEC_pos,      //!< S6
	SRP0_pos      //!< S7
} statusRegister1_pos;

/** Forward declarations for non-BSP functions */
ca_error W25Q80DLSNIG_WriteEnable();
ca_error W25Q80DLSNIG_WriteDisable();
ca_error W25Q80DLSNIG_PowerDown();
ca_error W25Q80DLSNIG_ReleasePowerDown();
ca_error W25Q80DLSNIG_ReadStatusRegister1(uint8_t *statusRegister1);

int test15_4_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	if (buf[0] == 0xB2)
		return 1;

	/* Insert Application-Specific Dispatches here in the same style */
	return 0;
}

static ca_error read_data_helper(void *aContext)
{
	bool                  readSuccessful = true;
	read_data_helper_args args;
	args = *((read_data_helper_args *)aContext);

	uint8_t         ret, msdu_buffer[TEST_MSDULENGTH];
	uint8_t         msduhandle = TEST_MSDUHANDLE;
	struct FullAddr full_address;
	full_address.AddressMode = MAC_MODE_SHORT_ADDR;
	full_address.PANId[0]    = 0x5C;
	full_address.PANId[1]    = 0xCA;

	memset(args.dataReadFromFlash, 0x00, sizeof(args.dataReadFromFlash));

	if (args.stressed)
	{
		memcpy(full_address.PANId, (uint8_t[]){TEST_PANID}, sizeof(full_address.PANId));
		memcpy(full_address.Address, (uint8_t[]){TEST_DSTADDR}, sizeof(full_address.Address));
		memcpy(msdu_buffer, (uint8_t[]){TEST_MSDU}, TEST_MSDULENGTH);

#if CASCODA_CA_VER >= 8212
		uint8_t tx_op[2] = {0x00, 0x00};
		MCPS_DATA_request(MAC_MODE_SHORT_ADDR, /* SrcAddrMode */
		                  full_address,        /* DstAddr */
		                  0,                   /* HeaderIELength */
		                  0,                   /* PayloadIELength */
		                  TEST_MSDULENGTH,     /* MsduLength */
		                  msdu_buffer,         /* pMsdu */
		                  TEST_MSDUHANDLE,     /* MsduHandle */
		                  tx_op,               /* pTxOptions */
		                  0,                   /* SchTimestamp */
		                  0,                   /* SchPeriod */
		                  0,                   /* TxChannel */
		                  NULL,                /* pHeaderIEList */
		                  NULL,                /* pPayloadIEList */
		                  NULL,                /* pSecurity */
		                  &dev);               /* pDeviceRef */
#else
		MCPS_DATA_request(
		    MAC_MODE_SHORT_ADDR, full_address, TEST_MSDULENGTH, msdu_buffer, TEST_MSDUHANDLE, 0x00, NULL, &dev);
#endif // CASCODA_CA_VER >= 8212
	}
	ca_error err = BSP_ExternalFlashReadData(args.startAddress + args.iterationNumber * NUMBER_OF_BYTES_IN_HALF_A_PAGE,
	                                         NUMBER_OF_BYTES_IN_HALF_A_PAGE,
	                                         args.dataReadFromFlash);

	if (err == CA_ERROR_NO_ACCESS)
		ca_log_note("Function was not scheduled...");

	// Make sure that all bytes in the array have the value that is expected
	for (uint8_t j = 0; j < NUMBER_OF_BYTES_IN_HALF_A_PAGE; j++)
	{
		if (args.dataReadFromFlash[j] != args.verificationByte)
			readSuccessful = false;
	}

	if (!readSuccessful)
		return CA_ERROR_FAIL;
	else if (args.iterationNumber == (args.numOfHalfPages - 1))
		return CA_ERROR_SUCCESS;
	else
		return CA_ERROR_NOT_HANDLED;
}

static ca_error page_program_helper(void *aContext)
{
	page_program_helper_args args;
	args = *((page_program_helper_args *)aContext);

	uint8_t         ret, msdu_buffer[TEST_MSDULENGTH];
	uint8_t         msduhandle = TEST_MSDUHANDLE;
	struct FullAddr full_address;
	full_address.AddressMode = MAC_MODE_SHORT_ADDR;
	full_address.PANId[0]    = 0x5C;
	full_address.PANId[1]    = 0xCA;

	if (args.stressed)
	{
		memcpy(full_address.PANId, (uint8_t[]){TEST_PANID}, sizeof(full_address.PANId));
		memcpy(full_address.Address, (uint8_t[]){TEST_DSTADDR}, sizeof(full_address.Address));
		memcpy(msdu_buffer, (uint8_t[]){TEST_MSDU}, TEST_MSDULENGTH);

#if CASCODA_CA_VER >= 8212
		uint8_t tx_op[2] = {0x00, 0x00};
		MCPS_DATA_request(MAC_MODE_SHORT_ADDR,
		                  full_address,
		                  0,
		                  0,
		                  TEST_MSDULENGTH,
		                  msdu_buffer,
		                  TEST_MSDUHANDLE,
		                  tx_op,
		                  0,
		                  0,
		                  0,
		                  NULL,
		                  NULL,
		                  NULL,
		                  &dev);
#else
		MCPS_DATA_request(
		    MAC_MODE_SHORT_ADDR, full_address, TEST_MSDULENGTH, msdu_buffer, TEST_MSDUHANDLE, 0x00, NULL, &dev);
#endif // CASCODA_CA_VER >= 8212
	}

	ca_error status =
	    BSP_ExternalFlashProgram(args.startAddress + args.iterationNumber * NUMBER_OF_BYTES_IN_HALF_A_PAGE,
	                             NUMBER_OF_BYTES_IN_HALF_A_PAGE,
	                             args.dataProgrammedToFlash);

	if (status)
		return status;

	if (args.iterationNumber == (args.numOfHalfPages - 1))
		return CA_ERROR_SUCCESS;
	else
		return CA_ERROR_NOT_HANDLED;
}

ca_error test_GetDeviceId()
{
	uint8_t id = 0xFF;

	ca_error err = BSP_ExternalFlashGetDeviceId(&id);
	if (err == CA_ERROR_NO_ACCESS)
		ca_log_note("Function was not scheduled...");

	ca_log_note("id: 0x%x", id);

	if (id != 0x13)
		ca_log_note("Test failed! The id is wrong.");
	else
		ca_log_note("Test Successful.");

	TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
	return CA_ERROR_SUCCESS;
}

ca_error test_ReadStatusRegister1()
{
	uint8_t statusRegister1 = 0xFF;

	while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_ReadStatusRegister1(&statusRegister1) != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;

	ca_log_note("Status register1: 0x%x", statusRegister1);
	ca_log_note("\tBUSY: %d", (statusRegister1 & BUSY_mask) >> BUSY_pos);
	ca_log_note("\tWEL: %d", (statusRegister1 & WEL_mask) >> WEL_pos);
	ca_log_note("\tBP0: %d", (statusRegister1 & BP0_mask) >> BP0_pos);
	ca_log_note("\tBP1: %d", (statusRegister1 & BP1_mask) >> BP1_pos);
	ca_log_note("\tBP2: %d", (statusRegister1 & BP2_mask) >> BP2_pos);
	ca_log_note("\tTB: %d", (statusRegister1 & TB_mask) >> TB_pos);
	ca_log_note("\tSEC: %d", (statusRegister1 & SEC_mask) >> SEC_pos);
	ca_log_note("\tSRP0: %d", (statusRegister1 & SRP0_mask) >> SRP0_pos);

	if (statusRegister1 != 0x00)
		ca_log_note("Test failed! The value of Status Register 1 is wrong.");
	else
		ca_log_note("Test Successful.");

	TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
	return CA_ERROR_SUCCESS;
}

ca_error test_WriteEnableAndDisable(void *aContext)
{
	(void)aContext;

	enum test_states
	{
		CHECK_BEFORE_WRITE_ENABLE = 0,
		CHECK_AFTER_WRITE_ENABLE,
		CHECK_AFTER_WRITE_DISABLE
	};
	static enum test_states cur_state = CHECK_BEFORE_WRITE_ENABLE;

	struct ExternalFlashStatus status;
	status.writeEnabled = 1;

	ca_error err = CA_ERROR_SUCCESS;

	switch (cur_state)
	{
	case CHECK_BEFORE_WRITE_ENABLE:;
		err = BSP_ExternalFlashGetStatus(&status);
		if (err == CA_ERROR_NO_ACCESS)
			ca_log_note("Function was not scheduled......");

		ca_log_note("Before Write Enable instruction:");
		ca_log_note("\tWrite Enable Latch: %d", status.writeEnabled);

		if (status.writeEnabled != 0)
		{
			ca_log_note("Test failed! The initial value of the external flash status is wrong.");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else
		{
			BSP_ExternalFlashScheduleCallback(&test_WriteEnableAndDisable, NULL);
			cur_state = CHECK_AFTER_WRITE_ENABLE;
		}
		break;
	case CHECK_AFTER_WRITE_ENABLE:
		while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
			;
		while (W25Q80DLSNIG_WriteEnable() != CA_ERROR_SUCCESS)
			;
		while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
			;

		err = BSP_ExternalFlashGetStatus(&status);
		if (err == CA_ERROR_NO_ACCESS)
			ca_log_note("Function was not scheduled......");

		ca_log_note("After Write Enable instruction:");
		ca_log_note("\tWrite Enable Latch: %d", status.writeEnabled);

		if (status.writeEnabled != 1)
		{
			ca_log_note(
			    "Test failed! The value of the external flash status is wrong after the Write Enable Instruction.");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else
		{
			BSP_ExternalFlashScheduleCallback(&test_WriteEnableAndDisable, NULL);
			cur_state = CHECK_AFTER_WRITE_DISABLE;
		}
		break;
	case CHECK_AFTER_WRITE_DISABLE:
		while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
			;
		while (W25Q80DLSNIG_WriteDisable() != CA_ERROR_SUCCESS)
			;
		while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
			;

		err = BSP_ExternalFlashGetStatus(&status);
		if (err == CA_ERROR_NO_ACCESS)
			ca_log_note("Function was not scheduled......");

		ca_log_note("After Write Disable instruction:");
		ca_log_note("\tWrite Enable Latch: %d", status.writeEnabled);

		if (status.writeEnabled != 0)
			ca_log_note(
			    "Test failed! The value of the external flash status is wrong after the Write Disable Instruction.");
		else
			ca_log_note("Test Successful.");

		TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error test_ChipEraseAndReadData(void *aContext)
{
	(void)aContext;

	enum test_states
	{
		ERASE = 0,
		READ
	};
	static enum test_states cur_state = ERASE;

	uint32_t       startAddress                                      = 0x000000;
	static uint8_t dataReadFromFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE] = {0x00};
	uint8_t        erasedByte                                        = 0xFF;
	static int     iterationNumber                                   = 0;
	ca_error       read_status;

	static read_data_helper_args args;
	args.numOfHalfPages    = NUMBER_OF_HALF_PAGES_TOTAL;
	args.iterationNumber   = iterationNumber;
	args.startAddress      = startAddress;
	args.dataReadFromFlash = dataReadFromFlash;
	args.verificationByte  = erasedByte;
	args.stressed          = false;

	switch (cur_state)
	{
	case ERASE:
		ca_log_note("Erasing chip...");
		ca_error err = BSP_ExternalFlashChipErase();
		if (err == CA_ERROR_NO_ACCESS)
			ca_log_note("Function was not scheduled......");

		BSP_ExternalFlashScheduleCallback(&test_ChipEraseAndReadData, NULL);
		cur_state = READ;
		break;
	case READ:;
		if (args.iterationNumber == 0)
			ca_log_note("Reading the contents of the external flash chip.");

		read_status = read_data_helper(&args);

		if (read_status == CA_ERROR_NOT_HANDLED)
		{
			iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_ChipEraseAndReadData, NULL);
		}
		else
		{
			if (read_status == CA_ERROR_FAIL)
				ca_log_note("Test failed! The chip erase and/or the read data has failed.");
			else if (read_status == CA_ERROR_SUCCESS)
				ca_log_note("Test successful.");

			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error test_PageProgramAndEraseSector(void *aContext)
{
	(void)aContext;

	enum test_states
	{
		READ = 0,
		PROGRAM,
		ERASE,
	};

	enum read_sub_states
	{
		READ_BEFORE_PROGRAM = 0,
		READ_AFTER_PROGRAM,
		READ_AFTER_ERASE
	};

	static enum test_states cur_state  = READ;
	static enum test_states read_state = READ_BEFORE_PROGRAM;

	uint32_t startAddress  = 0x08D000; //Arbitrarily chosen: Sector 13 (starting xxD000) of Block 8 (starting 080000).
	uint8_t  byteToProgram = 0x4E;     //Arbitrarily chosen
	uint8_t  erasedByte    = 0xFF;

	static uint8_t dataReadFromFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE]     = {0x00};
	uint8_t        dataProgrammedToFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE] = {0x00};
	memset(dataProgrammedToFlash, byteToProgram, sizeof(dataProgrammedToFlash));

	static int read_iterationNumber    = 0;
	static int program_iterationNumber = 0;
	ca_error   read_status;
	ca_error   program_status;

	static read_data_helper_args read_args;
	read_args.numOfHalfPages    = NUMBER_OF_HALF_PAGES_IN_A_SECTOR;
	read_args.iterationNumber   = read_iterationNumber;
	read_args.startAddress      = startAddress;
	read_args.dataReadFromFlash = dataReadFromFlash;
	read_args.verificationByte  = erasedByte;
	read_args.stressed          = false;

	static page_program_helper_args program_args;
	program_args.numOfHalfPages        = NUMBER_OF_HALF_PAGES_IN_A_SECTOR;
	program_args.iterationNumber       = program_iterationNumber;
	program_args.startAddress          = startAddress;
	program_args.dataProgrammedToFlash = dataProgrammedToFlash;
	program_args.stressed              = false;

	switch (cur_state)
	{
	case READ:
		if (read_state == READ_BEFORE_PROGRAM && read_iterationNumber == 0)
		{
			ca_log_note("Checking that the sector is erased to begin with.");
		}
		else if (read_state == READ_AFTER_PROGRAM)
		{
			read_args.verificationByte = byteToProgram;
			if (read_iterationNumber == 0)
				ca_log_note("Checking that the sector was programmed correctly.");
		}
		else if (read_state == READ_AFTER_ERASE && read_iterationNumber == 0)
		{
			ca_log_note("Checking that the sector was erased.");
		}

		read_status = read_data_helper(&read_args);

		if (read_status == CA_ERROR_NOT_HANDLED)
		{
			read_iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseSector, NULL);
		}
		else
		{
			if (read_status == CA_ERROR_FAIL)
			{
				if (read_state == READ_BEFORE_PROGRAM)
					ca_log_note("Test failed! The sector is not initially erased. Cannot program it.");
				else if (read_state == READ_AFTER_PROGRAM)
					ca_log_note("Test failed! The sector was not programmed correctly.");
				else if (read_state == READ_AFTER_ERASE)
					ca_log_note("Test failed! The sector was not erased properly.");

				TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
			}
			else
			{
				if (read_state == READ_BEFORE_PROGRAM)
				{
					read_iterationNumber = 0;
					BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseSector, NULL);
					read_state = READ_AFTER_PROGRAM;
					cur_state  = PROGRAM;
				}
				else if (read_state == READ_AFTER_PROGRAM)
				{
					read_iterationNumber = 0;
					BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseSector, NULL);
					read_state = READ_AFTER_ERASE;
					cur_state  = ERASE;
				}
				else if (read_state == READ_AFTER_ERASE)
				{
					ca_log_note("Test successful.");
					TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
				}
			}
		}
		break;
	case PROGRAM:
		if (program_iterationNumber == 0)
			ca_log_note("Programming the sector.");

		program_status = page_program_helper(&program_args);

		if (program_status == CA_ERROR_NOT_HANDLED)
		{
			program_iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseSector, NULL);
		}
		else if (program_status == CA_ERROR_NO_ACCESS)
		{
			ca_log_note("Function was not scheduled......");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else if (program_status == CA_ERROR_INVALID_ARGS)
		{
			ca_log_note(
			    "Test failed! The programming function was given invalid arguments (check the start address and number of bytes to program.)");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else
		{
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseSector, NULL);
			cur_state = READ;
		}

		break;
	case ERASE:
		ca_log_note("Erasing the sector.");
		ca_error err = BSP_ExternalFlashPartialErase(SECTOR_4KB, startAddress);
		if (err == CA_ERROR_NO_ACCESS)
			ca_log_note("Function was not scheduled......");

		BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseSector, NULL);
		cur_state = READ;
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error test_PageProgramAndEraseBlock32k(void *aContext)
{
	(void)aContext;

	enum test_states
	{
		READ = 0,
		PROGRAM,
		ERASE,
	};

	enum read_sub_states
	{
		READ_BEFORE_PROGRAM = 0,
		READ_AFTER_PROGRAM,
		READ_AFTER_ERASE
	};

	static enum test_states cur_state  = READ;
	static enum test_states read_state = READ_BEFORE_PROGRAM;

	uint32_t startAddress  = 0x028000; //Arbitrarily chosen: Middle of the 64KB Block 2 (027FFF)
	uint8_t  byteToProgram = 0xF5;     //Arbitrarily chosen
	uint8_t  erasedByte    = 0xFF;

	static uint8_t dataReadFromFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE]     = {0x00};
	uint8_t        dataProgrammedToFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE] = {0x00};
	memset(dataProgrammedToFlash, byteToProgram, sizeof(dataProgrammedToFlash));

	static int read_iterationNumber    = 0;
	static int program_iterationNumber = 0;
	ca_error   read_status;
	ca_error   program_status;

	static read_data_helper_args read_args;
	read_args.numOfHalfPages    = NUMBER_OF_HALF_PAGES_IN_A_32KB_BLOCK;
	read_args.iterationNumber   = read_iterationNumber;
	read_args.startAddress      = startAddress;
	read_args.dataReadFromFlash = dataReadFromFlash;
	read_args.verificationByte  = erasedByte;
	read_args.stressed          = false;

	static page_program_helper_args program_args;
	program_args.numOfHalfPages        = NUMBER_OF_HALF_PAGES_IN_A_32KB_BLOCK;
	program_args.iterationNumber       = program_iterationNumber;
	program_args.startAddress          = startAddress;
	program_args.dataProgrammedToFlash = dataProgrammedToFlash;
	program_args.stressed              = false;

	switch (cur_state)
	{
	case READ:
		if (read_state == READ_BEFORE_PROGRAM && read_iterationNumber == 0)
		{
			ca_log_note("Checking that the 32KB block is erased to begin with.");
		}
		else if (read_state == READ_AFTER_PROGRAM)
		{
			read_args.verificationByte = byteToProgram;
			if (read_iterationNumber == 0)
				ca_log_note("Checking that the 32KB block was programmed correctly.");
		}
		else if (read_state == READ_AFTER_ERASE && read_iterationNumber == 0)
		{
			ca_log_note("Checking that the 32KB block was erased.");
		}

		read_status = read_data_helper(&read_args);

		if (read_status == CA_ERROR_NOT_HANDLED)
		{
			read_iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock32k, NULL);
		}
		else
		{
			if (read_status == CA_ERROR_FAIL)
			{
				if (read_state == READ_BEFORE_PROGRAM)
					ca_log_note("Test failed! The 32KB block is not initially erased. Cannot program it.");
				else if (read_state == READ_AFTER_PROGRAM)
					ca_log_note("Test failed! The 32KB block was not programmed correctly.");
				else if (read_state == READ_AFTER_ERASE)
					ca_log_note("Test failed! The 32KB block was not erased properly.");

				TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
			}
			else
			{
				if (read_state == READ_BEFORE_PROGRAM)
				{
					read_iterationNumber = 0;
					BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock32k, NULL);
					read_state = READ_AFTER_PROGRAM;
					cur_state  = PROGRAM;
				}
				else if (read_state == READ_AFTER_PROGRAM)
				{
					read_iterationNumber = 0;
					BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock32k, NULL);
					read_state = READ_AFTER_ERASE;
					cur_state  = ERASE;
				}
				else if (read_state == READ_AFTER_ERASE)
				{
					ca_log_note("Test successful.");
					TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
				}
			}
		}
		break;
	case PROGRAM:
		if (program_iterationNumber == 0)
			ca_log_note("Programming the 32KB block.");

		program_status = page_program_helper(&program_args);

		if (program_status == CA_ERROR_NOT_HANDLED)
		{
			program_iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock32k, NULL);
		}
		else if (program_status == CA_ERROR_NO_ACCESS)
		{
			ca_log_note("Function was not scheduled......");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else if (program_status == CA_ERROR_INVALID_ARGS)
		{
			ca_log_note(
			    "Test failed! The programming function was given invalid arguments (check the start address and number of bytes to program.)");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else
		{
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock32k, NULL);
			cur_state = READ;
		}
		break;
	case ERASE:
		ca_log_note("Erasing the 32KB block.");
		ca_error err = BSP_ExternalFlashPartialErase(BLOCK_32KB, startAddress);
		if (err == CA_ERROR_NO_ACCESS)
			ca_log_note("Function was not scheduled......");
		BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock32k, NULL);
		cur_state = READ;
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error test_PageProgramAndEraseBlock64k(void *aContext)
{
	static bool stressed;
	stressed = *((bool *)aContext);

	enum test_states
	{
		READ = 0,
		PROGRAM,
		ERASE,
	};

	enum read_sub_states
	{
		READ_BEFORE_PROGRAM = 0,
		READ_AFTER_PROGRAM,
		READ_AFTER_ERASE
	};

	static enum test_states cur_state  = READ;
	static enum test_states read_state = READ_BEFORE_PROGRAM;

	uint32_t startAddress  = 0x060000; //Arbitrarily chosen: Block 6 (060000)
	uint8_t  byteToProgram = 0x08;     //Arbitrarily chosen
	uint8_t  erasedByte    = 0xFF;

	static uint8_t dataReadFromFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE]     = {0x00};
	uint8_t        dataProgrammedToFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE] = {0x00};
	memset(dataProgrammedToFlash, byteToProgram, sizeof(dataProgrammedToFlash));

	static int read_iterationNumber    = 0;
	static int program_iterationNumber = 0;
	ca_error   read_status;
	ca_error   program_status;

	static read_data_helper_args read_args;
	read_args.numOfHalfPages    = NUMBER_OF_HALF_PAGES_IN_A_64KB_BLOCK;
	read_args.iterationNumber   = read_iterationNumber;
	read_args.startAddress      = startAddress;
	read_args.dataReadFromFlash = dataReadFromFlash;
	read_args.verificationByte  = erasedByte;
	read_args.stressed          = stressed;

	static page_program_helper_args program_args;
	program_args.numOfHalfPages        = NUMBER_OF_HALF_PAGES_IN_A_64KB_BLOCK;
	program_args.iterationNumber       = program_iterationNumber;
	program_args.startAddress          = startAddress;
	program_args.dataProgrammedToFlash = dataProgrammedToFlash;
	program_args.stressed              = stressed;

	switch (cur_state)
	{
	case READ:
		if (read_state == READ_BEFORE_PROGRAM && read_iterationNumber == 0)
		{
			ca_log_note("Checking that the 64KB block is erased to begin with.");
		}
		else if (read_state == READ_AFTER_PROGRAM)
		{
			read_args.verificationByte = byteToProgram;
			if (read_iterationNumber == 0)
				ca_log_note("Checking that the 64KB block was programmed correctly.");
		}
		else if (read_state == READ_AFTER_ERASE && read_iterationNumber == 0)
		{
			ca_log_note("Checking that the 64KB block was erased.");
		}

		read_status = read_data_helper(&read_args);

		if (read_status == CA_ERROR_NOT_HANDLED)
		{
			read_iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
		}
		else
		{
			if (read_status == CA_ERROR_FAIL)
			{
				if (read_state == READ_BEFORE_PROGRAM)
					ca_log_note("Test failed! The 64KB block is not initially erased. Cannot program it.");
				else if (read_state == READ_AFTER_PROGRAM)
					ca_log_note("Test failed! The 64KB block was not programmed correctly.");
				else if (read_state == READ_AFTER_ERASE)
					ca_log_note("Test failed! The 64KB block was not erased properly.");

				//Reset state in case this test function is called again.
				read_iterationNumber    = 0;
				program_iterationNumber = 0;
				read_state              = READ_BEFORE_PROGRAM;
				cur_state               = READ;

				TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
			}
			else
			{
				if (read_state == READ_BEFORE_PROGRAM)
				{
					read_iterationNumber = 0;
					BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
					read_state = READ_AFTER_PROGRAM;
					cur_state  = PROGRAM;
				}
				else if (read_state == READ_AFTER_PROGRAM)
				{
					read_iterationNumber = 0;
					BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
					read_state = READ_AFTER_ERASE;
					cur_state  = ERASE;
				}
				else if (read_state == READ_AFTER_ERASE)
				{
					ca_log_note("Test successful.");

					//Reset state in case this test function is called again.
					read_iterationNumber    = 0;
					program_iterationNumber = 0;
					read_state              = READ_BEFORE_PROGRAM;
					cur_state               = READ;

					TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
				}
			}
		}
		break;
	case PROGRAM:
		if (program_iterationNumber == 0)
			ca_log_note("Programming the 64KB block.");

		program_status = page_program_helper(&program_args);

		if (program_status == CA_ERROR_NOT_HANDLED)
		{
			program_iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
		}
		else if (program_status == CA_ERROR_NO_ACCESS)
		{
			ca_log_note("Function was not scheduled......");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else if (program_status == CA_ERROR_INVALID_ARGS)
		{
			ca_log_note(
			    "Test failed! The programming function was given invalid arguments (check the start address and number of bytes to program.)");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else
		{
			BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
			cur_state = READ;
		}
		break;
	case ERASE:
		ca_log_note("Erasing the 64KB block.");
		ca_error err = BSP_ExternalFlashPartialErase(BLOCK_64KB, startAddress);
		if (err == CA_ERROR_NO_ACCESS)
			ca_log_note("Function was not scheduled......");

		BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
		cur_state = READ;
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error test_PowerDownAndReleasePowerDown(void *aContext)
{
	(void)aContext;

	uint8_t statusRegisterValue          = 0xFF;
	uint8_t expectedValueDuringPowerDown = 0xFF;
	uint8_t expectedValueAfterPowerDown  = 0x00;

	ca_error err = CA_ERROR_SUCCESS;

	ca_log_note("Checking that the external flash chip ignores the incoming status register read instruction.");

	while (W25Q80DLSNIG_ReadStatusRegister1(&statusRegisterValue) != CA_ERROR_SUCCESS)
		;

	if (statusRegisterValue != expectedValueDuringPowerDown)
	{
		ca_log_note(
		    "Test failed! The status register read instruction is supposed to be ignored when in power down mode!");
		TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		return CA_ERROR_SUCCESS;
	}

	ca_log_note("The external flash chip is being released from power down mode.");

	while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
		;

	ca_log_note(
	    "Checking that the external flash chip no longer ignores the incoming status register read instruction.");

	while (W25Q80DLSNIG_ReadStatusRegister1(&statusRegisterValue) != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;

	if (statusRegisterValue != expectedValueAfterPowerDown)
		ca_log_note("Test failed! The status register read was ignored!");
	else
		ca_log_note("Test successful.");

	TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);

	return CA_ERROR_SUCCESS;
}

ca_error test_OverWriteData(void *aContext)
{
	(void)aContext;

	enum test_states
	{
		PROGRAM_HALF_PAGE = 0,
		READ,
		OVERWRITE_HALF_PAGE
	};

	enum read_sub_states
	{
		READ_AFTER_PROGRAM = 0,
		READ_AFTER_OVERWRITE
	};

	static enum test_states cur_state  = PROGRAM_HALF_PAGE;
	static enum test_states read_state = READ_AFTER_PROGRAM;

	uint32_t startAddress      = 0x078900; //Arbitrarily chosen beginning of a page
	uint8_t  byteToProgram     = 0xAA;     //Arbitrarily chosen
	uint8_t  byteToOverwrite   = 0xA6;
	uint8_t  overwrittenResult = 0xA2;
	uint8_t  erasedByte        = 0xFF;

	static uint8_t dataReadFromFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE]     = {0x00};
	uint8_t        dataProgrammedToFlash[NUMBER_OF_BYTES_IN_HALF_A_PAGE] = {0x00};
	memset(dataProgrammedToFlash, 0, sizeof(dataProgrammedToFlash));

	static int read_iterationNumber    = 0;
	static int program_iterationNumber = 0;
	ca_error   read_status;
	ca_error   program_status;

	static read_data_helper_args read_args;
	read_args.numOfHalfPages    = 3;
	read_args.iterationNumber   = read_iterationNumber;
	read_args.startAddress      = startAddress - 0x80;
	read_args.dataReadFromFlash = dataReadFromFlash;
	read_args.verificationByte  = erasedByte;
	read_args.stressed          = false;

	static page_program_helper_args program_args;
	program_args.numOfHalfPages        = 1;
	program_args.iterationNumber       = program_iterationNumber;
	program_args.startAddress          = startAddress;
	program_args.dataProgrammedToFlash = dataProgrammedToFlash;
	program_args.stressed              = false;

	switch (cur_state)
	{
	case PROGRAM_HALF_PAGE:
		ca_log_note("Programming a single half page to 0x%x", byteToProgram);

		memset(dataProgrammedToFlash, byteToProgram, sizeof(dataProgrammedToFlash));
		program_status = page_program_helper(&program_args);

		if (program_status == CA_ERROR_NOT_HANDLED)
		{
			ca_log_note("Something went wrong...");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else if (program_status == CA_ERROR_NO_ACCESS)
		{
			ca_log_note("Function was not scheduled......");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else if (program_status == CA_ERROR_INVALID_ARGS)
		{
			ca_log_note(
			    "Test failed! The programming function was given invalid arguments (check the start address and number of bytes to program.)");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else
		{
			BSP_ExternalFlashScheduleCallback(&test_OverWriteData, NULL);
			cur_state = READ;
		}
		break;
	case READ:
		if (read_state == READ_AFTER_PROGRAM && read_iterationNumber == 0)
			ca_log_note(
			    "Reading the previously programmed half page, as well as the half page before it and after it, for comparison (because those were left untouched).");
		else if (read_state == READ_AFTER_OVERWRITE && read_iterationNumber == 0)
			ca_log_note(
			    "Reading the overwritten half page, as well as the half page before it and after it, for comparison (because those were left untouched).");

		if (read_state == READ_AFTER_PROGRAM && read_iterationNumber == 1)
			read_args.verificationByte = byteToProgram;
		else if (read_state == READ_AFTER_OVERWRITE && read_iterationNumber == 1)
			read_args.verificationByte = overwrittenResult;

		read_status = read_data_helper(&read_args);

		ca_log_note("Result (just one byte for simplicity), read_iteration %d: 0x%x",
		            read_args.iterationNumber,
		            read_args.dataReadFromFlash[0]);

		if (read_status == CA_ERROR_NOT_HANDLED)
		{
			read_iterationNumber++;
			BSP_ExternalFlashScheduleCallback(&test_OverWriteData, NULL);
		}
		else
		{
			if (read_status == CA_ERROR_FAIL)
			{
				if (read_state == READ_AFTER_PROGRAM)
					ca_log_note("Test failed! The half page was not programmed correctly.");
				else if (read_state == READ_AFTER_OVERWRITE)
					ca_log_note("Test failed! Overwritten result not correct.");

				TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
			}
			else
			{
				if (read_state == READ_AFTER_PROGRAM)
				{
					read_iterationNumber = 0;
					BSP_ExternalFlashScheduleCallback(&test_OverWriteData, NULL);
					read_state = READ_AFTER_OVERWRITE;
					cur_state  = OVERWRITE_HALF_PAGE;
				}
				else if (read_state == READ_AFTER_OVERWRITE)
				{
					ca_log_note("Test successful.");
					TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
				}
			}
		}
		break;
	case OVERWRITE_HALF_PAGE:
		ca_log_note("Overwriting a single half page to 0x%x", byteToOverwrite);

		memset(dataProgrammedToFlash, byteToOverwrite, sizeof(dataProgrammedToFlash));
		program_status = page_program_helper(&program_args);

		if (program_status == CA_ERROR_NOT_HANDLED)
		{
			ca_log_note("Something went wrong...");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else if (program_status == CA_ERROR_NO_ACCESS)
		{
			ca_log_note("Function was not scheduled......");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else if (program_status == CA_ERROR_INVALID_ARGS)
		{
			ca_log_note(
			    "Test failed! The programming function was given invalid arguments (check the start address and number of bytes to program.)");
			TASKLET_ScheduleDelta(&testTasklet, 1000, NULL);
		}
		else
		{
			BSP_ExternalFlashScheduleCallback(&test_OverWriteData, NULL);
			cur_state = READ;
		}
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error handle_tests()
{
	static bool stressed;

	switch (test_nextstate)
	{
	case FLASH_GET_DEVICE_ID:
		ca_log_note("===================================================");
		ca_log_note("===== TEST1 - GET DEVICE ID + SPI STRESS TEST =====");
		ca_log_note("===================================================");
		test_nextstate = FLASH_READ_STATUS_REG1;
		BSP_ExternalFlashScheduleCallback(&test_GetDeviceId, NULL);
		break;
	case FLASH_READ_STATUS_REG1:
		ca_log_note("==========================================");
		ca_log_note("===== TEST2 - READ STATUS REGISTER 1 =====");
		ca_log_note("==========================================");
		test_nextstate = FLASH_WRITE_ENABLE_DISABLE;
		BSP_ExternalFlashScheduleCallback(&test_ReadStatusRegister1, NULL);
		break;
	case FLASH_WRITE_ENABLE_DISABLE:
		ca_log_note("============================================");
		ca_log_note("===== TEST3 - WRITE ENABLE AND DISABLE =====");
		ca_log_note("============================================");
		test_nextstate = FLASH_CHIP_ERASE_AND_READ_DATA;
		BSP_ExternalFlashScheduleCallback(&test_WriteEnableAndDisable, NULL);
		break;
	case FLASH_CHIP_ERASE_AND_READ_DATA:
		ca_log_note("=====================================================");
		ca_log_note("===== TEST4 - ERASE THE ENTIRE CHIP AND READ IT =====");
		ca_log_note("=====================================================");
		test_nextstate = FLASH_PAGE_PROGRAM_AND_ERASE_SECTOR;
		BSP_ExternalFlashScheduleCallback(&test_ChipEraseAndReadData, NULL);
		break;
	case FLASH_PAGE_PROGRAM_AND_ERASE_SECTOR:
		ca_log_note("==============================================");
		ca_log_note("===== TEST5 - PROGRAM AND ERASE A SECTOR =====");
		ca_log_note("==============================================");
		test_nextstate = FLASH_PAGE_PROGRAM_AND_ERASE_BLOCK32K;
		BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseSector, NULL);
		break;
	case FLASH_PAGE_PROGRAM_AND_ERASE_BLOCK32K:
		ca_log_note("==================================================");
		ca_log_note("===== TEST6 - PROGRAM AND ERASE A 32KB BLOCK =====");
		ca_log_note("==================================================");
		test_nextstate = FLASH_PAGE_PROGRAM_AND_ERASE_BLOCK64K;
		BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock32k, NULL);
		break;
	case FLASH_PAGE_PROGRAM_AND_ERASE_BLOCK64K:;
		stressed = false;
		ca_log_note("==================================================");
		ca_log_note("===== TEST7 - PROGRAM AND ERASE A 64KB BLOCK =====");
		ca_log_note("==================================================");
		test_nextstate = FLASH_POWERDOWN_AND_RELEASE;
		BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
		break;
	case FLASH_POWERDOWN_AND_RELEASE:
		ca_log_note("=========================================================");
		ca_log_note("===== TEST8 - POWER DOWN AND RELEASE FROM POWERDOWN =====");
		ca_log_note("=========================================================");
		test_nextstate = FLASH_PROGRAM_READ_AND_ERASE_UNDER_STRESS;
		BSP_ExternalFlashScheduleCallback(&test_PowerDownAndReleasePowerDown, NULL);
		break;
	case FLASH_PROGRAM_READ_AND_ERASE_UNDER_STRESS:;
		stressed = true;
		ca_log_note("=========================================================");
		ca_log_note("===== TEST10 - PROGRAM, READ AND ERASE UNDER STRESS =====");
		ca_log_note("=========================================================");
		test_nextstate = FLASH_PROGRAM_AND_OVERWRITE;
		BSP_ExternalFlashScheduleCallback(&test_PageProgramAndEraseBlock64k, &stressed);
		break;
	case FLASH_PROGRAM_AND_OVERWRITE:
		ca_log_note("==========================================");
		ca_log_note("===== TEST11 - PROGRAM AND OVERWRITE =====");
		ca_log_note("==========================================");
		test_nextstate = END;
		BSP_ExternalFlashScheduleCallback(&test_OverWriteData, NULL);
		break;
	case END:
		ca_log_note("~~~~~~~~~~~~~~~~~~~~~~");
		ca_log_note("~~~~~ TESTS DONE ~~~~~");
		ca_log_note("~~~~~~~~~~~~~~~~~~~~~~");
		break;
	}

	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Main Program Endless Loop
 *******************************************************************************
 * \return Does not return
 *******************************************************************************
 ******************************************************************************/
int main(void)
{
	ca821x_api_init(&dev);

	/* Initialisation of Chip and EVBME */
	/* Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action */
	/* in case there is no UpStream Communications Channel available */
	EVBMEInitialise(CA_TARGET_NAME, &dev);

	/* Insert Application-Specific Initialisation Routines here */
	TASKLET_Init(&testTasklet, &handle_tests);
	TASKLET_ScheduleDelta(&testTasklet, 712, NULL);
	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);
	} /* while(1) */
}
