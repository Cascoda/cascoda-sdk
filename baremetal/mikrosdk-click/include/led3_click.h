/**
 * @file
 * @brief mikrosdk interface
 */
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
/*
 * Example click interface driver
*/

#ifndef LED3_CLICK_H
#define LED3_CLICK_H

#include <stdint.h>

/**
 * @brief Maximum length[bytes] for I2C led3 slave address + memory address (command).
 * 
 */
#define LED3_ADD_CMD_LEN 2

/**
 * @brief Defines for tx_buf array used in set_rgb and set_colour functions.
 * 
 */
#define TX_BUF_RED 0
#define TX_BUF_GREEN 1
#define TX_BUF_BLUE 2

/**
 * @brief Minimum and maximum values allowed for each colour.
 * @details Each colour (r,g,b) is controlled by a 5-bit PWM signal, hence a total of 32768 colours is available.
 * 
 */
#define RED_MIN 0x40
#define RED_MAX 0x5F
#define GREEN_MIN 0x60
#define GREEN_MAX 0x7F
#define BLUE_MIN 0x80
#define BLUE_MAX 0x9F

#define LED3_CMD_SHUT_DOWN 0x00

/**
 * @brief Defines for commonly used colours.
 * 
 */
#define LED3_COLOUR_RED 0x5F6080
#define LED3_COLOUR_ORANGE 0x5F6280
#define LED3_COLOUR_YELLOW 0x5F7480
#define LED3_COLOUR_GREEN 0x407F80
#define LED3_COLOUR_BLUE 0x40609F
#define LED3_COLOUR_WHITE 0x5F7F9F
#define LED3_COLOUR_PURPLE 0x58609C

/**
 * @brief Time intervals for dimming function (31 total combinations available).
 * @details The time interval could be added by using bitwise OR. E.g, for a time interval of 24ms, use (LED3_TIMER_8ms | LED3_TIMER_16ms).
 * 
 */
#define LED3_TIMER_8ms 0xE1
#define LED3_TIMER_16ms 0xE2
#define LED3_TIMER_32ms 0xE4
#define LED3_TIMER_64ms 0xE8
#define LED3_TIMER_128ms 0xE0

/**
 * @brief Defines for setting the light intensity of the LED (MIN = 1, MAX = 31).
 * @details The intensity could be added by using bitwise OR. E.g, for an intensity of 12, use (LED3_INTENSITY_4 | LED3_INTENSITY_8).
 * 
 */
#define LED3_INTENSITY_1 0x01
#define LED3_INTENSITY_2 0x02
#define LED3_INTENSITY_4 0x04
#define LED3_INTENSITY_8 0x08
#define LED3_INTENSITY_16 0x10
#define LED3_INTENSITY_MAX 0x1F

/**
 * @brief Defines for the dimming function.
 * @details LED3_INCREMENT and LED3_DECREMENT will increase/decrease the light intensity until the specified intensity value. LED3_CONSTANT will keep the light intensity at a constant value.
 * 
 */
#define LED3_INCREMENT 0xA0
#define LED3_DECREMENT 0xC0
#define LED3_CONSTANT 0x20

/**
 * @brief Enum (LED3 status).
 * 
 */
enum led3_status
{
	LED3_ST_OK   = 0,
	LED3_ST_FAIL = 1
};

/* new functions */
/**
 * @brief Function that sets the dimming mode (Increment, Decrement, Constant) and the light intensity.
 * 
 * @param cmd Bitwise OR of dimming mode and light intensity. E.g, to use constant brightness with max intensity, use (LED3_CONSTANT | LED3_INTENSITY_MAX).
 * @return uint8_t 
 */
uint8_t MIKROSDK_LED3_set_dimming_and_intensity(uint8_t cmd);

/**
 * @brief Function that sets the rgb value individually.
 * 
 * @param red Red colour value (MIN = 0x40, MAX = 0x5F).
 * @param green Green colour value (MIN = 0x60, MAX = 0x7F).
 * @param blue Blue colour value (MIN = 0x80, MAX = 0x9F).
 * @return uint8_t 
 */
uint8_t MIKROSDK_LED3_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Function that sets the LED using common colours.
 * 
 * @param colour Defined commonly used colours. E.g, LED3_COLOUR_PURPLE.
 * @return uint8_t 
 */
uint8_t MIKROSDK_LED3_set_colour(uint32_t colour);

/**
 * @brief Function that turns off the LED.
 * 
 * @return uint8_t 
 */
uint8_t MIKROSDK_LED3_shut_down(void);

/**
 * @brief Function that sets the time interval for the dimming function.
 * 
 * @param time Time interval for dimming. If set to 32ms, the LED will dim to the specified intensity at 32ms per intensity step. E.g, current intensity = 5, new intensity = 15, the total dimming sequence will take 320ms.
 * @return uint8_t 
 */
uint8_t MIKROSDK_LED3_set_timer(uint8_t time);

/**
 * @brief Function that initialises the I2C configuration of the LED3 click.
 * 
 * @return uint8_t 
 */
uint8_t MIKROSDK_LED3_Initialise(void);

#endif // LED3_CLICK_H