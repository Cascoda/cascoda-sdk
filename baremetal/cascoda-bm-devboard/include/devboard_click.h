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
 * CLICK sensor interface for devboard
*/

#ifndef DEVBOARD_CLICK_H
#define DEVBOARD_CLICK_H

#include "ca821x_error.h"

/* devboard comms interface configuration */
#define I2C_PORTNUM 1 // CLICK interface port for I2C
#define SPI_PORTNUM 1 // CLICK interface port for SPI

/* 36 hours (24-bit @ 1 sec / 128) */
#define DVBD_MAX_SLEEP_TIME 0x07B98A00

/* number of mikrobus CLICK interfaces on devboard */
#define DVBD_NUM_MIKROBUS 2

/* power-down of click sensors when power control (crowbar) is used */
#define DVBD_CLICK_POWER_ON 0
#define DVBD_CLICK_POWER_OFF 1

/* power ocntrol pin for crowbar is on pin 12 */
#define DVBD_CLICK_POWER_PIN 12

/* devboard pin assignments (chili2 pins) for mikrobus click interface */
#define CLICK_AN_PIN 36
#define CLICK_RST_PIN 6
#define CLICK_CS_PIN 34
#define CLICK_SCK_PIN 33
#define CLICK_MISO_PIN 32
#define CLICK_MOSI_PIN 31
#define CLICK_PWM_PIN 35
#define CLICK_INT_PIN 5
// Tx (UART) not connected
// Rx (UART) not connected
#define CLICK_SCL_PIN 2
#define CLICK_SDA_PIN 4

typedef enum dvbd_click_type
{
	STYPE_NONE         = 0,
	STYPE_THERMO       = 1,
	STYPE_THERMO3      = 2,
	STYPE_AIRQUALITY4  = 3,
	STYPE_ENVIRONMENT2 = 4,
	STYPE_SHT          = 5,
	STYPE_HVAC         = 6,
	STYPE_MOTION       = 7,
	STYPE_RELAY        = 8,
} dvbd_click_type;

typedef struct
{
	dvbd_click_type dev_type;
	ca_error (*click_handler)();
	uint8_t (*click_initialise)();
	uint8_t (*click_alarm)();
} mikrosdk_callbacks;

/* structures for click sensor data */

/* data structure for THERMO */
typedef struct
{
	uint8_t  status;                   // airquality4_status
	uint16_t thermocouple_temperature; // T['C] = compl(uint16_t /  4)
	uint16_t junction_temperature;     // T['C] = compl(uint16_t / 16)
} data_thermo;

/* data structure for THERMO3 */
typedef struct
{
	uint8_t  status;      // thermo3_status
	uint16_t temperature; // T['C] = compl(uint16_t / 16)
} data_thermo3;

/* data structure for AIRQUALITY4 */
typedef struct
{
	uint8_t  status;   // airquality4_status
	uint16_t co2_h2;   // AIRQUALITY4_MEASURE_RAW_SIGNALS = 0: CO2 [ppm];
	                   // AIRQUALITY4_MEASURE_RAW_SIGNALS = 1: H2 (raw value)
	uint16_t tvoc_eth; // AIRQUALITY4_MEASURE_RAW_SIGNALS = 0: TVOC [ppb]
	                   // AIRQUALITY4_MEASURE_RAW_SIGNALS = 1: ETH (raw value)
} data_airquality4;

/* data structure for ENVIRONMENT2 */
typedef struct
{
	uint8_t  status;      // environment2_status
	int16_t  humidity;    // RH[%] = int16_t / 100
	int16_t  temperature; // T['C] = int16_t / 100
	uint16_t air_quality; // [0-65535] (raw value)
	int32_t  voc_index;   // [0-500]
} data_environment2;

/* data structure for SHT */
typedef struct
{
	uint8_t status;      // environment2_status
	int16_t humidity;    // RH[%] = int16_t / 100
	int16_t temperature; // T['C] = int16_t / 100
} data_sht;

/* data structure for HVAC */
typedef struct
{
	uint8_t  status;      // hvac_status
	uint16_t co2content;  // CO2  [ppm] (0-40000 ppm)
	int16_t  humidity;    // RH[%] = int16_t / 100
	int16_t  temperature; // T['C] = int16_t / 100
} data_hvac;

typedef struct
{
	uint8_t  status;          // motion_status
	uint8_t  detection_state; // motion_detect_state_changes_t
	uint32_t detection_time;  // Motion Detected Time [ms] (only used if MOTION_USE_INTERRUPT=1, otherwise 0)
} data_motion;

typedef struct
{
	uint8_t status;        // relay_status
	uint8_t relay_1_state; // relay_state for relay 1 (RELAY_OFF / RELAY_ON)
	uint8_t relay_2_state; // relay_state for relay 2 (RELAY_OFF / RELAY_ON)
} data_relay;

/* globals */
extern uint8_t g_relay_1_state, g_relay_2_state; /* states for controlling RELAY */

/* general functions */
ca_error DVBD_select_click(mikrosdk_callbacks *callback, dvbd_click_type dev_type, ca_error (*handler)());
ca_error DVBD_click_power_init(void);
ca_error DVBD_click_power_set(uint8_t onoff);

/* data acquisition functions */
ca_error CLICK_THERMO3_acquisition(data_thermo3 *data);
ca_error CLICK_THERMO_acquisition(data_thermo *data);
ca_error CLICK_AIRQUALITY4_acquisition(data_airquality4 *data);
ca_error CLICK_ENVIRONMENT2_acquisition(data_environment2 *data);
ca_error CLICK_SHT_acquisition(data_sht *data);
ca_error CLICK_HVAC_acquisition(data_hvac *data);
ca_error CLICK_MOTION_acquisition(data_motion *data);
ca_error CLICK_RELAY_acquisition(data_relay *data);

#endif // DEVBOARD_CLICK_H
