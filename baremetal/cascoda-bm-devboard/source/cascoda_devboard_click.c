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
#include "airquality4.h"

#include "cascoda_devboard_click.h"
#include "environment2.h"
#include "hvac.h"
#include "motion.h"
#include "relay.h"
#include "sht.h"
#include "sif_ltr303als.h"
#include "sif_max30205.h"
#include "sif_si7021.h"
#include "sif_tmp102.h"
#include "thermo.h"
#include "thermo3.h"

/******************************************************************************/
/*******************************************
 * This function convert a floating point value to a number with 
 * integer and decimal. User can define its precision
*******************************************************************************
 ******************************************************************************/
void float_to_fixed_point_conversion(float target, int num_decimals, uint16_t *value)
{
	value[0] = (uint16_t)floor(target);                                                    //integer part
	value[1] = (uint16_t)round(target * 10 * num_decimals) - value[0] * 10 * num_decimals; //decimal part
}

void select_sensor_gpio(mikrosdk_callbacks *callback, int num, u16_t p1, u16_t p2, void (*handler)())
{
	switch (num)
	{
	case 10: // Motion
		// Enable = p1, Output = p2;
		motion_pin_mapping(p1, p2);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_MOTION_Initialise;
		break;

	case 11: // Relay
		// Relay1 = p1, Relay2 = p2
		relay_pin_mapping(p1, p2);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_RELAY_Initialise;
		break;
	}
	callback->sensor_handler = handler;
}

void select_sensor_comm(mikrosdk_callbacks *callback, int num, u32_t portnum, void (*handler)())
{
	switch (num)
	{
	case 0: // SI7021
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = NULL;
		break;

	case 1: // LTR303ALS
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = SIF_LTR303ALS_Initialise;
		break;

	case 2: // MAX30205
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = SIF_MAX30205_Initialise;
		break;

	case 3: // TMP102
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = SIF_TMP102_Initialise;
		break;

	case 4: // Airquality4
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_AIRQUALITY4_Initialise;
		break;

	case 5: // Environement2
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_ENVIRONMENT2_Initialise;
		break;

	case 6: // HVAC
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_HVAC_Initialise;
		break;

	case 7: // Thermo
		SENSORIF_SPI_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_THERMO_Initialise;
		break;

	case 8: // Thermo3
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_THERMO3_Initialise;
		break;

	case 9: // SHT
		SENSORIF_I2C_Config(portnum);
		callback->dev_number        = num;
		callback->sensor_initialise = MIKROSDK_SHT_Initialise;
		break;
	}
	callback->sensor_handler = handler;
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

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for AIRQUALITY4 Click
 *******************************************************************************
 ******************************************************************************/
void MIKROSDK_Handler_AIRQUALITY4(void)
{
	uint16_t data_buffer[2];

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	printf("--------------------------\n");
	printf("Device : Airquality4 click\n");

	SENSORIF_I2C_Init();

	air_quality4_set_baseline();
	air_quality4_get_co2_and_tvoc(data_buffer);
	printf("CO2 value is: %u\n", data_buffer[0]);
	printf("TVOC value is: %u\n\n", data_buffer[1]);

	SENSORIF_I2C_Deinit();
}

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
void MIKROSDK_Handler_ENVIRONMENT2(void)
{
	uint16_t air_quality;
	uint16_t hum[2], temp[2];
	float    humidity;
	float    temperature;
	int32_t  voc_index;
	int      num_decimals = 2;

	printf("-----------------------\n");
	printf("Device : Environment2 click\n");

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
	SENSORIF_I2C_Deinit();
	BSP_WaitTicks(2000);
}

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for HVAC Click
 *******************************************************************************
 ******************************************************************************/

void MIKROSDK_Handler_HVAC(void)
{
	measurement_data_t hvac_data;
	uint16_t           hum[2], temp[2];
	int                num_decimals = 2;
	// mass_and_num_cnt_data_t sps30_data;

	printf("-----------------------\n");
	printf("Device : HVAC Click\n");
	SENSORIF_I2C_Init();
	hvac_scd40_send_cmd(HVAC_MEASURE_SINGLE_SHOT);
	BSP_WaitTicks(6000);

	while (!hvac_scd40_get_ready_status())
		;

	hvac_scd40_read_measurement(&hvac_data);
	BSP_WaitTicks(100);

	float_to_fixed_point_conversion(hvac_data.temperature, num_decimals, temp);
	float_to_fixed_point_conversion(hvac_data.r_humidity, num_decimals, hum);

	printf("CO2 Content = %u ppm\n", hvac_data.co2_concent);

	printf("Temperature = %u.%02u 'C\n", temp[0], temp[1]);

	printf("R. Humidity = %u.%02u %%RH\n", hum[0], hum[1]);
	// printf("- - - - - - - - - - - - - \n");

	/* SPS30 is not working !!!*/
	// while ( HVAC_SPS30_NEW_DATA_IS_READY != hvac_sps30_get_ready_flag() );
	// hvac_sps30_read_measured_data(&sps30_data );
	// BSP_WaitTicks(100);

	// printf( "   Mass Concentration :   \n" );
	// printf( " PM 1.0 = %.2f ug/m3 \n", sps30_data.mass_pm_1_0 );
	// printf( " PM 2.5 = %.2f ug/m3 \n", sps30_data.mass_pm_2_5 );
	// printf( " PM 4.0 = %.2f ug/m3 \n", sps30_data.mass_pm_4_0 );
	// printf( " PM 10  = %.2f ug/m3 \n", sps30_data.mass_pm_10 );
	// printf( "-   -   -   -   -   -   - \n" );

	// printf( "  Number Concentration :  \n" );
	// printf( " PM 0.5 = %.2f n/cm3 \n", sps30_data.num_pm_0_5 );
	// printf( " PM 1.0 = %.2f n/cm3 \n", sps30_data.num_pm_1_0 );
	// printf( " PM 2.5 = %.2f n/cm3 \n", sps30_data.num_pm_2_5 );
	// printf( " PM 4.0 = %.2f n/cm3 \n", sps30_data.num_pm_4_0 );
	// printf( " PM 10  = %.2f n/cm3 \n", sps30_data.num_pm_10 );
	// printf( "--------------------------\n" );

	SENSORIF_I2C_Deinit();
}

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for MOTION Click
 *******************************************************************************
 ******************************************************************************/
/* Modify MEASUREMENT_PERIOD to reduce the interval time*/

void MIKROSDK_Handler_MOTION(void)
{
	motion_detect_state_t state;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	printf("-----------------------\n");
	printf("Device : Motion\n");
	state = motion_get_detected(); /* detect motion */

	switch (state)
	{
	case MOTION_NO_DETECT:
		printf("  There is no movement\n");
		printf("-------------------------\n");
		break;
	case MOTION_NOT_PRESENCE:
		printf("  No movement presence\n");
		printf("-------------------------\n");
		break;
	case MOTION_DETECTED:
		printf(" > Motion detected! <\n");
		printf("-------------------------\n");
		break;
	case MOTION_PRESENCE:
		printf(" > Ongoing Motion detected! <\n");
		printf("-------------------------\n");
		break;
	default:
		printf(" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
	}
	BSP_WaitTicks(5);
}

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for RELAY Click
 *******************************************************************************
 ******************************************************************************/
void MIKROSDK_Handler_RELAY(void)
{
	u8_t cnt;
	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	printf("-----------------------\n");
	printf("Device : Relay\n");

	for (cnt = 1; cnt <= 2; cnt++)
	{
		printf("*** Relay %d state is ON  ***\n", cnt);

		relay_set_state(cnt, RELAY_STATE_ON);

		BSP_WaitTicks(1000);

		printf("*** Relay %d state is OFF ***\n", cnt);

		relay_set_state(cnt, RELAY_STATE_OFF);

		BSP_WaitTicks(200);
	}
}

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for THERMO Click
 *******************************************************************************
 ******************************************************************************/
void MIKROSDK_Handler_THERMO(void)
{
	u16_t temp;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	printf("-----------------------\n");
	printf("Device : THERMO\n");

	SENSORIF_SPI_Init();
	temp = thermo_get_temperature(); /* read temperature */
	SENSORIF_SPI_Deinit();
	printf("Temperature = ");

	if (temp & 0x2000) /* 2s complement for temperature */
	{
		printf("-");
		temp = ~temp + 1;
	}
	/* display 'C and 4 digits of 'C */
	printf("%u.%02u'C", ((temp >> 2) & 0xFFF), (((u32_t)(temp & DECIMAL_MASK_TWO_BITS) * 25)));
	printf("\n");

	SENSORIF_SPI_Init();
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

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for THERMO3 Click
 *******************************************************************************
 ******************************************************************************/
void MIKROSDK_Handler_THERMO3(void)
{
	u16_t temp;

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */

	printf("-----------------------\n");
	printf("Device : THERMO3 Click\n");
	SENSORIF_I2C_Init();      /* enable interface */
	temp = get_temperature(); /* read TMP102 temperature */
	SENSORIF_I2C_Deinit();    /* disable interface */
	printf("Temperature = ");

	if (temp & 0x8000) /* 2s complement for temperature */
	{
		printf("-");
		temp = ~temp + 1;
	}

	/* display 'C and 4 digits of 'C */
	printf("%u.%04u'C", ((temp >> 8) & 0xFF), (((u32_t)((temp >> 4) & DECIMAL_MASK_FOUR_BITS) * 625)));
}

/******************************************************************************/
/*******************************************
********************************/ /**
 * \brief MIKROSDK Handler Example for SHT Click
 *******************************************************************************
 ******************************************************************************/

void MIKROSDK_Handler_SHT(void)
{
	int   num_decimals = 2;
	u16_t temp[2], hum[2];

	/* Note:
	 * This is a tick based handler for polling only
	 * In applications this should be based on timer-interrupts.
	 */
	printf("-----------------------\n");
	printf("Device : SHT click\n");

	SENSORIF_I2C_Init();
	float_to_fixed_point_conversion(sht_temp_ss(), num_decimals, temp);
	printf("Temperature : %u.%02u 'C\n", temp[0], temp[1]);
	BSP_WaitTicks(500);
	float_to_fixed_point_conversion(sht_hum_ss(), num_decimals, hum);
	printf("Humidity : %u.%02u %%RH\n", hum[0], hum[1]);
	SENSORIF_I2C_Deinit();
	BSP_WaitTicks(500);
}