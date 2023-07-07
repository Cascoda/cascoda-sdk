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

#ifndef MIKROSDK_APP_H
#define MIKROSDK_APP_H

/* measurement period in [ms] */
#define MIKROSDK_MEASUREMENT_PERIOD 10000
/* delta in [ms] around measurement time */
/* needs to be increased of more devices are tested */
#define MIKROSDK_MEASUREMENT_DELTA 100

/* report measurement times flag */
#define MIKROSDK_REPORT_TMEAS 1

/* flag which sensors to include in test */
#define MIKROSDK_TEST_AIRQUALITY4 0
#define MIKROSDK_TEST_ENVIRONMENT2 0
#define MIKROSDK_TEST_HVAC 0
#define MIKROSDK_TEST_MOTION 0
#define MIKROSDK_TEST_RELAY 0
#define MIKROSDK_TEST_THERMO 0
#define MIKROSDK_TEST_THERMO3 1
#define MIKROSDK_TEST_SHT 0

/* Decimal mask for temperature sensor */
#define DECIMAL_MASK_FOUR_BITS 0x0F
#define DECIMAL_MASK_TWO_BITS 0x03

/* Float to int conversion to required decimal places*/
#define FLOAT_TO_INT_CONVERT 1

/* functions */
/******************************************************************************/
/***************************************************************************/ /**
 * \brief MIKROSDK Initialisation
 *******************************************************************************
 ******************************************************************************/
uint8_t MIKROSDK_Initialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MIKROSDK Application Handler
 *******************************************************************************
 ******************************************************************************/
void MIKROSDK_Handler(struct ca821x_dev *pDeviceRef);

void MIKROSDK_Handler_AIRQUALITY4(void);
void MIKROSDK_Handler_ENVIRONMENT2(void);
void MIKROSDK_Handler_HVAC(void);
void MIKROSDK_Handler_MOTION(void);
void MIKROSDK_Handler_RELAY(void);
void MIKROSDK_Handler_THERMO(void);
void MIKROSDK_Handler_THERMO3(void);
void MIKROSDK_Handler_SHT(void);
#endif // MIKROSDK_APP_H
