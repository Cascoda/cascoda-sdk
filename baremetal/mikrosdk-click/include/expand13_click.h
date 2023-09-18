/**
 * @file
 * @brief mikrosdk interface
 */
/*
 *  Copyright (c) 2022, Cascoda Ltd.
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

#ifndef EXPAND13_CLICK_H
#define EXPAND13_CLICK_H

#include <stdint.h>

/* use alarm pin as interrupt (1) with continuous mode instead of polling (0) with one-shot mode */
#define EXPAND13_USE_INTERRUPT 1

/* pin masks */
#define EXPAND13_NO_PIN_MASK 0x00
#define EXPAND13_PIN_0_MASK 0x01
#define EXPAND13_PIN_1_MASK 0x02
#define EXPAND13_PIN_2_MASK 0x04
#define EXPAND13_PIN_3_MASK 0x08
#define EXPAND13_PIN_4_MASK 0x10
#define EXPAND13_PIN_5_MASK 0x20
#define EXPAND13_PIN_6_MASK 0x40
#define EXPAND13_PIN_7_MASK 0x80
#define EXPAND13_ALL_PINS_MASK 0xFF

/* port numbers (note: only port 0 used in implementation */
#define EXPAND13_PORT_0 0x00
#define EXPAND13_PORT_1 0x01
#define EXPAND13_PORT_2 0x02
#define EXPAND13_PORT_3 0x03
#define EXPAND13_PORT_4 0x04
#define EXPAND13_PORT_5 0x05

/*  i2c address */
#define EXPAND13_I2C_ADDR 0x20

/* timing parameters [ms] */
#define EXPAND13_T_RESET 1 /* 1 ms for reset */

/* interrupt active low */
enum expand13_alarm_state
{
	EXPAND13_INT_TRIGGERED = 0,
	EXPAND13_INT_CLEARED   = 1
};

/* data acquisition status */
enum expand13_status
{
	EXPAND13_ST_OK   = 0, /* read values ok */
	EXPAND13_ST_FAIL = 3  /* acquisition failed */
};

/* new functions */
void    MIKROSDK_EXPAND13_pin_mapping(uint8_t reset, uint8_t irq);
uint8_t MIKROSDK_EXPAND13_alarm_triggered(void);
uint8_t MIKROSDK_EXPAND13_Initialise(void);
uint8_t MIKROSDK_EXPAND13_Acquire(uint8_t *port0);
uint8_t MIKROSDK_EXPAND13_SetOutput(uint8_t io, uint8_t val);
uint8_t MIKROSDK_EXPAND13_SenseInput(uint8_t io, uint8_t *val);

#endif // EXPAND13_CLICK_H
