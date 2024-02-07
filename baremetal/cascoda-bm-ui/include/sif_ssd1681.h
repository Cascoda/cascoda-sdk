/*
 *  Copyright (c) 2023, Cascoda Ltd.
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
 * @defgroup bm-sensorif-ssd1681 SSD1681 E-Paper Display sensorif driver
 * @brief Library for communicating with the SSD1681 E-Paper display driver.
 *
 * @{
*/

#ifndef SIF_SIF_SSD1681_H
#define SIF_SIF_SSD1681_H

#include <stdint.h>
#include "ca821x_error.h"
#include "cascoda_chili_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CASCODA_CHILI2_CONFIG
#error CASCODA_CHILI2_CONFIG has to be defined! Please include the file "cascoda_chili_config.h"
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
#define SIF_SSD1681_BUSY_PIN 31

#if (CASCODA_CHILI2_CONFIG == 2)
#define SIF_SSD1681_RST_PIN 5
#else
#define SIF_SSD1681_RST_PIN 15
#endif

#define SIF_SSD1681_DC_PIN 34
//#define SIF_SSD1681_CS_PIN 34

/* Actual physical display resolution (in pixels) */
#define SIF_SSD1681_WIDTH_PHYSICAL 200
#define SIF_SSD1681_HEIGHT_PHYSICAL 200

#ifdef EPAPER_FULL_RESOLUTION
#define SIF_SSD1681_WIDTH_WINDOW SIF_SSD1681_WIDTH_PHYSICAL
#define SIF_SSD1681_HEIGHT_WINDOW SIF_SSD1681_HEIGHT_PHYSICAL

#define SIF_SSD1681_WIDTH SIF_SSD1681_WIDTH_PHYSICAL
#define SIF_SSD1681_HEIGHT SIF_SSD1681_HEIGHT_PHYSICAL
#else
// Display resolution that the window will be set to upon initialization
// Note: 192 was chosen because it is the closest number to 200, that is a
// multiple of 8 after having been divided by 2. (The problem with 200 is that
// 200/2 is 100, which is not divisible by 8).
#define SIF_SSD1681_WIDTH_WINDOW 192
#define SIF_SSD1681_HEIGHT_WINDOW 200

// Display resolution that the image buffer will be set to. This
// is the actual display resolution that ends up being used.
// This is done to save memory (200x200 resolution requires an image buffer
// of 5000 bytes, whereas 96*100 only requires 1200).
#define SIF_SSD1681_WIDTH (SIF_SSD1681_WIDTH_WINDOW / 2)
#define SIF_SSD1681_HEIGHT (SIF_SSD1681_WIDTH_WINDOW / 2)
#endif // EPAPER_FULL_RESOLUTION

/* QR code image array size */
#define ARRAY_SIZE (SIF_SSD1681_HEIGHT * SIF_SSD1681_WIDTH / 8)

/* Display update mode */
typedef enum
{
	FULL_UPDATE = 0,
	PARTIAL_UPDATE,
} SIF_SSD1681_Update_Mode;

/* functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Function which reports the sleep status of the eink display.
 *******************************************************************************
 * \return true if the eink is in deep sleep mode, false otherwise.
 *******************************************************************************
 ******************************************************************************/
bool SIF_SSD1681_IsAsleep(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EINK Initialisation
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_Initialise(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EINK De-Initialisation
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_Deinitialise(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Clears the display
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_ClearDisplay(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Clears the display many times to make sure there is no ghost image
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_StrongClearDisplay(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enter deep sleep mode. Device draws around 2uA in this mode.
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_DeepSleep(void);

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
ca_error SIF_SSD1681_overlay_qr_code(const char *text, uint8_t *image, uint8_t scale, uint8_t x, uint8_t y);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Copies the image into the eink display's RAM, for full update.
 *******************************************************************************
 * \param image - The image that is copied into the RAM.
 * \param full_resolution - Whether the image provided should be displayed
 * in full resolution or half resolution. This is is ignored when
 * EPAPER_FULL_RESOLUTION is defined, and therefore only applies to binaries
 * built with half resolution in mind. This allows the option to display an
 * image (typically stored in flash) using full resolution, as an exception.
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_SetFrameMemory(const uint8_t *image, bool full_resolution);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Copies the image into the eink display's RAM, for partial update.
 *******************************************************************************
 * \param image - The image that is copied into the RAM.
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_SetFrameMemoryPartial(const uint8_t *image);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Causes the eink to display what is currently in its RAM, using the
 * full update process.
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_DisplayFrame(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Causes the eink to display what is currently in its RAM, using the
 * partial update process.
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_DisplayPartFrame(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Causes the eink to display a white image, to be used as a "base"
 * image for subsequent partial updates.
 * NOTE: It is necessary to call this function for the first ever partial update,
 * or for the first partial update after waking up from deep sleep.
 *******************************************************************************
 ******************************************************************************/
void SIF_SSD1681_DisplayPartBaseImageWhite(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
// SIF_SIF_SSD1681_H
