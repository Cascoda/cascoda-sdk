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

#ifndef DEVBOARD_BATT_H
#define DEVBOARD_BATT_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "ca821x_error.h"

/* charging state */
enum dvbd_charging_state
{
	NOT_CHARGING = 0,
	CHARGING     = 1,
};

enum dvbd_vbus_state
{
	NOT_CONNECTED = 0,
	CONNECTED     = 1,
};

/* devboard pin assignments (chili2 pins) for battery functions */
#define BATT_USB_PRESENT_PIN 5
#define BATT_VOLTS_TEST_PIN 6
#define BATT_VOLTS_PIN 35
#define BATT_CHARGE_STAT_PIN 36

/**
 * \brief read the battery voltage
 * \return vbatt * 100 (10mV resolution)
 *
 */
uint16_t DVBD_BattGetVolts(void);

/**
 * \brief get the battery charging status
 * \return NOT_CHARGING (0) or CHARGING (1)
 *
 */
uint8_t DVBD_BattGetChargeStat(void);

/**
 * \brief check if +5V (USB Vbus or external) is connected
 * \return NOT_CONNECTED (0) or CONNECTED (1)
 *
 */
uint8_t DVBD_BattGetUSBPresent(void);

/**
 * \brief initialise CHARGE_STAT
 * \return status
 *
 */
ca_error DVBD_BattInitChargeStat(void);

/**
 * \brief initialise VOLTS and VOLTS_TEST
 * \return status
 *
 */
ca_error DVBD_BattInitVolts(void);

/**
 * \brief initialise USB_PRESENT
 * \return status
 *
 */
ca_error DVBD_BattInitUSBPresent(void);

#endif // DEVBOARD_BATT_H
