/*********************************************************************
This is a header file for the gfx library

Copyright 2023 Cascoda LTD.
*********************************************************************/
#ifndef GFX_LIBRARY_H
#define GFX_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

/****** Function Declarations for gfx library  ******/

/**
 * \brief draw line
 *
 * draws a line
 *
 * \param x0 the lower x coordinate
 * \param y0 the lower y coordinate
 * \param x1 the higher x coordinate
 * \param y1 the higher y coordinate
 * \param color the color of the line (BLACK or WHITE)
 */
void display_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * \brief draw rectangle
 *
 * draws a rectangle
 *
 * \param x0 the x coordinate
 * \param y0 the y coordinate
 * \param w the width of the rectangle
 * \param h the height of the rectangle
 * \param color the color of the line (BLACK or WHITE)
 */
void display_drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * \brief draw filled rectangle
 *
 * draws a filled rectangle
 *
 * \param x0 the x coordinate
 * \param y0 the y coordinate
 * \param w the width of the rectangle
 * \param h the height of the rectangle
 * \param color the color of the line (BLACK or WHITE)
 */
void display_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * \brief draw circle
 *
 * draws a circle
 *
 * \param x0 the x coordinate
 * \param y0 the y coordinate
 * \param r the radius in pixels
 * \param color the color of the circle (BLACK or WHITE)
 */
void display_drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * \brief draw a filled circle
 *
 * draws a filled circle
 *
 * \param x0 the x coordinate
 * \param y0 the y coordinate
 * \param r the radius in pixels
 * \param color the color of the circle (BLACK or WHITE)
 */
void display_fillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
    \brief  Set text cursor location
    \param  x    X coordinate in pixels
    \param  y    Y coordinate in pixels
*/
void display_setCursor(uint16_t x, uint16_t y);

/**
    \brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
    \param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
void display_setTextSize(uint8_t s);

/**
    \brief   Set text font color with custom background color
    \param   c   BLACK or WHITE
    \param   b   BLACK or WHITE
*/
void display_setTextColor(uint16_t c, uint16_t b);

/**
    \brief  Print one byte/character of data
    \param  c  The 8-bit ascii character to write
    
    note use snprintf to format text
*/
void display_putc(uint8_t c);

/**
    \brief  Print a string to the display
    \param  c  The 8-bit ascii character to write
    
    note use snprintf to format text
    snprintf does not display floating numbers
    use display_double() to display floating numbers
*/
void display_puts(const uint8_t *s);

/**
    \brief  convert a float to a string
    \param  text  the text buffer to write in
    \param  text_size  the size of the text buffer
    \param  v  the floating number to display
    \param  decimalDigits  the decimal digit to display
*/
void display_double(char *text, int text_size, double v, int decimalDigits);

/**
    \brief  set the rotation
    prints starting at the cursor position.
    \param  rotation 0 == no rotation, 1 = 90 degrees
*/
void display_setRotation(uint16_t rotation);

/**
    \brief  Get text cursor X location
    \returns    X coordinate in pixels
*/
uint16_t getCursorX(void);

/**
    \brief      Get text cursor Y location
    \returns    Y coordinate in pixels
*/
uint16_t getCursorY(void);

/**
    \brief      Get display Width
    \returns    width in pixels
*/
uint16_t display_getWidth();

/**
    \brief      Get display Height
    \returns    Height in pixels
*/
uint16_t display_getHeight();

/**
    \brief      clears the frame buffer
    intializes the background on white.
*/
void display_clear(void);

/* GENERAL NOTES ON USAGE, and differences between using the 2.9inch (and MIKROE 1.54inch) vs 
1.54inch (waveshare) functions:
- It is simple to do any kind of displaying with the 2.9inch eink display. Simply call the
`display_render()` function, with the arguments of your choice. E.g.
  . To display the frame buffer using full update, with a clear happening before displaying, 
  call `display_render(FULL_UPDATE, WITH_CLEAR)`.
  . For partial update, without clear, call `display_render(PARTIAL_UPDATE, WITHOUT_CLEAR)`.
  That is all that is required.
- However, if using the 1.54inch display: 
  . To do a full update (always preceded with a clear), simply call the function `display_render_full()`.
  . To do a partial update, call the function `display_render_partial()`, with `true` or `false` as the argument depending
    on whether you want the eink display to be put in Deep Sleep mode after the update is done.
*/
#if (defined EPAPER_2_9_INCH || defined EPAPER_MIKROE_1_54_INCH)

/**
    \brief Displays the frame buffer onto the eink display, in 4 possible different ways,
    as expressed by the combination of the two parameters.
    \param updt_mode FULL_UPDATE or PARTIAL_UPDATE. This determines whether the frame
    buffer will be displayed via the full update (slow) or the partial update (fast)
    sequence.
    \param clr_mode WITH_CLEAR or WITHOUT_CLEAR. This determines whether the screen
    will be cleared before the frame buffer is displayed.
*/
void display_render(
#ifdef EPAPER_2_9_INCH
    SIF_IL3820_Update_Mode updt_mode,
    SIF_IL3820_Clear_Mode  clr_mode);
#elif defined EPAPER_MIKROE_1_54_INCH
    SIF_SSD1608_Update_Mode updt_mode,
    SIF_SSD1608_Clear_Mode  clr_mode);
#endif

#elif defined EPAPER_WAVESHARE_1_54_INCH

/**
    \brief Displays the frame buffer onto the eink display using full update (slow)
*/
void display_render_full(void);

/**
    \brief Displays the frame buffer onto the eink display using partial update (fast). 
    \param sleep_when_done If true, will set the eink display into Deep Sleep
    mode after the partial update is done.
*/
void display_render_partial(bool sleep_when_done);

#endif

/**
    \brief Displays the image provided, using full update (slow).
    \param image The image to be displayed.
*/
void display_fixed_image(const uint8_t *image);

// not documented
//void display_setTextWrap(bool w);
void display_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void display_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void display_drawRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);
void display_fillRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);

void display_drawBitmapV1(uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color);
void display_drawBitmapV1_bg(uint16_t       x,
                             uint16_t       y,
                             const uint8_t *bitmap,
                             uint16_t       w,
                             uint16_t       h,
                             uint16_t       color,
                             uint16_t       bg);
void display_drawBitmapV2(uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h, uint16_t color);
void display_drawBitmapV2_bg(uint16_t       x,
                             uint16_t       y,
                             const uint8_t *bitmap,
                             uint16_t       w,
                             uint16_t       h,
                             uint16_t       color,
                             uint16_t       bg);
void display_drawPixel(int16_t x, int16_t y, uint16_t color);

uint8_t display_getRotation();

void display_customChar(const uint8_t *c);
void display_drawChar(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t size);

void display_setContrast(uint32_t contrast);
void display_slider(int x, int y, int lenght, int value);
void display_progressbar(int x, int y, int lenght, int value);

#ifdef __cplusplus
}
#endif

#endif