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

#ifndef CASCODA_BTN_EXT_H
#define CASCODA_BTN_EXT_H

#include "cascoda_btn.h"

/* Number of extended I/Os supported */
#define NUM_LEDBTN_EXT 8

/* functions */

/**
 * \brief Set the functionality as output/LED
 * \param ledBtn - reference to output/LED
 * \return status
 *
 */
ca_error Btn_RegisterOutputExt(uint8_t ledBtn);

/**
 * \brief Set the functionality of a button to be an input
 * \param ledBtn - reference to button
 * \return status
 *
 */
ca_error Btn_RegisterButtonInputExt(uint8_t ledBtn);

/**
 * \brief Set the functionality to be a general input (no interrupt, polling only)
 * \param ledBtn - reference number to input
 * \return status
 *
 */
ca_error Btn_RegisterGeneralInputExt(uint8_t ledBtn);

/**
 * \brief De-Register an LED or Button Pin
 * \param ledBtn - reference to LED/Button
 * \return status
 *
 */
ca_error Btn_DeRegisterExt(uint8_t ledBtn);

/**
 * \brief check if valid output/LED
 * \param ledBtn - reference to output/LED
 * \param val - the state of the output/LED
 * \return status
 *
 */
ca_error Btn_CheckOutputExt(uint8_t ledBtn);

/**
 * \brief check if valid input/Button
 * \param ionr - I/O number
 * \param val - the state of the input/Button
 * \return status
 *
 */
ca_error Btn_CheckInputOrButtonExt(uint8_t ledBtn);

/**
 * \brief Check if any button declared
 * \param values - the state of all buttons
 * \return true if any button is registered, false if not
 *
 */
bool Btn_HasButtonExt(void);

/**
 * \brief Set a callback function to a button when it is short pressed
 * \param ledBtn - reference to button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if no context is needed.
 * \param shortPressMode - short press mode (when pressed or when released).
 * \return status
 *
 */
ca_error Btn_SetButtonShortPressCallbackExt(uint8_t      ledBtn,
                                            btn_callback callback,
                                            void*        context,
                                            uint8_t      shortPressMode);

/**
 * \brief Set a callback function to a button when it is long pressed
 * \param ledBtn - reference to button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param timeThreshold - time above which a button press is considered a long press
 * \return status
 *
 */
ca_error Btn_SetButtonLongPressCallbackExt(uint8_t      ledBtn,
                                           btn_callback callback,
                                           void*        context,
                                           uint32_t     timeThreshold);

/**
 * \brief Set a callback function to a button when it is held
 * \param ledBtn - reference to button
 * \param callback - function to call
 * \param context - context for the callback, should be set to NULL if not context is needed.
 * \param TimeInterval - time interval in [ms] in which callback function is called
 * \return status
 *
 */
ca_error Btn_SetButtonHoldCallbackExt(uint8_t ledBtn, btn_callback callback, void* context, uint32_t TimeInterval);

/**
 * \brief Main polling function to activate callbacks for any buttons that are currently being pressed
 * \param port - gpio values read from extender chip
 * \return status
 *
 */
ca_error Btn_PollButtonsExt(uint8_t port);

/**
 * \brief Check if all buttons have been handled
 * \return true/false
 *
 */
bool Btn_CanSleepExt(void);

#endif // CASCODA_BTN_EXT_H
