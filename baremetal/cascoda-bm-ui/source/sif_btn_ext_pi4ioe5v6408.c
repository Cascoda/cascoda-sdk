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

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_time.h"
#include "cascoda_btn_ext.h"
#include "sif_pi4ioe5v6408.h"

ca_error SIF_InitialiseGPIOExt(void)
{
	uint8_t status;

	/* hardware initialisation */
	status = SIF_PI4IOE5V6408_Initialise();
	if (status == SIF_PI4IOE5V6408_ST_UNAVAILABLE)
		return CA_ERROR_NO_ACCESS;
	else if (status)
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Set the functionality to be output/LED
ca_error SIF_RegisterOutputExt(uint8_t ledBtn)
{
	/* check validity and register */
	if (Btn_RegisterOutputExt(ledBtn))
		return CA_ERROR_FAIL;

	/* configure expander */
	if (SIF_PI4IOE5V6408_ConfigureOutput(ledBtn))
	{
		Btn_DeRegisterExt(ledBtn);
		return CA_ERROR_FAIL;
	}

	return CA_ERROR_SUCCESS;
}

// Set the functionality of a button to be an input
ca_error SIF_RegisterButtonInputExt(uint8_t ledBtn)
{
	/* check validity and register */
	if (Btn_RegisterButtonInputExt(ledBtn))
		return CA_ERROR_FAIL;

	/* configure expander */
	if (SIF_PI4IOE5V6408_ConfigureInput(ledBtn, SIF_PI4IOE5V6408_IRQ_ON, SIF_PI4IOE5V6408_PUP_OFF))
	{
		Btn_DeRegisterExt(ledBtn);
		return CA_ERROR_FAIL;
	}

	Btn_SetGPIOWakeup();

	return CA_ERROR_SUCCESS;
}

// Set the functionality to be a general input (no interrupt, polling only)
ca_error SIF_RegisterGeneralInputExt(uint8_t ledBtn, uint8_t pullup_on)
{
	/* check validity and register */
	if (Btn_RegisterGeneralInputExt(ledBtn))
		return CA_ERROR_FAIL;

	/* configure expander */
	if (SIF_PI4IOE5V6408_ConfigureInput(ledBtn, SIF_PI4IOE5V6408_IRQ_OFF, pullup_on))
	{
		Btn_DeRegisterExt(ledBtn);
		return CA_ERROR_FAIL;
	}

	return CA_ERROR_SUCCESS;
}

// De-Register a button or LED
ca_error SIF_DeRegisterExt(uint8_t ledBtn)
{
	/* de-regsiter */
	Btn_DeRegisterExt(ledBtn);

	/* configure expander */
	if (SIF_PI4IOE5V6408_ConfigureInput(ledBtn, SIF_PI4IOE5V6408_IRQ_OFF, SIF_PI4IOE5V6408_PUP_EN))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Set the state of the output/LED
ca_error SIF_SetOutputExt(uint8_t ledBtn, uint8_t val)
{
	/* check validity */
	if (Btn_CheckOutputExt(ledBtn))
		return CA_ERROR_FAIL;

	/* Change the state of the LED */
	if (SIF_PI4IOE5V6408_SetOutput(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Get the state of the input/Button
ca_error SIF_SenseExt(uint8_t ledBtn, uint8_t *val)
{
	/* check validity */
	if (Btn_CheckInputOrButtonExt(ledBtn))
		return CA_ERROR_FAIL;

	/* read the state */
	if (SIF_PI4IOE5V6408_SenseInput(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Get the state of the output/LED
ca_error SIF_SenseOutputExt(uint8_t ledBtn, uint8_t *val)
{
	/* check validity */
	if (Btn_CheckOutputExt(ledBtn))
		return CA_ERROR_FAIL;

	/* read the state */
	if (SIF_PI4IOE5V6408_SenseOutput(ledBtn, val))
		return CA_ERROR_FAIL;

	return CA_ERROR_SUCCESS;
}

// Get the state of all buttons - saves on i2c comms when checking inputs
ca_error SIF_SenseAllExt(uint8_t *values)
{
	/* check at least one button is registered */
	if (!Btn_HasButtonExt())
		return CA_ERROR_FAIL;

	/* read the state */
	if (SIF_PI4IOE5V6408_SenseAllInputs(values))
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
/* Note: Only use when DVBD_PollButtonsExt() is in the main loop */
bool SIF_CanSleepExt(void)
{
	/* check if interrupt pin is active */
	if (Btn_HasButtonExt())
	{
		if (SIF_PI4IOE5V6408_IsInterruptActive())
			return false;
	}

	return Btn_CanSleepExt();
}
