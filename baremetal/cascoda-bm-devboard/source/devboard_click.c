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
 * CLICK sensor interface for devboard
*/

#include <stdio.h>

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#include "airquality4_click.h"
#include "devboard_btn.h"
#include "devboard_click.h"
#include "environment2_click.h"
#include "hvac_click.h"
#include "motion_click.h"
#include "relay_click.h"
#include "sht_click.h"
#include "thermo3_click.h"
#include "thermo_click.h"

/* globals */
/* states for controlling RELAY */
uint8_t g_relay_1_state = 0;
uint8_t g_relay_2_state = 0;

/* use power control (crowbar) for mikrobus vdd33 */
static bool g_use_power_control = false;

/* select click sensor */
ca_error DVBD_select_click(mikrosdk_callbacks *callback, dvbd_click_type dev_type, ca_error (*handler)())
{
	switch (dev_type)
	{
	case STYPE_NONE:
		callback->dev_type         = STYPE_NONE;
		callback->click_initialise = NULL;
		callback->click_handler    = NULL;
		callback->click_alarm      = NULL;
		return CA_ERROR_SUCCESS;
	case STYPE_THERMO:
		SENSORIF_SPI_Config(SPI_PORTNUM);
		callback->click_initialise = MIKROSDK_THERMO_Initialise;
		callback->click_alarm      = NULL;
		break;
	case STYPE_THERMO3:
		MIKROSDK_THERMO3_pin_mapping(CLICK_INT_PIN);
		SENSORIF_I2C_Config(I2C_PORTNUM);
#if (THERMO3_USE_INTERRUPT)
		DVBD_SetGPIOWakeup();
#endif
		callback->click_initialise = MIKROSDK_THERMO3_Initialise;
		callback->click_alarm      = MIKROSDK_THERMO3_alarm_triggered;
		break;
	case STYPE_AIRQUALITY4:
		SENSORIF_I2C_Config(I2C_PORTNUM);
		callback->click_initialise = MIKROSDK_AIRQUALITY4_Initialise;
		callback->click_alarm      = NULL;
		break;
	case STYPE_ENVIRONMENT2:
		SENSORIF_I2C_Config(I2C_PORTNUM);
		callback->click_initialise = MIKROSDK_ENVIRONMENT2_Initialise;
		callback->click_alarm      = NULL;
		break;
	case STYPE_SHT:
		// NReset, Alert
		MIKROSDK_SHT_pin_mapping(CLICK_RST_PIN, CLICK_INT_PIN);
		SENSORIF_I2C_Config(I2C_PORTNUM);
#if (SHT_USE_INTERRUPT)
		DVBD_SetGPIOWakeup();
#endif
		callback->click_initialise = MIKROSDK_SHT_Initialise;
		callback->click_alarm      = MIKROSDK_SHT_alarm_triggered;
		break;
	case STYPE_HVAC:
		SENSORIF_I2C_Config(I2C_PORTNUM);
		callback->click_initialise = MIKROSDK_HVAC_Initialise;
		callback->click_alarm      = NULL;
		break;
	case STYPE_MOTION:
		// Enable, Output
		MIKROSDK_MOTION_pin_mapping(CLICK_RST_PIN, CLICK_INT_PIN);
		callback->click_initialise = MIKROSDK_MOTION_Initialise;
		callback->click_alarm      = MIKROSDK_MOTION_alarm_triggered;
#if (MOTION_USE_INTERRUPT)
		DVBD_SetGPIOWakeup();
#endif
		break;
	case STYPE_RELAY:
		// Relay1, Relay2
		MIKROSDK_RELAY_pin_mapping(CLICK_PWM_PIN, CLICK_CS_PIN);
		callback->click_initialise = MIKROSDK_RELAY_Initialise;
		callback->click_alarm      = NULL;
		break;
	default:
		callback->dev_type         = STYPE_NONE;
		callback->click_handler    = NULL;
		callback->click_initialise = NULL;
		callback->click_alarm      = NULL;
		return CA_ERROR_FAIL;
	}
	callback->dev_type      = dev_type;
	callback->click_handler = handler;

	return CA_ERROR_SUCCESS;
}

/* initialise the click power control signal */
ca_error DVBD_click_power_init(void)
{
	ca_error err;

	// power control is on pin 12 (PA.14)
	err = BSP_ModuleRegisterGPIOOutput(DVBD_CLICK_POWER_PIN, MODULE_PIN_TYPE_GENERIC);

	if (!err)
	{
		BSP_ModuleSetGPIOPin(DVBD_CLICK_POWER_PIN, DVBD_CLICK_POWER_ON);
		BSP_ModuleSetGPIOOutputPermanent(DVBD_CLICK_POWER_PIN);
		g_use_power_control = true;
	}

	return err;
}

/* set click power control */
ca_error DVBD_click_power_set(uint8_t onoff)
{
	return BSP_ModuleSetGPIOPin(DVBD_CLICK_POWER_PIN, onoff);
}

/* THERMO click data acquisition */
ca_error CLICK_THERMO_acquisition(data_thermo *data)
{
	if (g_use_power_control)
	{
		DVBD_click_power_set(DVBD_CLICK_POWER_ON);
		if (MIKROSDK_THERMO_Reinitialise())
			return CA_ERROR_FAIL;
	}
	data->status = MIKROSDK_THERMO_Acquire(&data->thermocouple_temperature, &data->junction_temperature);
	if (g_use_power_control)
		DVBD_click_power_set(DVBD_CLICK_POWER_OFF);

	if (data->status == THERMO_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/* THERMO3 click data acquisition */
ca_error CLICK_THERMO3_acquisition(data_thermo3 *data)
{
#if (!THERMO3_USE_INTERRUPT)
	if (g_use_power_control)
	{
		DVBD_click_power_set(DVBD_CLICK_POWER_ON);
		if (MIKROSDK_THERMO3_Reinitialise())
			return CA_ERROR_FAIL;
	}
#endif
	data->status = MIKROSDK_THERMO3_Acquire(&data->temperature);
#if (!THERMO3_USE_INTERRUPT)
	if (g_use_power_control)
		DVBD_click_power_set(DVBD_CLICK_POWER_OFF);
#endif

	if (data->status == THERMO3_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/* AIRQUALITY4 click data acquisition */
ca_error CLICK_AIRQUALITY4_acquisition(data_airquality4 *data)
{
	if (g_use_power_control)
	{
		DVBD_click_power_set(DVBD_CLICK_POWER_ON);
		if (MIKROSDK_AIRQUALITY4_Reinitialise())
			return CA_ERROR_FAIL;
	}
	data->status = MIKROSDK_AIRQUALITY4_Acquire(&data->co2_h2, &data->tvoc_eth);
	if (g_use_power_control)
		DVBD_click_power_set(DVBD_CLICK_POWER_OFF);

	if (data->status == AIRQUALITY4_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/* ENVIRONMENT2 click data acquisition */
ca_error CLICK_ENVIRONMENT2_acquisition(data_environment2 *data)
{
	if (g_use_power_control)
	{
		DVBD_click_power_set(DVBD_CLICK_POWER_ON);
		if (MIKROSDK_ENVIRONMENT2_Reinitialise())
			return CA_ERROR_FAIL;
	}
	data->status =
	    MIKROSDK_ENVIRONMENT2_Acquire(&data->voc_index, &data->air_quality, &data->humidity, &data->temperature);
	if (g_use_power_control)
		DVBD_click_power_set(DVBD_CLICK_POWER_OFF);

	if (data->status == ENVIRONMENT2_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/* SHT click data acquisition */
ca_error CLICK_SHT_acquisition(data_sht *data)
{
#if (!SHT_USE_INTERRUPT)
	if (g_use_power_control)
	{
		DVBD_click_power_set(DVBD_CLICK_POWER_ON);
		if (MIKROSDK_SHT_Reinitialise())
			return CA_ERROR_FAIL;
	}
#endif
	data->status = MIKROSDK_SHT_Acquire(&data->humidity, &data->temperature);
#if (!SHT_USE_INTERRUPT)
	if (g_use_power_control)
		DVBD_click_power_set(DVBD_CLICK_POWER_OFF);
#endif

	if (data->status == SHT_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/* HVAC click data acquisition */
ca_error CLICK_HVAC_acquisition(data_hvac *data)
{
#if (HVAC_MODE == HVAC_MODE_SINGLE_WAIT)
	if (g_use_power_control)
	{
		DVBD_click_power_set(DVBD_CLICK_POWER_ON);
		if (MIKROSDK_HVAC_Reinitialise())
			return CA_ERROR_FAIL;
	}
#endif
	data->status = MIKROSDK_HVAC_Acquire(&data->co2content, &data->humidity, &data->temperature);
#if (HVAC_MODE == HVAC_MODE_SINGLE_WAIT)
	if (g_use_power_control)
		DVBD_click_power_set(DVBD_CLICK_POWER_OFF);
#endif

	if (data->status == HVAC_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/* MOTION click data acquisition */
ca_error CLICK_MOTION_acquisition(data_motion *data)
{
#if (!MOTION_USE_INTERRUPT)
	if (g_use_power_control)
	{
		DVBD_click_power_set(DVBD_CLICK_POWER_ON);
		if (MIKROSDK_MOTION_Reinitialise())
			return CA_ERROR_FAIL;
	}
#endif
	data->status = MIKROSDK_MOTION_Acquire(&data->detection_state, &data->detection_time);
#if (!MOTION_USE_INTERRUPT)
	if (g_use_power_control)
		DVBD_click_power_set(DVBD_CLICK_POWER_OFF);
#endif

	if (data->status == MOTION_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}

/* Handler Example for RELAY Click */
/* RELAY click data acquisition */
ca_error CLICK_RELAY_acquisition(data_relay *data)
{
	data->relay_1_state = g_relay_1_state;
	data->relay_2_state = g_relay_2_state;
	data->status        = MIKROSDK_RELAY_Driver(data->relay_1_state, data->relay_2_state);

	if (data->status == RELAY_ST_FAIL)
		return CA_ERROR_FAIL;
	return CA_ERROR_SUCCESS;
}
