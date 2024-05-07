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
 * Extended button/input/output Functions for Cascoda UI
*/

#include "cascoda_btn_ext.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_time.h"

/* for pin handling */
#define IS_INP(pin) ((extpin_is_inp >> pin) & 0x01)
#define IS_BTN(pin) ((extpin_is_btn >> pin) & 0x01)
#define IS_OUT(pin) ((extpin_is_out >> pin) & 0x01)
#define SET_INP(pin) (extpin_is_inp |= (0x01 << pin))
#define SET_BTN(pin) (extpin_is_btn |= (0x01 << pin))
#define SET_OUT(pin) (extpin_is_out |= (0x01 << pin))
#define CLR_INP(pin) (extpin_is_inp &= ~(0x01 << pin))
#define CLR_BTN(pin) (extpin_is_btn &= ~(0x01 << pin))
#define CLR_OUT(pin) (extpin_is_out &= ~(0x01 << pin))

/* mapping from Button -> Callback function */
static btn_callback_info buttonCallbacks[NUM_LEDBTN_EXT];

/* pin registration done here for checks and to avoid unnecessary i2c comms */
static uint8_t extpin_is_inp = 0x00; /* general input */
static uint8_t extpin_is_btn = 0x00; /* button input */
static uint8_t extpin_is_out = 0x00; /* general output or led */

/* Set the functionality functionality as output */
ca_error Btn_RegisterOutputExt(uint8_t ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (IS_INP(ledBtn) || IS_BTN(ledBtn) || IS_OUT(ledBtn))
		return CA_ERROR_FAIL;

	/* default setup */
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;

	/* Register the pin */
	SET_OUT(ledBtn);

	return CA_ERROR_SUCCESS;
}

/* Set the functionality of a button to be an input */
ca_error Btn_RegisterButtonInputExt(uint8_t ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (IS_INP(ledBtn) || IS_BTN(ledBtn) || IS_OUT(ledBtn))
		return CA_ERROR_FAIL;

	/* default setup */
	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;

	/* Register the pin */
	SET_BTN(ledBtn);

	return CA_ERROR_SUCCESS;
}

/* Set the functionality to be a general input (no interrupt, polling only) */
ca_error Btn_RegisterGeneralInputExt(uint8_t ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (IS_INP(ledBtn) || IS_BTN(ledBtn) || IS_OUT(ledBtn))
		return CA_ERROR_FAIL;

	/* Register the pin */
	SET_INP(ledBtn);

	return CA_ERROR_SUCCESS;
}

/* De-Register a button or output */
ca_error Btn_DeRegisterExt(uint8_t ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;

	/* check if GPIO Wakeup for that pin needs to be reset */
	if (IS_BTN(ledBtn))
		if (Btn_DecrementGPIOWakeup())
			return CA_ERROR_FAIL;

	/* de-register */
	CLR_INP(ledBtn);
	CLR_BTN(ledBtn);
	CLR_OUT(ledBtn);

	buttonCallbacks[ledBtn].lastState = BTN_RELEASED;

	return CA_ERROR_SUCCESS;
}

/* check if valid output/LED */
ca_error Btn_CheckOutputExt(uint8_t ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_OUT(ledBtn))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

/* check if valid input/Button */
ca_error Btn_CheckInputOrButtonExt(uint8_t ledBtn)
{
	/* read the state */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_INP(ledBtn) && !IS_BTN(ledBtn))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

/* check if any button declared */
bool Btn_HasButtonExt(void)
{
	/* check at least one input is registered */
	if (!extpin_is_btn)
		return false;

	return true;
}

/* Set a callback function to a button when it is short pressed */
ca_error Btn_SetButtonShortPressCallbackExt(uint8_t      ledBtn,
                                            btn_callback callback,
                                            void        *context,
                                            uint8_t      shortPressMode)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_BTN(ledBtn))
		return CA_ERROR_FAIL;

	/* populate structure */
	buttonCallbacks[ledBtn].shortPressCallback = callback;
	buttonCallbacks[ledBtn].shortPressContext  = context;
	buttonCallbacks[ledBtn].shortPressMode     = shortPressMode;
	return CA_ERROR_SUCCESS;
}

/* Set a callback function to a button when it is long pressed */
ca_error Btn_SetButtonLongPressCallbackExt(uint8_t ledBtn, btn_callback callback, void *context, uint32_t timeThreshold)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_BTN(ledBtn))
		return CA_ERROR_FAIL;

	/* populate structure */
	buttonCallbacks[ledBtn].longPressCallback      = callback;
	buttonCallbacks[ledBtn].longPressContext       = context;
	buttonCallbacks[ledBtn].longPressTimeThreshold = timeThreshold;
	return CA_ERROR_SUCCESS;
}

/* Set a callback function to a button when it is held */
ca_error Btn_SetButtonHoldCallbackExt(uint8_t ledBtn, btn_callback callback, void *context, uint32_t TimeInterval)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_BTN(ledBtn))
		return CA_ERROR_FAIL;

	/* populate structure */
	buttonCallbacks[ledBtn].holdCallback     = callback;
	buttonCallbacks[ledBtn].holdContext      = context;
	buttonCallbacks[ledBtn].holdTimeInterval = TimeInterval;
	return CA_ERROR_SUCCESS;
}

/* main polling function to activate callbacks for any buttons that are currently being pressed */
ca_error Btn_PollButtonsExt(uint8_t port)
{
	uint8_t pressed;

	/* Loop through each button */
	for (uint8_t ledBtn = 0; ledBtn < NUM_LEDBTN_EXT; ledBtn++)
	{
		/* Check if the button is registered with any callback */
		if ((IS_BTN(ledBtn)) &&
		    (buttonCallbacks[ledBtn].shortPressCallback != NULL || buttonCallbacks[ledBtn].longPressCallback != NULL ||
		     buttonCallbacks[ledBtn].holdCallback != NULL))
		{
			/* Get the current state of the button */
			pressed = (port >> (ledBtn)) & 0x01;

			/* process the button callbacks */
			Btn_HandleButtonCallbacks(&buttonCallbacks[ledBtn], pressed);
		}
		else
		{
			buttonCallbacks[ledBtn].lastState = BTN_RELEASED; /* not registererd as button */
		}
	}

	return CA_ERROR_SUCCESS;
}

/* Check if all buttons have been handled */
/* Note: Only use when Btn_PollButtonsExt() is in the main loop */
bool Btn_CanSleepExt(void)
{
	/* not used if no buttons or inputs declared - external interface might not be present */
	if (!extpin_is_btn)
		return true;

	for (uint8_t ledBtn = 0; ledBtn < NUM_LEDBTN_EXT; ledBtn++)
	{
		if (buttonCallbacks[ledBtn].lastState == BTN_PRESSED)
			return false;
	}

	return true;
}
