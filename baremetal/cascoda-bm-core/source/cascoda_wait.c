/**
 * @file
 * @brief  Helper 'wait' framework for blocking functions
 */
/*
 *  Copyright (c) 2019, Cascoda Ltd.
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
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "mac_messages.h"

static union ca821x_api_callback sTargetCallback;
static void *                    sCallbackContext    = NULL;
static bool                      sWaitCalled         = false;
static bool                      sWaitCallbackActive = false;
static bool                      sWaiting            = false;

/**
 * \brief Internal callback function for wait. It logs the fact that the command
 *        was received, and then calls the actual callback.
 *
 * \return Status CA_ERROR_SUCCESS handled, CA_ERROR_NOT_HANDLED unhandled
 */
static ca_error WAIT_CallbackInternal(void *params, struct ca821x_dev *pDeviceRef)
{
	ca_error status = CA_ERROR_NOT_HANDLED;

	sWaitCallbackActive = true;

	if (sTargetCallback.generic_callback)
		status = sTargetCallback.generic_callback(params, pDeviceRef);

	sWaitCallbackActive = false;
	sWaitCalled         = true;

	return status;
}

static ca_error WAIT_CallbackWithRef(union ca821x_api_callback *aTargetCallback,
                                     int                        aTimeoutMs,
                                     void *                     aCallbackContext,
                                     struct ca821x_dev *        pDeviceRef)
{
	ca_error status    = CA_ERROR_SUCCESS;
	int      startTime = TIME_ReadAbsoluteTime();

	//Strictly not re-entrant
	if (sWaiting)
		return CA_ERROR_INVALID_STATE;
	sWaiting = true;

	//Replace the current callback with a special callback that tracks whether it has
	//been called, and also calls the old callback
	sTargetCallback                   = *aTargetCallback;
	aTargetCallback->generic_callback = &WAIT_CallbackInternal;
	sWaitCalled                       = false;
	sCallbackContext                  = aCallbackContext;

	for (int timepassed = 0; timepassed < aTimeoutMs; timepassed = TIME_ReadAbsoluteTime() - startTime)
	{
		status = DISPATCH_FromCA821x(pDeviceRef);
		if (sWaitCalled || status == CA_ERROR_INVALID_STATE)
			break;
		BSP_Waiting();
	}
	if (!sWaitCalled && status == CA_ERROR_SUCCESS)
		status = CA_ERROR_SPI_WAIT_TIMEOUT;

	*aTargetCallback = sTargetCallback;
	sCallbackContext = NULL;
	sWaiting         = false;

	return status;
}

ca_error WAIT_Callback(uint8_t aCommandId, int aTimeoutMs, void *aCallbackContext, struct ca821x_dev *pDeviceRef)
{
	union ca821x_api_callback *callbackRef = ca821x_get_callback(aCommandId, pDeviceRef);
	ca_error                   status      = CA_ERROR_FAIL;

	if (callbackRef == NULL)
	{
		goto exit; //Invalid command ID
	}

	status = WAIT_CallbackWithRef(callbackRef, aTimeoutMs, aCallbackContext, pDeviceRef);

exit:
	return status;
}

ca_error WAIT_CallbackSwap(uint8_t                 aCommandId,
                           ca821x_generic_callback aCallback,
                           int                     aTimeoutMs,
                           void *                  aCallbackContext,
                           struct ca821x_dev *     pDeviceRef)
{
	union ca821x_api_callback *callbackRef = ca821x_get_callback(aCommandId, pDeviceRef);
	union ca821x_api_callback  oldCallback;
	ca_error                   status = CA_ERROR_FAIL;

	if (callbackRef == NULL)
	{
		goto exit; //Invalid command ID
	}

	oldCallback                   = *callbackRef;
	callbackRef->generic_callback = aCallback;
	status                        = WAIT_CallbackWithRef(callbackRef, aTimeoutMs, aCallbackContext, pDeviceRef);
	*callbackRef                  = oldCallback;

exit:
	return status;
}

void *WAIT_GetContext(void)
{
	if (!sWaitCallbackActive)
		return NULL;
	else
		return sCallbackContext;
}
