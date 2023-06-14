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

#ifndef DEVBOARD_BTN_H
#define DEVBOARD_BTN_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "ca821x_error.h"

#define NUM_LEDBTN 4 /* number of LED/Button pairs on devboard */

#define BTN_MIN_PRESS_TIME 5 /* minimum press time for additional de-bouncing */

/** Jumper position controls which module pin is used for the LED/Button */
typedef enum dvbd_led_btn_jumper_position
{
	JUMPER_POS_1 = 0, //!< Jumper connects pins 1 and 2
	JUMPER_POS_2 = 1  //!< Jumper connects pins 2 and 3
} dvbd_led_btn_jumper_position;

/** Number of the LED/Button */
typedef enum dvbd_led_btn
{
	DEV_SWITCH_1 = 0,
	DEV_SWITCH_2 = 1,
	DEV_SWITCH_3 = 2,
	DEV_SWITCH_4 = 3
} dvbd_led_btn;

/* Execute short press callback when pressed or on release
 * Note: BTN_SHORTPRESS_RELEASED should only be used when
 * the same button is being registered with a long press
 * callback.
 */
enum dvbd_shortpress_mode
{
	BTN_SHORTPRESS_PRESSED  = 0,
	BTN_SHORTPRESS_RELEASED = 1
};

/* Button state */
enum dvbd_button_state
{
	BTN_PRESSED  = 0,
	BTN_RELEASED = 1
};

/* dvbd callback type definition */
typedef void (*dvbd_btn_callback)(void* context);

/** Callbacks and associated timers for the buttons */
typedef struct dvbd_btn_callback_info
{
	dvbd_btn_callback shortPressCallback; /**< Callback function for a short button press */
	void*             shortPressContext;  /**< Context for shortPressCallback */

	dvbd_btn_callback longPressCallback; /**< Callback function for a long button press */
	void*             longPressContext;  /**< Context for longPressCallback */

	dvbd_btn_callback holdCallback; /**< Callback function for when the button is held */
	void*             holdContext;  /**< Context for holdCallback */

	u32_t currentPressTime;       /**< Time [ms] when the button has been pressed */
	u32_t longPressTimeThreshold; /**< Time limit [ms] above which a button press is considered a long press */
	u32_t holdTimeInterval;       /**< Time interval [ms] for triggering the hold function */
	u32_t holdTimeLast;           /**< Time [ms] when hold callback was last called */
	u8_t  lastState;              /**< Last button state */
	u8_t  shortPressMode;         /**< Short Press callback when pressed or on release */
} dvbd_btn_callback_info;

/**
 * \brief Set the functionality of an LED to be an output
 * \param ledBtn - reference to LED
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterLEDOutput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Set the state of the LED
 * \param ledBtn - reference to LED
 * \param val - the state of the LED
 * \return status
 *
 */
ca_error DVBD_SetLED(dvbd_led_btn ledBtn, u8_t val);

/**
 * \brief Set the functionality of a button to be an input
 * \param ledBtn - reference to button
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterButtonInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Set the functionality of a button to be an interrupt input
 * \param ledBtn - reference to button
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterButtonIRQInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief De-Register an LED or Button Pin
 * \param ledBtn - reference to LED/Button
 * \param jumperPos - position of the jumper
 * \return status
 *
 */
ca_error DVBD_DeRegister(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Get the state of the LED/Button
 * \param ledBtn - reference to LED/Button
 * \param val - the state of the LED/Button
 * \return status
 *
 */
ca_error DVBD_Sense(dvbd_led_btn ledBtn, u8_t* val);

/**
 * \brief Set a callback function to a button when it is short pressed
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if no context is needed.
 * \param shortPressMode - short press mode (when pressed or when released). Note:
 * BTN_SHORTPRESS_RELEASED should only be used when
 * the same button is being registered with a long press
 * callback.
 * \return status
 *
 */
ca_error DVBD_SetButtonShortPressCallback(dvbd_led_btn      ledBtn,
                                          dvbd_btn_callback callback,
                                          void*             context,
                                          uint8_t           shortPressMode);

/**
 * \brief Set a callback function to a button when it is long pressed
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param timeThreshold - time above which a button press is considered a long press
 * \return status
 *
 */
ca_error DVBD_SetButtonLongPressCallback(dvbd_led_btn      ledBtn,
                                         dvbd_btn_callback callback,
                                         void*             context,
                                         u32_t             timeThreshold);

/**
 * \brief Set a callback function to a button when it is held
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param TimeInterval - time interval in [ms] in which callback function is called
 * \return status
 *
 */
ca_error DVBD_SetButtonHoldCallback(dvbd_led_btn ledBtn, dvbd_btn_callback callback, void* context, u32_t TimeInterval);

/**
 * \brief Activate callbacks for any buttons that are currently being pressed
 * \return status
 *
 */
ca_error DVBD_PollButtons();

/**
 * \brief Modify possible pin mappings
 * \return status
 *
 */
ca_error DVBD_modifyPinMappings(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos, u8_t new_pin);

/**
 * \brief Register that GPIOs are used for wakeup
 * \return true/false
 *
 */
void DVBD_SetGPIOWakeup(void);

/**
 * \brief Check if all buttons have been handled
 * \return true/false
 *
 */
bool DVBD_CanSleep(void);

/**
 * \brief Put board to sleep / powerdown
 * \return status
 *
 */
ca_error DVBD_DevboardSleep(uint32_t aSleepTime, struct ca821x_dev* pDeviceRef);

#endif // DEVBOARD_BTN_H
