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

#include "cascoda_devboard_btn.h"
#include "cascoda-bm/cascoda_interface.h"

// For the following arrays,
// each LED/Button corresponds to an array index (from 0 to 3)

// Map from LED/Button -> POSSIBLE module pin number (depending on jumper position)
u8_t possiblePinMappings[][2] = {{5, 31}, {6, 32}, {35, 33}, {36, 34}};

// Map from LED/Button -> ACTUAL module pin number
u8_t registeredPinMappings[4];

// Map from Button -> Callback function
dvbd_btn_callback buttonCallbacks[4];

// Get the module pin number using the LED/Button and jumper position, and stores it in the map
ca_error mapLEDBtnToPin(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	// Get the correct pin number based upon jumper position
	// Store the mapping from this LED/Button to the pin number
	if (jumperPos == JUMPER_POS_1)
	{
		registeredPinMappings[ledBtn] = possiblePinMappings[ledBtn][0];
	}
	else
	{
		registeredPinMappings[ledBtn] = possiblePinMappings[ledBtn][1];
	}

	return 0;
}

// Set the functionality of the LED to an open drain output
ca_error DVBD_RegisterLEDOutput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	// Remap the pin
	mapLEDBtnToPin(ledBtn, jumperPos);

	// Register the pin
	return BSP_ModuleRegisterGPIOOutputOD(registeredPinMappings[ledBtn], MODULE_PIN_TYPE_LED);
}

// Set the state of the LED
ca_error DVBD_SetLED(dvbd_led_btn ledBtn, u8_t val)
{
	// Change the state of the LED
	return BSP_ModuleSetGPIOPin(registeredPinMappings[ledBtn], val);
}

// Set the functionality of a button to be an input
ca_error DVBD_RegisterButtonInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	// Remap the pin
	mapLEDBtnToPin(ledBtn, jumperPos);

	// Define the GPIO arguments
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_OFF;

	// Register the pin
	return BSP_ModuleRegisterGPIOInput(&args);
}

// Get the state of the LED/button
ca_error DVBD_Sense(dvbd_led_btn ledBtn, u8_t *val)
{
	// Change the state of the LED
	return BSP_ModuleSenseGPIOPin(registeredPinMappings[ledBtn], val);
}

// Set a callback function to a button when it is short pressed
ca_error DVBD_SetButtonShortPressCallback(dvbd_led_btn ledBtn, void (*callback)())
{
	buttonCallbacks[ledBtn].shortPressCallback = callback;

	return 0;
}

// Set a callback function to a button when it is long pressed
//ca_error DVBD_SetButtonLongPressCallback(dvbd_led_btn ledBtn, void (*callback)(), u32_t timeThreshold)
//{
//	buttonCallbacks[ledBtn].longPressCallback      = callback;
//	buttonCallbacks[ledBtn].longPressTimeThreshold = timeThreshold;
//
//	return 0;
//}

// Set a callback function to a button when it is held
ca_error DVBD_SetButtonHoldCallback(dvbd_led_btn ledBtn, void (*callback)())
{
	buttonCallbacks[ledBtn].holdCallback = callback;

	return 0;
}

// Activate callbacks for any buttons that are currently being pressed
ca_error DVBD_PollButtons()
{
	// Loop through each button
	for (u8_t ledBtn = 0; ledBtn < 4; ledBtn++)
	{
		// Check if the button is registered with any callback
		if (buttonCallbacks[ledBtn].shortPressCallback != NULL || buttonCallbacks[ledBtn].longPressCallback != NULL ||
		    buttonCallbacks[ledBtn].holdCallback != NULL)
		{
			// Get the current state of the button
			u8_t pressed;
			DVBD_Sense(ledBtn, &pressed);

			if (pressed == 0)
			{
				// If it was pressed, increment the pressed time
				buttonCallbacks[ledBtn].currentPressTime += 1;

				if (buttonCallbacks[ledBtn].holdCallback != NULL)
				{
					buttonCallbacks[ledBtn].holdCallback();
				}
			}
			else if (buttonCallbacks[ledBtn].currentPressTime != 0)
			{
				// The button press was lifted
				//if (buttonCallbacks[ledBtn].currentPressTime > buttonCallbacks[ledBtn].longPressTimeThreshold &&
				//    buttonCallbacks[ledBtn].longPressCallback != NULL)
				//{
				//	// Call the long press call back if the time has exceeded the threshold
				//	buttonCallbacks[ledBtn].longPressCallback();
				//}
				//else
				//{
				buttonCallbacks[ledBtn].shortPressCallback();
				//}

				// Reset the buttons press timer
				buttonCallbacks[ledBtn].currentPressTime = 0;
			}
		}
	}

	return 0;
}

// Modify possible pin mappings
ca_error DVBD_modifyPinMappings(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos, u8_t new_pin)
{
	possiblePinMappings[ledBtn][jumperPos] = new_pin;
	return 0;
}