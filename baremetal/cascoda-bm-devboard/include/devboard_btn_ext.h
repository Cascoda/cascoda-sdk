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
 * Extended Button interface for devboard, using a GPIO extender chip.
 * Implemented for EXPAND13 Click board.
*/

#ifndef DEVBOARD_BTN_EXT_H
#define DEVBOARD_BTN_EXT_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "ca821x_error.h"
#include "devboard_btn.h"

/* Number of extended I/Os supported
 * Set to 0 if no I/O expander chip used
 * Maximum is 8
 */
#define NUM_LEDBTN_EXT 0

/* Number of the extended LED/Button */
typedef enum dvbd_led_btn_ext
{
	DEV_SWITCH_EXT_1 = 0,
	DEV_SWITCH_EXT_2 = 1,
	DEV_SWITCH_EXT_3 = 2,
	DEV_SWITCH_EXT_4 = 3,
	DEV_SWITCH_EXT_5 = 4,
	DEV_SWITCH_EXT_6 = 5,
	DEV_SWITCH_EXT_7 = 6,
	DEV_SWITCH_EXT_8 = 7,
} dvbd_led_btn_ext;

/**
 * \brief Set the functionality of an LED to be an output
 * \param ledBtn - reference to LED
 * \return status
 *
 */
ca_error DVBD_RegisterLEDOutputExt(dvbd_led_btn_ext ledBtn);

/**
 * \brief Set the state of the LED
 * \param ledBtn - reference to LED
 * \param val - the state of the LED
 * \return status
 *
 */
ca_error DVBD_SetLEDExt(dvbd_led_btn_ext ledBtn, u8_t val);

/**
 * \brief Set the functionality of a button to be an input
 * \param ledBtn - reference to button
 * \return status
 *
 */
ca_error DVBD_RegisterButtonInputExt(dvbd_led_btn_ext ledBtn);

/**
 * \brief De-Register an LED or Button Pin
 * \param ledBtn - reference to LED/Button
 * \return status
 *
 */
ca_error DVBD_DeRegisterExt(dvbd_led_btn_ext ledBtn);

/**
 * \brief Get the state of the LED/Button
 * \param ionr - I/O number
 * \param val - the state of the LED/Button
 * \return status
 *
 */
ca_error DVBD_SenseExt(dvbd_led_btn_ext ledBtn, u8_t* val);

/**
 * \brief Get the state all buttons
 * \param values - the state of all buttons
 * \return status
 *
 */
ca_error DVBD_SenseAllExt(u8_t* values);

/**
 * \brief Set a callback function to a button when it is short pressed
 * \param ledBtn - reference to button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if no context is needed.
 * \param shortPressMode - short press mode (when pressed or when released). Note:
 * BTN_SHORTPRESS_RELEASED should only be used when
 * the same button is being registered with a long press
 * callback.
 * \return status
 *
 */
ca_error DVBD_SetButtonShortPressCallbackExt(dvbd_led_btn_ext  ledBtn,
                                             dvbd_btn_callback callback,
                                             void*             context,
                                             uint8_t           shortPressMode);

/**
 * \brief Set a callback function to a button when it is long pressed
 * \param ledBtn - reference to button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param timeThreshold - time above which a button press is considered a long press
 * \return status
 *
 */
ca_error DVBD_SetButtonLongPressCallbackExt(dvbd_led_btn_ext  ledBtn,
                                            dvbd_btn_callback callback,
                                            void*             context,
                                            u32_t             timeThreshold);

/**
 * \brief Set a callback function to a button when it is held
 * \param ledBtn - reference to button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param TimeInterval - time interval in [ms] in which callback function is called
 * \return status
 *
 */
ca_error DVBD_SetButtonHoldCallbackExt(dvbd_led_btn_ext  ledBtn,
                                       dvbd_btn_callback callback,
                                       void*             context,
                                       u32_t             TimeInterval);

#endif // DEVBOARD_BTN_EXT_H
