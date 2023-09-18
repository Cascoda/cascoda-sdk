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
 * Extended Button interface for devboard, using a GPIO extender chip.
 * Implemented for EXPAND13 Click board.
*/

#include "devboard_btn_ext.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_time.h"
#include "devboard_btn.h"
#include "expand13_click.h"

#define IS_INP(pin) ((dvbd_extpin_is_inp >> pin) & 0x01)
#define IS_OUT(pin) ((dvbd_extpin_is_out >> pin) & 0x01)
#define SET_INP(pin) (dvbd_extpin_is_inp |= (0x01 << pin))
#define SET_OUT(pin) (dvbd_extpin_is_out |= (0x01 << pin))
#define CLR_INP(pin) (dvbd_extpin_is_inp &= ~(0x01 << pin))
#define CLR_OUT(pin) (dvbd_extpin_is_out &= ~(0x01 << pin))

/* mapping from Button -> Callback function */
extern dvbd_btn_callback_info buttonCallbacks[];

/* pin registration if not done on extender chip for checks and to avoid unnecessary i2c comms */
static uint8_t dvbd_extpin_is_inp = 0x00;
static uint8_t dvbd_extpin_is_out = 0x00;

// Set the functionality of the LED to an open drain output
ca_error DVBD_RegisterLEDOutputExt(dvbd_led_btn_ext ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (IS_INP(ledBtn) || IS_OUT(ledBtn))
		return CA_ERROR_FAIL;

	/* default setup */
	buttonCallbacks[ledBtn + NUM_LEDBTN].lastState = BTN_RELEASED;

	/* Register the pin */
	SET_OUT(ledBtn);

	return CA_ERROR_SUCCESS;
}

// Set the state of the LED
ca_error DVBD_SetLEDExt(dvbd_led_btn_ext ledBtn, u8_t val)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_OUT(ledBtn))
		return CA_ERROR_FAIL;

	/* Change the state of the LED */
	if (MIKROSDK_EXPAND13_SetOutput(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Set the functionality of a button to be an input
ca_error DVBD_RegisterButtonInputExt(dvbd_led_btn_ext ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (IS_INP(ledBtn) || IS_OUT(ledBtn))
		return CA_ERROR_FAIL;

	/* default setup */
	buttonCallbacks[ledBtn + NUM_LEDBTN].lastState = BTN_RELEASED;

	/* Register the pin */
	SET_INP(ledBtn);

	return CA_ERROR_SUCCESS;
}

// De-Register a button or LED
ca_error DVBD_DeRegisterExt(dvbd_led_btn_ext ledBtn)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;

	/* de-register */
	CLR_INP(ledBtn);
	CLR_OUT(ledBtn);

	buttonCallbacks[ledBtn + NUM_LEDBTN].lastState = BTN_RELEASED;

	return CA_ERROR_SUCCESS;
}

// Get the state of the LED/button
ca_error DVBD_SenseExt(dvbd_led_btn_ext ledBtn, u8_t *val)
{
	/* check validity */
	if (!IS_INP(ledBtn))
		return CA_ERROR_FAIL;

	/* read the state */
	if (MIKROSDK_EXPAND13_SenseInput(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Get the state of all buttons - saves on i2c comms when checking inputs
ca_error DVBD_SenseAllExt(u8_t *values)
{
	/* check at least one input is registered */
	if (!dvbd_extpin_is_inp)
		return CA_ERROR_FAIL;

	/* read the state */
	if (MIKROSDK_EXPAND13_Acquire(values))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Set a callback function to a button when it is short pressed
ca_error DVBD_SetButtonShortPressCallbackExt(dvbd_led_btn_ext  ledBtn,
                                             dvbd_btn_callback callback,
                                             void             *context,
                                             u8_t              shortPressMode)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_INP(ledBtn))
		return CA_ERROR_FAIL;

	/* populate structure */
	buttonCallbacks[ledBtn + NUM_LEDBTN].shortPressCallback = callback;
	buttonCallbacks[ledBtn + NUM_LEDBTN].shortPressContext  = context;
	buttonCallbacks[ledBtn + NUM_LEDBTN].shortPressMode     = shortPressMode;
	return CA_ERROR_SUCCESS;
}

// Set a callback function to a button when it is long pressed
ca_error DVBD_SetButtonLongPressCallbackExt(dvbd_led_btn_ext  ledBtn,
                                            dvbd_btn_callback callback,
                                            void             *context,
                                            u32_t             timeThreshold)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_INP(ledBtn))
		return CA_ERROR_FAIL;

	/* populate structure */
	buttonCallbacks[ledBtn + NUM_LEDBTN].longPressCallback      = callback;
	buttonCallbacks[ledBtn + NUM_LEDBTN].longPressContext       = context;
	buttonCallbacks[ledBtn + NUM_LEDBTN].longPressTimeThreshold = timeThreshold;
	return CA_ERROR_SUCCESS;
}

// Set a callback function to a button when it is held
ca_error DVBD_SetButtonHoldCallbackExt(dvbd_led_btn_ext  ledBtn,
                                       dvbd_btn_callback callback,
                                       void             *context,
                                       u32_t             TimeInterval)
{
	/* check validity */
	if (ledBtn == NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (ledBtn > NUM_LEDBTN_EXT)
		return CA_ERROR_FAIL;
	else if (!IS_INP(ledBtn))
		return CA_ERROR_FAIL;

	/* populate structure */
	buttonCallbacks[ledBtn + NUM_LEDBTN].holdCallback     = callback;
	buttonCallbacks[ledBtn + NUM_LEDBTN].holdContext      = context;
	buttonCallbacks[ledBtn + NUM_LEDBTN].holdTimeInterval = TimeInterval;
	return CA_ERROR_SUCCESS;
}
