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
#include "ca821x_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************/
/********************* Pin connections *********************/
/***********************************************************/
/*
    Pin config for M2351:
    |---+- BUSY - Pin 31 (GPIO PB.5)
    |---+- RST  - Pin 15 (GPIO PA.15) on Chili2D, Pin 5 (GPIO PB.12) on Devboard
    |---+- DC   - Pin 34 (GPIO PB.2)
    |---+- CS   - GND
    |---+- CLK  - Pin 33 (GPIO PB.3)
    |---+- DIN  - Pin 32 (GPIO PB.4)
    |---+- GND  - (Pins 3/14/16/18-25/27/30)
    |---+- VCC  - Pin 13

*/

/* Pin configuration */
#define SIF_IL3820_BUSY_PIN 31

#if (CASCODA_CHILI2_CONFIG == 2)
#define SIF_IL3820_RST_PIN 5
#else
#define SIF_IL3820_RST_PIN 15
#endif

#define SIF_IL3820_DC_PIN 34
//#define SIF_IL3820_CS_PIN 34

/* Display resolution */
#define SIF_IL3820_WIDTH 128
#define SIF_IL3820_HEIGHT 296

/* QR code image array size */
#define ARRAY_SIZE (SIF_IL3820_HEIGHT * SIF_IL3820_WIDTH / 8)

/* Display update mode */
typedef enum
{
	FULL_UPDATE = 0,
	PARTIAL_UPDATE,
} SIF_IL3820_Update_Mode;

/* Clear or no clear */
typedef enum
{
	WITH_CLEAR = 0,
	WITHOUT_CLEAR
} SIF_IL3820_Clear_Mode;

/* functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EINK Initialisation
 * \param mode - Whether to initialise for FULL_UPDATE or PARTIAL_UPDATE
 *******************************************************************************
 ******************************************************************************/
ca_error SIF_IL3820_Initialise(SIF_IL3820_Update_Mode mode);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EINK De-Initialisation
 *******************************************************************************
 ******************************************************************************/
void SIF_IL3820_Deinitialise(void);

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
 * \param scale - scaling of the image, 1 or 2 supported.
 * \param x     - The x-coordinate of the top-left corner of the QR symbol.
 * \param y     - The y-coordinate of the top-left corner of the QR symbol.
 *******************************************************************************
 ******************************************************************************/
ca_error SIF_IL3820_overlay_qr_code(const char *text, uint8_t *image, uint8_t scale, uint8_t x, uint8_t y);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Follows Routines for clearing, waiting and displaying the image.
 *******************************************************************************
 * \param image - The image to be overlaid on the eink screen.
 * \param mode - Whether or not the display will be cleared before the image
 * is displayed
 *******************************************************************************
 ******************************************************************************/
void SIF_IL3820_DisplayImage(const uint8_t *image, SIF_IL3820_Clear_Mode mode);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
// SIF_SIF_IL3820_H