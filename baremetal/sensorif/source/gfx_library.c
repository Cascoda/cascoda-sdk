///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                            GFX_Library.c                          ////
////                                                                   ////
////                 Graphics library for mikroC compiler.             ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////               This is a free software with NO WARRANTY.           ////
////                     https://simple-circuit.com/                   ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.
Copyright (c) 2023 Cascoda LTD.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */
///////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "gfx_driver.h"
#include "gfx_library.h"

//*************************** User Functions ***************************//
/*
void display_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void display_drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void display_drawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t color);
void display_fillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void display_fillCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t delta, uint16_t color);
void display_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void display_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void display_drawRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);
void display_fillRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);

void display_setCursor(uint16_t x, uint16_t y);
void display_setTextColor(uint16_t c, uint16_t bg);
void display_setTextSize(uint8_t s);
void display_setTextWrap(bool w);
void display_putc(uint8_t c);
void display_puts(uint8_t *s);
void display_printf(const char *fmt, ...);
void display_customChar(const uint8_t *c);
void display_drawChar(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t size);

uint8_t  display_getRotation();
uint16_t getCursorX(void);
uint16_t getCursorY(void);
uint16_t display_getWidth();
uint16_t display_getHeight();
uint16_t display_color565(uint8_t red, uint8_t green, uint8_t blue);

void display_drawBitmapV1   (uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color);
void display_drawBitmapV1_bg(uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color, uint16_t bg);
void display_drawBitmapV2   (uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color);
void display_drawBitmapV2_bg(uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color, uint16_t bg);
*/

//************************* Non User Functions *************************//
void display_drawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t color);
void display_fillCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t delta, uint16_t color);

void    writeLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
uint8_t printNumber(uint32_t n, int8_t n_width, uint8_t _flags);
void    printFloat(float float_n, int8_t f_width, int8_t decimal, uint8_t _flags);
void    v_printf(const char *fmt, va_list arp);
void    fillRect(uint16_t x0, uint16_t y0, uint16_t x1, int16_t y1, uint16_t color);
//////////////////////////////////////////////////////////////////////////

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
	{                       \
		int16_t t = a;      \
		a         = b;      \
		b         = t;      \
	}
#endif

#ifdef EPAPER_2_9_INCH
uint16_t      display_width  = SIF_IL3820_WIDTH;
uint16_t      display_height = SIF_IL3820_HEIGHT;
#elif defined EPAPER_WAVESHARE_1_54_INCH
uint16_t display_width  = SIF_SSD1681_WIDTH;
uint16_t display_height = SIF_SSD1681_HEIGHT;
#elif defined EPAPER_MIKROE_1_54_INCH
uint16_t display_width  = SIF_SSD1608_WIDTH;
uint16_t display_height = SIF_SSD1608_HEIGHT;
#endif

// generic draw pixel
// only touches the frame buffer
void display_drawPixel(int16_t x, int16_t y, uint16_t color)
{
	gfx_drv_drawPixel(x, y, color);
}

// clears the frame buffer
void display_clear(void)
{
	gfx_drv_clearDisplay();
}

#ifdef EPAPER_2_9_INCH

void display_render(SIF_IL3820_Update_Mode updt_mode, SIF_IL3820_Clear_Mode clr_mode)
{
	SIF_IL3820_Initialise(updt_mode);
	SIF_IL3820_DisplayImage(get_framebuffer(), clr_mode);
	SIF_IL3820_Deinitialise();
}

void display_fixed_image(const uint8_t *image)
{
	SIF_IL3820_Initialise(FULL_UPDATE);
	SIF_IL3820_DisplayImage(image, WITH_CLEAR);
	SIF_IL3820_Deinitialise();
}

#elif defined EPAPER_MIKROE_1_54_INCH

void display_render(SIF_SSD1608_Update_Mode updt_mode, SIF_SSD1608_Clear_Mode clr_mode)
{
	SIF_SSD1608_Initialise(updt_mode);
	SIF_SSD1608_DisplayImage(get_framebuffer(), clr_mode, false);
	SIF_SSD1608_Deinitialise();
}

void display_fixed_image(const uint8_t *image)
{
	SIF_SSD1608_Initialise(FULL_UPDATE);
	SIF_SSD1608_DisplayImage(image, WITH_CLEAR, true);
	SIF_SSD1608_Deinitialise();
}

#elif defined EPAPER_WAVESHARE_1_54_INCH

void display_render_full(void)
{
	SIF_SSD1681_Initialise();
	SIF_SSD1681_SetFrameMemory(get_framebuffer(), false);
	SIF_SSD1681_DisplayFrame();
	SIF_SSD1681_DeepSleep();
	SIF_SSD1681_Deinitialise();
}

void display_render_partial(bool sleep_when_done)
{
	if (SIF_SSD1681_IsAsleep())
	{
		SIF_SSD1681_Initialise();
		SIF_SSD1681_DisplayPartBaseImageWhite();
	}

	SIF_SSD1681_SetFrameMemoryPartial(get_framebuffer());
	SIF_SSD1681_DisplayPartFrame();

	if (sleep_when_done)
	{
		SIF_SSD1681_DeepSleep();
		SIF_SSD1681_Deinitialise();
	}
}

void display_fixed_image(const uint8_t *image)
{
	SIF_SSD1681_Initialise();
	SIF_SSD1681_SetFrameMemory(image, true);
	SIF_SSD1681_DisplayFrame();
	SIF_SSD1681_DeepSleep();
	SIF_SSD1681_Deinitialise();
}

#endif

// generic functions below.
void display_setRotation(uint16_t rotation)
{
	rotation = rotation;
	gfx_drv_setRotation(rotation);
}

// generic fill rectangle
// only touches the frame buffer
void display_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	fillRect(x, y, w, h, color);
}

void display_drawHLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t color)
{
	writeLine(x0, y0, x0 + x1, y0, color);
}

void display_drawVLine(uint16_t x0, uint16_t y0, uint16_t y1, uint16_t color)
{
	writeLine(x0, y0, x0, y0 + y1, color);
}

// global variables
static int16_t  cursor_x    = 0;     ///< x location to start print()ing text
static int16_t  cursor_y    = 0;     ///< y location to start print()ing text
static uint16_t textcolor   = BLACK; ///< background color for print()
static uint16_t textbgcolor = WHITE; ///< text color for print()
static uint8_t  textsize    = 1;     ///< Desired magnification of text to print()
static bool     wrap        = true;  ///< If set, 'wrap' text at right edge of display
static uint8_t  rotation    = 0;     ///< no rotation

// Standard ASCII 5x7 font
#ifndef FONT5X7_H
static const uint8_t font[256][5] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x5B, 0x4F, 0x5B, 0x3E, 0x3E, 0x6B, 0x4F, 0x6B, 0x3E, 0x1C, 0x3E, 0x7C,
    0x3E, 0x1C, 0x18, 0x3C, 0x7E, 0x3C, 0x18, 0x1C, 0x57, 0x7D, 0x57, 0x1C, 0x1C, 0x5E, 0x7F, 0x5E, 0x1C, 0x00,
    0x18, 0x3C, 0x18, 0x00, 0xFF, 0xE7, 0xC3, 0xE7, 0xFF, 0x00, 0x18, 0x24, 0x18, 0x00, 0xFF, 0xE7, 0xDB, 0xE7,
    0xFF, 0x30, 0x48, 0x3A, 0x06, 0x0E, 0x26, 0x29, 0x79, 0x29, 0x26, 0x40, 0x7F, 0x05, 0x05, 0x07, 0x40, 0x7F,
    0x05, 0x25, 0x3F, 0x5A, 0x3C, 0xE7, 0x3C, 0x5A, 0x7F, 0x3E, 0x1C, 0x1C, 0x08, 0x08, 0x1C, 0x1C, 0x3E, 0x7F,
    0x14, 0x22, 0x7F, 0x22, 0x14, 0x5F, 0x5F, 0x00, 0x5F, 0x5F, 0x06, 0x09, 0x7F, 0x01, 0x7F, 0x00, 0x66, 0x89,
    0x95, 0x6A, 0x60, 0x60, 0x60, 0x60, 0x60, 0x94, 0xA2, 0xFF, 0xA2, 0x94, 0x08, 0x04, 0x7E, 0x04, 0x08, 0x10,
    0x20, 0x7E, 0x20, 0x10, 0x08, 0x08, 0x2A, 0x1C, 0x08, 0x08, 0x1C, 0x2A, 0x08, 0x08, 0x1E, 0x10, 0x10, 0x10,
    0x10, 0x0C, 0x1E, 0x0C, 0x1E, 0x0C, 0x30, 0x38, 0x3E, 0x38, 0x30, 0x06, 0x0E, 0x3E, 0x0E, 0x06, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x14, 0x7F, 0x14, 0x7F, 0x14,
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62, 0x36, 0x49, 0x56, 0x20, 0x50, 0x00, 0x08, 0x07,
    0x03, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x41, 0x22, 0x1C, 0x00, 0x2A, 0x1C, 0x7F, 0x1C, 0x2A, 0x08,
    0x08, 0x3E, 0x08, 0x08, 0x00, 0x80, 0x70, 0x30, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x60, 0x60,
    0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x72, 0x49,
    0x49, 0x49, 0x46, 0x21, 0x41, 0x49, 0x4D, 0x33, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x27, 0x45, 0x45, 0x45, 0x39,
    0x3C, 0x4A, 0x49, 0x49, 0x31, 0x41, 0x21, 0x11, 0x09, 0x07, 0x36, 0x49, 0x49, 0x49, 0x36, 0x46, 0x49, 0x49,
    0x29, 0x1E, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x40, 0x34, 0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x41, 0x14,
    0x14, 0x14, 0x14, 0x14, 0x00, 0x41, 0x22, 0x14, 0x08, 0x02, 0x01, 0x59, 0x09, 0x06, 0x3E, 0x41, 0x5D, 0x59,
    0x4E, 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22, 0x7F, 0x41,
    0x41, 0x41, 0x3E, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01, 0x3E, 0x41, 0x41, 0x51, 0x73,
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14,
    0x22, 0x41, 0x7F, 0x40, 0x40, 0x40, 0x40, 0x7F, 0x02, 0x1C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E,
    0x41, 0x41, 0x41, 0x3E, 0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29,
    0x46, 0x26, 0x49, 0x49, 0x49, 0x32, 0x03, 0x01, 0x7F, 0x01, 0x03, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x1F, 0x20,
    0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x63, 0x14, 0x08, 0x14, 0x63, 0x03, 0x04, 0x78, 0x04, 0x03,
    0x61, 0x59, 0x49, 0x4D, 0x43, 0x00, 0x7F, 0x41, 0x41, 0x41, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41,
    0x41, 0x7F, 0x04, 0x02, 0x01, 0x02, 0x04, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x03, 0x07, 0x08, 0x00, 0x20,
    0x54, 0x54, 0x78, 0x40, 0x7F, 0x28, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x28, 0x38, 0x44, 0x44, 0x28,
    0x7F, 0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x08, 0x7E, 0x09, 0x02, 0x18, 0xA4, 0xA4, 0x9C, 0x78, 0x7F, 0x08,
    0x04, 0x04, 0x78, 0x00, 0x44, 0x7D, 0x40, 0x00, 0x20, 0x40, 0x40, 0x3D, 0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x78, 0x04, 0x78, 0x7C, 0x08, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44,
    0x44, 0x38, 0xFC, 0x18, 0x24, 0x24, 0x18, 0x18, 0x24, 0x24, 0x18, 0xFC, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x48,
    0x54, 0x54, 0x54, 0x24, 0x04, 0x04, 0x3F, 0x44, 0x24, 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x1C, 0x20, 0x40, 0x20,
    0x1C, 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x44, 0x28, 0x10, 0x28, 0x44, 0x4C, 0x90, 0x90, 0x90, 0x7C, 0x44, 0x64,
    0x54, 0x4C, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0x41, 0x36, 0x08, 0x00,
    0x02, 0x01, 0x02, 0x04, 0x02, 0x3C, 0x26, 0x23, 0x26, 0x3C, 0x1E, 0xA1, 0xA1, 0x61, 0x12, 0x3A, 0x40, 0x40,
    0x20, 0x7A, 0x38, 0x54, 0x54, 0x55, 0x59, 0x21, 0x55, 0x55, 0x79, 0x41, 0x22, 0x54, 0x54, 0x78, 0x42, // a-umlaut
    0x21, 0x55, 0x54, 0x78, 0x40, 0x20, 0x54, 0x55, 0x79, 0x40, 0x0C, 0x1E, 0x52, 0x72, 0x12, 0x39, 0x55, 0x55,
    0x55, 0x59, 0x39, 0x54, 0x54, 0x54, 0x59, 0x39, 0x55, 0x54, 0x54, 0x58, 0x00, 0x00, 0x45, 0x7C, 0x41, 0x00,
    0x02, 0x45, 0x7D, 0x42, 0x00, 0x01, 0x45, 0x7C, 0x40, 0x7D, 0x12, 0x11, 0x12, 0x7D, // A-umlaut
    0xF0, 0x28, 0x25, 0x28, 0xF0, 0x7C, 0x54, 0x55, 0x45, 0x00, 0x20, 0x54, 0x54, 0x7C, 0x54, 0x7C, 0x0A, 0x09,
    0x7F, 0x49, 0x32, 0x49, 0x49, 0x49, 0x32, 0x3A, 0x44, 0x44, 0x44, 0x3A, // o-umlaut
    0x32, 0x4A, 0x48, 0x48, 0x30, 0x3A, 0x41, 0x41, 0x21, 0x7A, 0x3A, 0x42, 0x40, 0x20, 0x78, 0x00, 0x9D, 0xA0,
    0xA0, 0x7D, 0x3D, 0x42, 0x42, 0x42, 0x3D, // O-umlaut
    0x3D, 0x40, 0x40, 0x40, 0x3D, 0x3C, 0x24, 0xFF, 0x24, 0x24, 0x48, 0x7E, 0x49, 0x43, 0x66, 0x2B, 0x2F, 0xFC,
    0x2F, 0x2B, 0xFF, 0x09, 0x29, 0xF6, 0x20, 0xC0, 0x88, 0x7E, 0x09, 0x03, 0x20, 0x54, 0x54, 0x79, 0x41, 0x00,
    0x00, 0x44, 0x7D, 0x41, 0x30, 0x48, 0x48, 0x4A, 0x32, 0x38, 0x40, 0x40, 0x22, 0x7A, 0x00, 0x7A, 0x0A, 0x0A,
    0x72, 0x7D, 0x0D, 0x19, 0x31, 0x7D, 0x26, 0x29, 0x29, 0x2F, 0x28, 0x26, 0x29, 0x29, 0x29, 0x26, 0x30, 0x48,
    0x4D, 0x40, 0x20, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x38, 0x2F, 0x10, 0xC8, 0xAC, 0xBA,
    0x2F, 0x10, 0x28, 0x34, 0xFA, 0x00, 0x00, 0x7B, 0x00, 0x00, 0x08, 0x14, 0x2A, 0x14, 0x22, 0x22, 0x14, 0x2A,
    0x14, 0x08, 0x55, 0x00, 0x55, 0x00, 0x55, // #176 (25% block) missing in old code
    0xAA, 0x55, 0xAA, 0x55, 0xAA,             // 50% block
    0xFF, 0x55, 0xFF, 0x55, 0xFF,             // 75% block
    0x00, 0x00, 0x00, 0xFF, 0x00, 0x10, 0x10, 0x10, 0xFF, 0x00, 0x14, 0x14, 0x14, 0xFF, 0x00, 0x10, 0x10, 0xFF,
    0x00, 0xFF, 0x10, 0x10, 0xF0, 0x10, 0xF0, 0x14, 0x14, 0x14, 0xFC, 0x00, 0x14, 0x14, 0xF7, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0xFF, 0x14, 0x14, 0xF4, 0x04, 0xFC, 0x14, 0x14, 0x17, 0x10, 0x1F, 0x10, 0x10, 0x1F, 0x10,
    0x1F, 0x14, 0x14, 0x14, 0x1F, 0x00, 0x10, 0x10, 0x10, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x10, 0x10, 0x10,
    0x10, 0x1F, 0x10, 0x10, 0x10, 0x10, 0xF0, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0xFF, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x14, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x1F,
    0x10, 0x17, 0x00, 0x00, 0xFC, 0x04, 0xF4, 0x14, 0x14, 0x17, 0x10, 0x17, 0x14, 0x14, 0xF4, 0x04, 0xF4, 0x00,
    0x00, 0xFF, 0x00, 0xF7, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0xF7, 0x00, 0xF7, 0x14, 0x14, 0x14, 0x17,
    0x14, 0x10, 0x10, 0x1F, 0x10, 0x1F, 0x14, 0x14, 0x14, 0xF4, 0x14, 0x10, 0x10, 0xF0, 0x10, 0xF0, 0x00, 0x00,
    0x1F, 0x10, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x14, 0x00, 0x00, 0x00, 0xFC, 0x14, 0x00, 0x00, 0xF0, 0x10, 0xF0,
    0x10, 0x10, 0xFF, 0x10, 0xFF, 0x14, 0x14, 0x14, 0xFF, 0x14, 0x10, 0x10, 0x10, 0x1F, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x38, 0x44, 0x44, 0x38, 0x44, 0xFC, 0x4A, 0x4A, 0x4A,
    0x34, // sharp-s or beta
    0x7E, 0x02, 0x02, 0x06, 0x06, 0x02, 0x7E, 0x02, 0x7E, 0x02, 0x63, 0x55, 0x49, 0x41, 0x63, 0x38, 0x44, 0x44,
    0x3C, 0x04, 0x40, 0x7E, 0x20, 0x1E, 0x20, 0x06, 0x02, 0x7E, 0x02, 0x02, 0x99, 0xA5, 0xE7, 0xA5, 0x99, 0x1C,
    0x2A, 0x49, 0x2A, 0x1C, 0x4C, 0x72, 0x01, 0x72, 0x4C, 0x30, 0x4A, 0x4D, 0x4D, 0x30, 0x30, 0x48, 0x78, 0x48,
    0x30, 0xBC, 0x62, 0x5A, 0x46, 0x3D, 0x3E, 0x49, 0x49, 0x49, 0x00, 0x7E, 0x01, 0x01, 0x01, 0x7E, 0x2A, 0x2A,
    0x2A, 0x2A, 0x2A, 0x44, 0x44, 0x5F, 0x44, 0x44, 0x40, 0x51, 0x4A, 0x44, 0x40, 0x40, 0x44, 0x4A, 0x51, 0x40,
    0x00, 0x00, 0xFF, 0x01, 0x03, 0xE0, 0x80, 0xFF, 0x00, 0x00, 0x08, 0x08, 0x6B, 0x6B, 0x08, 0x36, 0x12, 0x36,
    0x24, 0x36, 0x06, 0x0F, 0x09, 0x0F, 0x06, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x30,
    0x40, 0xFF, 0x01, 0x01, 0x00, 0x1F, 0x01, 0x01, 0x1E, 0x00, 0x19, 0x1D, 0x17, 0x12, 0x00, 0x3C, 0x3C, 0x3C,
    0x3C, 0x00, 0x00, 0x00, 0x00, 0x00 // #255 NBSP
};
#endif // FONT5X7_H

/**************************************************************************/
/*!
   @brief    Write a line.  Bresenham's algorithm - thx wikpedia
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void writeLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	bool    steep = abs((int16_t)(y1 - y0)) > abs((int16_t)(x1 - x0));
	int16_t dx, dy, err, ystep;
	if (steep)
	{
		_swap_int16_t(x0, y0);
		_swap_int16_t(x1, y1);
	}

	if (x0 > x1)
	{
		_swap_int16_t(x0, x1);
		_swap_int16_t(y0, y1);
	}

	dx = x1 - x0;
	dy = abs((int16_t)(y1 - y0));

	err = dx / 2;
	ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			display_drawPixel(y0, x0, color);
		}
		else
		{
			display_drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

// display_fillRect(cursor_x + 5 * textsize, cursor_y, textsize, 8 * textsize, textbgcolor);
void fillRect(uint16_t x0, uint16_t y0, uint16_t w, int16_t h, uint16_t color)
{
	for (int i = 0; i < w; i++)
	{
		// write line works with x0, x1, y0 y1
		writeLine(x0 + i, y0, x0 + i, y0 + h, color);
	}
}

/**************************************************************************/
/*!
   @brief    Draw a line
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void display_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	// Update in subclasses if desired!
	if (x0 == x1)
	{
		if (y0 > y1)
			_swap_int16_t(y0, y1);
		display_drawVLine(x0, y0, y1 - y0 + 1, color);
	}
	else if (y0 == y1)
	{
		if (x0 > x1)
			_swap_int16_t(x0, x1);
		display_drawHLine(x0, y0, x1 - x0 + 1, color);
	}
	else
	{
		writeLine(x0, y0, x1, y1, color);
	}
}

/**************************************************************************/
/*!
   @brief    Draw a circle outline
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void display_drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	display_drawPixel(x0, y0 + r, color);
	display_drawPixel(x0, y0 - r, color);
	display_drawPixel(x0 + r, y0, color);
	display_drawPixel(x0 - r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		display_drawPixel(x0 + x, y0 + y, color);
		display_drawPixel(x0 - x, y0 + y, color);
		display_drawPixel(x0 + x, y0 - y, color);
		display_drawPixel(x0 - x, y0 - y, color);
		display_drawPixel(x0 + y, y0 + x, color);
		display_drawPixel(x0 - y, y0 + x, color);
		display_drawPixel(x0 + y, y0 - x, color);
		display_drawPixel(x0 - y, y0 - x, color);
	}
}

/**************************************************************************/
/*!
    @brief    Quarter-circle drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    cornername  Mask bit #1 or bit #2 to indicate which quarters of the circle we're doing
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void display_drawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4)
		{
			display_drawPixel(x0 + x, y0 + y, color);
			display_drawPixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2)
		{
			display_drawPixel(x0 + x, y0 - y, color);
			display_drawPixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8)
		{
			display_drawPixel(x0 - y, y0 + x, color);
			display_drawPixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1)
		{
			display_drawPixel(x0 - y, y0 - x, color);
			display_drawPixel(x0 - x, y0 - y, color);
		}
	}
}

/**************************************************************************/
/*!
   @brief    Draw a circle with filled color
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void display_fillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
	//display_drawVLine(x0, y0-r, x0+(2*r+1), color);
	display_drawVLine(x0, y0 - r, 2 * r + 1, color);
	display_fillCircleHelper(x0, y0, r, 3, 0, color);
}

/**************************************************************************/
/*!
    @brief  Quarter-circle drawer with fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void display_fillCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corners, uint16_t delta, uint16_t color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;
	int16_t px    = x;
	int16_t py    = y;

	delta++; // Avoid some +1's in the loop

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		// These checks avoid double-drawing certain lines, important
		// for the SSD1306 library which has an INVERT drawing mode.
		if (x < (y + 1))
		{
			if (corners & 1)
				display_drawVLine(x0 + x, y0 - y, 2 * y + delta, color);
			if (corners & 2)
				display_drawVLine(x0 - x, y0 - y, 2 * y + delta, color);
			//if(corners & 1) display_drawVLine(x0+x, y0-y, (x0+x) +(2*y+delta), color);
			//if(corners & 2) display_drawVLine(x0-x, y0-y, (x0-x) +(2*y+delta), color);
		}
		if (y != py)
		{
			if (corners & 1)
				display_drawVLine(x0 + py, y0 - px, 2 * px + delta, color);
			if (corners & 2)
				display_drawVLine(x0 - py, y0 - px, 2 * px + delta, color);
			//if(corners & 1) display_drawVLine(x0+py, y0-px,(x0+py) + (2*px+delta), color);
			//if(corners & 2) display_drawVLine(x0-py, y0-px, (x0-py) + (2*px+delta), color);
			py = y;
		}
		px = x;
	}
}

/**************************************************************************/
/*!
   @brief   Draw a rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void display_drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	display_drawHLine(x, y, w - 1, color);
	display_drawHLine(x, y + h - 1, w - 1, color);
	display_drawVLine(x, y, h - 1, color);
	display_drawVLine(x + w - 1, y, h - 1, color);
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void display_drawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
	int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
	if (r > max_radius)
		r = max_radius;
	// smarter version
	display_drawHLine(x + r, y, w - 2 * r, color);         // Top
	display_drawHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
	display_drawVLine(x, y + r, h - 2 * r, color);         // Left
	display_drawVLine(x + w - 1, y + r, h - 2 * r, color); // Right
	// draw four corners
	display_drawCircleHelper(x + r, y + r, r, 1, color);
	display_drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	display_drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	display_drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw/fill with
*/
/**************************************************************************/
void display_fillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
	int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
	if (r > max_radius)
		r = max_radius;
	// smarter version
	display_fillRect(x + r, y, w - 2 * r, h, color);
	// draw four corners
	display_fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	//display_fillCircleHelper(94, 18, 8, 1, 43, color);
	display_fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

/**************************************************************************/
/*!
   @brief   Draw a triangle with no fill color
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void display_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	display_drawLine(x0, y0, x1, y1, color);
	display_drawLine(x1, y1, x2, y2, color);
	display_drawLine(x2, y2, x0, y0, color);
}

/**************************************************************************/
/*!
   @brief     Draw a triangle with color-fill
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to fill/draw with
*/
/**************************************************************************/
void display_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	int16_t a, b, y, last, dx01, dy01, dx02, dy02, dx12, dy12;
	int32_t sa = 0, sb = 0;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1)
	{
		_swap_int16_t(y0, y1);
		_swap_int16_t(x0, x1);
	}
	if (y1 > y2)
	{
		_swap_int16_t(y2, y1);
		_swap_int16_t(x2, x1);
	}
	if (y0 > y1)
	{
		_swap_int16_t(y0, y1);
		_swap_int16_t(x0, x1);
	}

	if (y0 == y2)
	{ // Handle awkward all-on-same-line case as its own thing
		a = b = x0;
		if (x1 < a)
			a = x1;
		else if (x1 > b)
			b = x1;
		if (x2 < a)
			a = x2;
		else if (x2 > b)
			b = x2;
		display_drawHLine(a, y0, b - a + 1, color);
		return;
	}

	dx01 = x1 - x0;
	dy01 = y1 - y0;
	dx02 = x2 - x0;
	dy02 = y2 - y0;
	dx12 = x2 - x1;
	dy12 = y2 - y1;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if (y1 == y2)
		last = y1; // Include y1 scanline
	else
		last = y1 - 1; // Skip it

	for (y = y0; y <= last; y++)
	{
		a = x0 + sa / dy01;
		b = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		/* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
		if (a > b)
			_swap_int16_t(a, b);
		display_drawHLine(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++)
	{
		a = x1 + sa / dy12;
		b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		/* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
		if (a > b)
			_swap_int16_t(a, b);
		display_drawHLine(a, y, b - a + 1, color);
	}
}

/**************************************************************************/
/*!
    @brief  Print one byte/character of data
    @param  c  The 8-bit ascii character to write
*/
/**************************************************************************/
void display_putc(uint8_t c)
{
	uint8_t i, j;
	if (c == ' ' && cursor_x == 0 && wrap)
		return;
	if (c == '\r')
	{
		cursor_x = 0;
		return;
	}
	if (c == '\n')
	{
		cursor_y += textsize * 8;
		return;
	}

	for (i = 0; i < 5; i++)
	{
		uint8_t line = font[c][i];
		for (j = 0; j < 8; j++, line >>= 1)
		{
			if (line & 1)
			{
				if (textsize == 1)
					display_drawPixel(cursor_x + i, cursor_y + j, textcolor);
				else
					display_fillRect(cursor_x + i * textsize, cursor_y + j * textsize, textsize, textsize, textcolor);
			}
			else if (textbgcolor != textcolor)
			{
				if (textsize == 1)
					display_drawPixel(cursor_x + i, cursor_y + j, textbgcolor);
				else
					display_fillRect(cursor_x + i * textsize, cursor_y + j * textsize, textsize, textsize, textbgcolor);
			}
		}
	}

	if (textbgcolor != textcolor)
	{ // If opaque, draw vertical line for last column
		if (textsize == 1)
			display_drawVLine(cursor_x + 5, cursor_y, 7, textbgcolor);
		else
			display_fillRect(cursor_x + 5 * textsize, cursor_y, textsize, 8 * textsize, textbgcolor);
	}

	cursor_x += textsize * 6;

	if (cursor_x > ((uint16_t)display_width + textsize * 6))
		cursor_x = display_width;

	if (wrap && (cursor_x + (textsize * 5)) > display_width)
	{
		cursor_x = 0;
		cursor_y += textsize * 8;
		if (cursor_y > ((uint16_t)display_height + textsize * 8))
			cursor_y = display_height;
	}
}

// print string
void display_puts(uint8_t *s)
{
	while (*s) display_putc(*s++);
}

// print custom char (dimension: 7x5 or 8x5 pixel)
void display_customChar(const uint8_t *c)
{
	uint8_t i, j;
	for (i = 0; i < 5; i++)
	{
		uint8_t line = c[i];
		for (j = 0; j < 8; j++, line >>= 1)
		{
			if (line & 1)
			{
				if (textsize == 1)
					display_drawPixel(cursor_x + i, cursor_y + j, textcolor);
				else
					display_fillRect(cursor_x + i * textsize, cursor_y + j * textsize, textsize, textsize, textcolor);
			}
			else if (textbgcolor != textcolor)
			{
				if (textsize == 1)
					display_drawPixel(cursor_x + i, cursor_y + j, textbgcolor);
				else
					display_fillRect(cursor_x + i * textsize, cursor_y + j * textsize, textsize, textsize, textbgcolor);
			}
		}
	}

	if (textbgcolor != textcolor)
	{ // If opaque, draw vertical line for last column
		if (textsize == 1)
			display_drawVLine(cursor_x + 5, cursor_y, 8, textbgcolor);
		else
			display_fillRect(cursor_x + 5 * textsize, cursor_y, textsize, 8 * textsize, textbgcolor);
	}

	cursor_x += textsize * 6;

	if (cursor_x > ((uint16_t)display_width + textsize * 6))
		cursor_x = display_width;

	if (wrap && (cursor_x + (textsize * 5)) > display_width)
	{
		cursor_x = 0;
		cursor_y += textsize * 8;
		if (cursor_y > ((uint16_t)display_height + textsize * 8))
			cursor_y = display_height;
	}
}

/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
    @param    size  Font magnification level, 1 is 'original' size
*/
/**************************************************************************/
void display_drawChar(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t size)
{
	uint16_t prev_x = cursor_x, prev_y = cursor_y, prev_color = textcolor, prev_bg = textbgcolor;
	uint8_t  prev_size = textsize;

	display_setCursor(x, y);
	display_setTextSize(size);
	display_setTextColor(color, bg);
	display_putc(c);

	cursor_x    = prev_x;
	cursor_y    = prev_y;
	textcolor   = prev_color;
	textbgcolor = prev_bg;
	textsize    = prev_size;
}

/**************************************************************************/
/*!
    @brief  Set text cursor location
    @param  x    X coordinate in pixels
    @param  y    Y coordinate in pixels
*/
/**************************************************************************/
void display_setCursor(uint16_t x, uint16_t y)
{
	cursor_x = x;
	cursor_y = y;
}

/**************************************************************************/
/*!
    @brief  Get text cursor X location
    @returns    X coordinate in pixels
*/
/**************************************************************************/
uint16_t display_getCursorX(void)
{
	return cursor_x;
}

/**************************************************************************/
/*!
    @brief      Get text cursor Y location
    @returns    Y coordinate in pixels
*/
/**************************************************************************/
uint16_t display_getCursorY(void)
{
	return cursor_y;
}

/**************************************************************************/
/*!
    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
    @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
/**************************************************************************/
void display_setTextSize(uint8_t s)
{
	textsize = (s > 0) ? s : 1;
}

/**************************************************************************/
/*!
    @brief   Set text font color with custom background color
    @param   c   16-bit 5-6-5 Color to draw text with
    @param   b   16-bit 5-6-5 Color to draw background/fill with
*/
/**************************************************************************/
void display_setTextColor(uint16_t c, uint16_t b)
{
	textcolor   = c;
	textbgcolor = b;
}

/**************************************************************************/
/*!
    @brief      Whether text that is too long should 'wrap' around to the next line.
    @param  w Set true for wrapping, false for clipping
*/
/**************************************************************************/
void display_setTextWrap(bool w)
{
	wrap = w;
}

/**************************************************************************/
/*!
    @brief      Get rotation setting for display
    @returns    0 thru 3 corresponding to 4 cardinal rotations
*/
/**************************************************************************/
uint8_t display_getRotation(void)
{
	return rotation;
}

/**************************************************************************/
/*!
    @brief      Get width of the display, accounting for the current rotation
    @returns    Width in pixels
*/
/**************************************************************************/
uint16_t display_getWidth(void)
{
	return display_width;
}

/**************************************************************************/
/*!
    @brief      Get height of the display, accounting for the current rotation
    @returns    Height in pixels
*/
/**************************************************************************/
uint16_t display_getHeight(void)
{
	return display_height;
}

/**************************************************************************/
/*!
    @brief   Given 8-bit red, green and blue values, return a 'packed'
             16-bit color value in '565' RGB format (5 bits red, 6 bits
             green, 5 bits blue). This is just a mathematical operation,
             no hardware is touched.
    @param   red    8-bit red brightnesss (0 = off, 255 = max).
    @param   green  8-bit green brightnesss (0 = off, 255 = max).
    @param   blue   8-bit blue brightnesss (0 = off, 255 = max).
    @return  'Packed' 16-bit color value (565 format).
*/
/**************************************************************************/
uint16_t display_color565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((uint16_t)(red & 0xF8) << 8) | ((uint16_t)(green & 0xFC) << 3) | (blue >> 3);
}

/**************************************************************************/
/*!
   @brief     Draw a ROM-resident 1-bit image at the specified (x,y) position,
              using the specified foreground color (unset bits are transparent).
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Hieght of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void display_drawBitmapV1(uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color)
{
	uint16_t i, j;
	for (i = 0; i < h / 8; i++)
	{
		for (j = 0; j < w * 8; j++)
		{
			if (bitmap[j / 8 + i * w] & (1 << (j % 8)))
				display_drawPixel(x + j / 8, y + i * 8 + (j % 8), color);
		}
	}
}

/**************************************************************************/
/*!
   @brief     Draw a ROM-resident 1-bit image at the specified (x,y) position,
              using the specified foreground (for set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Hieght of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
/**************************************************************************/
void display_drawBitmapV1_bg(uint16_t       x,
                             uint16_t       y,
                             const uint8_t *bitmap,
                             uint16_t       w,
                             uint16_t       h,
                             uint16_t       color,
                             uint16_t       bg)
{
	uint16_t i, j;
	for (i = 0; i < h / 8; i++)
	{
		for (j = 0; j < w * 8; j++)
		{
			if (bitmap[j / 8 + i * w] & (1 << (j % 8)))
				display_drawPixel(x + j / 8, y + i * 8 + (j % 8), color);
			else
				display_drawPixel(x + j / 8, y + i * 8 + (j % 8), bg);
		}
	}
}

/**************************************************************************/
/*!
   @brief     Draw a ROM-resident 1-bit image at the specified (x,y) position,
              using the specified foreground color (unset bits are transparent).
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Hieght of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
*/
/**************************************************************************/
void display_drawBitmapV2(uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color)
{
	uint16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t  _byte     = 0;
	uint16_t i, j;

	for (j = 0; j < h; j++, y++)
	{
		for (i = 0; i < w; i++)
		{
			if (i & 7)
				_byte <<= 1;
			else
				_byte = bitmap[j * byteWidth + i / 8];
			if (_byte & 0x80)
				display_drawPixel(x + i, y, color);
		}
	}
}

/**************************************************************************/
/*!
   @brief     Draw a ROM-resident 1-bit image at the specified (x,y) position,
              using the specified foreground (for set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Hieght of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
/**************************************************************************/
void display_drawBitmapV2_bg(uint16_t       x,
                             uint16_t       y,
                             const uint8_t *bitmap,
                             uint16_t       w,
                             uint16_t       h,
                             uint16_t       color,
                             uint16_t       bg)
{
	uint16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t  _byte     = 0;
	uint16_t i, j;
	for (j = 0; j < h; j++, y++)
	{
		for (i = 0; i < w; i++)
		{
			if (i & 7)
				_byte <<= 1;
			else
				_byte = bitmap[j * byteWidth + i / 8];
			if (_byte & 0x80)
				display_drawPixel(x + i, y, color);
			else
				display_drawPixel(x + i, y, bg);
		}
	}
}

void display_double(char *text, int text_size, double v, int decimalDigits)
{
	int pow_ten_decimalDigits = pow(10, decimalDigits);
	int i                     = 1;
	int intPart, fractPart;
	for (int cnt = decimalDigits; cnt != 0; i *= 10, cnt--)
		;
	intPart   = (int)v;
	fractPart = (int)((v - (double)(int)v) * i);
	if (fractPart < 0)
		fractPart *= -1;

	if (v < 0 && intPart == 0)
	{
		snprintf(text, text_size, "-%d.%d", intPart, fractPart);
	}
	else if (fractPart < pow_ten_decimalDigits)
	{
		snprintf(text, text_size, "%d.%0*d", intPart, decimalDigits, fractPart);
	}
	else
	{
		snprintf(text, text_size, "%d.%d", intPart, fractPart);
	}
}

void display_slider(int x, int y, int lenght, int value)
{
	int radius = 4;
	display_drawLine(x, y, x + lenght, y, BLACK);
	display_drawLine(x, y + 1, x + lenght, y + 1, BLACK);
	int center = ((lenght - (radius / 2)) * value) / 100;
	display_fillCircle(x + center + (radius / 2), y, radius, BLACK);
}

void display_progressbar(int x, int y, int lenght, int value)
{
	// absolute values
	int center = (lenght * value) / 100;
	display_drawRect(x, y, lenght - x, 6, BLACK);
	display_fillRect(x, y, center, 5, BLACK);
}
// end of library code.