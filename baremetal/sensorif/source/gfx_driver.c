/*********************************************************************
This is a Driver for the EINK display

Copyright 2023 Cascoda LTD.
*********************************************************************/

#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "gfx_driver.h"
#include "sif_il3820.h"

//#define WRITE_RAM 0x24

// the memory buffer for the LCD
static uint8_t frame_buffer[LCDWIDTH * LCDHEIGHT / 8];

// reduces how much is refreshed, which speeds it up!
// originally derived from Steve Evans/JCW's mod but cleaned up and
// optimized
//#define enablePartialUpdate

#ifdef enablePartialUpdate
static uint16_t xUpdateMin;
static uint16_t xUpdateMax;
static uint16_t yUpdateMin;
static uint16_t yUpdateMax;
#endif

static uint16_t rotation = 0;

uint8_t* get_framebuffer()
{
	return frame_buffer;
}

// needed for partial update
// not yet implemented
static void updateBoundingBox(uint16_t xmin, uint16_t ymin, uint16_t xmax, uint16_t ymax)
{
#ifdef enablePartialUpdate
	if (xmin < xUpdateMin)
		xUpdateMin = xmin;
	if (xmax > xUpdateMax)
		xUpdateMax = xmax;
	if (ymin < yUpdateMin)
		yUpdateMin = ymin;
	if (ymax > yUpdateMax)
		yUpdateMax = ymax;
		//SIF_IL3820_SetWindow(xmin, xmax, ymin, ymax);
#endif
	uint64_t address = 0;

	/*
     xmin,ymax            xmax (), ymax()   ^  
        line --------                     | j
                                          |
     xmin,ymin            xmax, ymin
            ---> i
     
     width  = x
     height = y (e.g. the longer side)
     
     width == xmax - xmin
     height = ymax - ymin
  */

	printf("updating bounding box xMin=%d yMin=%d xMax=%d yMax=%d\n", xmin, ymin, xmax, ymax);

	// SIF_IL3820_SetWindow(0, SIF_IL3820_WIDTH, 0, SIF_IL3820_HEIGHT);
	//SIF_IL3820_SetWindow(xmin, xmax, ymin, ymax);

	uint16_t distance = xmax - xmin;
	uint16_t width    = (distance % 8 == 0) ? (distance / 8) : (distance / 8 + 1);
	printf("updating bounding box width %d %d\n", distance, width);

	/*
    //per line
	SIF_IL3820_SetWindow(0, SIF_IL3820_WIDTH, 0, SIF_IL3820_HEIGHT);
	for (u16_t j = 0; j < height; j++)
	{
		SIF_IL3820_SetCursor(0, j);
		SIF_IL3820_SendCommand(WRITE_RAM);
		for (u16_t i = 0; i < width; i++)
		{
			SIF_IL3820_SendData(0xFF);
		}
	}
*/

	//for (uint16_t j = yUpdateMin; j < yUpdateMax; j++)
	for (uint16_t j = ymin; j < ymax; j++)
	{
		//SIF_IL3820_SetCursor(xmin, j);
		// SIF_IL3820_SendCommand(WRITE_RAM);
		for (uint16_t i = xmin; i < xmin + width; i++)
		{
			address = i + j * width;
			//printf("gfx_drv_drawPixel address %d %d %d %d\n", i, j, width, address);
			//SIF_IL3820_SendData(frame_buffer[address]);
		}
	}
	//SIF_IL3820_TurnOnDisplay();
}

// the most basic function, set a single pixel
void gfx_drv_drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x < 0) || (x >= LCDWIDTH) || (y < 0) || (y >= LCDHEIGHT))
		return;

	int16_t t;
	switch (rotation)
	{
	case 1:
		t = x;
		x = y;
		y = LCDHEIGHT - 1 - t;
		break;
	case 2:
		x = LCDWIDTH - 1 - x;
		y = LCDHEIGHT - 1 - y;
		break;
	case 3:
		t = x;
		x = LCDWIDTH - 1 - y;
		y = t;
		break;
	}

	if ((x < 0) || (x >= LCDWIDTH) || (y < 0) || (y >= LCDHEIGHT))
		return;

	if (color)
		frame_buffer[(x + (y * LCDWIDTH)) / 8] |= (1 << (7 - (x % 8)));
	else
		frame_buffer[(x + (y * LCDWIDTH)) / 8] &= ~(1 << (7 - (x % 8)));
}

// the most basic function, get a single pixel
uint8_t gfx_drv_getPixel(int16_t x, int16_t y)
{
	if ((x < 0) || (x >= LCDWIDTH) || (y < 0) || (y >= LCDHEIGHT))
		return 0;

	return ((frame_buffer[x + (y / 8) * LCDWIDTH] >> (y % 8)) & 0x1);
}

// clear everything
void gfx_drv_clearDisplay(void)
{
	ca_log_note("gfx_drv_clearDisplay");
	//white = 0xFF
	//black = 0x00
	memset(frame_buffer, 0xFF, LCDWIDTH * LCDHEIGHT / 8);
	// check the first 8 pixels
	ca_log_note("  frame_buffer[0] %d\n", frame_buffer[0]);
}

// set the rotation for drawing
void gfx_drv_setRotation(uint16_t new_rotation)
{
	rotation = new_rotation;
}
