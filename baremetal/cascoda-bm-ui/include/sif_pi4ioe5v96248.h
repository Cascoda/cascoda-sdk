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
 * Sensor interface for Diodes/Pericom PI4IOE5V96248 48-bit gpio expander
 * (Currently limited to 8 (port 0)
*/

#ifndef SIF_PI4IOE5V96248_H
#define SIF_PI4IOE5V96248_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define I2C_PORTNUM 1 // CLICK interface port for I2C

/* interrupt pin */
#define SIF_PI4IOE5V96248_INT_PIN 5

/* reset pin */
#define SIF_PI4IOE5V96248_RST_PIN 6

/* pin masks */
#define SIF_PI4IOE5V96248_NO_PIN_MASK 0x00
#define SIF_PI4IOE5V96248_PIN_0_MASK 0x01
#define SIF_PI4IOE5V96248_PIN_1_MASK 0x02
#define SIF_PI4IOE5V96248_PIN_2_MASK 0x04
#define SIF_PI4IOE5V96248_PIN_3_MASK 0x08
#define SIF_PI4IOE5V96248_PIN_4_MASK 0x10
#define SIF_PI4IOE5V96248_PIN_5_MASK 0x20
#define SIF_PI4IOE5V96248_PIN_6_MASK 0x40
#define SIF_PI4IOE5V96248_PIN_7_MASK 0x80
#define SIF_PI4IOE5V96248_ALL_PINS_MASK 0xFF

/* port numbers (note: only port 0 used in implementation */
#define SIF_PI4IOE5V96248_PORT_0 0x00
#define SIF_PI4IOE5V96248_PORT_1 0x01
#define SIF_PI4IOE5V96248_PORT_2 0x02
#define SIF_PI4IOE5V96248_PORT_3 0x03
#define SIF_PI4IOE5V96248_PORT_4 0x04
#define SIF_PI4IOE5V96248_PORT_5 0x05

/*  i2c address */
#define SIF_PI4IOE5V96248_I2C_ADDR 0x20

/* timing parameters [ms] */
#define SIF_PI4IOE5V96248_T_RESET 1 /* 1 ms for reset */

/* interrupt active low */
enum expand13_alarm_state
{
	SIF_PI4IOE5V96248_INT_TRIGGERED = 0,
	SIF_PI4IOE5V96248_INT_CLEARED   = 1
};

/* data acquisition status */
enum expand13_status
{
	SIF_PI4IOE5V96248_ST_OK   = 0, /* read values ok */
	SIF_PI4IOE5V96248_ST_FAIL = 3  /* acquisition failed */
};

/* new functions */
uint8_t SIF_PI4IOE5V96248_alarm_triggered(void);
uint8_t SIF_PI4IOE5V96248_Initialise(void);
uint8_t SIF_PI4IOE5V96248_Acquire(uint8_t *port0);
uint8_t SIF_PI4IOE5V96248_SetOutput(uint8_t io, uint8_t val);
uint8_t SIF_PI4IOE5V96248_Sense(uint8_t io, uint8_t *val);

#endif // SIF_PI4IOE5V96248_H
