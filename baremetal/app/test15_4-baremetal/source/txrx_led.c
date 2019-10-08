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

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_time.h"

#include "txrx_led.h"

#ifndef TXRX_LED_TX
/// Pin to be used for the transmit LED
#define TXRX_LED_TX BSP_GetModuleSpecialPins().LED_GREEN
#endif
#ifndef TXRX_LED_RX
/// Pin to be used for the receive LED
#define TXRX_LED_RX BSP_GetModuleSpecialPins().LED_RED
#endif
#ifndef TXRX_LED_DECAY_MS
/// LED decay time so flashes are visible
#define TXRX_LED_DECAY_MS 200
#endif

static uint32_t prevTime     = 0;
static uint32_t tx_countdown = 0;
static uint32_t rx_countdown = 0;

static ca_error HandleDataIndication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	rx_countdown = TXRX_LED_DECAY_MS;
	BSP_ModuleSetGPIOPin(TXRX_LED_RX, LED_ON);

	//Return not handled so it is still passed up to higher layer
	return CA_ERROR_NOT_HANDLED;
}

static ca_error HandleDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	tx_countdown = TXRX_LED_DECAY_MS;
	BSP_ModuleSetGPIOPin(TXRX_LED_TX, LED_ON);

	//Return not handled so it is still passed up to higher layer
	return CA_ERROR_NOT_HANDLED;
}

void TXRX_LED_Initialise(struct ca821x_dev *pDeviceRef)
{
	BSP_ModuleRegisterGPIOOutput(TXRX_LED_TX, MODULE_PIN_TYPE_LED);
	BSP_ModuleRegisterGPIOOutput(TXRX_LED_RX, MODULE_PIN_TYPE_LED);

	if (!pDeviceRef->callbacks.MCPS_DATA_indication)
	{
		pDeviceRef->callbacks.MCPS_DATA_indication = HandleDataIndication;
	}
	if (!pDeviceRef->callbacks.MCPS_DATA_confirm)
	{
		pDeviceRef->callbacks.MCPS_DATA_confirm = HandleDataConfirm;
	}

	BSP_ModuleSetGPIOPin(TXRX_LED_TX, LED_OFF);
	BSP_ModuleSetGPIOPin(TXRX_LED_RX, LED_OFF);
}

void TXRX_LED_Handler(struct ca821x_dev *pDeviceRef)
{
	uint32_t diffTime, curTime;

	curTime  = TIME_ReadAbsoluteTime();
	diffTime = curTime - prevTime;
	prevTime = curTime;

	if (!diffTime)
		return;

	if (diffTime >= tx_countdown)
	{
		tx_countdown = 0;
		BSP_ModuleSetGPIOPin(TXRX_LED_TX, LED_OFF);
	}
	else
	{
		tx_countdown -= diffTime;
	}

	if (diffTime >= rx_countdown)
	{
		rx_countdown = 0;
		BSP_ModuleSetGPIOPin(TXRX_LED_RX, LED_OFF);
	}
	else
	{
		rx_countdown -= diffTime;
	}

	(void)pDeviceRef;
}
