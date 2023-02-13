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
 * Sensor interface for Silabs Si7021 temperature / humidity sensor
*/

#ifndef DEVBOARD_SENSORIF_H
#define DEVBOARD_SENSORIF_H

/* Decimal mask for temperature sensor */
#define DECIMAL_MASK_FOUR_BITS 0x0F
#define DECIMAL_MASK_TWO_BITS 0x03

/* devboard comms interface configuration */
#define I2C_PORTNUM 1 // interface port for I2C
#define SPI_PORTNUM 1 // interface port for SPI

/* devboard pin assignments (chili2 pins) for sensor interface */
/* at the moment same mapping as mikrobus click */
#define SENSORIF_AN_PIN 36
#define SENSORIF_RST_PIN 6
#define SENSORIF_CS_PIN 34
#define SENSORIF_SCK_PIN 33
#define SENSORIF_MISO_PIN 32
#define SENSORIF_MOSI_PIN 31
#define SENSORIF_PWM_PIN 35
#define SENSORIF_INT_PIN 5
// Tx (UART) not connected
// Rx (UART) not connected
#define SENSORIF_SCL_PIN 2
#define SENSORIF_SDA_PIN 4

typedef enum dvbd_sensorif_type
{
	STYPE_NONE      = 0,
	STYPE_SI7021    = 1,
	STYPE_LTR303ALS = 2,
	STYPE_MAX30205  = 3,
	STYPE_TMP102    = 4,
} dvbd_sensorif_type;

typedef struct
{
	dvbd_sensorif_type dev_type;
	void (*sensorif_handler)();
	uint8_t (*sensorif_initialise)();
} sensorif_callbacks;

/* functions */

void select_sensorif(sensorif_callbacks *callback, dvbd_sensorif_type dev_type, void (*handler)());

/* application handlers */
void SENSORIF_Handler_SI7021(void);
void SENSORIF_Handler_LTR303ALS(void);
void SENSORIF_Handler_MAX30205(void);
void SENSORIF_Handler_TMP102(void);

#endif // DEVBOARD_SENSORIF_H
