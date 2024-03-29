/*********************************************************************
This is a header file for the gfx driver

Copyright 2023 Cascoda LTD.
*********************************************************************/
#ifndef GFX_DRIVER_H
#define GFX_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EPAPER_2_9_INCH
#include "sif_il3820.h"

#define LCDWIDTH SIF_IL3820_WIDTH
#define LCDHEIGHT SIF_IL3820_HEIGHT
#elif defined EPAPER_WAVESHARE_1_54_INCH
#include "sif_ssd1681.h"

#define LCDWIDTH SIF_SSD1681_WIDTH
#define LCDHEIGHT SIF_SSD1681_HEIGHT
#elif defined EPAPER_MIKROE_1_54_INCH
#include "sif_ssd1608.h"

#define LCDWIDTH SIF_SSD1608_WIDTH
#define LCDHEIGHT SIF_SSD1608_HEIGHT
#endif

#define BLACK 0
#define WHITE 1

// retrieve the frame buffer
uint8_t* get_framebuffer(void);
// clear the frame buffer
void gfx_drv_clearDisplay(void);
// draw pixel in the frame buffer
void gfx_drv_drawPixel(int16_t x, int16_t y, uint16_t color);
// set the low level rotation
void gfx_drv_setRotation(uint16_t rotation);

#ifdef __cplusplus
}
#endif

#endif
