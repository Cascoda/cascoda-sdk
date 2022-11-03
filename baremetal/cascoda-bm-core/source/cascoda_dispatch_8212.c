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
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "mac_messages.h"

static bool             isScanInProgress = false;
static volatile uint8_t isReadPending    = 0;

static void DispatchFromCa821x(struct MAC_Message *aMessage, struct ca821x_dev *pDeviceRef);

/**
 * Check if MAC MLME SET/GET Command requires intervention (that isn't in the hard mac)
 *
 * @param msg - MAC message to process
 * @param response - Response for synchronous messages
 * @param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *
 * @returns ca_error value
 * @retval  CA_ERROR_SUCCESS  Message was not consumed (should go to CA821x)
 * @retval  CA_ERROR_ALREADY  Message was consumed, and workaround already applied
 */
static ca_error CheckSetGet(struct MAC_Message *msg, struct MAC_Message *response, struct ca821x_dev *pDeviceRef)
{
	uint8_t status, val;

	if (msg->CommandId == SPI_MLME_SET_REQUEST)
	{
		if (TDME_CheckPIBAttribute(msg->PData.SetReq.PIBAttribute,
		                           msg->PData.SetReq.PIBAttributeLength,
		                           msg->PData.SetReq.PIBAttributeValue))
		{
			if (response)
			{
				response->CommandId                      = SPI_MLME_SET_CONFIRM;
				response->Length                         = 3;
				response->PData.SetCnf.Status            = MAC_INVALID_PARAMETER;
				response->PData.SetCnf.PIBAttribute      = msg->PData.SetReq.PIBAttribute;
				response->PData.SetCnf.PIBAttributeIndex = msg->PData.SetReq.PIBAttributeIndex;
			}
			return CA_ERROR_ALREADY;
		}
	}

	// MLME set / get phyTransmitPower
	if ((msg->CommandId == SPI_MLME_SET_REQUEST) && (msg->PData.SetReq.PIBAttribute == phyTransmitPower))
	{
		status = TDME_SetTxPower(msg->PData.SetReq.PIBAttributeValue[0], pDeviceRef);
		if (response)
		{
			response->CommandId                      = SPI_MLME_SET_CONFIRM;
			response->Length                         = 3;
			response->PData.SetCnf.Status            = status;
			response->PData.SetCnf.PIBAttribute      = msg->PData.SetReq.PIBAttribute;
			response->PData.SetCnf.PIBAttributeIndex = msg->PData.SetReq.PIBAttributeIndex;
		}
		return CA_ERROR_ALREADY;
	}
	if ((msg->CommandId == SPI_MLME_GET_REQUEST) && (msg->PData.SetReq.PIBAttribute == phyTransmitPower))
	{
		status = TDME_GetTxPower(&val, pDeviceRef);
		if (response)
		{
			response->CommandId                         = SPI_MLME_GET_CONFIRM;
			response->Length                            = 5;
			response->PData.GetCnf.Status               = status;
			response->PData.GetCnf.PIBAttribute         = msg->PData.GetReq.PIBAttribute;
			response->PData.GetCnf.PIBAttributeIndex    = msg->PData.GetReq.PIBAttributeIndex;
			response->PData.GetCnf.PIBAttributeLength   = 1;
			response->PData.GetCnf.PIBAttributeValue[0] = val;
		}
		return CA_ERROR_ALREADY;
	}

	return CA_ERROR_SUCCESS;
}

/** Checks to run before sending a message to the CA821x */
static ca_error PreCheckToCA821x(const uint8_t *buf, uint8_t *rsp, struct ca821x_dev *pDeviceRef)
{
	ca_error            Status = CA_ERROR_SUCCESS;
	struct MAC_Message *msg    = (struct MAC_Message *)buf;

	if (isScanInProgress)
	{
		Status = CA_ERROR_SPI_SCAN_IN_PROGRESS;
		goto exit;
	}

	if ((Status = CheckSetGet(msg, (struct MAC_Message *)rsp, pDeviceRef)))
		goto exit;

exit:
	return Status;
}

/** Checks to run after sending a message to the CA821x */
static ca_error PostCheckToCA821x(const uint8_t *buf, const uint8_t *rspbuf, struct ca821x_dev *pDeviceRef)
{
	(void)rspbuf;
	(void)pDeviceRef;

	ca_error            Status = CA_ERROR_SUCCESS;
	struct MAC_Message *msg    = (struct MAC_Message *)buf;

	if (msg->CommandId == SPI_MLME_SCAN_REQUEST)
	{
		isScanInProgress = true;
	}

	return Status;
}

/** Checks to run after receiving command from CA821x, but before dispatching */
static ca_error PreCheckFromCA821x(struct MAC_Message *aMessage)
{
	ca_error error = CA_ERROR_SUCCESS;

	if (aMessage->CommandId == SPI_MLME_SCAN_CONFIRM || aMessage->CommandId == SPI_HWME_WAKEUP_INDICATION)
	{
		isScanInProgress = false;
	}

	return error;
}

/** Dispatch a command as if it came from the ca821x */
static void DispatchFromCa821x(struct MAC_Message *aMessage, struct ca821x_dev *pDeviceRef)
{
	ca_error ret;

	ret = ca821x_upstream_dispatch(aMessage, pDeviceRef);
	if (ret != CA_ERROR_SUCCESS)
	{
		ca_log_crit("Err %s dispatching on SPI %02x!", ca_error_str(ret), aMessage->CommandId);
	}
}

void DISPATCH_ReadCA821x(struct ca821x_dev *pDeviceRef)
{
	uint8_t rfirq;

	BSP_DisableRFIRQ();
	rfirq = BSP_SenseRFIRQ();

	if (SPI_IsExchangeWithCA821xInProgress())
	{
		//Do nothing...
	}
	else if (!rfirq && !SPI_IsFifoAlmostFull() && !SPI_IsSyncChainInFlight())
	{
		SPI_Exchange(NULLP, pDeviceRef);
		isReadPending = 0;
	}
	else if (!rfirq)
	{
		isReadPending = 1;
	}
	else
	{
		isReadPending = 0;
	}

	BSP_EnableRFIRQ();
}

ca_error DISPATCH_ToCA821x(const uint8_t *buf, u8_t *response, struct ca821x_dev *pDeviceRef)
{
	ca_error Status = CA_ERROR_SUCCESS;

	if (response)
		response[0] = SPI_IDLE;

	if (!Status)
		Status = PreCheckToCA821x(buf, response, pDeviceRef);
	if (!Status)
		Status = SPI_Send(buf, response, pDeviceRef);
	if (!Status)
		Status = PostCheckToCA821x(buf, response, pDeviceRef);

	if (Status == CA_ERROR_ALREADY)
		Status = CA_ERROR_SUCCESS;
	return Status;
}

ca_error ca821x_api_downstream(const uint8_t *buf, uint8_t *response, struct ca821x_dev *pDeviceRef)
{
	return DISPATCH_ToCA821x(buf, response, pDeviceRef);
}

ca_error DISPATCH_FromCA821x(struct ca821x_dev *pDeviceRef)
{
	struct MAC_Message *RxMessage;
	static bool         isDispatching = false;

	if (isDispatching)
		return CA_ERROR_INVALID_STATE;

	isDispatching = true;
	while ((RxMessage = SPI_PeekFullBuf()) != NULL)
	{
		PreCheckFromCA821x(RxMessage);
		DispatchFromCa821x(RxMessage, pDeviceRef);
		SPI_DequeueFullBuf();
	}
	isDispatching = false;

	//If stalled, read pending message
	if (isReadPending)
		DISPATCH_ReadCA821x(pDeviceRef);

	return CA_ERROR_SUCCESS;
}
