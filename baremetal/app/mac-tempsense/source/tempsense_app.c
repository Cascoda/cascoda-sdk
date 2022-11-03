/**
 * @file
 * @brief Chili temperature sensing app functions for device and coordinator
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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"

#include "ca821x_api.h"
#include "tempsense_app.h"
#include "tempsense_evbme.h"
#if APP_USE_DEBUG
#include "tempsense_debug.h"
#endif /* APP_USE_DEBUG */

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
u8_t APP_STATE      = APP_ST_NORMAL; //!< module state
u8_t APP_STATE_new  = APP_ST_NORMAL; //!< module state set in interrupts
u8_t APP_INITIALISE = 0;             //!< flag for re-initialising application
u8_t APP_CONNECTED  = 0;             //!< sensor module is connected to coordinator

/* MAC PIB Values */
u16_t APP_PANId;
u16_t APP_ShortAddress;
u8_t  APP_LongAddress[8];
u8_t  APP_Channel = 18;

void TEMPSENSE_APP_Handler(struct ca821x_dev *pDeviceRef)
{
	if (APP_INITIALISE)
	{
		TEMPSENSE_APP_Initialise(pDeviceRef);
	}
	else
	{
		if (APP_STATE == APP_ST_COORDINATOR)
		{
			/* coordinator */
			TEMPSENSE_APP_Coordinator_Handler(pDeviceRef);
		}

		else if (APP_STATE == APP_ST_DEVICE)
		{
			/* densor device */
			TEMPSENSE_APP_Device_Handler(pDeviceRef);
		}
	}

	TEMPSENSE_APP_LED_Handler();
} // End of TEMPSENSE_APP_Handler()

void TEMPSENSE_APP_Initialise(struct ca821x_dev *pDeviceRef)
{
	APP_INITIALISE = 0;
	APP_STATE      = APP_STATE_new; // take over new state from interrupt routines

	/* dynamically register callbacks */
	TEMPSENSE_RegisterCallbacks(pDeviceRef);

	if (APP_STATE == APP_ST_COORDINATOR)
	{
		/* coordinator */
		TEMPSENSE_APP_Coordinator_Initialise(pDeviceRef);
	}
	else if (APP_STATE == APP_ST_DEVICE)
	{
		/* end device */
		TEMPSENSE_APP_Device_Initialise(pDeviceRef);
	}
	else
	{
		/* normal */
		/* reset program PIB */
		APP_Channel = 18;
		APP_PANId   = 0xFFFF;
		memcpy(APP_LongAddress, (u8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 8);
		APP_ShortAddress = 0xFFFF;
/* reset MAC PIB */
#if CASCODA_CA_VER >= 8212
		if (MLME_SET_request_sync(macExtendedAddress, 0, 8, APP_LongAddress, pDeviceRef))
#else
		if (MLME_SET_request_sync(nsIEEEAddress, 0, 8, APP_LongAddress, pDeviceRef))
#endif // CASCODA_CA_VER >= 8212
		{
#if APP_USE_DEBUG
			APP_Debug_Error(0x01);
#endif /* APP_USE_DEBUG */
		}
		if (MLME_RESET_request_sync(1, pDeviceRef)) // SetDefaultPIB = TRUE
		{
#if APP_USE_DEBUG
			APP_Debug_Error(0x02);
#endif /* APP_USE_DEBUG */
		}
	}

} // End of TEMPSENSE_APP_Initialise()

int TEMPSENSE_APP_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	/* switch clock otherwise chip is locking up as it loses external clock */
	if (((SerialRxBuffer->CmdId == EVBME_SET_REQUEST) && (SerialRxBuffer->Data[0] == EVBME_RESETRF)) ||
	    (SerialRxBuffer->CmdId == EVBME_HOST_CONNECTED))
	{
		printf("Clock has been switched due to device reset\n");
		EVBME_SwitchClock(pDeviceRef, 0);
	}
	else if (SerialRxBuffer->CmdId == EVBME_TSENSE_SETMODE)
	{
		TEMPSENSE_APP_SwitchMode(SerialRxBuffer->Data[0]);
		return 1;
	}
	else if (SerialRxBuffer->CmdId == EVBME_TSENSE_REPORT)
	{
		TEMPSENSE_APP_Coordinator_ReportStatus();
		return 1;
	}
	return 0;
} // End of TEMPSENSE_APP_UpStreamDispatch()

void TEMPSENSE_APP_SwitchMode(u8_t mode)
{
	if (mode > APP_ST_DEVICE)
	{
		printf("Invalid Mode\n");
		return;
	}
	else if (mode == APP_ST_NORMAL)
	{
		printf("Exiting TEMPSENSE\n");
	}
	APP_STATE_new  = mode;
	APP_INITIALISE = 1;
}

void TEMPSENSE_APP_InitPIB(struct ca821x_dev *pDeviceRef)
{
	u16_t ptime;
	u8_t  param;

	/* reset PIB */
	if (MLME_RESET_request_sync(1, pDeviceRef)) // SetDefaultPIB = TRUE
	{
#if APP_USE_DEBUG
		APP_Debug_Error(0x03);
#endif /* APP_USE_DEBUG */
		return;
	}

	MLME_SET_request_sync(phyCurrentChannel, 0, 1, &APP_Channel, pDeviceRef); // set 15.4 Channel
	MLME_SET_request_sync(macPANId, 0, 2, &APP_PANId, pDeviceRef);            // set local PANId

#if CASCODA_CA_VER >= 8212
	MLME_SET_request_sync(macExtendedAddress, 0, 8, APP_LongAddress, pDeviceRef); // set local long address
#else
	MLME_SET_request_sync(nsIEEEAddress, 0, 8, APP_LongAddress, pDeviceRef); // set local long address
#endif // CASCODA_CA_VER >= 8212

	MLME_SET_request_sync(macShortAddress, 0, 2, &APP_ShortAddress, pDeviceRef); // set local short address

	/* change PIB defaults */
	param = 3;
	MLME_SET_request_sync(macMinBE, 0, 1, &param, pDeviceRef); // default = 3
	param = 5;
	MLME_SET_request_sync(macMaxBE, 0, 1, &param, pDeviceRef); // default = 5
	param = 5;
	MLME_SET_request_sync(macMaxCSMABackoffs, 0, 1, &param, pDeviceRef); // default = 4
	param = 7;
	MLME_SET_request_sync(macMaxFrameRetries, 0, 1, &param, pDeviceRef); // default = 3

	if (APP_STATE == APP_ST_COORDINATOR)
	{
		/* transaction persistence time needs to be adjusted for long APP_WAKEUPINTERVALL */
		/* note: unit is aBaseSuperframeDuration, so 16 * 60 * 16 us = 15.36 ms */
		if (APP_WAKEUPINTERVALL != 0)
		{
			ptime = (u16_t)(APP_WAKEUPINTERVALL * 2 * 66);
			MLME_SET_request_sync(macTransactionPersistenceTime, 0, 2, &ptime, pDeviceRef);
		}
	}

} // End of TEMPSENSE_APP_InitPIB()

u8_t TEMPSENSE_APP_GetTempVal(void)
{
	i32_t t32val;
	u8_t  tval;
	// u8_t  len;

	t32val = BSP_GetTemperature() / 10;
	tval   = LS0_BYTE(t32val);

	// HWME_GET_request_sync(HWME_TEMPERATURE, &len, &tval, pDeviceRef);

	return (tval);
} // End of TEMPSENSE_APP_GetTempVal()

u16_t TEMPSENSE_APP_GetVoltsVal(void)
{
	u32_t adcval;
	u16_t vval;

	adcval = BSP_ADCGetVolts();
	vval   = (u16_t)adcval;

	return (vval);
} // End of TEMPSENSE_APP_GetVoltsVal()

u32_t TEMPSENSE_APP_GetScanChannels(void)
{
	u32_t scanchannels = 0; /* All: 0x07FFF800 */
	u8_t  lsize;
	u8_t  i;
	u8_t  ch;

	lsize = sizeof((u8_t[])MAC_CHANNELLIST);

	for (i = 0; i < lsize; ++i)
	{
		ch = (u8_t[])MAC_CHANNELLIST[i];
		if ((ch >= 11) && (ch <= 26))
		{
			if (!(scanchannels & (1 << ch))) /* avoid duplicates in channel list */
				scanchannels += (1 << ch);
		}
	}

	return (scanchannels);
} // End of TEMPSENSE_APP_GetScanChannels()

void TEMPSENSE_APP_PrintScanChannels(void)
{
	u8_t lsize;
	u8_t i;

	lsize = sizeof((u8_t[])MAC_CHANNELLIST);

	printf("Scanning Channels ");
	for (i = 0; i < lsize; ++i)
	{
		printf("%u", (u8_t[])MAC_CHANNELLIST[i]);
		if (i < lsize - 1)
			printf(", ");
	}
	printf(".\n");

} // End of TEMPSENSE_APP_PrintScanChannels()

void TEMPSENSE_APP_PrintSeconds(void)
{
	printf("%us; ", TIME_ReadAbsoluteTime() / 1000);
} // End of TEMPSENSE_APP_PrintSeconds()

void TEMPSENSE_APP_LED_Handler(void)
{
	struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();
	u32_t                    ledtime_g, ledtime_r;
	u32_t                    TON_G, TOFF_G;
	u32_t                    TON_R, TOFF_R;

	if (APP_STATE == APP_ST_DEVICE)
	{
		if (APP_CONNECTED)
		{
			/* device connected to coordinator */
			TON_G  = 1000;
			TOFF_G = 0;
			TON_R  = 0;
			TOFF_R = 1000;
		}
		else
		{
			/* device looking for coordinator */
			TON_G  = 1000;
			TOFF_G = 0;
			TON_R  = 1000;
			TOFF_R = 0;
		}
	}
	else
	{
		if (BSP_GetChargeStat())
		{
			/* charging */
			TON_G  = 500;
			TOFF_G = 500;
			TON_R  = 0;
			TOFF_R = 1000;
		}
		else
		{
			/* battery full */
			TON_G  = 1000;
			TOFF_G = 0;
			TON_R  = 0;
			TOFF_R = 1000;
		}
	}

	ledtime_g = TIME_ReadAbsoluteTime() % (TON_G + TOFF_G);
	ledtime_r = TIME_ReadAbsoluteTime() % (TON_R + TOFF_R);

	/* green */
	if (ledtime_g < TON_G)
		BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_ON);
	else
		BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_OFF);

	/* red */
	if (ledtime_r < TON_R)
		BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_ON);
	else
		BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_OFF);
}
