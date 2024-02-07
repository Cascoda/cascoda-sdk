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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"

#include "mikrosdk_app.h"

#if MIKROSDK_TEST_AIRQUALITY4
#include "airquality4_click.h"
#endif
#if MIKROSDK_TEST_ENVIRONMENT2
#include "environment2_click.h"
#endif
#if MIKROSDK_TEST_HVAC
#include "hvac_click.h"
#endif
#if MIKROSDK_TEST_MOTION
#include "motion_drv_click.h"
#endif
#if MIKROSDK_TEST_RELAY
#include "relay_click.h"
#endif
#if MIKROSDK_TEST_THERMO
#include "thermo_click.h"
#endif
#if MIKROSDK_TEST_THERMO3
#include "thermo3_click.h"
#endif
#if MIKROSDK_TEST_SHT
#include "sht_click.h"
#endif

u8_t MIKROSDK_Initialise(struct ca821x_dev *pDeviceRef)
{
	u8_t status = 0;
	/* select device-specific initialisation here */

#if (MIKROSDK_TEST_AIRQUALITY4)
	SENSORIF_I2C_Config(1);
	status |= MIKROSDK_AIRQUALITY4_Initialise();
#endif
#if (MIKROSDK_TEST_ENVIRONMENT2)
	SENSORIF_I2C_Config(1);
	status |= MIKROSDK_ENVIRONMENT2_Initialise();
#endif
#if (MIKROSDK_TEST_HVAC)
	SENSORIF_I2C_Config(1);
	status |= MIKROSDK_HVAC_Initialise();
#endif
#if (MIKROSDK_TEST_MOTION)
	MIKROSDK_MOTION_pin_mapping(6, 5);
	status |= MIKROSDK_MOTION_Initialise();
#endif
#if (MIKROSDK_TEST_RELAY)
	relay_pin_mapping(35, 34);
	status |= MIKROSDK_RELAY_Initialise();
#endif
#if (MIKROSDK_TEST_THERMO)
	SENSORIF_SPI_Config(1);
	status |= MIKROSDK_THERMO_Initialise();
#endif
#if (MIKROSDK_TEST_THERMO3)
	MIKROSDK_THERMO3_pin_mapping(5);
	SENSORIF_I2C_Config(1);
	status |= MIKROSDK_THERMO3_Initialise();
#endif
#if (MIKROSDK_TEST_SHT)
	SENSORIF_I2C_Config(1);
	status |= MIKROSDK_SHT_Initialise();
#endif
	return (status);
}

void MIKROSDK_Handler(struct ca821x_dev *pDeviceRef)
{
	/* select device-specific handlers here */

#if (MIKROSDK_TEST_AIRQUALITY4)
	MIKROSDK_Handler_AIRQUALITY4();
#endif
#if (MIKROSDK_TEST_ENVIRONMENT2)
	MIKROSDK_Handler_ENVIRONMENT2();
#endif
#if (MIKROSDK_TEST_HVAC)
	MIKROSDK_Handler_HVAC();
#endif
#if (MIKROSDK_TEST_MOTION)
	MIKROSDK_Handler_MOTION();
#endif
#if (MIKROSDK_TEST_RELAY)
	MIKROSDK_Handler_RELAY();
#endif
#if (MIKROSDK_TEST_THERMO)
	MIKROSDK_Handler_THERMO();
#endif
#if (MIKROSDK_TEST_THERMO3)
	MIKROSDK_Handler_THERMO3();
#endif
#if (MIKROSDK_TEST_SHT)
	MIKROSDK_Handler_SHT();
#endif
}

/******************************************************************************/
/*******************************************
 * This function convert a floating point value to a number with
 * integer and decimal. User can define its precision
*******************************************************************************
 ******************************************************************************/

#if (FLOAT_TO_INT_CONVERT)
void float_to_fixed_point_conversion(float target, int num_decimals, uint16_t *value)
{
	value[0] = (uint16_t)floor(target);                                                    //integer part
	value[1] = (uint16_t)round(target * 10 * num_decimals) - value[0] * 10 * num_decimals; //decimal part
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for AIRQUALITY4 Click
 *******************************************************************************
 ******************************************************************************/

#if (MIKROSDK_TEST_AIRQUALITY4)
void MIKROSDK_Handler_AIRQUALITY4(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u32_t       t1, t2;
	uint16_t    data_buffer[2];

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		printf("Airquality4 click\n");
		++ticker;
		handled = 1;
		printf("Meas %u:\n", ticker);

		SENSORIF_I2C_Init();

		air_quality4_set_baseline();
		air_quality4_get_co2_and_tvoc(data_buffer);
		printf("CO2 value is: %u\n", data_buffer[0]);
		printf("TVOC value is: %u\n\n", data_buffer[1]);

		SENSORIF_I2C_Deinit();
	}

	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for Environment2 Click
 * It takes around 7 minutes for the VOC index output correctly.
 * When the device outputs 0 for VOC index, the device starts to be set up.
 * The expected stabilised value for VOC is rougly 100,
 * NOTICE: this device outputs a relative VOC not absolute VOC.
 *******************************************************************************
 ******************************************************************************/

#if (MIKROSDK_TEST_ENVIRONMENT2)
void MIKROSDK_Handler_ENVIRONMENT2(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u32_t       t1, t2;

	uint16_t air_quality;
	uint16_t hum[2], temp[2];
	float    humidity;
	float    temperature;
	int32_t  voc_index;
	int      num_decimals = 2;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		printf("<< Environment2 click >>\n");
		++ticker;
		handled = 1;
		printf("Meas %u:\n", ticker);

		SENSORIF_I2C_Init();
		environment2_get_temp_hum(&humidity, &temperature);

		float_to_fixed_point_conversion(temperature, num_decimals, temp);
		float_to_fixed_point_conversion(humidity, num_decimals, hum);
		printf(" Humidity : %u.%02u %%RH\n", hum[0], hum[1]);
		printf(" Temperature : %u.%02u 'C\n", temp[0], temp[1]);

		environment2_get_air_quality(&air_quality);

		printf(" Air Quality : %u \n", (uint16_t)air_quality);

		environment2_get_voc_index(&voc_index);
		printf(" VOC Index : %u\n", (uint16_t)voc_index);
		printf("-----------------------\n");
		SENSORIF_I2C_Deinit();
		BSP_WaitTicks(2000);
	}

	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for HVAC Click
 *******************************************************************************
 ******************************************************************************/

#if (MIKROSDK_TEST_HVAC)
void MIKROSDK_Handler_HVAC(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u32_t       t1, t2;

	measurement_data_t hvac_data;
	uint16_t hum[2], temp[2];
	int      num_decimals = 2;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		SENSORIF_I2C_Init();
		hvac_scd40_send_cmd(HVAC_MEASURE_SINGLE_SHOT);
		BSP_WaitTicks(6000);

		while (!hvac_scd40_get_ready_status())
			;

		hvac_scd40_read_measurement(&hvac_data);
		BSP_WaitTicks(100);

		float_to_fixed_point_conversion(hvac_data.temperature, num_decimals, temp);
		float_to_fixed_point_conversion(hvac_data.r_humidity, num_decimals, hum);

		printf("CO2 Concent = %u ppm\n", hvac_data.co2_concent);

		printf("Temperature = %u.%02u 'C\n", temp[0], temp[1]);

		printf("R. Humidity = %u.%02u %%RH\n", hum[0], hum[1]);
		printf("- - - - - - - - - - - - - \n");


		SENSORIF_I2C_Deinit();
	}

	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for MOTION Click
 *******************************************************************************
 ******************************************************************************/
/* Modify MIKROSDK_MEASUREMENT_PERIOD to reduce the interval time*/

#if (MIKROSDK_TEST_MOTION)
void MIKROSDK_Handler_MOTION(void)
{
	static u8_t                   ticker = 0;
	u32_t                         t1, t2;
	static u8_t                   handled = 0;
	motion_detect_state_changes_t state;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		printf("Motion:\n");
		++ticker;
		handled = 1;
		t1      = TIME_ReadAbsoluteTime();
		state   = MIKROSDK_MOTION_get_detected(); /* detect motion */
		t2      = TIME_ReadAbsoluteTime();
		printf("Meas %u:", ticker);

		switch (state)
		{
		case MOTION_CH_NO_DETECT:
			printf(" Number: %d \n", ticker);
			printf("  There is no movement\n");
			printf("-------------------------\n");
			break;
		case MOTION_CH_NOT_PRESENT:
			printf(" Number: %d \n", ticker);
			printf("  No movement presence\n");
			printf("-------------------------\n");
			break;
		case MOTION_CH_DETECTED:
			printf(" Number: %d \n", ticker);
			printf(" > Motion detected! <\n");
			printf("-------------------------\n");
			break;
		case MOTION_CH_PRESENT:
			printf(" Number: %d \n", ticker);
			printf(" > Ongoing Motion detected! <\n");
			printf("-------------------------\n");
			break;
		default:
			printf(" Number: %d \n", ticker);
			printf(" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
		}
	}
	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for RELAY Click
 *******************************************************************************
 ******************************************************************************/

#if (MIKROSDK_TEST_RELAY)
void MIKROSDK_Handler_RELAY(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u32_t       t1, t2;
	u8_t        cnt;
	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		printf("Relay\n");
		++ticker;
		handled = 1;
		printf("Meas %u:\n", ticker);

		for (cnt = 1; cnt <= 2; cnt++)
		{
			printf("*** Relay %d state is ON ***\n", cnt);

			relay_set_state(cnt, RELAY_STATE_ON);

			BSP_WaitTicks(1000);

			printf("*** Relay %d state is OFF ***\n", cnt);

			relay_set_state(cnt, RELAY_STATE_OFF);

			BSP_WaitTicks(200);
		}
	}
	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for THERMO Click
 *******************************************************************************
 ******************************************************************************/
#if (MIKROSDK_TEST_THERMO)
void MIKROSDK_Handler_THERMO(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u16_t       temp;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		printf("\n------------------------------------------------------------\n");
		printf("THERMO: ");
		++ticker;
		handled = 1;

		SENSORIF_SPI_Init(false);
		temp = thermo_get_temperature(); /* read temperature */
		SENSORIF_SPI_Deinit();
		printf("Meas %u:\n", ticker);
		printf("Temperature = ");

		if (temp & 0x2000) /* 2s complement for temperature */
		{
			printf("-");
			temp = ~temp + 1;
		}
		/* display 'C and 4 digits of 'C */
		printf("%u.%02u'C", ((temp >> 2) & 0xFFF), (((u32_t)(temp & DECIMAL_MASK_TWO_BITS) * 25)));
		printf("\n");

		SENSORIF_SPI_Init(false);
		temp = thermo_get_junction_temperature(); /* read temperature */
		SENSORIF_SPI_Deinit();

		printf("Junction Temperature = ");
		if (temp & 0x0800) /* 2s complement for temperature */
		{
			printf("-");
			temp = ~temp + 1;
		}
		/* display 'C and 4 digits of 'C */
		printf("%u.%04u'C", ((temp >> 4) & 0xFF), (((u32_t)((temp & DECIMAL_MASK_FOUR_BITS) * 625))));
	}

	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for THERMO3 Click
 *******************************************************************************
 ******************************************************************************/
#if (MIKROSDK_TEST_THERMO3)
void MIKROSDK_Handler_THERMO3(void)
{
	static u8_t ticker  = 0;
	static u8_t handled = 0;
	u32_t       t1, t2;
	u16_t       temp;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		printf("THERMO3: ");
		++ticker;
		handled = 1;
		SENSORIF_I2C_Init(); /* enable interface */
		t1 = TIME_ReadAbsoluteTime();
		MIKROSDK_THERMO3_get_temperature(&temp); /* read TMP102 temperature */
		t2 = TIME_ReadAbsoluteTime();
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
	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for SHT Click
 *******************************************************************************
 ******************************************************************************/

#if (MIKROSDK_TEST_SHT)
void MIKROSDK_Handler_SHT(void)
{
	static u8_t ticker       = 0;
	static u8_t handled      = 0;
	int         num_decimals = 2;
	u32_t       t1, t2;
	float       temperature, humidity;
	u16_t       temp[2], hum[2];

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	if (((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) < MIKROSDK_MEASUREMENT_DELTA) && (!handled))
	{
		printf("-----------------------\n");
		printf("SHT click\n");
		++ticker;
		handled = 1;
		printf("Meas %u:\n", ticker);

		SENSORIF_I2C_Init();

		float_to_fixed_point_conversion(sht_temp_ss(), num_decimals, temp);
		printf(" Temperature : %u.%02u 'C\n", temp[0], temp[1]);
		BSP_WaitTicks(500);
		float_to_fixed_point_conversion(sht_hum_ss(), num_decimals, hum);
		printf(" Humidity : %u.%02u %%RH\n", hum[0], hum[1]);
		SENSORIF_I2C_Deinit();
	}

	if ((TIME_ReadAbsoluteTime() % MIKROSDK_MEASUREMENT_PERIOD) >
	    (MIKROSDK_MEASUREMENT_PERIOD - MIKROSDK_MEASUREMENT_DELTA))
	{
		handled = 0;
	}
}
#endif

