/*
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
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * External Flash Functions for Winbond W25Q80DLSNIG, see cascoda-bm/cascoda_interface.h for descriptions
*/

/* System */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Platform */
#include "M2351.h"

/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#include "cascoda_secure.h"

/* External flash chip base address (corresponds to address 0x000000 on the flash chip) */
#define EXTERNAL_FLASH_BASE 0xB0000000

/* SPI Instructions */
#define WRITE_ENABLE 0x06
#define WRITE_DISABLE 0x04
#define READ_STATUS_REGISTER_1 0x05
#define PAGE_PROGRAM 0x02
#define SECTOR_ERASE_4KB 0x20
#define BLOCK_ERASE_32KB 0x52
#define BLOCK_ERASE_64KB 0xD8
#define CHIP_ERASE 0xC7
#define POWER_DOWN 0xB9
#define READ_DATA 0x03
#define RELEASE_POWERDOWN_ID 0xAB
/* Dummy/Don't care bytes */
#define DUMMY_BYTE 0xFF     //Transmit this byte wherever it says to transmit a dummy byte in the datasheet
#define DONT_CARE_BYTE 0x00 //Transmit this byte while expecting to receive something from the external flash

/* Length of an instruction in bytes */
#define INSTRUCTION_BYTE_LENGTH 1
/* Length of an address in bytes */
#define ADDRESS_BYTE_LENGTH 3

/* Number of bytes in a page */
#define NUM_OF_BYTES_IN_A_PAGE 256
/* Maximum number of bytes that can theoretically be programmed at once */
#define PAGE_PROGRAM_MAX_BYTES_THEORETICAL NUM_OF_BYTES_IN_A_PAGE
/* Maximum number of bytes that will actually be allowed to be read at once */
#define PAGE_PROGRAM_MAX_BYTES_ACTUAL (PAGE_PROGRAM_MAX_BYTES_THEORETICAL / 2)
/* Maximum number of bytes that can theoretically be read at once */
#define READ_DATA_MAX_BYTES_THEORETICAL 1048576
/* Maximum number of bytes that will actually be allowed to be read at once */
#define READ_DATA_MAX_BYTES_ACTUAL PAGE_PROGRAM_MAX_BYTES_ACTUAL

/* Page Program instruction transmit buffer length */
#define PAGE_PROGRAM_MAX_TX_LEN (INSTRUCTION_BYTE_LENGTH + ADDRESS_BYTE_LENGTH + PAGE_PROGRAM_MAX_BYTES_ACTUAL)
/* Read Data maximum receive buffer length */
#define READ_DATA_MAX_RX_LEN PAGE_PROGRAM_MAX_TX_LEN

/* Timing requirements in milliseconds (see W25Q80DLSNIG datasheet for more detail) */
#define T_PP 4     // Page Program Time, in milliseconds
#define T_SE 300   // Sector Erase Time (4KB), in milliseconds
#define T_CE 6000  // Chip Erase Time, in milliseconds
#define T_BE1 800  // Block Erase Time (32KB), in milliseconds
#define T_BE2 1000 // Block Erase Time (64KB), in milliseconds
#define T_DP 4     // /CS High to Power-down Mode, in microseconds
#define T_RES1 4   // /CS High to Standby Mode without ID Read, in microseconds

/* Info about this external flash chip */
const struct ExternalFlashInfo BSP_ExternalFlashInfo = {EXTERNAL_FLASH_BASE,
                                                        PAGE_PROGRAM_MAX_BYTES_ACTUAL,
                                                        NUM_OF_BYTES_IN_A_PAGE};

/* SPI receive buffer */
static uint8_t rxbuf[READ_DATA_MAX_RX_LEN];

/* Whether SPI communication with the external flash is in progress */
static volatile bool spiInProgress = false;

/* Whether an internal cycle (e.g. chip erase, page program etc.) is in progress */
static bool cycleInProgress = false;

/* Tasklet to handle waiting for a certain amount of time after some instructions as defined in the datasheet */
static ca_tasklet postInstructionWaitTasklet;

/* Tasklet to handle scheduling callbacks for the next higher layer */
static ca_tasklet callbackSchedulingTasklet;

/* Callback and context pointer for the next higher layer */
static ExternalFlashCallback nextCallbackToRun = NULL;
static void *                callbackContext;

/* Whether a BSP_ExternalFlash* function is called from within a higher layer callback
 * This is to ensure that those functions are only called via a callback scheduled by BSP_ExternalFlashScheduleCallback*/
static bool isCalledWithinCallback = false;

/** Masks for individual bits in Status Register 1 */
typedef enum statusRegister1_masks
{
	// Note: add more members if other bits need to be used in the future
	WEL_mask = 0x02, //!< S1 mask
} statusRegister1_masks;

/** Bit positions for Status Register 1 */
typedef enum statusRegister1_pos
{
	//Note: add more members if other bits need to be used in the future
	WEL_pos = 1 //!< S1 position
} statusRegister1_pos;

/**
 * Function called internally before a page program instruction
 * to make sure that there are enough erased bytes left in the page.
 * This is to prevent the addresses wrapping around the page,
 * which leads to the beginning of the page being overwritten.
 *
 * @param startAddress The desired start address of the page program instruction.
 * @param numOfBytes   The desired number of bytes to be sequentially programmed, starting from aStartAddress.
 *
 * @return Status of the command.
 * @retval true  The page would overflow, given the current startAddress and numOfBytes.
 * @retval false The page wouldn't overflow.
*/
static bool pageOverflow(uint32_t startAddress, uint8_t numOfBytes)
{
	int numOfBytesLeftInThePage = NUM_OF_BYTES_IN_A_PAGE - (startAddress % NUM_OF_BYTES_IN_A_PAGE) - numOfBytes;

	if (numOfBytesLeftInThePage < 0)
		return true;
	else
		return false;
}

/**
 * Callback function that is used to call the "ExternalFlashCallback" "nextCallbackToRun",
 * which is set in BSP_ExternalFlashScheduleCallback().
 *
 * This function is scheduled within BSP_ExternalFlashScheduleCallback(). This is
 * to ensure that the next instruction from the higher layer is called by TASKLET_Process(),
 * which is called in cascoda_io_handler(). This allows the Chili module to have the
 * opportunity to handle other processes between consecutive instructions
 * for the external flash chip.
 *
 * @param _notused Not used. Requirement for tasklet callbacks.
 *
 * @return CA_ERROR_SUCCESS
*/
static ca_error external_flash_schedule(void *_notused)
{
	(void)_notused;

	isCalledWithinCallback = true;

	if (nextCallbackToRun)
	{
		ExternalFlashCallback tempCallback = nextCallbackToRun;
		nextCallbackToRun                  = NULL;
		tempCallback(callbackContext);
	}

	// In case no BSP_ExternalFlash* functions were called in the scheduled callback.
	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/**
 * Callback function that is used to call the "ExternalFlashCallback" "nextCallbackToRun",
 * which is set in BSP_ExternalFlashScheduleCallback().
 *
 * This function is scheduled within the driver functions (W25Q80DLSNIG_*()) which
 * cause the external flash chip to enter an internal cycle, for example during a page program
 * or chip erase. This is to ensure that the next instruction from the higher layer
 * doesn't happen while the external chip is in the middle of a cycle.
 *
 * @param _notused Not used. Requirement for tasklet callbacks.
 *
 * @return CA_ERROR_SUCCESS
*/
static ca_error handlePostInstructionWaiting(void *_notused)
{
	(void)_notused;

	cycleInProgress = false;

	isCalledWithinCallback = true;

	if (nextCallbackToRun)
	{
		ExternalFlashCallback tempCallback = nextCallbackToRun;
		nextCallbackToRun                  = NULL;
		tempCallback(callbackContext);
	}

	// In case no BSP_ExternalFlash* functions were called in the scheduled callback.
	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/**
 * Set the /CS high, which deselects the external flash chip.
*/
static void W25Q80DLSNIG_CSHigh()
{
	// Chip not selected
	EXTERNAL_SPI_FLASH_CS_PVAL = 1;
}

/**
 * Function to be called from the BSP when an SPI exchange operation has been completed.
 * This function deselects the external flash chip and sets up the SPI to be used
 * for communication with the RF chip.
*/
static void W25Q80DLSNIG_SPIComplete()
{
	// End access to the external flash
	W25Q80DLSNIG_CSHigh();

	// Change the SPI mode to 2
	SPI_Close(SPI1);
	SPI_Open(SPI, SPI_MASTER, SPI_MODE_2, 8, FCLK_SPI);

	spiInProgress = false;
	SPI_SetExternallyInUseStatus(false, NULL);
}

/**
 * Set the /CS low, which selects the external flash chip.
 * This function also sets up the SPI to be used for communication
 * with the external flash chip.
 *
 * @return Status of the function.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    Aborted early, SPI is currently being used to communicate with the RF chip.
*/
static ca_error W25Q80DLSNIG_CSLow()
{
	if (SPI_SetExternallyInUseStatus(true, &W25Q80DLSNIG_SPIComplete))
		return CA_ERROR_BUSY;

	spiInProgress = true;

	// Change the SPI mode to 3
	SPI_Close(SPI1);
	SPI_Open(SPI, SPI_MASTER, SPI_MODE_3, 8, FCLK_SPI);

	// Chip selected
	EXTERNAL_SPI_FLASH_CS_PVAL = 0;

	return CA_ERROR_SUCCESS;
}

/**
 * Send an instruction to the external flash chip over SPI, receiving into RxBuf and transferring from TxBuf.
 *
 * @param RxBuf Buffer to fill with received bytes. Must be at least RxLen big.
 * @param TxBuf Buffer to send bytes from. Must be at least TxLen big.
 * @param RxLen Count of bytes to receive.
 * @param TxLen Count of bytes to transmit.
 *
 * @return Status of the function.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    Aborted early, SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_SendSPIInstruction(uint8_t *RxBuf, const uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen)
{
	//Select the chip
	if (W25Q80DLSNIG_CSLow())
		return CA_ERROR_BUSY;

	//Send instruction
	BSP_SPIExchange(RxBuf, TxBuf, RxLen, TxLen);
	//Wait until instruction is over
	while (spiInProgress)
		;

	return CA_ERROR_SUCCESS;
}

/**
 * Set the Write Enable Latch bit in the Status Register to a 1.
 * The WEL bit must be set prior to every Page Program, Sector Erase,
 * Block Erase, Chip Erase and Write Status Register instruction.
 * The WEL bit is reset to 0 upon completion of every aforementioned instruction.
 * (Function used internally by BSP_ExternalFlashChipErase, BSP_ExternalFlashPartialErase and BSP_ExternalFlashProgram)
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
ca_error W25Q80DLSNIG_WriteEnable()
{
	uint8_t txbuf[1] = {WRITE_ENABLE};
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	return W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);
}

/**
 * Set the Write Enable Latch bit in the Status Register to a 0.
 * (Function used for testing purposes).
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
ca_error W25Q80DLSNIG_WriteDisable()
{
	uint8_t txbuf[1] = {WRITE_DISABLE};
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	return W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);
}

/**
 * Read the 8-bit Status Register 1. (Function used internally by BSP_ExternalFlashGetStatus)
 *
 * @param statusRegister1 Stores the value of Status Register 1 that was read.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
ca_error W25Q80DLSNIG_ReadStatusRegister1(uint8_t *statusRegister1)
{
	uint8_t txbuf[2] = {READ_STATUS_REGISTER_1, DONT_CARE_BYTE};
	uint8_t txbufLen = sizeof(txbuf);
	uint8_t rxbufLen = txbufLen;

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(rxbuf, txbuf, rxbufLen, txbufLen);

	*statusRegister1 = rxbuf[1];

	return status;
}

/**
 * Read (IN THEORY) as many data bytes as needed from the memory.
 * In this implementation, however, a maximum of 128 bytes of data can be read.
 *
 * @param startAddress The start address of the read data instruction.
 * @param numOfBytes   The number of bytes to be sequentially read, starting from startAddress.
 * @param rxData       Stores the data read from the external flash.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_ReadData(uint32_t startAddress, uint8_t numOfBytes, uint8_t *rxData)
{
	uint8_t txbuf[READ_DATA_MAX_RX_LEN] = {DONT_CARE_BYTE};

	uint8_t addressByte1 = startAddress >> 16;
	uint8_t addressByte2 = (startAddress >> 8) & 0xFF;
	uint8_t addressByte3 = startAddress & 0xFF;

	txbuf[0] = READ_DATA;
	txbuf[1] = addressByte1;
	txbuf[2] = addressByte2;
	txbuf[3] = addressByte3;

	//Number of bytes that will actually be transmitted
	uint8_t txbufLen = numOfBytes + INSTRUCTION_BYTE_LENGTH + ADDRESS_BYTE_LENGTH;
	uint8_t rxbufLen = txbufLen;

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(rxbuf, txbuf, rxbufLen, txbufLen);

	memcpy(rxData, &rxbuf[4], numOfBytes);

	return status;
}

/**
 * Program up to (IN THEORY) 255 bytes of data at previously erased (0xFF) memory locations.
 * In this implementation, however, a maximum of 128 bytes of data can be programmed.
 * Can only program memory within the page that startAddress is contained in.
 * This means that if numOfBytes is greater than the number of bytes left
 * in the current page based on startAddress, then the addresses will wrap around
 * to the beginning of the current page and memory will be overwritten.
 * However, the BSP function which calls this function stops that from happening
 * by checking the validity of numOfBytes and startAddress.
 *
 * @param startAddress The start address of the page program instruction.
 * @param numOfBytes   The number of bytes to be sequentially programmed, starting from startAddress.
 * @param programData  The data to program.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_PageProgram(uint32_t startAddress, uint8_t numOfBytes, uint8_t *programData)
{
	uint8_t txbuf[PAGE_PROGRAM_MAX_TX_LEN];

	uint8_t addressByte1 = startAddress >> 16;
	uint8_t addressByte2 = (startAddress >> 8) & 0xFF;
	uint8_t addressByte3 = startAddress & 0xFF;

	txbuf[0] = PAGE_PROGRAM;
	txbuf[1] = addressByte1;
	txbuf[2] = addressByte2;
	txbuf[3] = addressByte3;
	memcpy(&txbuf[4], programData, numOfBytes);

	uint8_t txbufLen = numOfBytes + INSTRUCTION_BYTE_LENGTH + ADDRESS_BYTE_LENGTH;

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);

	cycleInProgress = true;

	TASKLET_ScheduleDelta(&postInstructionWaitTasklet, T_PP, NULL);

	return status;
}

/**
 * Erase (i.e. set to all 1s) all the memory of the external flash chip.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_ChipErase()
{
	uint8_t txbuf[1] = {CHIP_ERASE};
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);

	cycleInProgress = true;

	TASKLET_ScheduleDelta(&postInstructionWaitTasklet, T_CE, NULL);

	return status;
}

/**
 * Erase (i.e. set to all 1s) all memory within a specified sector of 4KB.
 *
 * @param sectorAddress The address contained in the 4KB sector that will be erased.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_SectorErase(uint32_t sectorAddress)
{
	uint8_t txbuf[4];

	uint8_t addressByte1 = sectorAddress >> 16;
	uint8_t addressByte2 = (sectorAddress >> 8) & 0xFF;
	uint8_t addressByte3 = sectorAddress & 0xFF;

	txbuf[0]         = SECTOR_ERASE_4KB;
	txbuf[1]         = addressByte1;
	txbuf[2]         = addressByte2;
	txbuf[3]         = addressByte3;
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);

	cycleInProgress = true;

	TASKLET_ScheduleDelta(&postInstructionWaitTasklet, T_SE, NULL);

	return status;
}

/**
 * Erase (i.e. set to all 1s) all memory within a specified block of 32KB.
 *
 * @param blockAddress The address contained in the 32KB block that will be erased.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_BlockErase32KB(uint32_t blockAddress)
{
	uint8_t txbuf[4];

	uint8_t addressByte1 = blockAddress >> 16;
	uint8_t addressByte2 = (blockAddress >> 8) & 0xFF;
	uint8_t addressByte3 = blockAddress & 0xFF;

	txbuf[0]         = BLOCK_ERASE_32KB;
	txbuf[1]         = addressByte1;
	txbuf[2]         = addressByte2;
	txbuf[3]         = addressByte3;
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);

	cycleInProgress = true;

	TASKLET_ScheduleDelta(&postInstructionWaitTasklet, T_BE1, NULL);

	return status;
}

/**
 * Erase (i.e. set to all 1s) all memory within a specified block of 64KB.
 *
 * @param blockAddress The address contained in the 32KB block that will be erased.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_BlockErase64KB(uint32_t blockAddress)
{
	uint8_t txbuf[4];

	uint8_t addressByte1 = blockAddress >> 16;
	uint8_t addressByte2 = (blockAddress >> 8) & 0xFF;
	uint8_t addressByte3 = blockAddress & 0xFF;

	txbuf[0]         = BLOCK_ERASE_64KB;
	txbuf[1]         = addressByte1;
	txbuf[2]         = addressByte2;
	txbuf[3]         = addressByte3;
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);

	cycleInProgress = true;

	TASKLET_ScheduleDelta(&postInstructionWaitTasklet, T_BE2, NULL);

	return status;
}

/**
 * Put the external flash chip in power down mode.
 * When in power down mode, the external flash chip consumes less power and
 * will ignore all instructions except for RELEASE_POWERDOWN_ID.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
ca_error W25Q80DLSNIG_PowerDown()
{
	uint8_t txbuf[1] = {POWER_DOWN};
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);

	BSP_WaitUs(T_DP);

	return status;
}

/**
 * Get the ID of the external flash chip.
 *
 * @param id Stores the ID of the external flash chip.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
static ca_error W25Q80DLSNIG_GetDeviceId(uint8_t *id)
{
	uint8_t txbuf[5] = {RELEASE_POWERDOWN_ID, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DONT_CARE_BYTE};
	uint8_t txbufLen = sizeof(txbuf);
	uint8_t rxbufLen = txbufLen;

	//Send instruction
	ca_error status = W25Q80DLSNIG_SendSPIInstruction(rxbuf, txbuf, rxbufLen, txbufLen);

	*id = rxbuf[4];

	return status;
}

/**
 * Release the external flash chip from power down mode.
 * This is a multi-purpose instruction, because it can also
 * get the ID of the device at the same time.
 *
 * @return Status of the command.
 * @retval CA_ERROR_SUCCESS Success.
 * @retval CA_ERROR_BUSY    SPI is currently being used to communicate with the RF chip.
 */
ca_error W25Q80DLSNIG_ReleasePowerDown(void)
{
	ca_error status;

	uint8_t txbuf[1] = {RELEASE_POWERDOWN_ID};
	uint8_t txbufLen = sizeof(txbuf);

	//Send instruction
	status = W25Q80DLSNIG_SendSPIInstruction(NULL, txbuf, 0, txbufLen);

	BSP_WaitUs(T_RES1);

	return status;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_ExternalFlashInit(void)
{
	ca_log_note("External Flash initialised.");
	TASKLET_Init(&postInstructionWaitTasklet, &handlePostInstructionWaiting);
	TASKLET_Init(&callbackSchedulingTasklet, &external_flash_schedule);

	W25Q80DLSNIG_CSHigh();
	memset(rxbuf, 0xFF, sizeof(rxbuf));

	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ExternalFlashProgram(uint32_t aStartAddress, uint8_t aNumOfBytes, uint8_t *aTxData)
{
	if (!isCalledWithinCallback)
		return CA_ERROR_NO_ACCESS;

	if (aStartAddress > 0xFFFFF || aNumOfBytes == 0 || aNumOfBytes > PAGE_PROGRAM_MAX_BYTES_ACTUAL ||
	    pageOverflow(aStartAddress, aNumOfBytes))
		return CA_ERROR_INVALID_ARGS;

	while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_WriteEnable() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PageProgram(aStartAddress, aNumOfBytes, aTxData) != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;

	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ExternalFlashReadData(uint32_t aStartAddress, uint8_t aNumOfBytes, uint8_t *aRxData)
{
	if (!isCalledWithinCallback)
		return CA_ERROR_NO_ACCESS;

	if ((int)aStartAddress > (0x100000 - aNumOfBytes) || aNumOfBytes == 0 || aNumOfBytes > READ_DATA_MAX_BYTES_ACTUAL)
		return CA_ERROR_INVALID_ARGS;

	while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_ReadData(aStartAddress, aNumOfBytes, aRxData) != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;

	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ExternalFlashPartialErase(ExternalFlashPartialEraseType aEraseType, uint32_t aAddress)
{
	if (!isCalledWithinCallback)
		return CA_ERROR_NO_ACCESS;

	if (aAddress > 0xFFFFF)
		return CA_ERROR_INVALID_ARGS;

	while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
		;

	while (W25Q80DLSNIG_WriteEnable() != CA_ERROR_SUCCESS)
		;

	if (aEraseType == SECTOR_4KB)
		while (W25Q80DLSNIG_SectorErase(aAddress) != CA_ERROR_SUCCESS)
			;
	else if (aEraseType == BLOCK_32KB)
		while (W25Q80DLSNIG_BlockErase32KB(aAddress) != CA_ERROR_SUCCESS)
			;
	else if (aEraseType == BLOCK_64KB)
		while (W25Q80DLSNIG_BlockErase64KB(aAddress) != CA_ERROR_SUCCESS)
			;

	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;

	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ExternalFlashChipErase(void)
{
	if (!isCalledWithinCallback)
		return CA_ERROR_NO_ACCESS;

	while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_WriteEnable() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_ChipErase() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;

	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ExternalFlashGetDeviceId(uint8_t *aId)
{
	if (!isCalledWithinCallback)
		return CA_ERROR_NO_ACCESS;

	while (W25Q80DLSNIG_GetDeviceId(aId) != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;
	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ExternalFlashGetStatus(struct ExternalFlashStatus *status)
{
	if (!isCalledWithinCallback)
		return CA_ERROR_NO_ACCESS;

	uint8_t statusRegister;

	while (W25Q80DLSNIG_ReleasePowerDown() != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_ReadStatusRegister1(&statusRegister) != CA_ERROR_SUCCESS)
		;
	while (W25Q80DLSNIG_PowerDown() != CA_ERROR_SUCCESS)
		;

	status->writeEnabled = (statusRegister & WEL_mask) >> WEL_pos;

	isCalledWithinCallback = false;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ExternalFlashScheduleCallback(ExternalFlashCallback aCallback, void *aContext)
{
	if (nextCallbackToRun)
		return CA_ERROR_ALREADY;

	nextCallbackToRun = aCallback;
	callbackContext   = aContext;

	if (!cycleInProgress)
		return TASKLET_ScheduleDelta(&callbackSchedulingTasklet, 0, NULL);

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_ExternalFlashGetInfo(struct ExternalFlashInfo *aFlashInfoOut)
{
	memcpy(aFlashInfoOut, &BSP_ExternalFlashInfo, sizeof(BSP_ExternalFlashInfo));
}
