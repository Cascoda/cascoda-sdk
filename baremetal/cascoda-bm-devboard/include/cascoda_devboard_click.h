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

#ifndef SENSOR_APP_H
#define SENSOR_APP_H

/* Decimal mask for temperature sensor */
#define DECIMAL_MASK_FOUR_BITS 0x0F
#define DECIMAL_MASK_TWO_BITS 0x03

typedef struct
{
	int dev_number;
	void (*sensor_handler)();
	uint8_t (*sensor_initialise)();
} mikrosdk_callbacks;

/* functions */

void select_sensor_comm(mikrosdk_callbacks *callback, int num, u32_t portnum, void (*handler)());
void select_sensor_gpio(mikrosdk_callbacks *callback, int num, u16_t p1, u16_t p2, void (*handler)());

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sensor Application Handler
 *******************************************************************************
 ******************************************************************************/
void MIKROSDK_Handler_AIRQUALITY4(void);
void MIKROSDK_Handler_ENVIRONMENT2(void);
void MIKROSDK_Handler_HVAC(void);
void MIKROSDK_Handler_MOTION(void);
void MIKROSDK_Handler_RELAY(void);
void MIKROSDK_Handler_THERMO(void);
void MIKROSDK_Handler_THERMO3(void);
void MIKROSDK_Handler_SHT(void);
void MIKROSDK_Handler_SPS30(void);
void SENSORIF_Handler_SI7021(void);
void SENSORIF_Handler_MAX30205(void);
void SENSORIF_Handler_LTR303ALS(void);
void SENSORIF_Handler_TMP102(void);
#endif // SENSOR_APP_H