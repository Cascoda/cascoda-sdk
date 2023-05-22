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

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_time.h"
#include "devboard_btn.h"

#include "devboard_batt.h"

// get the battery voltage
// Vbatt[V] = uint16_t / 100
uint16_t DVBD_BattGetVolts(void)
{
	uint32_t adcval;
	uint16_t vbatt;

	/* scale * 100 (10 mV resolution) */
	adcval = BSP_ADCGetVolts() * 100;
	/* input voltage (volts) divided on board by 330/550, so vbatt = 5.5/3.3 * volts */
	/* vbatt = adcval / 4096 * 5.5 = adcval / 745 */
	/* Note: reading is off by factor 1.085 measured over several boards (settling/rin/tolerance?) */
	/* => 745 -> 690 */
	vbatt = (uint16_t)(adcval / 690);

	return (vbatt);
}

// get the battery charging status
uint8_t DVBD_BattGetChargeStat(void)
{
	return (BSP_GetChargeStat());
}

// check if +5V (Vbus or external) is connected
uint8_t DVBD_BattGetUSBPresent(void)
{
	return (BSP_GetVBUSConnected());
}

// initialise CHARGE_STAT
ca_error DVBD_BattInitChargeStat(void)
{
	struct gpio_input_args args;

	/* input, pull-up, debounced */
	args.mpin     = BATT_CHARGE_STAT_PIN;
	args.pullup   = MODULE_PIN_PULLUP_ON;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_OFF;

	return BSP_ModuleRegisterGPIOInput(&args);
}

// initialise VOLTS (and VOLTS_TEST)
ca_error DVBD_BattInitVolts(void)
{
	ca_error               status;
	struct gpio_input_args args;

	/* VOLTS_TEST */
	/* output, no pull-up, permanent, set to 0 */
	if ((status = BSP_ModuleRegisterGPIOOutput(BATT_VOLTS_TEST_PIN, MODULE_PIN_TYPE_GENERIC)))
		return status;
	BSP_ModuleSetGPIOPin(BATT_VOLTS_TEST_PIN, 0);
	BSP_ModuleSetGPIOOutputPermanent(BATT_VOLTS_TEST_PIN);

	/* VOLTS */
	/* input, no pull-up */
	/* ADC input (analog) is switched in dynamically */
	args.mpin     = BATT_VOLTS_PIN;
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_OFF;
	args.irq      = MODULE_PIN_IRQ_OFF;
	if ((status = BSP_ModuleRegisterGPIOInput(&args)))
		return status;

	return CA_ERROR_SUCCESS;
}

// initialise USB_PRESENT
ca_error DVBD_BattInitUSBPresent(void)
{
	struct gpio_input_args args;

	/* USB_PRESENT (VBUS_CONNECTED) */
	/* input, no pull-up */
	args.mpin     = BATT_USB_PRESENT_PIN;
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_OFF;

	return BSP_ModuleRegisterGPIOInput(&args);
}
