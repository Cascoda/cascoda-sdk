/*
 *  Copyright (c) 2023, Cascoda Ltd.
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
 * Button / LED Functions for Cascoda UI
*/

#include "cascoda_btn.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"

/* Map from Button declaration to actual module pin number */
uint8_t registeredPinMappings[NUM_LEDBTN];

/* Map from Button -> Callback function */
static btn_callback_info buttonCallbacks[NUM_LEDBTN];

/* Store LED/Button type */
static uint8_t registeredPinTypes[NUM_LEDBTN];

// Powerdown/sleep mode selection:
// PDM_POWERDOWN if any GPIO interrupt wakeups declared
// PDM_POWEROFF  if timer only
static uint8_t UseGPIOWakeup = 0;

// button interrupt occured
static uint8_t BtnHasInterrupt = 0;

/* ISR for buttons */
static int btn_isr(void)
{
	/* set interrupt flag */
	BtnHasInterrupt = 1;
	/* only wakeup, the rest is handled in in polling function */
	return 0;
}

/* register LED output (open drain) */
ca_error Btn_RegisterLEDOutput(uint8_t ledBtn)
{
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_LED;

	/* Register the pin */
	return BSP_ModuleRegisterGPIOOutputOD(registeredPinMappings[ledBtn], MODULE_PIN_TYPE_LED);
}

/* register button input */
ca_error Btn_RegisterButtonInput(uint8_t ledBtn)
{
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_BTN;

	/* define the GPIO arguments */
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_OFF;

	/* Register the pin */
	return BSP_ModuleRegisterGPIOInput(&args);
}

/* register button input with interrupt (for sleepy devices) */
ca_error Btn_RegisterButtonIRQInput(uint8_t ledBtn)
{
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_BTN;

	/* define the GPIO arguments */
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_FALL;
	args.callback = btn_isr;
	Btn_SetGPIOWakeup();

	/* Register the pin */
	return BSP_ModuleRegisterGPIOInput(&args);
}

/* register button as shared input/output */
ca_error Btn_RegisterSharedButtonLED(uint8_t ledBtn)
{
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_SHARED;

	/* define the GPIO arguments */
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_OFF;

	/* Register the pin */
	return BSP_ModuleRegisterGPIOSharedInputOutputOD(&args, MODULE_PIN_TYPE_LED);
}

/* register pin to be shared LED and button with interrupt (for sleepy devices) */
ca_error Btn_RegisterSharedIRQButtonLED(uint8_t ledBtn)
{
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;
	registeredPinTypes[ledBtn]        = PINTYPE_SHARED;

	/* define the GPIO arguments */
	struct gpio_input_args args;
	args.mpin     = registeredPinMappings[ledBtn];
	args.pullup   = MODULE_PIN_PULLUP_OFF;
	args.debounce = MODULE_PIN_DEBOUNCE_ON;
	args.irq      = MODULE_PIN_IRQ_FALL;
	args.callback = btn_isr;
	Btn_SetGPIOWakeup();

	/* Register the pin */
	return BSP_ModuleRegisterGPIOSharedInputOutputOD(&args, MODULE_PIN_TYPE_LED);
}

/* De-Register an LED or Button Pin */
ca_error Btn_DeRegister(uint8_t ledBtn)
{
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;

	return BSP_ModuleDeregisterGPIOPin(registeredPinMappings[ledBtn]);
}

/* Set a callback function to a button when it is short pressed */
ca_error Btn_SetButtonShortPressCallback(uint8_t ledBtn, btn_callback callback, void *context, uint8_t shortPressMode)
{
	buttonCallbacks[ledBtn].shortPressCallback = callback;
	buttonCallbacks[ledBtn].shortPressContext  = context;
	buttonCallbacks[ledBtn].shortPressMode     = shortPressMode;
	return CA_ERROR_SUCCESS;
}

/* Set a callback function to a button when it is long pressed */
ca_error Btn_SetButtonLongPressCallback(uint8_t ledBtn, btn_callback callback, void *context, uint32_t timeThreshold)
{
	buttonCallbacks[ledBtn].longPressCallback      = callback;
	buttonCallbacks[ledBtn].longPressContext       = context;
	buttonCallbacks[ledBtn].longPressTimeThreshold = timeThreshold;
	return CA_ERROR_SUCCESS;
}

/* Set a callback function to a button when it is held */
ca_error Btn_SetButtonHoldCallback(uint8_t ledBtn, btn_callback callback, void *context, uint32_t TimeInterval)
{
	buttonCallbacks[ledBtn].holdCallback     = callback;
	buttonCallbacks[ledBtn].holdContext      = context;
	buttonCallbacks[ledBtn].holdTimeInterval = TimeInterval;
	return CA_ERROR_SUCCESS;
}

/* set the state of the LED */
ca_error Btn_SetLED(uint8_t ledBtn, uint8_t val)
{
	// Change the state of the LED
	return BSP_ModuleSetGPIOPin(registeredPinMappings[ledBtn], val);
}

// Get the state of the LED/button
ca_error Btn_Sense(uint8_t ledBtn, uint8_t *val)
{
	uint8_t  save;
	ca_error ret;
	uint8_t  type = registeredPinTypes[ledBtn];

	if (type == PINTYPE_SHARED)
	{
		// Save the current output latch value
		ret = Btn_SenseOutput(ledBtn, &save);
		if (ret != CA_ERROR_SUCCESS)
			return ret;
		// Set the output to high briefly
		// only works since we use open drain outputs.
		Btn_SetLED(ledBtn, 1);

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
		ret |= Btn_SetLED(ledBtn, save);
	}
	return ret;
}

/* get the output state of the LED */
ca_error Btn_SenseOutput(uint8_t ledBtn, uint8_t *val)
{
	// Get the output state of a pin
	return BSP_ModuleSenseGPIOPinOutput(registeredPinMappings[ledBtn], val);
}

/* main polling function to activate callbacks for any buttons that are currently being pressed */
ca_error Btn_PollButtons(void)
{
	uint8_t pressed;

	/* Loop through each button */
	for (uint8_t ledBtn = 0; ledBtn < NUM_LEDBTN; ledBtn++)
	{
		/* Check if the button is registered with any callback */
		if (buttonCallbacks[ledBtn].shortPressCallback != NULL || buttonCallbacks[ledBtn].longPressCallback != NULL ||
		    buttonCallbacks[ledBtn].holdCallback != NULL)
		{
			/* Get the current state of the button */
			Btn_Sense(ledBtn, &pressed);

			/* process the button callbacks */
			Btn_HandleButtonCallbacks(&buttonCallbacks[ledBtn], pressed);
		}
		else
		{
			buttonCallbacks[ledBtn].lastState = BTN_RELEASED; /* not registererd as button */
		}
	}

	BtnHasInterrupt = 0;
	return CA_ERROR_SUCCESS;
}

/* process the button callbacks and timing */
/* This is shared by Btn_PollButtons() and Btn_PollButtonsExt() */
ca_error Btn_HandleButtonCallbacks(btn_callback_info *callback, uint8_t pressed)
{
	uint32_t current_time;

	current_time = TIME_ReadAbsoluteTime();

	/* button pressed */
	if ((pressed == BTN_PRESSED) && (callback->lastState == BTN_RELEASED))
	{
		callback->currentPressTime = current_time;
		callback->lastState        = pressed;
		callback->holdTimeLast     = current_time;
		/* handle shortPressCallback immediately */
		if ((callback->shortPressCallback != NULL) && (callback->shortPressMode == BTN_SHORTPRESS_PRESSED))
		{
			callback->shortPressCallback(callback->shortPressContext);
		}
	}

	/* hold functionality */
	if ((callback->holdCallback != NULL) && (callback->lastState == BTN_PRESSED))
	{
		if (current_time >= (callback->holdTimeLast + callback->holdTimeInterval))
		{
			callback->holdCallback(callback->holdContext);
			callback->holdTimeLast += callback->holdTimeInterval;
		}
	}

	/* button released */
	if ((pressed == BTN_RELEASED) && (callback->lastState == BTN_PRESSED))
	{
		/* handle short press functionality if not already handled */
		if ((callback->shortPressCallback != NULL) && (callback->shortPressMode == BTN_SHORTPRESS_RELEASED))
		{
			if (((current_time <= (callback->currentPressTime + callback->longPressTimeThreshold)) ||
			     (callback->longPressCallback == NULL)) &&
			    (current_time > (callback->currentPressTime + BTN_MIN_PRESS_TIME)))
			{
				callback->shortPressCallback(callback->shortPressContext);
			}
		}

		/* long press functionality */
		if (callback->longPressCallback != NULL)
		{
			if (current_time > (callback->currentPressTime + callback->longPressTimeThreshold))
			{
				callback->longPressCallback(callback->longPressContext);
			}
		}

		callback->lastState        = pressed;
		callback->currentPressTime = 0;
	}
}

/* Register that GPIOs are used for wakeup */
void Btn_SetGPIOWakeup(void)
{
	UseGPIOWakeup = 1;
}

/* Check if all buttons have been handled */
/* Note: Only use when Btn_PollButtons() is in the main loop */
bool Btn_CanSleep(void)
{
	if (BtnHasInterrupt)
		return false;

	for (uint8_t ledBtn = 0; ledBtn < NUM_LEDBTN; ledBtn++)
	{
		if (buttonCallbacks[ledBtn].lastState == BTN_PRESSED)
			return false;
	}

	return true;
}

/* Sleep / Powerdown */
ca_error Btn_DevboardSleep(uint32_t aSleepTime, struct ca821x_dev *pDeviceRef)
{
	uint8_t pdmode;

	if (UseGPIOWakeup)
		pdmode = PDM_POWERDOWN;
	else
		pdmode = PDM_POWEROFF;

	EVBME_PowerDown(pdmode, aSleepTime, pDeviceRef);

	return CA_ERROR_SUCCESS;
}
