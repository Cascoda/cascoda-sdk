/**
 * @file
 * @brief test15_4 main program loop and supporting functions
 */
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
 * Example application for external sensor interfaces
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"

#include "sensorif_app.h"
#if SENSORIF_TEST_SI7021
#include "sif_si7021.h"
#endif
#if SENSORIF_TEST_MAX30205
#include "sif_max30205.h"
#endif
#if SENSORIF_TEST_LTR303ALS
#include "sif_ltr303als.h"
#endif
#if SENSORIF_TEST_TMP102
#include "sif_tmp102.h"
#endif

u8_t SENSORIF_Initialise(struct ca821x_dev *pDeviceRef)
{
	u8_t status = 0;

	/* select device-specific initialisation here */
	SENSORIF_I2C_Init();
#if (SENSORIF_TEST_SI7021)
	; /* no initialisation */
#endif
#if (SENSORIF_TEST_MAX30205)
	status |= SIF_MAX30205_Initialise();
#endif
#if (SENSORIF_TEST_LTR303ALS)
	status |= SIF_LTR303ALS_Initialise();
#endif
#if (SENSORIF_TEST_TMP102)
	status |= SIF_TMP102_Initialise();
#endif
	SENSORIF_I2C_Deinit(); /* disable interface */

	return (status);
}

void SENSORIF_Handler(struct ca821x_dev *pDeviceRef)
{
	/* select device-specific handlers here */
#if (SENSORIF_TEST_SI7021)
	SENSORIF_Handler_SI7021();
#endif
#if (SENSORIF_TEST_MAX30205)
	SENSORIF_Handler_MAX30205();
#endif
#if (SENSORIF_TEST_LTR303ALS)
	SENSORIF_Handler_LTR303ALS();
#endif
#if (SENSORIF_TEST_TMP102)
	SENSORIF_Handler_TMP102();
#endif
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Handler Example for SI7021
 *******************************************************************************
 ******************************************************************************/
#if (SENSORIF_TEST_SI7021)
void SENSORIF_Handler_SI7021(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u8_t        id      = 0;
	u8_t        temp;
	u8_t        hum;
	u32_t       t1, t2, t3;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	if (((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) < SENSORIF_MEASUREMENT_DELTA) && (!handled))
	{
		printf("SI7021: ");
		++ticker;
		handled = 1;
		SENSORIF_I2C_Init();        /* enable interface */
		id   = SIF_SI7021_ReadID(); /* read SI7021 ID */
		t1   = TIME_ReadAbsoluteTime();
		temp = SIF_SI7021_ReadTemperature(); /* read SI7021 temperature */
		t2   = TIME_ReadAbsoluteTime();
		hum  = SIF_SI7021_ReadHumidity(); /* read SI7021 humidity */
		t3   = TIME_ReadAbsoluteTime();
		SENSORIF_I2C_Deinit(); /* disable interface */
		printf("Meas %u: ID=%02X", ticker, id);
		printf("; Temperature=");
		if (temp & 0x80) /* 2s complement for temperature */
		{
			printf("-");
			temp = ~temp + 1;
		}
		printf("%u'C", temp);
		printf("; Humidity=");
		printf("%u%c", hum, 37);
#if SENSORIF_REPORT_TMEAS
		printf("; TmeasT=%ums, TmeasH=%ums", (t2 - t1), (t3 - t2));
#endif
		printf("\n");
	}
	if ((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) >
	    (SENSORIF_MEASUREMENT_PERIOD - SENSORIF_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Handler Example for MAX30205
 *******************************************************************************
 ******************************************************************************/
#if (SENSORIF_TEST_MAX30205)
void SENSORIF_Handler_MAX30205(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u16_t       temp;
	u32_t       t1, t2;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	if (((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) < SENSORIF_MEASUREMENT_DELTA) && (!handled))
	{
		printf("MAX30205: ");
		++ticker;
		handled = 1;
		SENSORIF_I2C_Init(); /* enable interface */
		t1   = TIME_ReadAbsoluteTime();
		temp = SIF_MAX30205_ReadTemperature(); /* read MAX30205 temperature */
		t2   = TIME_ReadAbsoluteTime();
		SENSORIF_I2C_Deinit(); /* disable interface */
		printf("Meas %u:", ticker);
		printf("; Temperature=");
		if (temp & 0x8000) /* 2s complement for temperature */
		{
			printf("-");
			temp = ~temp + 1;
		}
		/* display 'C and 3 digits of 'C */
		printf("%u.%03u'C", ((temp >> 8) & 0xFF), (((u32_t)(temp & 0x00FF) * 1000) / 256));
#if SENSORIF_REPORT_TMEAS
		printf("; TmeasT=%ums", (t2 - t1));
#endif
		printf("\n");
	}
	if ((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) >
	    (SENSORIF_MEASUREMENT_PERIOD - SENSORIF_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Handler Example for LTR303ALS
 *******************************************************************************
 ******************************************************************************/
#if (SENSORIF_TEST_LTR303ALS)
void SENSORIF_Handler_LTR303ALS(void)
{
	static u8_t ticker    = 0;
	static u8_t handled   = 0;
	u16_t       light_ch0 = 0;
	u16_t       light_ch1 = 0;
	u32_t       t1, t2;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	if (((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) < SENSORIF_MEASUREMENT_DELTA) && (!handled))
	{
		printf("LTR303ALS: ");
		++ticker;
		handled = 1;
		SENSORIF_I2C_Init(); /* enable interface */
		t1 = TIME_ReadAbsoluteTime();
		/* read LTR303ALS light measurement for both channels */
		SIF_LTR303ALS_ReadLight(&light_ch0, &light_ch1);
		t2 = TIME_ReadAbsoluteTime();
		SENSORIF_I2C_Deinit(); /* disable interface */
		printf("Meas %u:", ticker);
		printf("; Light Ch0 (visible) = 0x%04X; Ch1 (IR) = 0x%04X", light_ch0, light_ch1);
#if SENSORIF_REPORT_TMEAS
		printf("; TmeasL=%ums", (t2 - t1));
#endif
		printf("\n");
	}
	if ((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) >
	    (SENSORIF_MEASUREMENT_PERIOD - SENSORIF_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief SENSORIF Handler Example for TMP102
 *******************************************************************************
 ******************************************************************************/
#if (SENSORIF_TEST_TMP102)
void SENSORIF_Handler_TMP102(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u16_t       temp;
	u32_t       t1, t2;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	if (((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) < SENSORIF_MEASUREMENT_DELTA) && (!handled))
	{
		printf("TMP102: ");
		++ticker;
		handled = 1;
		SENSORIF_I2C_Init(); /* enable interface */
		t1   = TIME_ReadAbsoluteTime();
		temp = SIF_TMP102_ReadTemperature(); /* read TMP102 temperature */
		t2   = TIME_ReadAbsoluteTime();
		SENSORIF_I2C_Deinit(); /* disable interface */
		printf("Meas %u:", ticker);
		printf("; Temperature=");

		if (temp & 0x8000) /* 2s complement for temperature */
		{
			printf("-");
			temp = ~temp + 1;
		}

		/* display 'C and 4 digits of 'C */
		printf("%u.%04u'C", ((temp >> 8) & 0xFF), (((u32_t)((temp >> 4) & DECIMAL_MASK_FOUR_BITS) * 625)));
#if SENSORIF_REPORT_TMEAS
		printf("; TmeasT=%ums", (t2 - t1));
#endif
		printf("\n");
	}
	if ((TIME_ReadAbsoluteTime() % SENSORIF_MEASUREMENT_PERIOD) >
	    (SENSORIF_MEASUREMENT_PERIOD - SENSORIF_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif
