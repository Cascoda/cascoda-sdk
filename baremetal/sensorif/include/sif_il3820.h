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
 * @ingroup bm-sensorif
 * @defgroup bm-sensorif-il3820 IL3820 E-Paper Display sensorif driver
 * @brief Library for communicating with the IL3820 E-Paper display driver.
 *
 * @{
*/

#ifndef SIF_SIF_IL3820_H
#define SIF_SIF_IL3820_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************/
/********************* Pin connections *********************/
/***********************************************************/
/*

    |---+- BUSY - Pin 31 (GPIO PB.5)
    |---+- RST  - Pin 15 (GPIO PA.15)
    |---+- DC   - Pin 34 (GPIO PB.2)
    |---+- CS   - GND
    |---+- CLK  - Pin 33 (GPIO PB.3)
    |---+- DIN  - Pin 32 (GPIO PB.4)
    |---+- GND  - (Pins 3/14/16/18-25/27/30)
    |---+- VCC  - Pin 13

*/

/* Pin configuration */
#define SIF_IL3820_BUSY_PIN 31
#define SIF_IL3820_RST_PIN 15
#define SIF_IL3820_DC_PIN 34
//#define SIF_IL3820_CS_PIN 34

/* Display resolution */
#define SIF_IL3820_WIDTH 128
#define SIF_IL3820_HEIGHT 296

/* QR code image array size */
#define ARRAY_SIZE (SIF_IL3820_HEIGHT * SIF_IL3820_WIDTH / 8)

/* Look-up table declarations */
struct SIF_IL3820_lut
{
	uint8_t lut_array[30];
};

/********************************************************************************************/
/*****************************************************************************************/ /**
 * Using this LUT will cause the display to fully update every time a new image is drawn.
 * This will happen even if the new image is only a slight modification of the old image.
 * A full update of the display to clear the screen should be performed regularly.
 *********************************************************************************************
 ********************************************************************************************/
extern const struct SIF_IL3820_lut lut_full_update;

/********************************************************************************************/
/*****************************************************************************************/ /**
 * Using this LUT will allow the display to only update the pixels that are different
 * in the new image. A full update of the display should be done to clear the screen
 * after several partial updates, otherwise the e-Paper will be permanently damaged.
 *********************************************************************************************
 ********************************************************************************************/
extern const struct SIF_IL3820_lut lut_partial_update;

/* functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EINK Initialisation
 * \param lut - Pointer to the struct containing the LUT to use: 
 *              lut_full_update or lut_partial_update.
 *******************************************************************************
 ******************************************************************************/
ca_error SIF_IL3820_Initialise(const struct SIF_IL3820_lut *lut);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Displays an image
 * \param image - Image to display
 *******************************************************************************
 ******************************************************************************/
void SIF_IL3820_Display(const uint8_t *image);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Clears the display
 *******************************************************************************
 ******************************************************************************/
void SIF_IL3820_ClearDisplay(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Clears the display many times to make sure there is no ghost image
 *******************************************************************************
 ******************************************************************************/
void SIF_IL3820_StrongClearDisplay(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enter deep sleep mode. Device draws around 2uA in this mode. 
 *******************************************************************************
 ******************************************************************************/
void SIF_IL3820_DeepSleep(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Creates a QR code and overlays it on top of a pre-existing image
 *        at the given coordinates.
 *******************************************************************************
 * \param text  - The text string that is encoded into a QR symbol.
 * \param image - The image that is overlaid by the QR symbol.
 * \param x     - The x-coordinate of the top-left corner of the QR symbol.
 * \param y     - The y-coordinate of the top-left corner of the QR symbol.
 *******************************************************************************
 ******************************************************************************/
ca_error SIF_IL3820_overlay_qr_code(const char *text, uint8_t *image, uint8_t x, uint8_t y);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
// SIF_SIF_IL3820_H
