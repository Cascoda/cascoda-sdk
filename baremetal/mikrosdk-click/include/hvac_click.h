/**
 * @file
 * @brief mikrosdk interface
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
 * Example click interface driver
*/

#ifndef HVAC_CLICK_H
#define HVAC_CLICK_H

#include <stdint.h>

/* operational modes */
#define HVAC_MODE_PERIODIC 0 /* periodic mode. wakeup interval has to be >= 5 seconds (data sampling period) */
#define HVAC_MODE_PERIODIC_LP \
	1 /* low-power periodic mode. wakeup interval has to be >= 30 seconds (data sampling period) */
#define HVAC_MODE_SINGLE_WAIT 2       /* single-shot mode. 5 seconds wait time for data aquisition */
#define HVAC_MODE_SINGLE_WAIT_SLEEP 3 /* single-shot mode with sleep. 5 seconds wait time for data aquisition */
#define HVAC_MODE_SINGLE_NEXT 4       /* single shot, read data on next wake-up */

#define HVAC_MODE HVAC_MODE_PERIODIC

/* perform factory reset on initialisation (power-up) when 1 */
#define HVAC_FACTORY_RESET 0

/* all scd41 maximum data length */
#define SCD41_MAXDLEN 9
/* I2C scd41 slave address + memory address (command) */
#define SCD41_ADDLEN 3

/* timing parameters [ms] */
#define SCD41_T_STOP 500           /* scd41 500 ms after stop periodic measurement */
#define SCD41_T_CAL 400            /* scd41 400 ms for forced calibration */
#define SCD41_T_SELF_TEST 10000    /* scd41 10 seconds for self test */
#define SCD41_T_FACTORY_RESET 1200 /* scd41 1200 ms factory reset */
#define SCD41_T_REINIT 20          /* scd41 20 ms reinitialisation */
#define SCD41_T_MEAS_SINGLE 5000   /* scd41 5 seconds measure single shot */
#define SCD41_T_MEAS_SINGLE_RHT 50 /* scd41 50 ms measure single shot for rh and t only */
#define SCD41_T_WAKEUP 20          /* scd41 20 ms wakeup time */
#define SCD41_T_TIMEOUT 10000      /* scd41 10 seconds timeout waiting for data */
#define SCD41_T_WAITPOLL 100       /* scd41 100 ms interval for wait polls */

/* data ready status */
#define HVAC_SCD41_NEW_DATA_NOT_READY 0x00
#define HVAC_SCD41_NEW_DATA_IS_READY 0x01

/* target CO2 concentration for forced calibration */
#define SCD41_CO2_TARGET 400

/* scd41 command set (updated to datasheet version 1.3 - september 2022 */
#define SCD41_CMD_START_PERIODIC_MEASUREMENT 0x21B1
#define SCD41_CMD_READ_MEASUREMENT 0xEC05
#define SCD41_CMD_STOP_PERIODIC_MEASUREMENT 0x3F86
#define SCD41_CMD_SET_TEMPERATURE_OFFSET 0x241D
#define SCD41_CMD_GET_TEMPERATURE_OFFSET 0x2318
#define SCD41_CMD_SET_SENSOR_ALTITUDE 0x2427
#define SCD41_CMD_GET_SENSOR_ALTITUDE 0x2322
#define SCD41_CMD_SET_AMBIENT_PRESSURE 0xE000
#define SCD41_CMD_PERFORM_FORCED_RECALIBRATION 0x362F
#define SCD41_CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x2416
#define SCD41_CMD_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x2313
#define SCD41_CMD_START_LOW_POWER_PERIODIC_MEASUREMENT 0x21AC
#define SCD41_CMD_GET_DATA_READY_STATUS 0xE4B8
#define SCD41_CMD_PERSIST_SETTINGS 0x3615
#define SCD41_CMD_GET_SERIAL_NUMBER 0x3682
#define SCD41_CMD_PERFORM_SELF_TEST 0x3639
#define SCD41_CMD_PERFORM_FACTORY_RESET 0x3632
#define SCD41_CMD_REINIT 0x3646
#define SCD41_CMD_MEASURE_SINGLE_SHOT 0x219D
#define SCD41_CMD_MEASURE_SINGLE_SHOT_RHT_ONLY 0x2196
#define SCD41_CMD_POWER_DOWN 0x36E0
#define SCD41_CMD_WAKE_UP 0x36F6
/* below commands are not in datasheet but are declared in original code */
#define SCD41_CMD_START_ULTRA_LOW_POWER_PERIODIC_MEASUREMENT 0x21A7
#define SCD41_CMD_GET_FEATURE_SET_VERSION 0x202F
#define SCD41_CMD_GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD 0x2340
#define SCD41_CMD_SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD 0x2445
#define SCD41_CMD_GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD 0x234B
#define SCD41_CMD_SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD 0x244E

/* data acquisition status */
enum hvac_status
{
	HVAC_ST_OK      = 0, /* read values ok */
	HVAC_ST_WAITING = 1, /* waiting for data */
	HVAC_ST_FAIL    = 2  /* acquisition failed */
};

/* new functions */
uint8_t MIKROSDK_HVAC_scd41_start_periodic_measurement(void);
uint8_t MIKROSDK_HVAC_scd41_start_low_power_periodic_measurement(void);
uint8_t MIKROSDK_HVAC_scd41_read_measurement(uint16_t *co2content, int16_t *temperature, int16_t *humitidity);
uint8_t MIKROSDK_HVAC_scd41_stop_periodic_measurement(void);
uint8_t MIKROSDK_HVAC_scd41_get_data_ready_status(uint8_t *ready);
uint8_t MIKROSDK_HVAC_scd41_set_temperature_offset(int16_t temp_offset);
uint8_t MIKROSDK_HVAC_scd41_get_temperature_offset(int16_t *temp_offset);
uint8_t MIKROSDK_HVAC_scd41_set_sensor_altitude(uint16_t altitude);
uint8_t MIKROSDK_HVAC_scd41_get_sensor_altitude(uint16_t *altitude);
uint8_t MIKROSDK_HVAC_scd41_set_ambient_pressure(uint16_t pressure);
uint8_t MIKROSDK_HVAC_scd41_perform_forced_calibration(void);
uint8_t MIKROSDK_HVAC_scd41_set_automatic_self_calibration_enabled(uint8_t enable);
uint8_t MIKROSDK_HVAC_scd41_get_automatic_self_calibration_enabled(uint8_t *enabled);
uint8_t MIKROSDK_HVAC_scd41_persist_settings(void);
uint8_t MIKROSDK_HVAC_scd41_get_serial_number(uint16_t *serial_number);
uint8_t MIKROSDK_HVAC_scd41_perform_self_test(void);
uint8_t MIKROSDK_HVAC_scd41_perform_factory_reset(void);
uint8_t MIKROSDK_HVAC_scd41_reinit(void);
uint8_t MIKROSDK_HVAC_scd41_measure_single_shot(void);
uint8_t MIKROSDK_HVAC_scd41_measure_single_shot_rht_only(void);
uint8_t MIKROSDK_HVAC_scd41_power_down(void);
uint8_t MIKROSDK_HVAC_scd41_wake_up(void);
uint8_t MIKROSDK_HVAC_Initialise(void);
uint8_t MIKROSDK_HVAC_Reinitialise(void);
uint8_t MIKROSDK_HVAC_Acquire(uint16_t *co2content, int16_t *humidity, int16_t *temperature);

#endif // HVAC_CLICK_H
