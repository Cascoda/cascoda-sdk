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
#include "devboard_btn_ext.h"

// For the following arrays,
// each LED/Button corresponds to an array index (from 0 to 3)

// if an external GPIO extender chip is used, NUM_LEDBTN_EXT has to be set in devboard_btn_ext.h
// Default for NUM_LEDBTN_EXT is 0
// Maximum for NUM_LEDBTN_EXT is 8
// Note that even if NUM_LEDBTN_EXT is non-zero and no extender chip is declared, all button
// functions behave as normal if no callbacks for extended buttons are declared.

// Map from LED/Button -> POSSIBLE module pin number (depending on jumper position)
u8_t possiblePinMappings[][2] = {{5, 31}, {6, 32}, {35, 33}, {36, 34}};

// Map from LED/Button -> ACTUAL module pin number
u8_t registeredPinMappings[NUM_LEDBTN];

// Map from Button -> Callback function
dvbd_btn_callback_info buttonCallbacks[NUM_LEDBTN + NUM_LEDBTN_EXT];

// Store LED/Button type
u8_t registeredPinTypes[NUM_LEDBTN];

// Powerdown/sleep mode selection:
// PDM_POWERDOWN if any GPIO interrupts wakeups declared
// PDM_POWEROFF  if timer only
static u8_t UseGPIOWakeup = 0;

// ISR for buttons
static int btn_isr(void)
{
	/* only wakeup, the rest is handled in DVBD_PollButtons */
	return 0;
}

// Get the module pin number using the LED/Button and jumper position, and stores it in the map
static void mapLEDBtnToPin(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
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
}

// Set the functionality of the LED to an open drain output
ca_error DVBD_RegisterLEDOutput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	// Remap the pin
	mapLEDBtnToPin(ledBtn, jumperPos);
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_LED;

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
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_BTN;

	// Define the GPIO arguments
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_OFF;

	// Register the pin
	return BSP_ModuleRegisterGPIOInput(&args);
}

// Set the functionality of a button to be an input controlled with interrupt
ca_error DVBD_RegisterButtonIRQInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	// Remap the pin
	mapLEDBtnToPin(ledBtn, jumperPos);
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_BTN;

	// Define the GPIO arguments
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_FALL;
	args.callback = btn_isr;
	DVBD_SetGPIOWakeup();

	// Register the pin
	return BSP_ModuleRegisterGPIOInput(&args);
}

// set the functionality of a pin to be shared LED and button
ca_error DVBD_RegisterSharedButtonLED(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	mapLEDBtnToPin(ledBtn, jumperPos);
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_SHARED;

	// Define the GPIO arguments
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_OFF;

	// Register the pin
	return BSP_ModuleRegisterGPIOSharedInputOutputOD(&args, MODULE_PIN_TYPE_LED);
}

// set the functionality of a pin to be shared LED and IRQ button
ca_error DVBD_RegisterSharedIRQButtonLED(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	mapLEDBtnToPin(ledBtn, jumperPos);
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_SHARED;

	// Define the GPIO arguments
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_FALL;
	args.callback = btn_isr;
	DVBD_SetGPIOWakeup();

	// Register the pin
	return BSP_ModuleRegisterGPIOSharedInputOutputOD(&args, MODULE_PIN_TYPE_LED);
}

ca_error DVBD_DeRegister(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos)
{
	// Remap the pin
	mapLEDBtnToPin(ledBtn, jumperPos);
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;

	return BSP_ModuleDeregisterGPIOPin(registeredPinMappings[ledBtn]);
}

// Get the state of the LED/button
ca_error DVBD_Sense(dvbd_led_btn ledBtn, u8_t *val)
{
	u8_t     save;
	ca_error ret;
	u8_t     type = registeredPinTypes[ledBtn];

	if (type == PINTYPE_SHARED)
	{
		// Save the current output latch value
		ret = DVBD_SenseOutput(ledBtn, &save);
		if (ret != CA_ERROR_SUCCESS)
			return ret;
		// Set the output to high briefly
		// only works since we use open drain outputs.
		DVBD_SetLED(ledBtn, 1);

		// We need a very small delay here to allow for
		// RC time constant of pullup resistor
		WAIT_ms(BTN_SHARED_SENSE_DELAY);
	}

	// Detect the state of the button
	ret = BSP_ModuleSenseGPIOPin(registeredPinMappings[ledBtn], val);

	if (type == PINTYPE_SHARED && ret == CA_ERROR_SUCCESS)
	{
		// Restore the output latch value
		// Ensure we set-back the saved value
		// even if there is a read error
		ret |= DVBD_SetLED(ledBtn, save);
	}
	return ret;
}

ca_error DVBD_SenseOutput(dvbd_led_btn ledBtn, u8_t *val)
{
	// Get the output state of a pin
	return BSP_ModuleSenseGPIOPinOutput(registeredPinMappings[ledBtn], val);
}

// Set a callback function to a button when it is short pressed
ca_error DVBD_SetButtonShortPressCallback(dvbd_led_btn      ledBtn,
                                          dvbd_btn_callback callback,
                                          void             *context,
                                          u8_t              shortPressMode)
{
	buttonCallbacks[ledBtn].shortPressCallback = callback;
	buttonCallbacks[ledBtn].shortPressContext  = context;
	buttonCallbacks[ledBtn].shortPressMode     = shortPressMode;
	return CA_ERROR_SUCCESS;
}

// Set a callback function to a button when it is long pressed
ca_error DVBD_SetButtonLongPressCallback(dvbd_led_btn      ledBtn,
                                         dvbd_btn_callback callback,
                                         void             *context,
                                         u32_t             timeThreshold)
{
	buttonCallbacks[ledBtn].longPressCallback      = callback;
	buttonCallbacks[ledBtn].longPressContext       = context;
	buttonCallbacks[ledBtn].longPressTimeThreshold = timeThreshold;
	return CA_ERROR_SUCCESS;
}

// Set a callback function to a button when it is held
ca_error DVBD_SetButtonHoldCallback(dvbd_led_btn ledBtn, dvbd_btn_callback callback, void *context, u32_t TimeInterval)
{
	buttonCallbacks[ledBtn].holdCallback     = callback;
	buttonCallbacks[ledBtn].holdContext      = context;
	buttonCallbacks[ledBtn].holdTimeInterval = TimeInterval;
	return CA_ERROR_SUCCESS;
}

// Activate callbacks for any buttons that are currently being pressed
ca_error DVBD_PollButtons()
{
#define CALLBACKS (buttonCallbacks[ledBtn])

	u32_t current_time;

	// Loop through each button
	for (u8_t ledBtn = 0; ledBtn < NUM_LEDBTN + NUM_LEDBTN_EXT; ledBtn++)
	{
		// Check if the button is registered with any callback
		if (CALLBACKS.shortPressCallback != NULL || CALLBACKS.longPressCallback != NULL ||
		    CALLBACKS.holdCallback != NULL)
		{
			// Get the current state of the button
			u8_t pressed, port;

			if (ledBtn < NUM_LEDBTN)
				DVBD_Sense(ledBtn, &pressed);
			else
			{
				if (ledBtn == NUM_LEDBTN) // read all extended at once to minimise i2c comms
				{
					DVBD_SenseAllExt(&port);
				}
				pressed = (port >> (ledBtn - NUM_LEDBTN)) & 0x01;
			}

			current_time = TIME_ReadAbsoluteTime();

			/* button pressed */
			if ((pressed == BTN_PRESSED) && (CALLBACKS.lastState == BTN_RELEASED))
			{
				CALLBACKS.currentPressTime = current_time;
				CALLBACKS.lastState        = pressed;
				CALLBACKS.holdTimeLast     = current_time;
				/* handle shortPressCallback immediately */
				if ((CALLBACKS.shortPressCallback != NULL) && (CALLBACKS.shortPressMode == BTN_SHORTPRESS_PRESSED))
				{
					CALLBACKS.shortPressCallback(CALLBACKS.shortPressContext);
				}
			}

			/* hold functionality */
			if ((CALLBACKS.holdCallback != NULL) && (CALLBACKS.lastState == BTN_PRESSED))
			{
				if (current_time >= (CALLBACKS.holdTimeLast + CALLBACKS.holdTimeInterval))
				{
					CALLBACKS.holdCallback(CALLBACKS.holdContext);
					CALLBACKS.holdTimeLast += CALLBACKS.holdTimeInterval;
				}
			}

			/* button released */
			if ((pressed == BTN_RELEASED) && (CALLBACKS.lastState == BTN_PRESSED))
			{
				/* handle short press functionality if not already handled */
				if ((CALLBACKS.shortPressCallback != NULL) && (CALLBACKS.shortPressMode == BTN_SHORTPRESS_RELEASED))
				{
					if (((current_time <= (CALLBACKS.currentPressTime + CALLBACKS.longPressTimeThreshold)) || (CALLBACKS.longPressCallback == NULL)) &&
					    (current_time > (CALLBACKS.currentPressTime + BTN_MIN_PRESS_TIME)))
					{
						CALLBACKS.shortPressCallback(CALLBACKS.shortPressContext);
					}
				}

				/* long press functionality */
				if (CALLBACKS.longPressCallback != NULL)
				{
					if (current_time > (CALLBACKS.currentPressTime + CALLBACKS.longPressTimeThreshold))
					{
						CALLBACKS.longPressCallback(CALLBACKS.longPressContext);
					}
				}

				CALLBACKS.lastState        = pressed;
				CALLBACKS.currentPressTime = 0;
			}
		}
		else
		{
			CALLBACKS.lastState = BTN_RELEASED; /* not registererd as button */
		}
	}

#undef CALLBACKS

	return CA_ERROR_SUCCESS;
}

// Modify possible pin mappings
ca_error DVBD_modifyPinMappings(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos, u8_t new_pin)
{
	possiblePinMappings[ledBtn][jumperPos] = new_pin;
	return CA_ERROR_SUCCESS;
}

// Register that GPIOs are used for wakeup
void DVBD_SetGPIOWakeup(void)
{
	UseGPIOWakeup = 1;
}

// Check if all buttons have been handled
// Note: Only use when DVBD_PollButtons() is in the main loop
bool DVBD_CanSleep(void)
{
	for (u8_t ledBtn = 0; ledBtn < NUM_LEDBTN + NUM_LEDBTN_EXT; ledBtn++)
	{
		if (buttonCallbacks[ledBtn].lastState == BTN_PRESSED)
			return false;
	}

	return true;
}

// Sleep / Powerdown
ca_error DVBD_DevboardSleep(uint32_t aSleepTime, struct ca821x_dev *pDeviceRef)
{
	u8_t pdmode;

	if (UseGPIOWakeup)
		pdmode = PDM_POWERDOWN;
	else
		pdmode = PDM_POWEROFF;

	EVBME_PowerDown(pdmode, aSleepTime, pDeviceRef);

	return CA_ERROR_SUCCESS;
}
