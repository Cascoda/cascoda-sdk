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
#include "cascoda_btn.h"

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

/* re-mapping for functions that have 1-to-1 functionality */
#define DVBD_SetLED Btn_SetLED
#define DVBD_SetGPIOWakeup Btn_IncrementGPIOWakeup
#define DVBD_SetSleepPermanently Btn_SetSleepPermanently
#define DVBD_Sense Btn_Sense
#define DVBD_SenseOutput Btn_SenseOutput
#define DVBD_SetButtonShortPressCallback Btn_SetButtonShortPressCallback
#define DVBD_SetButtonLongPressCallback Btn_SetButtonLongPressCallback
#define DVBD_SetButtonHoldCallback Btn_SetButtonHoldCallback
#define DVBD_PollButtons Btn_PollButtons
#define DVBD_CanSleep Btn_CanSleep
#define DVBD_DevboardSleep Btn_DevboardSleep

/**
 * \brief Register LED output (open drain)
 * \param ledBtn - reference to LED
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterLEDOutput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Register button input
 * \param ledBtn - reference to button
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterButtonInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Register button input with interrupt (for sleepy devices)
 * \param ledBtn - reference to button
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterButtonIRQInput(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Register button as shared input/output
 * \param ledBtn - reference to button
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterSharedButtonLED(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Set the functionality of a button to be shared interrupt input/output
 * \param ledBtn - reference to button
 * \param jumperPos - posititon of the jumper
 * \return status
 *
 */
ca_error DVBD_RegisterSharedIRQButtonLED(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief De-Register an LED or Button Pin
 * \param ledBtn - reference to LED/Button
 * \param jumperPos - position of the jumper
 * \return status
 *
 */
ca_error DVBD_DeRegister(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos);

/**
 * \brief Modify possible pin mappings
 * \return status
 *
 */
ca_error DVBD_modifyPinMappings(dvbd_led_btn ledBtn, dvbd_led_btn_jumper_position jumperPos, u8_t new_pin);

#endif // DEVBOARD_BTN_H
