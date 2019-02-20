/**
 * @file   cascoda_wait.c
 * @brief  Helper 'wait' framework for blocking functions
 * @author Ciaran Woodward
 * @date   31/01/19
 *//*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "ca821x_api.h"
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
 * \return Status 1 handled, 0 unhandled
 */
static int WAIT_CallbackInternal(void *params, struct ca821x_dev *pDeviceRef)
{
	int status = 0;

	sWaitCallbackActive = true;

	if (sTargetCallback.generic_callback)
		status = sTargetCallback.generic_callback(params, pDeviceRef);

	sWaitCallbackActive = false;
	sWaitCalled         = true;

	return status;
}

int WAIT_Callback(union ca821x_api_callback *aTargetCallback,
                  int                        timeout,
                  void *                     aCallbackContext,
                  struct ca821x_dev *        pDeviceRef)
{
	int status    = EVBME_SUCCESS;
	int startTime = TIME_ReadAbsoluteTime();

	//Strictly not re-entrant
	if (sWaiting)
		return EVBME_FAIL;
	sWaiting = true;

	//Replace the current callback with a special callback that tracks whether it has
	//been called, and also calls the old callback
	sTargetCallback                   = *aTargetCallback;
	aTargetCallback->generic_callback = &WAIT_CallbackInternal;
	sWaitCalled                       = false;
	sCallbackContext                  = aCallbackContext;

	for (int timepassed = 0; timepassed < timeout; timepassed = TIME_ReadAbsoluteTime() - startTime)
	{
		EVBME_Dispatch(pDeviceRef);
		if (sWaitCalled)
			break;
	}
	if (!sWaitCalled)
		status = EVBME_SPI_WAIT_TIMEOUT;

	*aTargetCallback = sTargetCallback;
	sCallbackContext = NULL;
	sWaiting         = false;

	return status;
}

void *WAIT_GetContext(void)
{
	if (!sWaitCallbackActive)
		return NULL;
	else
		return sCallbackContext;
}

/**
 * \brief Internal callback function for legacy wait, just copies the received
 *        command into the context buffer.
 *
 * \return Status 1 handled, 0 unhandled
 */
static int WAIT_CallbackLegacy(void *params, struct ca821x_dev *pDeviceRef)
{
	int      status = 0;
	uint8_t *buf    = WAIT_GetContext();

	if (buf && params)
	{
		uint8_t *source = params;
		source -= 2;
		memcpy(buf, source, source[1] + 2);
		status = 1;
	}

	return status;
}

int WAIT_Legacy(uint8_t cmdid, int timeout_ms, uint8_t *buf, struct ca821x_dev *pDeviceRef)
{
	union ca821x_api_callback *callbackRef = ca821x_get_callback(cmdid, pDeviceRef);
	union ca821x_api_callback  oldCallback;
	int                        status = EVBME_FAIL;

	if (callbackRef == NULL)
	{
		goto exit; //Invalid command ID
	}

	oldCallback                   = *callbackRef;
	callbackRef->generic_callback = &WAIT_CallbackLegacy;
	status                        = WAIT_Callback(callbackRef, timeout_ms, buf, pDeviceRef);
	*callbackRef                  = oldCallback;

exit:
	return status;
}
