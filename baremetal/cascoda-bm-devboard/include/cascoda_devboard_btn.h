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

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_error.h"

/** Jumper position controls which module pin is used for the LED/Button */
typedef enum dvbd_led_btn_jumper_position
{
	JUMPER_POS_1 = 0, //!< Jumper connects pins 1 and 2
	JUMPER_POS_2 = 1  //!< Jumper connects pins 2 and 3
} dvbd_led_btn_jumper_position;

/** Number of the LED/Button */
typedef enum dvbd_led_btn
{
	LED_BTN_0 = 0,
	LED_BTN_1 = 1,
	LED_BTN_2 = 2,
	LED_BTN_3 = 3
} dvbd_led_btn;

/** Callbacks and associated timers for the buttons */
typedef struct dvbd_btn_callback
{
	void (*shortPressCallback)(); /**< Callback function for a short button press */
	void (*longPressCallback)();  /**< Callback function for a long button press */
	void (*holdCallback)();       /**< Callback function for when the button is held */
	u32_t currentPressTime;       /**< Duration the button has been pressed for */
	u32_t longPressTimeThreshold; /**< Time above which a button press is considered a long press */
} dvbd_btn_callback;

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
 * \brief Get the state of the LED/Button
 * \param ledBtn - reference to LED/Button
 * \param val - the state of the LED/Button
 * \return status
 *
 */
ca_error DVBD_Sense(dvbd_led_btn ledBtn, u8_t *val);

/**
 * \brief Set a callback function to a button when it is short pressed
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \return status
 *
 */
ca_error DVBD_SetButtonShortPressCallback(dvbd_led_btn ledBtn, void (*callback)());

/*
 * \brief Set a callback function to a button when it is long pressed
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \param timeThreshold - time above which a button press is considered a long press
 * \return status
 *
 */
//ca_error DVBD_SetButtonLongPressCallback(dvbd_led_btn ledBtn, void (*callback)(), u32_t timeThreshold);

/**
 * \brief Set a callback function to a button when it is held
 * \param ledBtn - reference to LED/Button
 * \param callback - function to call
 * \return status
 *
 */
ca_error DVBD_SetButtonHoldCallback(dvbd_led_btn ledBtn, void (*callback)());

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