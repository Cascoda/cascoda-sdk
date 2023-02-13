/****************************************************************************
  Interfacing PIC18F46K22 microcontroller with Nokia 5110 (3310) LCD display.
  Graphics test example.
  C Code for mikroC PRO for PIC compiler.
  Internal oscillator used @ 16MHz
  Configuration words: CONFIG1H = 0x0028
                       CONFIG2L = 0x0018
                       CONFIG2H = 0x003C
                       CONFIG3H = 0x0037
                       CONFIG4L = 0x0081
                       CONFIG5L = 0x000F
                       CONFIG5H = 0x00C0
                       CONFIG6L = 0x000F
                       CONFIG6H = 0x00E0
                       CONFIG7L = 0x000F
                       CONFIG7H = 0x0040
  This is a free software with NO WARRANTY.
  https://simple-circuit.com/

*****************************************************************************
  This is an example sketch for our Monochrome Nokia 5110 LCD Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/338

  These displays use SPI to communicate, 4 or 5 pins are required to
  interface

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada  for Adafruit Industries.
  BSD license, check license.txt for more information
  All text above, and the splash screen must be included in any redistribution

  Modified by Cascoda for EINK displays and running on CHILI
  Copyright 2023 Cascoda LTD.
*****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "cascoda-bm/test15_4_evbme.h"
#include "sif_il3820.h"

#include "gfx_driver.h"  // include  controller driver source code
#include "gfx_library.h" // include graphics library header

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTA_Y 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16
const char logo16_glcd_bmp[] = {0b00000000, 0b11000000, 0b00000001, 0b11000000, 0b00000001, 0b11000000, 0b00000011,
                                0b11100000, 0b11110011, 0b11100000, 0b11111110, 0b11111000, 0b01111110, 0b11111111,
                                0b00110011, 0b10011111, 0b00011111, 0b11111100, 0b00001101, 0b01110000, 0b00011011,
                                0b10100000, 0b00111111, 0b11100000, 0b00111111, 0b11110000, 0b01111100, 0b11110000,
                                0b01110000, 0b01110000, 0b00000000, 0b00110000};

static ca_tasklet testTasklet;

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

void testdrawbitmap(const uint8_t* bitmap, uint8_t w, uint8_t h)
{
	uint8_t icons[NUMFLAKES][3];
	uint8_t f;
	uint8_t x_pos;
	uint8_t y_pos;

	// initialize
	for (f = 0; f < NUMFLAKES; f++)
	{
		icons[f][XPOS]    = rand();
		icons[f][YPOS]    = 0;
		icons[f][DELTA_Y] = (rand() % 5) + 1;
	}
	icons[0][XPOS] = 30;

	while (1)
	{
		// draw each icon
		for (f = 0; f < NUMFLAKES; f++)
		{
			x_pos = icons[f][XPOS];
			y_pos = icons[f][YPOS];
			display_drawBitmapV2(x_pos, y_pos, bitmap, w, h, BLACK);
		}
		display_render();

		// then erase it + move it
		for (f = 0; f < NUMFLAKES; f++)
		{
			x_pos = icons[f][XPOS];
			y_pos = icons[f][YPOS];
			display_drawBitmapV2(x_pos, y_pos, bitmap, w, h, WHITE);
			// move it
			icons[f][YPOS] += icons[f][DELTA_Y];
			// if its gone, reinit
			if (icons[f][YPOS] > display_getHeight())
			{
				icons[f][XPOS]    = rand();
				icons[f][YPOS]    = 0;
				icons[f][DELTA_Y] = (rand() % 5) + 1;
			}
		}
	}
}

void testdrawchar(void)
{
	char i;
	display_setTextSize(1);
	display_setTextColor(BLACK, WHITE);
	display_setCursor(0, 0);

	for (i = 0; i < 168; i++)
	{
		if (i == '\n' || i == '\r')
			continue;
		display_putc(i);
	}
	display_render();
}

void testdrawcircle(void)
{
	uint8_t i;
	int     display_width  = display_getWidth();
	int     display_height = display_getHeight();
	for (i = 0; i < display_height; i += 2)
	{
		display_drawCircle(display_width / 2, display_height / 2, i, BLACK);
		display_render();
	}
}

void testfillrect(void)
{
	uint8_t i;
	uint8_t color          = 1;
	int     display_width  = display_getWidth();
	int     display_height = display_getHeight();
	for (i = 0; i < display_height / 2; i += 3)
	{
		// alternate colors
		display_fillRect(i, i, display_width - (i * 2), display_height - (i * 2), color % 2);
		display_render();
		color++;
	}
}

void testdrawtriangle(void)
{
	uint8_t i;
	int     display_width  = display_getWidth();
	int     display_height = display_getHeight();
	for (i = 0; i < min(display_width, display_height) / 2; i += 5)
	{
		display_clear();
		display_drawTriangle(display_width / 2,
		                     (display_height / 2) - i,
		                     (display_width / 2) - i,
		                     (display_height / 2) + i,
		                     (display_width / 2) + i,
		                     (display_height / 2) + i,
		                     BLACK);
		display_render();
	}
}

void testfilltriangle(void)
{
	uint16_t i;
	uint16_t color          = BLACK;
	int      display_width  = display_getWidth();
	int      display_height = display_getHeight();
	display_clear();
	for (i = min(display_width, display_height) / 2; i > 0; i -= 5)
	{
		display_fillTriangle(display_width / 2,
		                     (display_height / 2) - i,
		                     (display_width / 2) - i,
		                     (display_height / 2) + i,
		                     (display_width / 2) + i,
		                     (display_height / 2) + i,
		                     color);
		if (color == WHITE)
			color = BLACK;
		else
			color = WHITE;
		display_render();
	}
}

void testdrawroundrect(void)
{
	uint8_t i;
	int     display_width  = display_getWidth();
	int     display_height = display_getHeight();
	for (i = 0; i < display_height / 2 - 2; i += 2)
	{
		display_clear();
		display_drawRoundRect(i, i, display_width - (2 * i), display_height - (2 * i), display_height / 4, BLACK);
		display_render();
	}
}

void testfillroundrect(void)
{
	uint8_t i;
	uint8_t color          = BLACK;
	int     display_width  = display_getWidth();
	int     display_height = display_getHeight();
	for (i = 0; i < display_height / 2 - 2; i += 2)
	{
		display_clear();
		display_fillRoundRect(i, i, display_width - (2 * i), display_height - (2 * i), display_height / 4, color);
		if (color == WHITE)
			color = BLACK;
		else
			color = WHITE;
		display_render();
	}
}

void testdrawrect(void)
{
	uint8_t i;
	int     display_width  = display_getWidth();
	int     display_height = display_getHeight();
	ca_log_note("testdrawrect");

	for (i = 0; i < display_height / 2; i += 2)
	{
		display_clear();
		display_drawRect(i, i, display_width - (2 * i), display_height - (2 * i), BLACK);

		ca_log_note("testdrawrect - %d", i);
		display_render();
	}
	ca_log_note("testdrawrect - done");
}

void testdrawline(void)
{
	uint16_t i;
	int      display_width  = display_getWidth();
	int      display_height = display_getHeight();

	ca_log_note("testdrawline");

	display_clear();
	for (i = 0; i < display_width; i += 4)
	{
		display_drawLine(0, 0, i, display_height - 1, BLACK);
	}
	for (i = 0; i < display_height; i += 4)
	{
		display_drawLine(0, 0, display_width - 1, i, BLACK);
	}

	ca_log_note("testdrawline: 1");
	display_render();

	display_clear();
	for (i = 0; i < display_width; i += 4)
	{
		display_drawLine(0, display_height - 1, i, 0, BLACK);
	}
	for (i = display_height - 1; i >= 0; i -= 4)
	{
		display_drawLine(0, display_height - 1, display_width - 1, i, BLACK);
	}

	ca_log_note("testdrawline: 2");
	display_render();

	display_clear();
	for (i = display_width - 1; i >= 0; i -= 4)
	{
		display_drawLine(display_width - 1, display_height - 1, i, 0, BLACK);
	}
	for (i = display_height - 1; i >= 0; i -= 4)
	{
		display_drawLine(display_width - 1, display_height - 1, 0, i, BLACK);
	}
	ca_log_note("testdrawline: 3");
	display_render();

	display_clear();
	for (i = 0; i < display_height; i += 4)
	{
		display_drawLine(display_width - 1, 0, 0, i, BLACK);
	}
	for (i = 0; i < display_width; i += 4)
	{
		display_drawLine(display_width - 1, 0, i, display_height - 1, BLACK);
	}

	ca_log_note("testdrawline: 2");
	display_render();
	ca_log_note("testdrawline: done");
}

ca_error handle_tests(void* args)
{
	char text[100];
	ca_log_note("===================================================");
	ca_log_note("Clearing display");
	display_clear(); // clears the frame buffer

	int display_width  = display_getWidth();
	int display_height = display_getHeight();
	ca_log_note(" width height = %d, %d", display_width, display_height);
	ca_log_note("draw graphics");
	display_drawRect(10, 10, display_width - 20, display_height - 20, BLACK);
	display_drawCircle(20, 20, 20, BLACK);
	display_fillCircle(40, 40, 20, BLACK);

	ca_log_note("draw qr codes");
	SIF_IL3820_overlay_qr_code_scale("https://www.cascoda.com", get_framebuffer(), 2, 65, 30);

	SIF_IL3820_overlay_qr_code_scale("https://www.cascoda.com", get_framebuffer(), 1, 65, 150);

	display_setTextColor(BLACK, WHITE);
	ca_log_note("draw text");
	display_setCursor(100, 100);
	display_puts("Rotation\r\n");
	display_setTextSize(2);
	display_puts("Example!\r\n");

	ca_log_note("draw via snprintf");
	display_setCursor(10, 10);
	display_setRotation(1);
	display_setTextSize(4);
	snprintf(text, 99, " %d", 100);
	printf(text);
	display_puts(text);
	float f_x = 15.3;
	display_double(text, 99, f_x, 1);
	printf(text);
	display_setCursor(10, 50);
	display_puts(text);
	display_setTextSize(1);
	f_x = -21.8;
	display_double(text, 99, f_x, 1);
	display_puts(text);

	display_render();

	ca_log_note("done>===================================================");
	return CA_ERROR_SUCCESS;
}

// main function
void main(void)
{
	struct ca821x_dev dev;
	ca821x_api_init(&dev);
	SENSORIF_SPI_Config(1);
	//SIF_IL3820_overlay_qr_code("https://www.cascoda.com", cascoda_img_2in9, 90, 20);

	/* Initialisation of Chip and EVBME */
	/* Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action */
	/* in case there is no UpStream Communications Channel available */
	EVBMEInitialise(CA_TARGET_NAME, &dev);

	ca_log_note("=====initalize==================");
	SIF_IL3820_Initialise(&lut_full_update);
	ca_log_note("=====Deinitalize==================");
	SIF_IL3820_Deinitialise();
	/* Application-Specific Initialisation Routines */

	SIF_IL3820_Initialise(&lut_full_update);
	ca_log_note("=====INIT done==================");
	ca_log_note("schedule task (5 sec)");
	/* Insert Application-Specific Initialisation Routines here */
	TASKLET_Init(&testTasklet, &handle_tests);
	TASKLET_ScheduleDelta(&testTasklet, 5712, NULL);

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);
	} /* while(1) */
}

// end of code.