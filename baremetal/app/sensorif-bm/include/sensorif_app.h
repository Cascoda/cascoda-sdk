/*
 *  Copyright (c) 2019, Cascoda Ltd.
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

#ifndef SENSORIF_APP_H
#define SENSORIF_APP_H

/* measurement period in [ms] */
#define SENSORIF_MEASUREMENT_PERIOD 10000
/* delta in [ms] around measurement time */
/* needs to be increased of more devices are tested */
#define SENSORIF_MEASUREMENT_DELTA 100

/* report measurement times flag */
// #define SENSORIF_REPORT_TMEAS 1

/* flag which sensors to include in test */
#define SENSORIF_TEST_SI7021 0
#define SENSORIF_TEST_MAX30205 0
#define SENSORIF_TEST_LTR303ALS 0
#define SENSORIF_TEST_TMP102 0

/* Decimal mask for temperature sensor */
#define DECIMAL_MASK_FOUR_BITS 0x0F

/* functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Initialisation
 *******************************************************************************
 ******************************************************************************/
u8_t SENSORIF_Initialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Application Handler
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_Handler(struct ca821x_dev *pDeviceRef);
void SENSORIF_Handler_SI7021(void);
void SENSORIF_Handler_MAX30205(void);
void SENSORIF_Handler_LTR303ALS(void);
void SENSORIF_Handler_TMP102(void);

#endif // SENSORIF_APP_H
