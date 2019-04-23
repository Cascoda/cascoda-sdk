/**
 * @file tempsense_app.c
 * @brief Chili temperature sensing app functions for device and coordinator
 * @author Wolfgang Bruchner
 * @date 14/09/15
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"

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

/* MAC PIB Values */
u16_t APP_PANId;
u16_t APP_ShortAddress;
u8_t  APP_LongAddress[8];
u8_t  APP_Channel = 18;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Handler
 *******************************************************************************
 ******************************************************************************/
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
		else
		{
			/* normal mode */
			if (BSP_IsUSBPresent())
			{
				if (BSP_GetChargeStat())
					BSP_LEDSigMode(LED_M_CONNECTED_BAT_CHRG); /* charging */
				else
					BSP_LEDSigMode(LED_M_CONNECTED_BAT_FULL); /* not charging */
			}
		}
	}

} // End of TEMPSENSE_APP_Handler()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation of MAC etc.
 *******************************************************************************
 ******************************************************************************/
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
		/* disable watchdog timer for normal mode */
		if (APP_USE_WATCHDOG)
			BSP_WatchdogDisable();
		/* reset program PIB */
		APP_Channel = 18;
		APP_PANId   = 0xFFFF;
		memcpy(APP_LongAddress, (u8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 8);
		APP_ShortAddress = 0xFFFF;
		/* reset MAC PIB */
		if (MLME_SET_request_sync(nsIEEEAddress, 0, 8, APP_LongAddress, pDeviceRef))
		{
			BSP_LEDSigMode(LED_M_SETERROR);
#if APP_USE_DEBUG
			APP_Debug_Error(0x01);
#endif /* APP_USE_DEBUG */
		}
		if (MLME_RESET_request_sync(1, pDeviceRef)) // SetDefaultPIB = TRUE
		{
			BSP_LEDSigMode(LED_M_SETERROR);
#if APP_USE_DEBUG
			APP_Debug_Error(0x02);
#endif /* APP_USE_DEBUG */
		}
	}

} // End of TEMPSENSE_APP_Initialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Dispatch Branch (UpStream, Serial)
 *******************************************************************************
 ******************************************************************************/
int TEMPSENSE_APP_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	/* switch clock otherwise chip is locking up as it loses external clock */
	if (((SerialRxBuffer->CmdId == EVBME_SET_REQUEST) && (SerialRxBuffer->Data[0] == EVBME_RESETRF)) ||
	    (SerialRxBuffer->CmdId == EVBME_GUI_CONNECTED))
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Switch Mode
 *******************************************************************************
 ******************************************************************************/
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
	APP_STATE_new = mode;
	BSP_LEDSigMode(LED_M_CLRERROR);
	APP_INITIALISE = 1;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation of MAC PIB
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_InitPIB(struct ca821x_dev *pDeviceRef)
{
	u16_t ptime;
	u8_t  param;

	/* reset PIB */
	if (MLME_RESET_request_sync(1, pDeviceRef)) // SetDefaultPIB = TRUE
	{
		BSP_LEDSigMode(LED_M_SETERROR);
#if APP_USE_DEBUG
		APP_Debug_Error(0x03);
#endif /* APP_USE_DEBUG */
		return;
	}

	MLME_SET_request_sync(phyCurrentChannel, 0, 1, &APP_Channel, pDeviceRef);    // set 15.4 Channel
	MLME_SET_request_sync(macPANId, 0, 2, &APP_PANId, pDeviceRef);               // set local PANId
	MLME_SET_request_sync(nsIEEEAddress, 0, 8, APP_LongAddress, pDeviceRef);     // set local long address
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Save or Restore Address from Dataflash
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_SaveOrRestoreAddress(struct ca821x_dev *pDeviceRef)
{
	u8_t  rand[2];
	u8_t  len = 0;
	u32_t dsaved;

	dsaved = 0;

	/* read stored address */
	BSP_ReadDataFlash(0, &dsaved, 1);

	if (dsaved == 0xFFFFFFFF)
	{
		/* save address if not yet programmed */
		HWME_GET_request_sync(HWME_RANDOMNUM, &len, rand, pDeviceRef);
		dsaved = dsaved + (rand[1] << 24) + (rand[0] << 16);
		HWME_GET_request_sync(HWME_RANDOMNUM, &len, rand, pDeviceRef);
		dsaved = dsaved + (rand[1] << 8) + (rand[0]);
		BSP_WriteDataFlashInitial(0, &dsaved, 1);
		/* re-read stored address */
		BSP_ReadDataFlash(0, &dsaved, 1);
	}

	printf("Device Address 0x%08X\n", dsaved);

} // End of TEMPSENSE_APP_SaveOrRestoreAddress()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Get Temperature Reading
 *******************************************************************************
 * \return Temperature, 8 bit signed (1s complement)
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Get Battery Voltage Reading
 *******************************************************************************
 * \return 8-Bit VBat, full range = 5.5V
 *******************************************************************************
 ******************************************************************************/
u16_t TEMPSENSE_APP_GetVoltsVal(void)
{
	u32_t adcval;
	u16_t vval;

	adcval = BSP_ADCGetVolts();
	vval   = (u16_t)adcval;

	return (vval);
} // End of TEMPSENSE_APP_GetVoltsVal()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Get ScanChannels from MAC_CHANNELLIST
 *******************************************************************************
 * \return ScanChannels Parameter for Scan Requests
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Print MAC_CHANNELLIST
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Prints AboluteTime in Seconds
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_PrintSeconds(void)
{
	printf("%us; ", TIME_ReadAbsoluteTime() / 1000);
} // End of TEMPSENSE_APP_PrintSeconds()
