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

#ifndef CASCODA_BTN_H
#define CASCODA_BTN_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "ca821x_error.h"

/* max. number of LED/Button pairs on board */
#define NUM_LEDBTN 4

/* minimum press time for additional de-bouncing in [ms] */
#define BTN_MIN_PRESS_TIME 5

#ifndef BTN_SHARED_SENSE_DELAY
#define BTN_SHARED_SENSE_DELAY 2
#endif

/* Execute short press callback when pressed or on release
 * Note: BTN_SHORTPRESS_RELEASED should only be used when
 * the same button is being registered with a long press
 * callback.
 */
enum btn_shortpress_mode
{
	BTN_SHORTPRESS_PRESSED  = 0,
	BTN_SHORTPRESS_RELEASED = 1
};

/* Button state */
enum btn_button_state
{
	BTN_PRESSED  = 0,
	BTN_RELEASED = 1
};

/* pin types */
enum btn_pin_type
{
	PINTYPE_NONE   = 0,
	PINTYPE_LED    = 1,
	PINTYPE_BTN    = 2,
	PINTYPE_SHARED = 3,
};

/* dvbd callback type definition */
typedef void (*btn_callback)(void* context);

/** Callbacks and associated timers for the buttons */
typedef struct btn_callback_info
{
	btn_callback shortPressCallback; /**< Callback function for a short button press */
	void*        shortPressContext;  /**< Context for shortPressCallback */

	btn_callback longPressCallback; /**< Callback function for a long button press */
	void*        longPressContext;  /**< Context for longPressCallback */

	btn_callback holdCallback; /**< Callback function for when the button is held */
	void*        holdContext;  /**< Context for holdCallback */

	uint32_t currentPressTime;       /**< Time [ms] when the button has been pressed */
	uint32_t longPressTimeThreshold; /**< Time limit [ms] above which a button press is considered a long press */
	uint32_t holdTimeInterval;       /**< Time interval [ms] for triggering the hold function */
	uint32_t holdTimeLast;           /**< Time [ms] when hold callback was last called */
	uint8_t  lastState;              /**< Last button state */
	uint8_t  shortPressMode;         /**< Short Press callback when pressed or on release */
} btn_callback_info;

/* Map from Button declaration to actual module pin number */
extern uint8_t registeredPinMappings[];

/* functions */

/**
 * \brief Register LED output (open drain)
 * \param ledBtn - reference to LED
 * \return status
 *
 */
ca_error Btn_RegisterLEDOutput(uint8_t ledBtn);

/**
 * \brief Register button input
 * \param ledBtn - reference to button
 * \return status
 *
 */
ca_error Btn_RegisterButtonInput(uint8_t ledBtn);

/**
 * \brief Register button input with interrupt (for sleepy devices)
 * \param ledBtn - reference to button
 * \return status
 *
 */
ca_error Btn_RegisterButtonIRQInput(uint8_t ledBtn);

/**
 * \brief Register button as shared input/output
 * \param ledBtn - reference to button
 * \return status
 *
 */
ca_error Btn_RegisterSharedButtonLED(uint8_t ledBtn);

/**
 * \brief Register button as shared input/output with interrupt (for sleepy devices)
 * \param ledBtn - reference to button
 * \return status
 *
 */
ca_error Btn_RegisterSharedIRQButtonLED(uint8_t ledBtn);

/**
 * \brief De-Register an LED or Button Pin
 * \param ledBtn - reference to LED/Button
 * \return status
 *
 */
ca_error Btn_DeRegister(uint8_t ledBtn);

/**
 * \brief Set a callback function to a button when it is short pressed
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if no context is needed.
 * \param shortPressMode - short press mode (when pressed or when released).
 * \return status
 *
 */
ca_error Btn_SetButtonShortPressCallback(uint8_t ledBtn, btn_callback callback, void* context, uint8_t shortPressMode);

/**
 * \brief Set a callback function to a button when it is long pressed
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param timeThreshold - time above which a button press is considered a long press
 * \return status
 *
 */
ca_error Btn_SetButtonLongPressCallback(uint8_t ledBtn, btn_callback callback, void* context, uint32_t timeThreshold);

/**
 * \brief Set a callback function to a button when it is held
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param TimeInterval - time interval in [ms] in which callback function is called
 * \return status
 *
 */
ca_error Btn_SetButtonHoldCallback(uint8_t ledBtn, btn_callback callback, void* context, uint32_t TimeInterval);

/**
 * \brief Set the state of the LED
 * \param ledBtn - reference to LED
 * \param val - the state of the LED
 * \return status
 *
 */
ca_error Btn_SetLED(uint8_t ledBtn, uint8_t val);

/**
 * \brief Get the state of the LED/Button
 * \param ledBtn - reference to LED/Button
 * \param val - the state of the LED/Button
 * \return status
 *
 */
ca_error Btn_Sense(uint8_t ledBtn, uint8_t* val);

/**
 * \brief Get the output state of the LED
 * \param ledBtn - reference to LED
 * \param val - the state of the LED
 * \return status
 *
 */
ca_error Btn_SenseOutput(uint8_t ledBtn, uint8_t* val);

/**
 * \brief Main polling function to activate callbacks for any buttons that are currently being pressed
 * \return status
 *
 */
ca_error Btn_PollButtons(void);

/**
 * \brief Process the button callbacks and timing
 * \param callback - callback info struct for button
 * \param pressed - the state of the Button
 *
 */
ca_error Btn_HandleButtonCallbacks(btn_callback_info* callback, uint8_t pressed);

/**
 * \brief Register that GPIOs are used for wakeup
 *
 */
void Btn_SetGPIOWakeup(void);

/**
 * \brief Check if all buttons have been handled
 * \return true/false
 *
 */
bool Btn_CanSleep(void);

/**
 * \brief Put board to sleep / powerdown
 * \return status
 *
 */
ca_error Btn_DevboardSleep(uint32_t aSleepTime, struct ca821x_dev* pDeviceRef);

#endif // CASCODA_BTN_H
