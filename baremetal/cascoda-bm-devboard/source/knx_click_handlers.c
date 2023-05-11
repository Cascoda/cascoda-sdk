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
 * default examples for click sensor handlers - simple printing to console
*/
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cascoda-util/cascoda_time.h"

#include "devboard_click.h"
#include "knx_click_handlers.h"

#include "airquality4_click.h"
#include "environment2_click.h"
#include "hvac_click.h"
#include "motion_click.h"
#include "relay_click.h"
#include "sht_click.h"
#include "thermo3_click.h"
#include "thermo_click.h"

/* value reporting for debugging */
static void report_THERMO(knx_data_thermo data);
static void report_THERMO3(knx_data_thermo3 data);
static void report_AIRQUALITY4(knx_data_airquality4 data);
static void report_ENVIRONMENT2(knx_data_environment2 data);
static void report_SHT(knx_data_sht data);
static void report_HVAC(knx_data_hvac data);
static void report_MOTION(knx_data_motion data);
static void report_RELAY(knx_data_relay data);

/* for reporting only */
const char *click_name_knx[] = {
    "-",
    "THERMO",
    "THERMO3",
    "AIRQUALITY4",
    "ENVIRONMENT2",
    "SHT",
    "HVAC",
    "MOTION",
    "RELAY",
};

/* print floating point for debugging (for debugging) */
/* 3 digits precision for fraction */
static void printfloat(float val)
{
	if (val >= 670760) // F16_DATA_INVALID
	{
		printf("--");
		return;
	}

	char  tmpSign = (val < 0) ? '-' : '+';
	float tmpVal  = (val < 0) ? -val : val;

	int   tmpInt1 = tmpVal;               // get integer
	float tmpFrac = tmpVal - tmpInt1;     // get fraction
	int   tmpInt2 = trunc(tmpFrac * 100); // turn into integer (* precision)

	// print parts, note that you need 0-padding for fractional bit.
	printf("%c%d.%02d", tmpSign, tmpInt1, tmpInt2);
}

/* convert mantissa to KNX F16 (final conversion) */
static F16_t convert_mant_to_F16(int32_t val)
{
	int32_t mant;
	bool    negative;
	uint8_t exp;
	uint8_t round;
	F16_t   result;

	mant = val;

	if (mant & 0x80000000)
		negative = true;
	else
		negative = false;

	exp   = 0;
	round = 0;
	if (negative)
	{
		while (mant < (int32_t)(-2048))
		{
			++exp;
			round = (uint8_t)(mant)&1;
			mant >>= 1;
			mant |= 0x80000000;
		}
	}
	else
	{
		while (mant > (int32_t)(2047))
		{
			++exp;
			round = (uint8_t)(mant)&1;
			mant >>= 1;
		}
	}

	if (round)
		mant += 1;

	result = mant & 0x07FF;
	result += (exp << 11) & 0x7800;
	if (negative)
		result += 0x8000;

	return result;
}

/* convert KNX F16 to float (for debugging) */
static float convert_F16_to_float(F16_t val)
{
	uint8_t  exp;
	uint32_t mant;
	int32_t  sign;
	float    result;

	/* get mantissa */
	mant = val & 0x07FF;
	/* 2s complement */
	if (val & 0x8000)
	{
		mant = mant | 0xFFFFF800; /* 32-bit sign extension */
		mant = ~mant + 1;         /* 2s complement */
		sign = -1;
	}
	else
	{
		sign = +1;
	}
	/* get exponent */
	exp = (val & 0x7800) >> 11;

	result = 0.01 * (float)((int32_t)((mant << exp) * sign));

	return result;
}

/* convert uint16_t with scaling factor to F16 */
static F16_t convert_uint16_to_F16(uint16_t din, uint8_t scale)
{
	int32_t mant;

	/* extend to 32 bit */
	mant = (int32_t)din;

	if (scale != 100)
	{
		mant *= 100;
		mant /= scale;
	}

	return convert_mant_to_F16(mant);
}

/* convert uint16_t 2s complement with scaling factor to F16 */
static F16_t convert_uint16_2cpl_to_F16(uint16_t din, uint8_t scale)
{
	int32_t mant;
	bool    negative;

	/* extend to 32 bit */
	if (din & 0x8000) /* 2s complement for temperature */
	{
		negative = true;
		mant     = 0xFFFF0000 | (int32_t)din; /* sign extension */
	}
	else
	{
		negative = false;
		mant     = (int32_t)din;
	}

	if (scale != 100)
	{
		mant *= 100;
		mant /= scale;
	}

	if (negative)
		mant |= 0xF0000000; /* sign extension from shift */

	return convert_mant_to_F16(mant);
}

/* convert int16_t with scaling factor to F16 */
static F16_t convert_int16_to_F16(int16_t din, uint8_t scale)
{
	int32_t mant;
	bool    negative;

	/* extend to 32 bit */
	if (din & 0x8000) /* 2s complement for temperature */
	{
		negative = true;
		mant     = 0xFFFF0000 | (int32_t)din; /* sign extension */
	}
	else
	{
		negative = false;
		mant     = (int32_t)din;
	}

	if (scale != 100)
	{
		mant *= 100;
		mant /= scale;
	}

	if (negative)
		mant |= 0xF0000000; /* sign extension from shift */

	return convert_mant_to_F16(mant);
}

/* convert int32_t with scaling factor to F16 */
static F16_t convert_int32_to_F16(int32_t din, uint8_t scale)
{
	int32_t mant;

	mant = (int32_t)din;

	if (scale != 100)
	{
		mant *= 100;
		mant /= scale;
	}

	return convert_mant_to_F16(mant);
}

/* convert THERMO data to knx */
void convert_THERMO_to_knx(data_thermo data, knx_data_thermo *knx_data)
{
	/* input format T['C] *  4 in 2s complement */
	knx_data->thermocouple_temperature = convert_uint16_2cpl_to_F16(data.thermocouple_temperature, 4);
	/* input format T['C] * 16 in 2s complement */
	knx_data->junction_temperature = convert_uint16_2cpl_to_F16(data.junction_temperature, 16);
}

/* convert THERMO3 data to knx */
void convert_THERMO3_to_knx(data_thermo3 data, knx_data_thermo3 *knx_data)
{
	/* input format T['C] * 16 in 2s complement */
	knx_data->temperature = ((float)(data.temperature)) / 16.0;
}

/* convert AIRQUALITY4 data to knx */
void convert_AIRQUALITY4_to_knx(data_airquality4 data, knx_data_airquality4 *knx_data)
{
	/* input format CO2 [ppm] or H2 (raw value) */
	knx_data->co2_h2 = convert_uint16_to_F16(data.co2_h2, 1);
	/* input format TVOC [ppb] or ETH (raw value) */
	knx_data->tvoc_eth = convert_uint16_to_F16(data.tvoc_eth, 1);
}

/* convert ENVIRONMENT2 data to knx */
void convert_ENVIRONMENT2_to_knx(data_environment2 data, knx_data_environment2 *knx_data)
{
	/* input format RH[%] * 100 */
	knx_data->humidity = convert_int16_to_F16(data.humidity, 100);

	/* input format T['C] * 100 */
	knx_data->temperature = convert_int16_to_F16(data.temperature, 100);

	/* input format [0 - 0xFFFF] (raw value) */
	knx_data->air_quality = convert_uint16_to_F16(data.air_quality, 1);

	/* input format [0 - 500] (VOC index algorithm output) */
	knx_data->voc_index = convert_int32_to_F16(data.voc_index, 1);
}

/* convert SHT data to knx */
void convert_SHT_to_knx(data_sht data, knx_data_sht *knx_data)
{
	/* input format RH[%] * 100 */
	knx_data->humidity = ((float)(data.humidity)) / 100.0;
	/* input format T['C] * 100 */
	knx_data->temperature = ((float)(data.temperature)) / 100.0;
}

/* convert HVAC data to knx */
void convert_HVAC_to_knx(data_hvac data, knx_data_hvac *knx_data)
{
	/* input format CO2 [ppm] */
	knx_data->co2content = (float)data.co2content;
	/* input format RH[%] * 100 */
	knx_data->humidity = ((float)data.humidity) / 100.0;
	/* input format T['C] * 100 */
	knx_data->temperature = ((float)data.temperature) / 100.0;
}

/* convert MOTION data to knx */
void convert_MOTION_to_knx(data_motion data, knx_data_motion *knx_data)
{
	/* input format motion_detect_state_changes_t */
	if ((data.detection_state == MOTION_CH_DETECTED) || (data.detection_state == MOTION_CH_PRESENT))
		knx_data->detection_state = 1;
	else
		knx_data->detection_state = 0;
	/* input format [ms] */
	knx_data->detection_time = (U16_t)(data.detection_time / 1000);
}

/* convert RELAY data to knx */
void convert_RELAY_to_knx(data_relay data, knx_data_relay *knx_data)
{
	/* input format relay_state */
	if (data.relay_1_state == RELAY_ON)
		knx_data->relay_1_state = 1;
	else
		knx_data->relay_1_state = 0;
	/* input format relay_state */
	if (data.relay_2_state == RELAY_ON)
		knx_data->relay_2_state = 1;
	else
		knx_data->relay_2_state = 0;
}

/* Handler for THERMO Click */
ca_error CLICK_Handler_THERMO(void)
{
	data_thermo     data;
	knx_data_thermo knx_data;

	/* data acquisition */
	if (CLICK_THERMO_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_THERMO_to_knx(data, &knx_data);

	/* data processing */
	report_THERMO(knx_data);

	return CA_ERROR_SUCCESS;
}

/* Handler for THERMO3 Click */
ca_error CLICK_Handler_THERMO3(void)
{
	data_thermo3     data;
	knx_data_thermo3 knx_data;

	/* data acquisition */
	if (CLICK_THERMO3_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_THERMO3_to_knx(data, &knx_data);

	/* data processing */
	report_THERMO3(knx_data);

	return CA_ERROR_SUCCESS;
}

/* Handler for AIRQUALITY4 Click */
ca_error CLICK_Handler_AIRQUALITY4(void)
{
	data_airquality4     data;
	knx_data_airquality4 knx_data;

	/* data acquisition */
	if (CLICK_AIRQUALITY4_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_AIRQUALITY4_to_knx(data, &knx_data);

	/* data processing */
	report_AIRQUALITY4(knx_data);

	return CA_ERROR_SUCCESS;
}

/* Handler for ENVIRONMENT2 Click */
ca_error CLICK_Handler_ENVIRONMENT2(void)
{
	data_environment2     data;
	knx_data_environment2 knx_data;

	/* data acquisition */
	if (CLICK_ENVIRONMENT2_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_ENVIRONMENT2_to_knx(data, &knx_data);

	/* data processing */
	report_ENVIRONMENT2(knx_data);

	return CA_ERROR_SUCCESS;
}

/* Handler for SHT Click */
ca_error CLICK_Handler_SHT(void)
{
	data_sht     data;
	knx_data_sht knx_data;

	/* data acquisition */
	if (CLICK_SHT_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_SHT_to_knx(data, &knx_data);

	/* data processing */
	report_SHT(knx_data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for HVAC Click */
ca_error CLICK_Handler_HVAC(void)
{
	data_hvac     data;
	knx_data_hvac knx_data;

	/* data acquisition */
	if (CLICK_HVAC_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_HVAC_to_knx(data, &knx_data);
	if (data.status != HVAC_ST_OK)
	{
		knx_data.co2content  = F16_DATA_INVALID;
		knx_data.humidity    = F16_DATA_INVALID;
		knx_data.temperature = F16_DATA_INVALID;
	}

	/* data processing */
	report_HVAC(knx_data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for MOTION Click */
ca_error CLICK_Handler_MOTION(void)
{
	data_motion     data;
	knx_data_motion knx_data;

	/* data acquisition */
	if (CLICK_MOTION_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_MOTION_to_knx(data, &knx_data);

	/* data processing */
	report_MOTION(knx_data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for RELAY Click */
ca_error CLICK_Handler_RELAY(void)
{
	data_relay     data;
	knx_data_relay knx_data;

	/* data acquisition */
	if (CLICK_RELAY_acquisition(&data))
		return CA_ERROR_FAIL;

	/* convert results into KNX format */
	convert_RELAY_to_knx(data, &knx_data);

	/* data processing */
	report_RELAY(knx_data);

	return CA_ERROR_SUCCESS;
}

/* print time and click type */
static void printClickType(dvbd_click_type type)
{
	printf("%4us %-12s: ", (TIME_ReadAbsoluteTime() / 1000), click_name_knx[type]);
}

/* Reporting for THERMO Click */
static void report_THERMO(knx_data_thermo data)
{
	printClickType(STYPE_THERMO);
	printf("Tthermo: ");
	printfloat(convert_F16_to_float(data.thermocouple_temperature));
	printf("; Tjunction: ");
	printfloat(convert_F16_to_float(data.junction_temperature));
	printf("\n");
}

/* Reporting for THERMO3 Click */
static void report_THERMO3(knx_data_thermo3 data)
{
	printClickType(STYPE_THERMO3);
	printf("T: ");
	printfloat(data.temperature);
	printf("\n");
}

/* Reporting for AIRQUALITY4 Click */
static void report_AIRQUALITY4(knx_data_airquality4 data)
{
	printClickType(STYPE_AIRQUALITY4);
#if (AIRQUALITY4_MEASURE_RAW_SIGNALS)
	printf("H2: ");
#else
	printf("CO2: ");
#endif
	printfloat(convert_F16_to_float(data.co2_h2));
#if (AIRQUALITY4_MEASURE_RAW_SIGNALS)
	printf("; ETH: ");
#else
	printf("; TVOC: ");
#endif
	printfloat(convert_F16_to_float(data.tvoc_eth));
	printf("\n");
}

/* Reporting for ENVIRONMENT2 Click */
static void report_ENVIRONMENT2(knx_data_environment2 data)
{
	printClickType(STYPE_ENVIRONMENT2);
	printf("RH: ");
	printfloat(convert_F16_to_float(data.humidity));
	printf("; T: ");
	printfloat(convert_F16_to_float(data.temperature));
	printf("; AirQRaw: ");
	printfloat(convert_F16_to_float(data.air_quality));
	printf("; VOCIdx: ");
	printfloat(convert_F16_to_float(data.voc_index));
	printf("\n");
}

/* Reporting for SHT Click */
static void report_SHT(knx_data_sht data)
{
	printClickType(STYPE_SHT);
	printf("T: ");
	printfloat(data.temperature);
	printf("; RH: ");
	printfloat(data.humidity);
	printf("\n");
}

/* Reporting for HVAC Click */
static void report_HVAC(knx_data_hvac data)
{
	printClickType(STYPE_HVAC);
	printf("CO2: ");
	printfloat(convert_F16_to_float(data.co2content));
	printf("; T: ");
	printfloat(convert_F16_to_float(data.temperature));
	printf("; RH: ");
	printfloat(convert_F16_to_float(data.humidity));
	printf("\n");
}

/* Reporting for MOTION Click */
static void report_MOTION(knx_data_motion data)
{
	printClickType(STYPE_MOTION);
	if (data.detection_state)
		printf("Motion detected!");
	else
		printf("Motion stopped after %u seconds.", data.detection_time);
	printf("\n");
}

/* Reporting for RELAY Click */
static void report_RELAY(knx_data_relay data)
{
	printClickType(STYPE_RELAY);
	printf(" Relay 1 is %s; Relay 2 is %s", (data.relay_1_state ? " ON" : "OFF"), (data.relay_2_state ? " ON" : "OFF"));
	printf("\n");
}
