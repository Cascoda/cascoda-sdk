/**
 * @file
 * @brief test15_4 main program loop and supporting functions
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
 * Example application for external sensor interfaces
*/
#include <stdio.h>

#include "cascoda-bm/cascoda_sensorif.h"

#include "devboard_btn.h"
#include "devboard_sensorif.h"
#include "sif_ltr303als.h"
#include "sif_max30205.h"
#include "sif_si7021.h"
#include "sif_tmp102.h"

void select_sensorif(sensorif_callbacks *callback, dvbd_sensorif_type dev_type, void (*handler)())
{
	switch (dev_type)
	{
	case STYPE_NONE:
		callback->sensorif_initialise = NULL;
		break;

	case STYPE_SI7021:
		SENSORIF_I2C_Config(I2C_PORTNUM);
		callback->sensorif_initialise = NULL;
		break;

	case STYPE_LTR303ALS:
		SENSORIF_I2C_Config(I2C_PORTNUM);
		callback->sensorif_initialise = SIF_LTR303ALS_Initialise;
		break;

	case STYPE_MAX30205:
		SENSORIF_I2C_Config(I2C_PORTNUM);
		callback->sensorif_initialise = SIF_MAX30205_Initialise;
		break;

	case STYPE_TMP102:
		SENSORIF_I2C_Config(I2C_PORTNUM);
		callback->sensorif_initialise = SIF_TMP102_Initialise;
		break;

	default:
		printf("Invalid Device Type: %d\n", dev_type);
		callback->sensorif_handler = NULL;
		return;
	}
	callback->dev_type         = dev_type;
	callback->sensorif_handler = handler;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Handler Example for SI7021
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_Handler_SI7021(void)
{
	u8_t id = 0;
	u8_t temp;
	u8_t hum;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	printf("-----------------------\n");
	printf("Device : SI7021\n");
	SENSORIF_I2C_Init();                 /* enable interface */
	id   = SIF_SI7021_ReadID();          /* read SI7021 ID */
	temp = SIF_SI7021_ReadTemperature(); /* read SI7021 temperature */
	hum  = SIF_SI7021_ReadHumidity();    /* read SI7021 humidity */
	SENSORIF_I2C_Deinit();               /* disable interface */
	printf("ID=%02X", id);
	printf("; Temperature=");
	if (temp & 0x80) /* 2s complement for temperature */
	{
		printf("-");
		temp = ~temp + 1;
	}
	printf("%u'C", temp);
	printf("; Humidity=");
	printf("%u%c", hum, 37);
}

// /******************************************************************************/
// /***************************************************************************/ /**
//  * \brief SENSORIF Handler Example for MAX30205
//  *******************************************************************************
//  ******************************************************************************/
void SENSORIF_Handler_MAX30205(void)
{
	u16_t temp;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	printf("-----------------------\n");
	printf("Device : MAX30205\n");

	SENSORIF_I2C_Init(); /* enable interface */

	temp = SIF_MAX30205_ReadTemperature(); /* read MAX30205 temperature */

	SENSORIF_I2C_Deinit(); /* disable interface */

	printf("; Temperature=");
	if (temp & 0x8000) /* 2s complement for temperature */
	{
		printf("-");
		temp = ~temp + 1;
	}
	/* display 'C and 3 digits of 'C */
	printf("%u.%03u'C", ((temp >> 8) & 0xFF), (((u32_t)(temp & 0x00FF) * 1000) / 256));
}

// /******************************************************************************/
// /***************************************************************************/ /**
//  * \brief SENSORIF Handler Example for LTR303ALS
//  *******************************************************************************
//  ******************************************************************************/
void SENSORIF_Handler_LTR303ALS(void)
{
	u16_t light_ch0 = 0;
	u16_t light_ch1 = 0;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	printf("-----------------------\n");
	printf("Device : LTR303ALS\n");
	SENSORIF_I2C_Init(); /* enable interface */
	/* read LTR303ALS light measurement for both channels */
	SIF_LTR303ALS_ReadLight(&light_ch0, &light_ch1);
	SENSORIF_I2C_Deinit(); /* disable interface */
	printf("; Light Ch0 (visible) = 0x%04X; Ch1 (IR) = 0x%04X", light_ch0, light_ch1);
}

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief SENSORIF Handler Example for TMP102
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_Handler_TMP102(void)
{
	u16_t temp;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	printf("-----------------------\n");
	printf("Device : TMP102\n");
	SENSORIF_I2C_Init();                 /* enable interface */
	temp = SIF_TMP102_ReadTemperature(); /* read TMP102 temperature */
	SENSORIF_I2C_Deinit();               /* disable interface */
	printf("; Temperature=");

	if (temp & 0x8000) /* 2s complement for temperature */
	{
		printf("-");
		temp = ~temp + 1;
	}

	/* display 'C and 4 digits of 'C */
	printf("%u.%04u'C", ((temp >> 8) & 0xFF), (((u32_t)((temp >> 4) & DECIMAL_MASK_FOUR_BITS) * 625)));
}
