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
#include <stdio.h>
#include <stdlib.h>

#include "cascoda-util/cascoda_time.h"

#include "devboard_click.h"

#include "airquality4_click.h"
#include "ambient8_click.h"
#include "environment2_click.h"
#include "fan_click.h"
#include "hvac_click.h"
#include "motion_click.h"
#include "relay_click.h"
#include "sht_click.h"
#include "thermo3_click.h"
#include "thermo_click.h"

const char *click_name_default[] = {
    "-",
    "THERMO",
    "THERMO3",
    "AIRQUALITY4",
    "ENVIRONMENT2",
    "SHT",
    "HVAC",
    "MOTION",
    "RELAY",
    "AMBIENT8",
    "FAN",
};

/* reporting functions used here */
static void report_THERMO(data_thermo data);
static void report_THERMO3(data_thermo3 data);
static void report_AIRQUALITY4(data_airquality4 data);
static void report_ENVIRONMENT2(data_environment2 data);
static void report_SHT(data_sht data);
static void report_HVAC(data_hvac data);
static void report_MOTION(data_motion data);
static void report_RELAY(data_relay data);
static void report_AMBIENT8(data_ambient8 data);
static void report_FAN(data_fan data);

/* Handler for THERMO Click */
ca_error CLICK_Handler_Default_THERMO(void)
{
	data_thermo data;

	/* data acquisition */
	if (CLICK_THERMO_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_THERMO(data);

	return CA_ERROR_SUCCESS;
}

/* Handler for THERMO3 Click */
ca_error CLICK_Handler_Default_THERMO3(void)
{
	data_thermo3 data;

	/* data acquisition */
	if (CLICK_THERMO3_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_THERMO3(data);

	return CA_ERROR_SUCCESS;
}

/* Handler for AIRQUALITY4 Click */
ca_error CLICK_Handler_Default_AIRQUALITY4(void)
{
	data_airquality4 data;

	/* data acquisition */
	if (CLICK_AIRQUALITY4_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_AIRQUALITY4(data);

	return CA_ERROR_SUCCESS;
}

/* Handler for ENVIRONMENT2 Click */
ca_error CLICK_Handler_Default_ENVIRONMENT2(void)
{
	data_environment2 data;

	/* data acquisition */
	if (CLICK_ENVIRONMENT2_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_ENVIRONMENT2(data);

	return CA_ERROR_SUCCESS;
}

/* Handler for SHT Click */
ca_error CLICK_Handler_Default_SHT(void)
{
	data_sht data;

	/* data acquisition */
	if (CLICK_SHT_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_SHT(data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for HVAC Click */
ca_error CLICK_Handler_Default_HVAC(void)
{
	data_hvac data;

	/* data acquisition */
	if (CLICK_HVAC_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_HVAC(data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for MOTION Click */
ca_error CLICK_Handler_Default_MOTION(void)
{
	data_motion data;

	/* data acquisition */
	if (CLICK_MOTION_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_MOTION(data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for RELAY Click */
ca_error CLICK_Handler_Default_RELAY(void)
{
	data_relay data;

	/* data acquisition */
	if (CLICK_RELAY_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_RELAY(data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for AMBIENT8 Click */
ca_error CLICK_Handler_Default_AMBIENT8(void)
{
	data_ambient8 data;

	/* data acquisition */
	if (CLICK_AMBIENT8_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_AMBIENT8(data);

	return CA_ERROR_SUCCESS;
}

/* Handler Example for FAN Click */
ca_error CLICK_Handler_Default_FAN(void)
{
	data_fan data;

	/* data acquisition */
	if (CLICK_FAN_acquisition(&data))
		return CA_ERROR_FAIL;

	/* data processing */
	report_FAN(data);

	return CA_ERROR_SUCCESS;
}

/* print time and click type */
static void printClickType(dvbd_click_type type)
{
	printf("%4us %-12s: ", (TIME_ReadAbsoluteTime() / 1000), click_name_default[type]);
}

/* Reporting for THERMO Click */
static void report_THERMO(data_thermo data)
{
	uint8_t  sign1, sign2;
	uint16_t temp1, temp2;

	printClickType(STYPE_THERMO);

	if (data.status == THERMO_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
		if (data.thermocouple_temperature & 0x8000) /* 2s complement for temperature */
		{
			sign1 = '-';
			temp1 = ~data.thermocouple_temperature + 1;
		}
		else
		{
			sign1 = '+';
			temp1 = data.thermocouple_temperature;
		}
		if (data.junction_temperature & 0x8000) /* 2s complement for temperature */
		{
			sign2 = '-';
			temp2 = ~data.junction_temperature + 1;
		}
		else
		{
			sign2 = '+';
			temp2 = data.junction_temperature;
		}

		printf("Tthermo: ");
		/* display 'C and 4 digits of 'C after decimal point (10^2/4 = 25) */
		printf("%c%u.%02u'C", sign1, (temp1 >> 2), (uint32_t)(temp1 & 0x0003) * 25);
		printf("; Tjunction: ");
		/* display 'C and 4 digits of 'C after decimal point (10^4/16 = 625) */
		printf("%c%u.%04u'C", sign2, (temp2 >> 4), (uint32_t)(temp2 & 0x000F) * 625);
	}
	printf("\n");
}

/* Reporting for THERMO Click */
static void report_THERMO3(data_thermo3 data)
{
	uint8_t  sign;
	uint16_t temp;

	printClickType(STYPE_THERMO3);

	if (data.status == THERMO3_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
		if (data.temperature & 0x8000) /* 2s complement for temperature */
		{
			sign = '-';
			temp = ~data.temperature + 1;
		}
		else
		{
			sign = '+';
			temp = data.temperature;
		}

		printf("Temperature = ");
		/* display 'C and 4 digits of 'C after decimal point (10^4/16 = 625) */
		printf("%c%u.%04u'C", sign, (temp >> 4), (uint32_t)(temp & 0x000F) * 625);
		if (data.status == THERMO3_ST_ALARM_TRIGGERED)
			printf("; Alarm: High Limit Exceeded.");
		else if (data.status == THERMO3_ST_ALARM_CLEARED)
			printf("; Alarm: Temperature in Range.");
	}
	printf("\n");
}

/* Reporting for AIRQUALITY4 Click */
static void report_AIRQUALITY4(data_airquality4 data)
{
	printClickType(STYPE_AIRQUALITY4);

	if (data.status == AIRQUALITY4_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
#if (AIRQUALITY4_MEASURE_RAW_SIGNALS)
		printf("H2: %u", data.co2_h2);
		printf("; ETH: %u", data.tvoc_eth);
#else
		printf("CO2: %5u ppm", data.co2_h2);
		printf("; TVOC: %5u ppb", data.tvoc_eth);
		if (data.status == AIRQUALITY4_ST_SLEEP)
			printf("; Sleep Mode..");
		else if (data.status == AIRQUALITY4_ST_INIT)
			printf("; Initialising..");
		else if (data.status == AIRQUALITY4_ST_NCAL)
			printf("; Calibrating..");
#endif
	}
	printf("\n");
}

/* Reporting for ENVIRONMENT2 Click */
static void report_ENVIRONMENT2(data_environment2 data)
{
	printClickType(STYPE_ENVIRONMENT2);

	if (data.status == ENVIRONMENT2_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
		printf("RH: %2d.%02u %c", (data.humidity / 100), abs(data.humidity % 100), '%');
		printf("; T: %2d.%02u 'C", (data.temperature / 100), abs(data.temperature % 100));
		printf("; AirQRaw: %5u", data.air_quality);
		printf("; VOCIdx: %3u", (uint16_t)data.voc_index);
		if (data.status == ENVIRONMENT2_ST_SLEEP)
			printf("; Sleep Mode..");
		else if (data.status == ENVIRONMENT2_ST_INIT)
			printf("; Initialising..");
		else if (data.status == ENVIRONMENT2_ST_SETTLING)
			printf("; Settling..");
	}
	printf("\n");
}

/* Reporting for SHT Click */
static void report_SHT(data_sht data)
{
	printClickType(STYPE_SHT);

	if (data.status == SHT_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
		printf("T: %2d.%02u 'C", (data.temperature / 100), abs(data.temperature % 100));
		printf("; RH: %2d.%02u %c", (data.humidity / 100), abs(data.humidity % 100), '%');
#if (SHT_USE_INTERRUPT)
		if (data.status == SHT_ST_ALARM_TRIGGERED)
			printf("; Alarm triggered");
		else if (data.status == SHT_ST_ALARM_CLEARED)
			printf("; Alarm cleared");
#endif
	}
	printf("\n");
}

/* Reporting for HVAC Click */
static void report_HVAC(data_hvac data)
{
	printClickType(STYPE_HVAC);

	if (data.status == HVAC_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else if (data.status == HVAC_ST_WAITING)
	{
		printf("Waiting for Data");
	}
	else
	{
		printf("CO2: %5u ppm", data.co2content);
		printf("; T: %2d.%02u 'C", (data.temperature / 100), abs(data.temperature % 100));
		printf("; RH: %2d.%02u %c", (data.humidity / 100), abs(data.humidity % 100), '%');
	}
	printf("\n");
}

/* Reporting for MOTION Click */
static void report_MOTION(data_motion data)
{
	printClickType(STYPE_MOTION);

	if (data.status == MOTION_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
		switch (data.detection_state)
		{
		case MOTION_CH_NO_DETECT:
#if (!MOTION_USE_INTERRUPT)
			printf("No movement detected.");
			if (data.detection_time != 0)
				printf(" Warning: Time != 0\n");
#else
			if (data.detection_time > 0)
				printf("Motion stopped after %u seconds.", (data.detection_time / 1000));
			else
				printf("No movement detected.");
#endif
			break;
		case MOTION_CH_NOT_PRESENT:
			printf("No movement present.");
			break;
		case MOTION_CH_DETECTED:
			printf("Motion detected!");
			break;
		case MOTION_CH_PRESENT:
			printf("Ongoing Motion detected!");
			break;
		default:
			printf("Invalid state!");
		}
	}
	printf("\n");
}

/* Reporting for RELAY Click */
static void report_RELAY(data_relay data)
{
	printClickType(STYPE_RELAY);

	if (data.status == RELAY_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
		printf(" Relay 1 is %s; Relay 2 is %s",
		       (data.relay_1_state == RELAY_ON ? " ON" : "OFF"),
		       (data.relay_2_state == RELAY_ON ? " ON" : "OFF"));
	}
	printf("\n");
}

/* Reporting for AMBIENT8 Click */
static void report_AMBIENT8(data_ambient8 data)
{
	printClickType(STYPE_AMBIENT8);

	if (data.status == AMBIENT8_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
		printf("CH0 (VIS+IR): %5u.%02u lux", (data.illuminance_ch0 / 100), (data.illuminance_ch0 % 100));
		printf("; CH1 (IR): %5u.%02u lux", (data.illuminance_ch1 / 100), (data.illuminance_ch1 % 100));
		printf("; Ambient: %5u.%02u lux", (data.illuminance_ambient / 100), (data.illuminance_ambient % 100));
	}
	printf("\n");
}

/* Reporting for FAN Click */
static void report_FAN(data_fan data)
{
	printClickType(STYPE_FAN);

	if (data.status == FAN_ST_FAIL)
	{
		printf("Data Acquisition Fail");
	}
	else
	{
#if (FAN_MODE == FAN_MODE_CLOSED_LOOP)
		printf("RPM set to %5u rpm", g_fan_speed_tach_rpm);
#else
		printf("PWM set to %3u%c", g_fan_speed_pwm_percent, '%');
#endif
		printf("; Speed Reading: %3u%c", data.speed_pwm_percent, '%');
		printf("; Tachometer Reading: %5u rpm", data.speed_tach_rpm);
		if (data.status == FAN_ST_ALARM_FNSTL)
			printf("; Alarm: Driver stalled.");
		else if (data.status == FAN_ST_ALARM_FNSPIN)
			printf("; Alarm: Spin Up failed.");
		else if (data.status == FAN_ST_ALARM_DVFAIL)
			printf("; Alarm: Driver failed.");
	}
	printf("\n");
}
