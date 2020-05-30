/**
 * @file
 *//*
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
/*
 * Application for E-ink display of images.
*/
#include <stdio.h>
#include <stdlib.h>

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "qrcodegen.h"
#include "sif_eink.h"

/* EPD commands */
#define DRIVER_OUTPUT_CONTROL 0x01
#define BOOSTER_SOFT_START_CONTROL 0x0C
#define DEEP_SLEEP_MODE 0x10
#define DATA_ENTRY_MODE_SETTING 0x11
#define SW_RESET 0x12
#define TEMPERATURE_SENSOR_CONTROL 0x1A
#define MASTER_ACTIVATION 0x20
#define DISPLAY_UPDATE_CONTROL_1 0x21
#define DISPLAY_UPDATE_CONTROL_2 0x22
#define WRITE_RAM 0x24
#define WRITE_VCOM_REGISTER 0x2C
#define WRITE_LUT_REGISTER 0x32
#define SET_DUMMY_LINE_PERIOD 0x3A
#define SET_GATE_LINE_WIDTH 0x3B
#define BORDER_WAVEFORM_CONTROL 0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION 0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION 0X45
#define SET_RAM_X_ADDRESS_COUNTER 0x4E
#define SET_RAM_Y_ADDRESS_COUNTER 0X4F
#define TERMINATE_FRAME_READ_WRITE 0xFF

/***************************************************************************/
/* Look Up Table values copied from the example code provided by WaveShare */
/***************************************************************************/
const struct EINK_lut lut_full_update = {{0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
                                          0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
                                          0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00}};

const struct EINK_lut lut_partial_update = {{0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends a command using SPI
 * \param command_byte - The command byte to send
 *******************************************************************************
 ******************************************************************************/
static void EINK_SendCommand(uint8_t command_byte)
{
	ca_error error = CA_ERROR_SUCCESS;
	BSP_ModuleSetGPIOPin(EINK_DC_PIN, 0);
	do
	{
		error = SENSORIF_SPI_Write(command_byte);
	} while (error != CA_ERROR_SUCCESS);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends a data byte using SPI
 * \param data_byte - The data byte to send
 *******************************************************************************
 ******************************************************************************/
static void EINK_SendData(uint8_t data_byte)
{
	ca_error error = CA_ERROR_SUCCESS;
	BSP_ModuleSetGPIOPin(EINK_DC_PIN, 1);
	do
	{
		error = SENSORIF_SPI_Write(data_byte);
	} while (error != CA_ERROR_SUCCESS);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets the display window
 * \param Xstart - Start position of the window address in the X direction.
 * \param Xend   - End position of the window address in the X direction.
 * \param Ystart - Start position of the window address in the Y direction.
 * \param Yend   - End position of the window address in the Y direction.
 *******************************************************************************
 ******************************************************************************/
static void EINK_SetWindow(u16_t Xstart, u16_t Xend, u16_t Ystart, u16_t Yend)
{
	EINK_SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
	EINK_SendData((Xstart >> 3) & 0xFF);
	EINK_SendData((Xend >> 3) & 0xFF);

	EINK_SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
	EINK_SendData(Ystart & 0xFF);
	EINK_SendData((Ystart >> 8) & 0xFF);
	EINK_SendData(Yend & 0xFF);
	EINK_SendData((Yend >> 8) & 0xFF);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets the cursor
 * \param Xstart - Initial settings for the RAM X address in the address counter.
 * \param Ystart - Initial settings for the RAM Y address in the address counter.
 *******************************************************************************
 ******************************************************************************/
static void EINK_SetCursor(u16_t Xstart, u16_t Ystart)
{
	EINK_SendCommand(SET_RAM_X_ADDRESS_COUNTER);
	EINK_SendData((Xstart >> 3) & 0xFF);

	EINK_SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
	EINK_SendData(Ystart & 0xFF);
	EINK_SendData((Ystart >> 8) & 0xFF);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wait until BUSY pin is LOW
 *******************************************************************************
 ******************************************************************************/
static void EINK_WaitUntilIdle(void)
{
	u8_t BUSY_value = 0;
	BSP_ModuleSenseGPIOPin(EINK_BUSY_PIN, &BUSY_value);
	while (BUSY_value == 1)
	{
		WAIT_ms(2);
		BSP_ModuleSenseGPIOPin(EINK_BUSY_PIN, &BUSY_value);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Turns on the display
 *******************************************************************************
 ******************************************************************************/
static void EINK_TurnOnDisplay(void)
{
	EINK_SendCommand(DISPLAY_UPDATE_CONTROL_2);
	EINK_SendData(0xC4);
	EINK_SendCommand(MASTER_ACTIVATION);
	EINK_SendCommand(TERMINATE_FRAME_READ_WRITE);

	EINK_WaitUntilIdle();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Resets Eink Panel
 *******************************************************************************
 ******************************************************************************/
static void EINK_Reset(void)
{
	BSP_ModuleSetGPIOPin(EINK_RST_PIN, 1);
	WAIT_ms(2);
	BSP_ModuleSetGPIOPin(EINK_RST_PIN, 0);
	WAIT_ms(2);
	BSP_ModuleSetGPIOPin(EINK_RST_PIN, 1);
	WAIT_ms(2);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief This function replaces the appropriate bits of the image with the bits
 *        of the QR code based on the specified x and y coordinates.
 *******************************************************************************
 * \param qrcode - The array containing the QR code data.
 * \param image  - The image array that will be modified.
 * \param x_pos  - The x-coordinate of the top-left corner of the QR symbol.
 * \param y_pos  - The y-coordinate of the top-left corner of the QR symbol.
 *******************************************************************************
 ******************************************************************************/
static void EINK_embed_qr(const uint8_t *qrcode, uint8_t *image, uint8_t x_pos, uint8_t y_pos)
{
	int size = qrcodegen_getSize(qrcode);

	for (int y = y_pos; y < y_pos + size; y++)
	{
		for (int x = x_pos; x < x_pos + size; x++)
		{
			if (!qrcodegen_getModule(qrcode, x - x_pos, y - y_pos))
				image[(x + (y * EINK_WIDTH)) / 8] |= (1 << (7 - (x % 8)));
			else
				image[(x + (y * EINK_WIDTH)) / 8] &= ~(1 << (7 - (x % 8)));
		}
	}
}

ca_error EINK_Initialise(const struct EINK_lut *lut)
{
	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 1);

	/*****************************************/
	/*** Configure GPIO input and outputs  ***/
	/*****************************************/
	/* BUSY - Pin 31 */
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
	    EINK_BUSY_PIN, MODULE_PIN_PULLUP_ON, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_OFF, NULL});
	/* RST - Pin 15 */
	BSP_ModuleRegisterGPIOOutput(EINK_RST_PIN, MODULE_PIN_TYPE_GENERIC);
	/* D/C - Pin 5 */
	BSP_ModuleRegisterGPIOOutput(EINK_DC_PIN, MODULE_PIN_TYPE_GENERIC);
	/* CS - Pin 34 */
	//BSP_ModuleRegisterGPIOOutput(EINK_CS_PIN, MODULE_PIN_TYPE_GENERIC);

	/*****************************************/
	/*** Initialise the SPI communication  ***/
	/*****************************************/
	SENSORIF_SPI_Init();

	/*****************************************/
	/*** Set GPIO pins high                ***/
	/*****************************************/
	/* RST - Pin 15 - High */
	BSP_ModuleSetGPIOPin(EINK_RST_PIN, 1);
	/* D/C - Pin 5 - High */
	BSP_ModuleSetGPIOPin(EINK_DC_PIN, 1);
	/* CS - Pin 34 - High */
	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 1);

	/*****************************************/
	/*** Panel Reset                       ***/
	/*****************************************/
	EINK_Reset();

	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 0);

	EINK_SendCommand(DRIVER_OUTPUT_CONTROL);
	EINK_SendData((EINK_HEIGHT - 1) & 0xFF);
	EINK_SendData(((EINK_HEIGHT - 1)) >> 8 & 0xFF);
	EINK_SendData(0x00); //GD = 0; SM = 0; TB = 0;

	EINK_SendCommand(BOOSTER_SOFT_START_CONTROL);
	EINK_SendData(0xD7);
	EINK_SendData(0xD6);
	EINK_SendData(0x9D);

	EINK_SendCommand(WRITE_VCOM_REGISTER);
	EINK_SendData(0xA8);

	EINK_SendCommand(SET_DUMMY_LINE_PERIOD);
	EINK_SendData(0x1A);

	EINK_SendCommand(SET_GATE_LINE_WIDTH);
	EINK_SendData(0x08);

	EINK_SendCommand(BORDER_WAVEFORM_CONTROL);
	EINK_SendData(0x03);

	EINK_SendCommand(DATA_ENTRY_MODE_SETTING);
	EINK_SendData(0x03);

	//Set the LUT register
	EINK_SendCommand(WRITE_LUT_REGISTER);
	for (u8_t i = 0; i < sizeof(*lut); ++i)
	{
		EINK_SendData(lut->lut_array[i]);
	}

	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 1);

	return CA_ERROR_SUCCESS;
}

void EINK_Display(const uint8_t *image)
{
	u16_t width, height;
	width  = (EINK_WIDTH % 8 == 0) ? (EINK_WIDTH / 8) : (EINK_WIDTH / 8 + 1);
	height = EINK_HEIGHT;

	u64_t address = 0;

	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 0);

	EINK_SetWindow(0, EINK_WIDTH, 0, EINK_HEIGHT);
	for (u16_t j = 0; j < height; j++)
	{
		EINK_SetCursor(0, j);
		EINK_SendCommand(WRITE_RAM);
		for (u16_t i = 0; i < width; i++)
		{
			address = i + j * width;
			EINK_SendData(image[address]);
		}
	}

	EINK_TurnOnDisplay();

	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 1);
}

void EINK_ClearDisplay(void)
{
	u16_t width, height;
	width  = (EINK_WIDTH % 8 == 0) ? (EINK_WIDTH / 8) : (EINK_WIDTH / 8 + 1);
	height = EINK_HEIGHT;

	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 0);

	EINK_SetWindow(0, EINK_WIDTH, 0, EINK_HEIGHT);
	for (u16_t j = 0; j < height; j++)
	{
		EINK_SetCursor(0, j);
		EINK_SendCommand(WRITE_RAM);
		for (u16_t i = 0; i < width; i++)
		{
			EINK_SendData(0xFF);
		}
	}
	EINK_TurnOnDisplay();

	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 0);
}

void EINK_StrongClearDisplay(void)
{
	EINK_ClearDisplay();
	WAIT_ms(500);
	EINK_ClearDisplay();
	WAIT_ms(500);
	EINK_ClearDisplay();
	WAIT_ms(500);
}

void EINK_DeepSleep(void)
{
	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 0);
	EINK_SendCommand(DEEP_SLEEP_MODE);
	EINK_SendData(0x01);
	//BSP_ModuleSetGPIOPin(EINK_CS_PIN, 1);
}

ca_error EINK_overlay_qr_code(const char *text, uint8_t *image, uint8_t x, uint8_t y)
{
	enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW; // Error correction level

	// Make the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];
	bool    success = qrcodegen_encodeText(
        text, tempBuffer, qrcode, errCorLvl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	if (success)
	{
		EINK_embed_qr(qrcode, image, x, y);
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_FAIL;
}
