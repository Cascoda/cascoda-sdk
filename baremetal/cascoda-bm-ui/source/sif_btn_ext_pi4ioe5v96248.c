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
 * Extended Button Functions using a GPIO extender chip.
 * Implemented for PI4IOE5V96248.
*/

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_time.h"
#include "cascoda_btn_ext.h"
#include "sif_pi4ioe5v96248.h"

ca_error SIF_InitialiseGPIOExt(void)
{
	uint8_t status;

	/* hardware initialisation */
	status = SIF_PI4IOE5V96248_Initialise();
	if (status)
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

/* Set the functionality of the output/LED to an open drain output */
ca_error SIF_RegisterOutputExt(uint8_t ledBtn)
{
	return Btn_RegisterOutputExt(ledBtn);
}

/* Set the functionality of a button to be an input */
ca_error SIF_RegisterButtonInputExt(uint8_t ledBtn)
{
	Btn_IncrementGPIOWakeup();
	return Btn_RegisterButtonInputExt(ledBtn);
}

/* Set the functionality to be a general input (no interrupt, polling only) */
ca_error SIF_RegisterGeneralInputExt(uint8_t ledBtn, uint8_t pullup_on)
{
	/* not implemented as chip not configurable */
	(void)ledBtn;
	(void)pullup_on;

	return CA_ERROR_FAIL;
}

/* De-Register a button or LED */
ca_error SIF_DeRegisterExt(uint8_t ledBtn)
{
	return Btn_DeRegisterExt(ledBtn);
}

/* Set the state of the LED */
ca_error SIF_SetOutputExt(uint8_t ledBtn, u8_t val)
{
	/* check validity */
	if (Btn_CheckOutputExt(ledBtn))
		return CA_ERROR_FAIL;

	/* Change the state of the LED */
	if (SIF_PI4IOE5V96248_SetOutput(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

/* Get the state of the button or input */
ca_error SIF_SenseExt(uint8_t ledBtn, u8_t *val)
{
	/* check validity */
	if (Btn_CheckInputOrButtonExt(ledBtn))
		return CA_ERROR_FAIL;

	/* read the state */
	if (SIF_PI4IOE5V96248_Sense(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Get the state of the output/LED
ca_error SIF_SenseOutputExt(uint8_t ledBtn, u8_t *val)
{
	/* check validity */
	if (Btn_CheckOutputExt(ledBtn))
		return CA_ERROR_FAIL;

	/* read the state */
	if (SIF_PI4IOE5V96248_Sense(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Get the state of all buttons - saves on i2c comms when checking inputs
ca_error SIF_SenseAllExt(u8_t *values)
{
	/* check at least one button is registered */
	if (!Btn_HasButtonExt())
		return CA_ERROR_FAIL;

	/* read the state */
	if (SIF_PI4IOE5V96248_Acquire(values))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

/* main polling function to activate callbacks for any buttons that are currently being pressed */
ca_error SIF_PollButtonsExt(void)
{
	uint8_t port;

	if (SIF_SenseAllExt(&port))
		return CA_ERROR_FAIL;

	/* call polling handling function */
	if (Btn_PollButtonsExt(port))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

/* Check if all buttons have been handled */
/* Note: Only use when SIF_PollButtonsExt() is in the main loop */
bool SIF_CanSleepExt(void)
{
	/* check if interrupt pin is active */
	if (Btn_HasButtonExt())
	{
		if (SIF_PI4IOE5V96248_alarm_triggered())
			return false;
	}

	return Btn_CanSleepExt();
}
