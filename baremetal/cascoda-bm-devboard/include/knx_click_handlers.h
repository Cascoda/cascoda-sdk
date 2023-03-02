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

#ifndef KNX_CLICK_HANDLERS_H
#define KNX_CLICK_HANDLERS_H

typedef int F16_t;
typedef int B1_t;
typedef int U16_t;

#define F16_DATA_INVALID (F16_t)0x7FFF;

/* knx data structure for THERMO */
typedef struct
{
	F16_t thermocouple_temperature; // DPT_Value_Temp, 9.001, dpa.320.51
	F16_t junction_temperature;     // DPT_Value_Temp, 9.001, dpa.320.51
} knx_data_thermo;

/* knx data structure for THERMO3 */
typedef struct
{
	float temperature; // DPT_Value_Temp, 9.001, dpa.320.51
} knx_data_thermo3;

/* knx data structure for AIRQUALITY4 */
typedef struct
{
	F16_t co2_h2;   // DPT_Value_AirQuality, 9.008, dpa.65534.51
	F16_t tvoc_eth; // DPT_Value_AirQuality, 9.008, dpa.65534.51  (note: [ppb], not [ppm]. Convert to [ppm]?)
} knx_data_airquality4;

/* knx data structure for ENVIRONMENT2 */
typedef struct
{
	F16_t humidity;    // DPT_Value_Humidity, 9.007, dpa.337.51
	F16_t temperature; // DPT_Value_Temp, 9.001, dpa.320.51
	F16_t air_quality; // DPT_Value_AirQuality, 9.008, dpa.65534.51 (note: relative, not [ppm])
	F16_t voc_index;   // DPT_Value_AirQuality, 9.008, dpa.65534.51 (note: relative, not [ppm])
} knx_data_environment2;

/* knx data structure for SHT */
typedef struct
{
	F16_t humidity;    // DPT_Value_Humidity, 9.007, dpa.337.51
	F16_t temperature; // DPT_Value_Temp, 9.001, dpa.320.51
} knx_data_sht;

/* knx data structure for HVAC */
typedef struct
{
	float co2content;  // DPT_Value_AirQuality, 9.008, dpa.65534.51
	float humidity;    // DPT_Value_Humidity, 9.007, dpa.337.51
	float temperature; // DPT_Value_Temp, 9.001, dpa.320.51
} knx_data_hvac;

typedef struct
{
	B1_t  detection_state; // DPT_Occupancy, 1.018, dpa.391.51
	U16_t detection_time;  // DPT_TimePeriodSec, ?.??? dpa.???.??
} knx_data_motion;

typedef struct
{
	B1_t relay_1_state; // DPT_Switch, 1.001, dpa.420.61
	B1_t relay_2_state; // DPT_Switch, 1.001, dpa.420.61
} knx_data_relay;

/* application handlers */
ca_error CLICK_Handler_THERMO(void);
ca_error CLICK_Handler_THERMO3(void);
ca_error CLICK_Handler_AIRQUALITY4(void);
ca_error CLICK_Handler_ENVIRONMENT2(void);
ca_error CLICK_Handler_SHT(void);
ca_error CLICK_Handler_HVAC(void);
ca_error CLICK_Handler_MOTION(void);
ca_error CLICK_Handler_RELAY(void);

/* value conversion functions from sensor value to knx types */
void convert_THERMO_to_knx(data_thermo data, knx_data_thermo *knx_data);
void convert_THERMO3_to_knx(data_thermo3 data, knx_data_thermo3 *knx_data);
void convert_AIRQUALITY4_to_knx(data_airquality4 data, knx_data_airquality4 *knx_data);
void convert_ENVIRONMENT2_to_knx(data_environment2 data, knx_data_environment2 *knx_data);
void convert_SHT_to_knx(data_sht data, knx_data_sht *knx_data);
void convert_HVAC_to_knx(data_hvac data, knx_data_hvac *knx_data);
void convert_MOTION_to_knx(data_motion data, knx_data_motion *knx_data);
void convert_RELAY_to_knx(data_relay data, knx_data_relay *knx_data);

#endif // KNX_CLICK_HANDLERS_H
