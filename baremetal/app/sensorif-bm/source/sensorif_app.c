/**
 * @file test15_4_main.c
 * @brief test15_4 main program loop and supporting functions
 * @author Wolfgang Bruchner
 * @date 19/07/14
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Example application for external sensor interfaces
*/
#include <stdio.h>
#include <stdlib.h>

#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "sensorif_app.h"
#if SENSORIF_TEST_SI7021
#include "sif_si7021.h"
#endif
#if SENSORIF_TEST_MAX30205
#include "sif_max30205.h"
#endif

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Initialisation
 *******************************************************************************
 ******************************************************************************/
u8_t SENSORIF_Initialise(struct ca821x_dev *pDeviceRef)
{
	u8_t status = 0;

	/* select device-specific initialisation here */
#if (SENSORIF_TEST_SI7021)
	; /* no initialisation */
#endif
#if (SENSORIF_TEST_MAX30205)
	status = SIF_MAX30205_Initialise();
#endif
	return (status);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SENSORIF Application Handler
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_Handler(struct ca821x_dev *pDeviceRef)
{
	/* select device-specific handlers here */
#if (SENSORIF_TEST_SI7021)
	SENSORIF_Handler_SI7021();
#endif
#if (SENSORIF_TEST_MAX30205)
	SENSORIF_Handler_MAX30205();
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
