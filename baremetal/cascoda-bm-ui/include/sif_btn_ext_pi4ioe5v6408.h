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
 * Extended Button Functions using a GPIO extender chip.
 * Implemented for PI4IOE5V6408.
*/

#ifndef SIF_BTN_EXT_PI4IOE5V6408_H
#define SIF_BTN_EXT_PI4IOE5V6408_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "ca821x_error.h"
#include "cascoda_btn_ext.h"

/* re-mapping for functions that have 1-to-1 functionality */
#define SIF_SetButtonShortPressCallbackExt Btn_SetButtonShortPressCallbackExt
#define SIF_SetButtonLongPressCallbackExt Btn_SetButtonLongPressCallbackExt
#define SIF_SetButtonHoldCallbackExt Btn_SetButtonHoldCallbackExt

/**
 * \brief Initialise GPIO extender
 * \return status
 *
 */
ca_error SIF_InitialiseGPIOExt(void);

/**
 * \brief Set the functionality of an output/LED to be an output
 * \param ledBtn - reference to output/LED
 * \return status
 *
 */
ca_error SIF_RegisterOutputExt(uint8_t ledBtn);

/**
 * \brief Set the functionality of a button to be an input
 * \param ledBtn - reference to button
 * \return status
 *
 */
ca_error SIF_RegisterButtonInputExt(uint8_t ledBtn);

/**
 * \brief Set the functionality to be a general input (no interrupt, polling only)
 * \param ledBtn - reference number to input
 * \param pullup_on - pull-up enabled when (1)
 * \return status
 *
 */
ca_error SIF_RegisterGeneralInputExt(uint8_t ledBtn, uint8_t pullup_on);

/**
 * \brief De-Register an LED or Button Pin
 * \param ledBtn - reference to LED/Button
 * \return status
 *
 */
ca_error SIF_DeRegisterExt(uint8_t ledBtn);

/**
 * \brief Set the state of the output/LED
 * \param ledBtn - reference to output/LED
 * \param val - the state of the output/LED
 * \return status
 *
 */
ca_error SIF_SetOutputExt(uint8_t ledBtn, uint8_t val);

/**
 * \brief Get the state of the LED/Button
 * \param ionr - I/O number
 * \param val - the state of the LED/Button
 * \return status
 *
 */
ca_error SIF_SenseExt(uint8_t ledBtn, uint8_t* val);

/**
 * \brief Get the state of the ouput/LED
 * \param ionr - I/O number
 * \param val - the state of the output/LED
 * \return status
 *
 */
ca_error SIF_SenseOutputExt(uint8_t ledBtn, uint8_t* val);

/**
 * \brief Get the state all buttons
 * \param values - the state of all buttons
 * \return status
 *
 */
ca_error SIF_SenseAllExt(uint8_t* values);

/**
 * \brief Main polling function to activate callbacks for any buttons that are currently being pressed
 * \return status
 *
 */
ca_error SIF_PollButtonsExt(void);

/**
 * \brief Check if all buttons have been handled
 * \return true/false
 *
 */
bool SIF_CanSleepExt(void);

#endif // SIF_BTN_EXT_PI4IOE5V6408_H
