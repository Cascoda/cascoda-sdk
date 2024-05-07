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
 * Library for communicating with the SSD1681 E-Paper display driver.
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
#include "sif_ssd1681.h"

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
#define WRITE_RAM_RED 0x26
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

//waveform full refresh
static const uint8_t SIF_SSD1681_lut_full_update[] = {
    0x80, 0x48, 0x40, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x40, 0x48, 0x80, 0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0, 0x80, 0x48, 0x40, 0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x40, 0x48, 0x80, 0x0,
    0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0xA,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x8,  0x1,  0x0,  0x8, 0x1, 0x0,  0x2,  0xA,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0,  0x22, 0x17, 0x41, 0x0,  0x32, 0x20};

// waveform partial refresh (fast)
static const uint8_t SIF_SSD1681_lut_partial_update[] = {
    0x0, 0x40, 0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x80, 0x80, 0x0,  0x0,  0x0,  0x0,  0x0, 0x0,
    0x0, 0x0,  0x0, 0x0, 0x40, 0x40, 0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x80, 0x0, 0x0,
    0x0, 0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,
    0xF, 0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x1,  0x1,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,
    0x0, 0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,
    0x0, 0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,
    0x0, 0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,
    0x0, 0x0,  0x0, 0x0, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x00, 0x02, 0x17, 0x41, 0xB0, 0x32, 0x28};

// Used so that the first partial update is done with a clear
static bool g_is_asleep = true;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends a command using SPI
 * \param command_byte - The command byte to send
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_SendCommand(uint8_t command_byte)
{
	ca_error error = CA_ERROR_SUCCESS;
	BSP_ModuleSetGPIOPin(SIF_SSD1681_DC_PIN, 0);
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
static void SIF_SSD1681_SendData(uint8_t data_byte)
{
	ca_error error = CA_ERROR_SUCCESS;
	BSP_ModuleSetGPIOPin(SIF_SSD1681_DC_PIN, 1);
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
static void SIF_SSD1681_SetWindow(u16_t Xstart, u16_t Xend, u16_t Ystart, u16_t Yend)
{
	SIF_SSD1681_SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
	SIF_SSD1681_SendData((Xstart >> 3) & 0xFF);
	SIF_SSD1681_SendData((Xend >> 3) & 0xFF);

	SIF_SSD1681_SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
	SIF_SSD1681_SendData(Ystart & 0xFF);
	SIF_SSD1681_SendData((Ystart >> 8) & 0xFF);
	SIF_SSD1681_SendData(Yend & 0xFF);
	SIF_SSD1681_SendData((Yend >> 8) & 0xFF);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wait until BUSY pin is LOW
 *******************************************************************************
 ******************************************************************************/
static ca_error SIF_SSD1681_WaitUntilIdle(void)
{
	u32_t t_tstart   = TIME_ReadAbsoluteTime();
	u8_t  BUSY_value = 0;

	BSP_ModuleSenseGPIOPin(SIF_SSD1681_BUSY_PIN, &BUSY_value);
	while (BUSY_value == 1)
	{
		if ((TIME_ReadAbsoluteTime() - t_tstart) > SIF_SSD1681_BUSY_TIMEOUT)
			return CA_ERROR_TIMEOUT;
		WAIT_ms(2);
		BSP_ModuleSenseGPIOPin(SIF_SSD1681_BUSY_PIN, &BUSY_value);
	}

	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets the cursor
 * \param Xstart - Initial settings for the RAM X address in the address counter.
 * \param Ystart - Initial settings for the RAM Y address in the address counter.
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_SetCursor(u16_t Xstart, u16_t Ystart)
{
	SIF_SSD1681_SendCommand(SET_RAM_X_ADDRESS_COUNTER);
	SIF_SSD1681_SendData((Xstart >> 3) & 0xFF);

	SIF_SSD1681_SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
	SIF_SSD1681_SendData(Ystart & 0xFF);
	SIF_SSD1681_WaitUntilIdle();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Turns on the display for partial update
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_TurnOnDisplay(SIF_SSD1681_Update_Mode mode)
{
	SIF_SSD1681_SendCommand(DISPLAY_UPDATE_CONTROL_2);

	if (mode == FULL_UPDATE)
		SIF_SSD1681_SendData(0xC7);
	else if (mode == PARTIAL_UPDATE)
		SIF_SSD1681_SendData(0xCF);

	SIF_SSD1681_SendCommand(MASTER_ACTIVATION);
	SIF_SSD1681_WaitUntilIdle();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Resets Eink Panel
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_Reset(void)
{
	BSP_ModuleSetGPIOPin(SIF_SSD1681_RST_PIN, 1);
	WAIT_ms(50); // Originally WAIT_ms(200)
	BSP_ModuleSetGPIOPin(SIF_SSD1681_RST_PIN, 0);
	WAIT_ms(2);
	BSP_ModuleSetGPIOPin(SIF_SSD1681_RST_PIN, 1);
	WAIT_ms(50); // Originally WAIT_ms(200)
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief This function replaces the appropriate bits of the image with the bits
 *        of the QR code based on the specified x and y coordinates.
 *******************************************************************************
 * \param qrcode - The array containing the QR code data.
 * \param image  - The image array that will be modified.
 * \param scale  - The image scaling, currently supported 1 & 2.
 * \param x_pos  - The x-coordinate of the top-left corner of the QR symbol.
 * \param y_pos  - The y-coordinate of the top-left corner of the QR symbol.
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_embed_qr(const uint8_t *qrcode, uint8_t *image, uint8_t scale, uint8_t x_pos, uint8_t y_pos)
{
	int size        = qrcodegen_getSize(qrcode);
	int size_scaled = size * scale;

	for (int y = y_pos; y < y_pos + size_scaled; y++)
	{
		for (int x = x_pos; x < x_pos + size_scaled; x++)
		{
			if (!qrcodegen_getModule(qrcode, (x - x_pos) / scale, (y - y_pos) / scale))
				image[(x + (y * SIF_SSD1681_WIDTH)) / 8] |= (1 << (7 - (x % 8)));
			else
				image[(x + (y * SIF_SSD1681_WIDTH)) / 8] &= ~(1 << (7 - (x % 8)));
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief This function sets the Lookup table (LUT) register in the display
 *******************************************************************************
 * \param mode - Determines whether the lookup table for FULL_UPDATE or
 * PARTIAL_UPDATE is used.
 *******************************************************************************
 ******************************************************************************/
static ca_error SIF_SSD1681_SetLut(SIF_SSD1681_Update_Mode mode)
{
	ca_error err = CA_ERROR_SUCCESS;

	SIF_SSD1681_SendCommand(WRITE_LUT_REGISTER);
	if (mode == FULL_UPDATE)
	{
		for (u8_t i = 0; i < 153; ++i)
		{
			SIF_SSD1681_SendData(SIF_SSD1681_lut_full_update[i]);
		}
		err = SIF_SSD1681_WaitUntilIdle();
		if (err)
			return err;
		SIF_SSD1681_SendCommand(0x3f);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_full_update[153]);

		SIF_SSD1681_SendCommand(0x03);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_full_update[154]);

		SIF_SSD1681_SendCommand(0x04);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_full_update[155]);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_full_update[156]);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_full_update[157]);

		SIF_SSD1681_SendCommand(0x2c);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_full_update[158]);
	}
	else if (mode == PARTIAL_UPDATE)
	{
		for (u8_t i = 0; i < 153; ++i)
		{
			SIF_SSD1681_SendData(SIF_SSD1681_lut_partial_update[i]);
		}
		err = SIF_SSD1681_WaitUntilIdle();
		if (err)
			return err;
		SIF_SSD1681_SendCommand(0x3f);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_partial_update[153]);

		SIF_SSD1681_SendCommand(0x03);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_partial_update[154]);

		SIF_SSD1681_SendCommand(0x04);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_partial_update[155]);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_partial_update[156]);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_partial_update[157]);

		SIF_SSD1681_SendCommand(0x2c);
		SIF_SSD1681_SendData(SIF_SSD1681_lut_partial_update[158]);
	}
	else
	{
		err = CA_ERROR_INVALID_ARGS;
		ca_log_debg("Error, invalid display update mode");
	}

	return err;
}

#ifndef EPAPER_FULL_RESOLUTION
/******************************************************************************/
/***************************************************************************/ /**
 * \brief This function will transform an input byte into an output buffer
 * containing two bytes, in the following manner:
 * Every bit of the input byte is doubled, e.g.
 * (0xC3) 1  1  0  0  0  0  1  1
 * Turns into
 *        11 11 00 00 00 00 11 11
 * Which is 11110000 00001111
 * Result: 0xC3 gets transformed into {0xF0, 0x0F}
 *******************************************************************************
 * \param[in] input_byte - The input byte that will be transformed by this function.
 * \param[out] output_buf - Two-byte output generated from input transformation.
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_Transform(uint8_t input_byte, uint8_t output_buf[2])
{
	int8_t       offset0         = 0;
	int8_t       offset1         = 0;
	const int8_t midpoint_offset = 4;

	output_buf[0] = 0;
	output_buf[1] = 0;

	for (int8_t i = 7; i >= 0; --i)
	{
		u8_t bit_val = ((input_byte >> i) & 0x01);

		if (i > 3)
		{
			if (bit_val == 0)
			{
				--offset0;
				continue;
			}

			output_buf[0] += (1 << (i + offset0));
			output_buf[0] += (1 << (i + (--offset0)));
		}
		else
		{
			if (bit_val == 0)
			{
				--offset1;
				continue;
			}

			output_buf[1] += (1 << (i + midpoint_offset + offset1));
			output_buf[1] += (1 << (i + midpoint_offset + (--offset1)));
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Actually copies the image into the eink's RAM.
 * Half resolution.
 *******************************************************************************
 * \param[in] image - The image that is to be copied.
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_DisplayHalfRes(const uint8_t *image)
{
	u16_t width  = (SIF_SSD1681_WIDTH % 8 == 0) ? (SIF_SSD1681_WIDTH / 8) : (SIF_SSD1681_WIDTH / 8 + 1);
	u16_t height = SIF_SSD1681_HEIGHT;

	u64_t address  = 0;
	u16_t offset_y = 0;

	u8_t transformation_result[2] = {0x00};

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 0);

	for (u16_t j = 0; j < height; ++j)
	{
		// Write jth line of image to the next line in eink RAM.
		SIF_SSD1681_SetCursor(0, j + offset_y);
		SIF_SSD1681_SendCommand(WRITE_RAM);
		for (u16_t i = 0; i < width; i++)
		{
			address = i + j * width;

			// Generates two bytes from ith byte in image
			SIF_SSD1681_Transform(image[address], transformation_result);

			// Writes those two generated bytes to eink RAM.
			SIF_SSD1681_SendData(transformation_result[0]);
			SIF_SSD1681_SendData(transformation_result[1]);
		}

		// Write the same jth line of image to the next line in eink RAM.
		SIF_SSD1681_SetCursor(0, j + (++offset_y));
		SIF_SSD1681_SendCommand(WRITE_RAM);
		for (u16_t i = 0; i < width; i++)
		{
			address = i + j * width;
			SIF_SSD1681_Transform(image[address], transformation_result);

			SIF_SSD1681_SendData(transformation_result[0]);
			SIF_SSD1681_SendData(transformation_result[1]);
		}
	}

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 1);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Clears the right edge of the screen which is not being used at this
 * resolution.
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_ClearRightEdge(void)
{
#define SIF_SSD1681_RIGHT_EDGE_WIDTH (SIF_SSD1681_WIDTH_PHYSICAL - SIF_SSD1681_WIDTH_WINDOW)
#define SIF_SSD1681_RIGHT_EDGE_HEIGHT (SIF_SSD1681_HEIGHT_PHYSICAL)
	u16_t width  = (SIF_SSD1681_RIGHT_EDGE_WIDTH % 8 == 0) ? (SIF_SSD1681_RIGHT_EDGE_WIDTH / 8)
	                                                       : (SIF_SSD1681_RIGHT_EDGE_WIDTH / 8 + 1);
	u16_t height = SIF_SSD1681_RIGHT_EDGE_HEIGHT;

	for (int j = 0; j < height; j++)
	{
		SIF_SSD1681_SetCursor(SIF_SSD1681_WIDTH_WINDOW, j);
		SIF_SSD1681_SendCommand(WRITE_RAM);
		for (int i = 0; i < width; i++)
		{
			SIF_SSD1681_SendData(0xff);
		}
	}

	for (int j = 0; j < height; j++)
	{
		SIF_SSD1681_SetCursor(SIF_SSD1681_WIDTH_WINDOW, j);
		SIF_SSD1681_SendCommand(WRITE_RAM_RED);
		for (int i = 0; i < width; i++)
		{
			SIF_SSD1681_SendData(0xff);
		}
	}

#undef SIF_SSD1681_RIGHT_EDGE_WIDTH
#undef SIF_SSD1681_RIGHT_EDGE_HEIGHT
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Clears the bottom edge of the screen which is not being used at this
 * resolution.
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_ClearBottomEdge(void)
{
#define SIF_SSD1681_BOTTOM_EDGE_WIDTH (SIF_SSD1681_WIDTH_PHYSICAL)
#define SIF_SSD1681_BOTTOM_EDGE_HEIGHT (SIF_SSD1681_HEIGHT_PHYSICAL - SIF_SSD1681_WIDTH_WINDOW)
	u16_t width  = (SIF_SSD1681_BOTTOM_EDGE_WIDTH % 8 == 0) ? (SIF_SSD1681_BOTTOM_EDGE_WIDTH / 8)
	                                                        : (SIF_SSD1681_BOTTOM_EDGE_WIDTH / 8 + 1);
	u16_t height = SIF_SSD1681_BOTTOM_EDGE_HEIGHT;

	for (int j = 0; j < height; j++)
	{
		SIF_SSD1681_SetCursor(0, j + SIF_SSD1681_WIDTH_WINDOW);
		SIF_SSD1681_SendCommand(WRITE_RAM);
		for (int i = 0; i < width; i++)
		{
			SIF_SSD1681_SendData(0xff);
		}
	}

	for (int j = 0; j < height; j++)
	{
		SIF_SSD1681_SetCursor(0, j + SIF_SSD1681_WIDTH_WINDOW);
		SIF_SSD1681_SendCommand(WRITE_RAM_RED);
		for (int i = 0; i < width; i++)
		{
			SIF_SSD1681_SendData(0xff);
		}
	}

#undef SIF_SSD1681_BOTTOM_EDGE_WIDTH
#undef SIF_SSD1681_BOTTOM_EDGE_HEIGHT
}
#endif // EPAPER_FULL_RESOLUTION

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Actually copies the image into the eink's RAM. In full resolution.
 *******************************************************************************
 * \param[in] image - The image that is to be copied.
 *******************************************************************************
 ******************************************************************************/
static void SIF_SSD1681_DisplayFullRes(const uint8_t *image)
{
	u16_t width, height;
	width =
	    (SIF_SSD1681_WIDTH_PHYSICAL % 8 == 0) ? (SIF_SSD1681_WIDTH_PHYSICAL / 8) : (SIF_SSD1681_WIDTH_PHYSICAL / 8 + 1);
	height = SIF_SSD1681_HEIGHT_PHYSICAL;

	u64_t address = 0;

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 0);

	for (u16_t j = 0; j < height; j++)
	{
		for (u16_t i = 0; i < width; i++)
		{
			address = i + j * width;
			SIF_SSD1681_SendData(image[address]);
		}
	}

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 1);
}

static ca_error SIF_SSD1681_WakeUpAndSetConfig(void)
{
	ca_error status;

	/*****************************************/
	/*** Panel Reset                       ***/
	/*****************************************/
	SIF_SSD1681_Reset();

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 0);

	status = SIF_SSD1681_WaitUntilIdle();
	if (status)
		return status;
	SIF_SSD1681_SendCommand(SW_RESET);
	status = SIF_SSD1681_WaitUntilIdle();
	if (status)
		return status;

	SIF_SSD1681_SendCommand(DRIVER_OUTPUT_CONTROL);
	SIF_SSD1681_SendData(0xC7);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);

	SIF_SSD1681_SendCommand(DATA_ENTRY_MODE_SETTING);
	SIF_SSD1681_SendData(0x03);

	SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_WINDOW - 1, 0, SIF_SSD1681_HEIGHT_WINDOW - 1);

	SIF_SSD1681_SendCommand(BORDER_WAVEFORM_CONTROL);
	SIF_SSD1681_SendData(0x01);

	SIF_SSD1681_SendCommand(0x18);
	SIF_SSD1681_SendData(0x80);

	SIF_SSD1681_SendCommand(DISPLAY_UPDATE_CONTROL_2);
	SIF_SSD1681_SendData(0xB1);
	SIF_SSD1681_SendCommand(MASTER_ACTIVATION);

	SIF_SSD1681_SendCommand(SET_RAM_X_ADDRESS_COUNTER);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
	SIF_SSD1681_SendData(0xc7);
	SIF_SSD1681_SendData(0x00);
	status = SIF_SSD1681_WaitUntilIdle();
	if (status)
		return status;

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 1);

	return SIF_SSD1681_SetLut(FULL_UPDATE);
}

bool SIF_SSD1681_IsAsleep(void)
{
	return g_is_asleep;
}

ca_error SIF_SSD1681_Initialise(void)
{
	g_is_asleep = false;
	/*****************************************/
	/*** Configure GPIO input and outputs  ***/
	/*****************************************/
	/* De-register all non-SPI pins first in case they are used */
	BSP_ModuleDeregisterGPIOPin(SIF_SSD1681_RST_PIN);
	BSP_ModuleDeregisterGPIOPin(SIF_SSD1681_DC_PIN);
	BSP_ModuleDeregisterGPIOPin(SIF_SSD1681_BUSY_PIN);
	/* BUSY - Pin 31 */
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
	    SIF_SSD1681_BUSY_PIN, MODULE_PIN_PULLUP_ON, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_OFF, NULL});
	/* RST - Pin 15 */
	BSP_ModuleRegisterGPIOOutput(SIF_SSD1681_RST_PIN, MODULE_PIN_TYPE_GENERIC);
	/* D/C - Pin 34 */
	BSP_ModuleRegisterGPIOOutput(SIF_SSD1681_DC_PIN, MODULE_PIN_TYPE_GENERIC);

	/*****************************************/
	/*** Initialise the SPI communication  ***/
	/*****************************************/
	SENSORIF_SPI_Init(true);

	/*****************************************/
	/*** Set GPIO pins high                ***/
	/*****************************************/
	/* RST - Pin 15 - High */
	BSP_ModuleSetGPIOPin(SIF_SSD1681_RST_PIN, 1);
	/* D/C - Pin 5 - High */
	BSP_ModuleSetGPIOPin(SIF_SSD1681_DC_PIN, 1);
	/* CS - Pin 34 - High */
	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 1);

	return SIF_SSD1681_WakeUpAndSetConfig();
}

void SIF_SSD1681_Deinitialise(void)
{
	/* tristate and pull-up interface pins */

	BSP_ModuleDeregisterGPIOPin(SIF_SSD1681_RST_PIN);
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
	    SIF_SSD1681_RST_PIN, MODULE_PIN_PULLUP_ON, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_OFF, NULL});

	BSP_ModuleDeregisterGPIOPin(SIF_SSD1681_DC_PIN);
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
	    SIF_SSD1681_DC_PIN, MODULE_PIN_PULLUP_ON, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_OFF, NULL});

	BSP_ModuleDeregisterGPIOPin(SIF_SSD1681_BUSY_PIN);
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
	    SIF_SSD1681_BUSY_PIN, MODULE_PIN_PULLUP_ON, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_OFF, NULL});
}

void SIF_SSD1681_ClearDisplay()
{
	u16_t width, height;
	width =
	    (SIF_SSD1681_WIDTH_PHYSICAL % 8 == 0) ? (SIF_SSD1681_WIDTH_PHYSICAL / 8) : (SIF_SSD1681_WIDTH_PHYSICAL / 8 + 1);
	height = SIF_SSD1681_HEIGHT_PHYSICAL;

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 0);

	// Temporarily set the Window to full physical resolution, so that all pixels on the screen get erased.
	SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_PHYSICAL - 1, 0, SIF_SSD1681_HEIGHT_PHYSICAL - 1);

	SIF_SSD1681_SendCommand(WRITE_RAM);
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			SIF_SSD1681_SendData(0xff);
		}
	}
	SIF_SSD1681_SendCommand(WRITE_RAM_RED);
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			SIF_SSD1681_SendData(0xff);
		}
	}
	// Set window resolution back to what it was.
	SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_WINDOW - 1, 0, SIF_SSD1681_HEIGHT_WINDOW - 1);

	SIF_SSD1681_TurnOnDisplay(FULL_UPDATE);

	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 0);
}

void SIF_SSD1681_StrongClearDisplay(void)
{
	SIF_SSD1681_ClearDisplay();
	SIF_SSD1681_ClearDisplay();
	SIF_SSD1681_ClearDisplay();
}

void SIF_SSD1681_DeepSleep(void)
{
	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 0);
	SIF_SSD1681_SendCommand(DEEP_SLEEP_MODE);
	SIF_SSD1681_SendData(0x01);
	g_is_asleep = true;
	//BSP_ModuleSetGPIOPin(SIF_SSD1681_CS_PIN, 1);
}

ca_error SIF_SSD1681_overlay_qr_code(const char *text, uint8_t *image, uint8_t scale, uint8_t x, uint8_t y)
{
	enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW; // Error correction level

	// Make the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];
	bool    success = qrcodegen_encodeText(
        text, tempBuffer, qrcode, errCorLvl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	if (success)
	{
		SIF_SSD1681_embed_qr(qrcode, image, scale, x, y);
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_FAIL;
}

void SIF_SSD1681_SetFrameMemory(const uint8_t *image, bool full_resolution)
{
#ifdef EPAPER_FULL_RESOLUTION
	(void)full_resolution;
#endif

	SIF_SSD1681_Reset();

	SIF_SSD1681_SendCommand(BORDER_WAVEFORM_CONTROL);
	SIF_SSD1681_SendData(0x80);

#ifndef EPAPER_FULL_RESOLUTION
	if (full_resolution)
		SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_PHYSICAL - 1, 0, SIF_SSD1681_HEIGHT_PHYSICAL - 1);
	else
#endif
		SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_WINDOW - 1, 0, SIF_SSD1681_HEIGHT_WINDOW - 1);

	SIF_SSD1681_SetCursor(0, 0);
	SIF_SSD1681_SendCommand(WRITE_RAM);

#ifndef EPAPER_FULL_RESOLUTION
	if (!full_resolution)
	{
		SIF_SSD1681_DisplayHalfRes(image);
		SIF_SSD1681_ClearBottomEdge();
		SIF_SSD1681_ClearRightEdge();
	}
	else
#endif
	{
		SIF_SSD1681_DisplayFullRes(image);
	}

	SIF_SSD1681_TurnOnDisplay(FULL_UPDATE);
}

void SIF_SSD1681_SetFrameMemoryPartial(const uint8_t *image)
{
	SIF_SSD1681_Reset();

	SIF_SSD1681_SetLut(PARTIAL_UPDATE);
	SIF_SSD1681_SendCommand(0x37);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x40);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);
	SIF_SSD1681_SendData(0x00);

	SIF_SSD1681_SendCommand(BORDER_WAVEFORM_CONTROL);
	SIF_SSD1681_SendData(0x80);

	SIF_SSD1681_SendCommand(0x22);
	SIF_SSD1681_SendData(0xc0);
	SIF_SSD1681_SendCommand(0x20);
	SIF_SSD1681_WaitUntilIdle();

	SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_WINDOW - 1, 0, SIF_SSD1681_HEIGHT_WINDOW - 1);
	SIF_SSD1681_SetCursor(0, 0);
	SIF_SSD1681_SendCommand(WRITE_RAM);

#ifdef EPAPER_FULL_RESOLUTION
	SIF_SSD1681_DisplayFullRes(image);
#else
	SIF_SSD1681_DisplayHalfRes(image);
#endif
}

void SIF_SSD1681_DisplayFrame(void)
{
	SIF_SSD1681_TurnOnDisplay(FULL_UPDATE);
}

void SIF_SSD1681_DisplayPartFrame(void)
{
	SIF_SSD1681_TurnOnDisplay(PARTIAL_UPDATE);
}

void SIF_SSD1681_DisplayPartBaseImageWhite(void)
{
	u16_t width =
	    (SIF_SSD1681_WIDTH_PHYSICAL % 8 == 0) ? (SIF_SSD1681_WIDTH_PHYSICAL / 8) : (SIF_SSD1681_WIDTH_PHYSICAL / 8 + 1);
	u16_t height = SIF_SSD1681_HEIGHT_PHYSICAL;

	int address = 0;

	SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_PHYSICAL - 1, 0, SIF_SSD1681_HEIGHT_PHYSICAL - 1);
	SIF_SSD1681_SendCommand(WRITE_RAM);
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			address = i + j * width;
			SIF_SSD1681_SendData(0xff);
		}
	}

	SIF_SSD1681_SendCommand(WRITE_RAM_RED);
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			address = i + j * width;
			SIF_SSD1681_SendData(0xff);
		}
	}
	SIF_SSD1681_SetWindow(0, SIF_SSD1681_WIDTH_WINDOW - 1, 0, SIF_SSD1681_HEIGHT_WINDOW - 1);

	SIF_SSD1681_DisplayFrame();
}
