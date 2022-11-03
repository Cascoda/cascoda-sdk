/**
 * @file
 * @brief Over-Air Debug Queue
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

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#include "tempsense_app.h"
#include "tempsense_debug.h"

#if APP_USE_DEBUG

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
u8_t Debug_App_State = 0; // Application State
u8_t Debug_App_Error = 0; // Application Error
u8_t Debug_Handle    = 0; // Packet handle

extern u8_t SPI_MLME_SCANNING; // from spi driver

void APP_Debug_Send(struct ca821x_dev *pDeviceRef)
{
	static u32_t    start_time = 0;
	u8_t            channel;
	u8_t            rxidle;
	u8_t            msdu[23];
	struct FullAddr fadd;
	u8_t            status = 0;
	u8_t            msdulength;
	u8_t            report_error;
	/* local debug variables */
	u32_t app_debug_reg    = 0;
	u32_t app_debug_sysclk = 0;

	report_error = Debug_App_Error;

	/* don't send debug packet when scanning */
	if (SPI_MLME_SCANNING)
	{
		return;
	}
	/* send packed immediately if error occured, if not check mode and time intervals */
	if (!report_error)
	{
		if (APP_STATE == APP_ST_DEVICE)
		{
			if (((TIME_ReadAbsoluteTime() - start_time) < (1000 * DEBUG_INTERVALL)) &&
			    (DEBUG_INTERVALL > APP_WAKEUPINTERVALL))
				return;
		}
		else if (APP_STATE == APP_ST_COORDINATOR)
		{
			if ((TIME_ReadAbsoluteTime() - start_time) < (1000 * DEBUG_INTERVALL))
				return;
		}
		else
		{
			return;
		}
	}

	start_time = TIME_ReadAbsoluteTime();

	/* app_debug_reg: common register for system variables */
	/* bit 0: USBPresent */
	/*     1: CLKExternal */
	/*     8: !CLK_CLKSTATUS_PLL_STB */
	/*     9: CLK_CLKSTATUS_CLK_SW_FAIL */
	/*    16: RFIRQ enabled */
	app_debug_reg = 0;
	if (BSP_IsUSBPresent())
		app_debug_reg += 0x00000001;
	if (APP_USE_EXTERNAL_CLOCK) /* currently no BSP function */
		app_debug_reg += 0x00000002;

	MLME_SET_request_sync(macDSN, 0, 1, &Debug_Handle, pDeviceRef);

	/* DstAddr Shortaddress is the last 2 bytes (inverted) of APP_LongAddress for device Id */
	PUTLE16((u16_t)(APP_LongAddress[0] << 8) + APP_LongAddress[1], fadd.Address);
	PUTLE16(MAC_PANID, fadd.PANId);
	fadd.AddressMode = MAC_MODE_SHORT_ADDR;

	/* populate MSDU */
	/* insert FF for separators */
	msdu[0]  = 0xFF;
	msdu[1]  = Debug_App_State; /* B1: Application State */
	msdu[2]  = 0xFF;
	msdu[3]  = Debug_App_Error; /* B2: Application Error */
	msdu[4]  = 0xFF;
	msdu[5]  = 0x00;
	msdu[6]  = 0xFF;
	msdu[7]  = 0x00;
	msdu[8]  = 0xFF;
	msdu[9]  = (u8_t)((app_debug_reg >> 24) & 0xFF); /* B6-9: System Variables (see above) */
	msdu[10] = (u8_t)((app_debug_reg >> 16) & 0xFF);
	msdu[11] = (u8_t)((app_debug_reg >> 8) & 0xFF);
	msdu[12] = (u8_t)((app_debug_reg >> 0) & 0xFF);
	msdu[13] = 0xFF;
	msdu[14] = (u8_t)((app_debug_sysclk >> 24) & 0xFF); /* B10-13: System Clock Frequency */
	msdu[15] = (u8_t)((app_debug_sysclk >> 16) & 0xFF);
	msdu[16] = (u8_t)((app_debug_sysclk >> 8) & 0xFF);
	msdu[17] = (u8_t)((app_debug_sysclk >> 0) & 0xFF);
	msdu[18] = 0xFF;
	msdu[19] = (u8_t)((start_time >> 24) & 0xFF); /* B11-B14: Time Stamp */
	msdu[20] = (u8_t)((start_time >> 16) & 0xFF);
	msdu[21] = (u8_t)((start_time >> 8) & 0xFF);
	msdu[22] = (u8_t)((start_time >> 0) & 0xFF);

	msdulength = 23;

	if (APP_STATE == APP_ST_COORDINATOR)
	{
		/* turn off receiver before switching channel */
		rxidle = 0;
		MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxidle, pDeviceRef);
	}

	channel = DEBUG_CHANNEL;
	if (MLME_SET_request_sync(phyCurrentChannel, 0, 1, &channel, pDeviceRef))
	{
		printf("DEBUG Channel Set Fail\n");
		return;
	}

#if CASCODA_CA_VER >= 8212
	uint8_t tx_op[2] = {0x00, 0x00};
	status           = MCPS_DATA_request(MAC_MODE_NO_ADDR, /* SrcAddrMode */
                               fadd,             /* DstAddr */
                               0,                /* HeaderIELength */
                               0,                /* PayloadIELength */
                               msduLength,       /* MsduLength */
                               msdu,             /* pMsdu */
                               Debug_Handle,     /* MsduHandle */
                               tx_op,            /* pTxOptions */
                               0,                /* SchTimestamp */
                               0,                /* SchPeriod */
                               0,                /* TxChannel */
                               NULLP,            /* pHeaderIEList */
                               NULLP,            /* pPayloadIEList */
                               NULLP,            /* pSecurity */
                               pDeviceRef);      /* pDeviceRef */
#else
	status = MCPS_DATA_request(MAC_MODE_NO_ADDR, /* SrcAddrMode */
	                           fadd,             /* DstAddr */
	                           msdulength,       /* MsduLength */
	                           msdu,             /* *pMsdu */
	                           Debug_Handle,     /* MsduHandle */
	                           0,                /* TxOptions */
	                           NULLP,            /* *pSecurity */
	                           pDeviceRef);
#endif // CASCODA_CA_VER >= 8212

	if (status)
	{
		printf("DEBUG Data Request Failure: %02x\n", status);
	}
	else
	{
		status =
		    WAIT_CallbackSwap(SPI_MCPS_DATA_CONFIRM, (ca821x_generic_callback)&APP_Debug_Sent, 100, NULL, pDeviceRef);
		if (status)
		{
			printf("DEBUG Confirm Fail\n");
		}
	}

} // End of APP_Debug_Send()

int APP_Debug_Sent(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	u8_t rxidle;
	if (MLME_SET_request_sync(phyCurrentChannel, 0, 1, &APP_Channel, pDeviceRef))
	{
		printf("DEBUG Channel Reset Fail\n");
		return 1;
	}
	if (APP_STATE == APP_ST_COORDINATOR)
	{
		/* turn on receiver before switching channel */
		rxidle = 1;
		MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxidle, pDeviceRef);
	}
	if (params->Status == MAC_SUCCESS)
	{
		++Debug_Handle;
		APP_Debug_Reset();
	}
	return 1;
} // End of APP_Debug_Sent()

void APP_Debug_Reset(void)
{
	if (APP_STATE == APP_ST_NORMAL)
		Debug_App_State = 0x00;
	else if (APP_STATE == APP_ST_COORDINATOR)
		Debug_App_State = 0x01;
	else if (APP_STATE == APP_ST_DEVICE)
		Debug_App_State = 0x02;
	else
		Debug_App_State = 0x0F; /* ?? */
	Debug_App_Error = 0;
} // End of APP_Debug_Reset()

void APP_Debug_Error(u8_t code)
{
	if (Debug_App_Error == 0)
	{
		Debug_App_Error = code;
	}
} // End of APP_Debug_Error()

void APP_Debug_SetAppState(u8_t state)
{
	Debug_App_State = state;
} // End of APP_Debug_SetAppState()

#endif // APP_USE_DEBUG
