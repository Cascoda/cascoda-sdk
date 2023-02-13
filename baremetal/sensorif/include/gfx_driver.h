/*********************************************************************
This is a header file for the gfx driver

Copyright 2023 Cascoda LTD.
*********************************************************************/
#ifndef GFX_DRIVER_H
#define GFX_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sif_il3820.h"

#define BLACK 0
#define WHITE 1

#define LCDWIDTH SIF_IL3820_WIDTH
#define LCDHEIGHT SIF_IL3820_HEIGHT

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
