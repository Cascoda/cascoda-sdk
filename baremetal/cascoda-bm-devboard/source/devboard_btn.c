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

#include "devboard_btn.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* Map from LED/Button -> module pin number (depending on jumper position) */
u8_t dvbd_possiblePinMappings[][2] = {{5, 31}, {6, 32}, {35, 33}, {36, 34}};

/* Get the module pin number using the LED/Button and jumper position, and stores it in the map */
static void DVBD_mapLEDBtnToPin(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	// Get the correct pin number based upon jumper position
	// Store the mapping from this LED/Button to the pin number
	if (jumperPos == JUMPER_POS_1)
	{
		registeredPinMappings[ledBtn] = dvbd_possiblePinMappings[ledBtn][0];
	}
	else
	{
		registeredPinMappings[ledBtn] = dvbd_possiblePinMappings[ledBtn][1];
	}
}

/* Register LED output (open drain) */
ca_error DVBD_RegisterLEDOutput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	/* map the pin */
	DVBD_mapLEDBtnToPin(ledBtn, jumperPos);
	return Btn_RegisterLEDOutput(ledBtn);
}

/* register button input */
ca_error DVBD_RegisterButtonInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	/* map the pin */
	DVBD_mapLEDBtnToPin(ledBtn, jumperPos);
	return Btn_RegisterButtonInput(ledBtn);
}

/* register button input with interrupt (for sleepy devices) */
ca_error DVBD_RegisterButtonIRQInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	/* map the pin */
	DVBD_mapLEDBtnToPin(ledBtn, jumperPos);

	Btn_IncrementGPIOWakeup();

	return Btn_RegisterButtonIRQInput(ledBtn);
}

/* register pin to be shared LED and button */
ca_error DVBD_RegisterSharedButtonLED(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	/* map the pin */
	DVBD_mapLEDBtnToPin(ledBtn, jumperPos);

	return Btn_RegisterSharedButtonLED(ledBtn);
}

/* register pin to be shared LED and button with interrupt (for sleepy devices) */
ca_error DVBD_RegisterSharedIRQButtonLED(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	/* map the pin */
	DVBD_mapLEDBtnToPin(ledBtn, jumperPos);

	Btn_IncrementGPIOWakeup();

	return Btn_RegisterSharedIRQButtonLED(ledBtn);
}

/* De-Register an LED or Button Pin */
ca_error DVBD_DeRegister(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	/* map the pin */
	DVBD_mapLEDBtnToPin(ledBtn, jumperPos);

	return Btn_DeRegister(ledBtn);
}

/* Modify possible pin mappings */
ca_error DVBD_modifyPinMappings(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos, u8_t new_pin)
{
	dvbd_possiblePinMappings[ledBtn][jumperPos] = new_pin;
	return CA_ERROR_SUCCESS;
}
