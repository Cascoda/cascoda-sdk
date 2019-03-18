/**
 * @file tempsense_app.c
 * @brief Chili temperature sensing app functions for coordinator
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
static u8_t APP_NDEVICES = 0; //!< number of devices connected to coordinator

/* devices associated  with coordinator */
/* short address assigned will be CA 01 to CA APP_MAX_DEVICES */
static u8_t  APP_DevState[APP_MAX_DEVICES]        = {APP_CST_DONE}; /* device communications state */
static u8_t  APP_DevAssociated[APP_MAX_DEVICES]   = {0};            /* device is associated flag */
static u32_t APP_DevTimeout[APP_MAX_DEVICES]      = {0};            /* device timeout */
static u32_t APP_DevLongAddrLSBs[APP_MAX_DEVICES] = {0};            /* device lower 4 bytes of long address */
static u32_t APP_DevHandle[APP_MAX_DEVICES]       = {0xFFFFFFFF};   /* device handle / sequence number */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Handler for Coordinator
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_Handler(struct ca821x_dev *pDeviceRef)
{
	/* LEDs */
	if (BSP_GetChargeStat())
		BSP_LEDSigMode(LED_M_CONNECTED_BAT_CHRG); // charging
	else
		BSP_LEDSigMode(LED_M_CONNECTED_BAT_FULL); // not charging

	/* check time-outs */
	TEMPSENSE_APP_Coordinator_CheckTimeouts(pDeviceRef);

#if APP_USE_DEBUG
	APP_Debug_Send(pDeviceRef);
#endif /* APP_USE_DEBUG */

} // End of TEMPSENSE_APP_Coordinator_Handler()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation for Coordinator
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_Initialise(struct ca821x_dev *pDeviceRef)
{
	u8_t i;

	/* disable watchdog timer for coordinator */
	if (APP_USE_WATCHDOG)
		BSP_WatchdogDisable();

	/* initialise PIB */
	/* IEEE Address: upper 4 bytes are CA 5C 0D A0, lower 4 bytes are 00 00 00 00 */
	APP_PANId        = MAC_PANID;
	APP_ShortAddress = 0xFFFE; /* use long address */
	memcpy(APP_LongAddress, (u8_t[])MAC_LONGADD, 8);

	for (i = 0; i < APP_MAX_DEVICES; ++i)
	{
		APP_DevState[i]        = APP_CST_DONE;
		APP_DevAssociated[i]   = 0;
		APP_DevTimeout[i]      = 0;
		APP_DevLongAddrLSBs[i] = 0;
		APP_DevHandle[i]       = 0xFFFFFFFF;
	}
	APP_NDEVICES = 0;

	/* initialise MAC PIB */
	TEMPSENSE_APP_InitPIB(pDeviceRef);

	/* start as coordinator */
	TEMPSENSE_APP_Coordinator_Start(pDeviceRef);

} // End of TEMPSENSE_APP_Coordinator_Initialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Start Procedure
 *******************************************************************************
 * \param restart - Nonzero if already started previously
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_Start(struct ca821x_dev *pDeviceRef)
{
	u8_t status;

	TEMPSENSE_APP_PrintSeconds();
	printf("Starting as Coordinator\n");
	printf("Scanning for Channel Selection, please wait ...\n");
	TEMPSENSE_APP_PrintScanChannels();

	APP_Channel = 0;

	/* perform energy scan */
	status = MLME_SCAN_request(ENERGY_DETECT, TEMPSENSE_APP_GetScanChannels(), 5, NULLP, pDeviceRef);

	if (status)
	{
/* scan request fail (highly unlikely) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xA1);
		APP_Debug_Error(0x04);
#endif /* APP_USE_DEBUG */
		BSP_LEDSigMode(LED_M_SETERROR);
	}

} // End of TEMPSENSE_APP_Coordinator_Start()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Coordinator Process incoming Scan Confirm
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_ProcessScanCnf(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	u8_t status;
	u8_t assocpermit;
	u8_t i;
	u8_t edvalmin;

	if ((params->ScanType != ENERGY_DETECT) || (params->Status != MAC_SUCCESS) ||
	    (params->ResultListSize != sizeof((u8_t[])MAC_CHANNELLIST)))
	{
/* scan confirm fail or no channels scanned */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xA3);
		APP_Debug_Error(0x06);
#endif /* APP_USE_DEBUG */
		BSP_LEDSigMode(LED_M_SETERROR);
		return;
	}

/* success */
#if APP_USE_DEBUG
	APP_Debug_SetAppState(0xA4);
#endif /* APP_USE_DEBUG */
	edvalmin = 0xFF;
	for (i = 0; i < params->ResultListSize; ++i)
	{
		if (params->ResultList[i] < edvalmin)
		{
			/* select channel with lowest ed value */
			edvalmin    = params->ResultList[i];
			APP_Channel = (u8_t[])MAC_CHANNELLIST[i];
		}
	}

	if (APP_Channel == 0)
	{
		APP_Channel = 18;
		return;
	}
	else
	{
		TEMPSENSE_APP_PrintSeconds();
		printf("Selected Channel %u\n", APP_Channel);
	}

	/* start as coordinator */
	status = MLME_START_request_sync(APP_PANId,   /* PANId */
	                                 APP_Channel, /* LogicalChannel */
	                                 15,          /* BeaconOrder */
	                                 15,          /* SuperframeOrder */
	                                 1,           /* PANCoordinator */
	                                 0,           /* BatteryLifeExtension */
	                                 0,           /* CoordRealignment */
	                                 NULLP,       /* *pCoordRealignSecurity */
	                                 NULLP,       /* *pBeaconSecurity */
	                                 pDeviceRef);

	if (status)
	{
/* start request fail (highly unlikely) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xA5);
		APP_Debug_Error(0x07);
#endif /* APP_USE_DEBUG */
		BSP_LEDSigMode(LED_M_SETERROR);
	}
	else
	{
/* success */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xA6);
#endif /* APP_USE_DEBUG */
		/* set macAssociationPermit */
		assocpermit = 1;
		MLME_SET_request_sync(macAssociationPermit, 0, 1, &assocpermit, pDeviceRef);
	}

} // End of TEMPSENSE_APP_Coordinator_ProcessScanCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Association Response
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_AssociateResponse(struct MLME_ASSOCIATE_indication_pset *params,
                                                 struct ca821x_dev *                    pDeviceRef)
{
	u8_t  i;
	u8_t  status;
	u16_t shortadd;
	u8_t  associated;
	u32_t longaddr_lsbs;
	u8_t  already_in_list;

#if APP_USE_DEBUG
	APP_Debug_SetAppState(0xA7);
#endif /* APP_USE_DEBUG */

	/* get 4 low bytes of ieee address to see if already associated */
	longaddr_lsbs = (params->DeviceAddress[3] << 24) + (params->DeviceAddress[2] << 16) +
	                (params->DeviceAddress[1] << 8) + (params->DeviceAddress[0]);

	/* compare upper 4 bytes of long address (CA5C0DA0) */
	if (memcmp(params->DeviceAddress + 4, APP_LongAddress + 4, 4))
	{
/* addresses not matching network - highly unlikely as panid checked by address filter */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xA8);
#endif /* APP_USE_DEBUG */
		return;
	}

	/* check if device is already in device list by comparing lower 4 bytes of long address */
	already_in_list = 0;
	for (i = 0; i < APP_MAX_DEVICES; ++i)
	{
		if (longaddr_lsbs == APP_DevLongAddrLSBs[i])
		{
			already_in_list = 1;
			break;
		}
	}

	if (already_in_list)
	{
/* device already in device list */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xA9);
#endif /* APP_USE_DEBUG */
		return;
	}

	/* look for empty entry in device list and add device */
	associated = 0;
	for (i = 0; i < APP_MAX_DEVICES; ++i)
	{
		if (APP_DevAssociated[i] == 0)
		{
			associated             = 1;
			APP_DevLongAddrLSBs[i] = longaddr_lsbs;
			APP_DevAssociated[i]   = 1;
			APP_DevState[i]        = APP_CST_DONE;
			APP_DevTimeout[i]      = TIME_ReadAbsoluteTime();
			shortadd               = MAC_SHORTADD + (i + 1);
			break;
		}
	}

	if (!associated)
	{
/* no empty entry found in device list */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xAA);
#endif /* APP_USE_DEBUG */
		TEMPSENSE_APP_PrintSeconds();
		printf("Cannot associate, maximum number of sensors reached\n");
		return;
	}

	/* send out associate response */
	status = MLME_ASSOCIATE_response(params->DeviceAddress, /* *pDeviceAddress */
	                                 shortadd,              /* AssocShortAddress */
	                                 MAC_SUCCESS,           /* Status */
	                                 NULLP,                 /* *pSecurity */
	                                 pDeviceRef);

	if (status)
	{
/* associate response fail (highly unlikely) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xAB);
		APP_Debug_Error(0x08);
#endif /* APP_USE_DEBUG */
		BSP_LEDSigMode(LED_M_SETERROR);
		return;
	}

	/* success */
	++APP_NDEVICES;
#if APP_USE_DEBUG
	APP_Debug_SetAppState(0x0A);
#endif /* APP_USE_DEBUG */
	TEMPSENSE_APP_PrintSeconds();
	printf("Detected Sensor %u on Channel %u, Addr=0x", (u8_t)(shortadd & 0x00FF), APP_Channel);
	for (i = 3; i < 4; i--)
	{
		printf("%02X", params->DeviceAddress[i]);
	}
	printf("; Devices connected: %u\n", APP_NDEVICES);

} // End of TEMPSENSE_APP_Coordinator_AssociateResponse()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Coordinator Process incoming Data Indications
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	u8_t            devnum;
	u16_t           shortadd, panid;
	u8_t            status;
	u32_t           serialnr;
	struct FullAddr DeviceFAdd;
	u8_t            msdu[1];
	u8_t            edvallp;
	u8_t            len;

	/* get ed / rssi value for last data packet */
	HWME_GET_request_sync(HWME_EDVALLP, &len, &edvallp, pDeviceRef);

	/* analyse packet */
	panid    = (u16_t)(params->Src.PANId[1] << 8) + params->Src.PANId[0];
	shortadd = (u16_t)(params->Src.Address[1] << 8) + params->Src.Address[0];
	devnum   = (u8_t)((shortadd & 0xFF) - 1);

	if ((panid != APP_PANId) || (devnum >= APP_MAX_DEVICES))
	{
/* packet for coordinator not from associated device */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xC1);
#endif /* APP_USE_DEBUG */
		return;
	}
	if (APP_DevAssociated[devnum] != 1)
	{
		/* device not associated (anymore) */
		APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xC2);
#endif /* APP_USE_DEBUG */
		return;
	}
	if ((params->Msdu[0] != PT_MSDU_D_WAKEUP) && (params->Msdu[0] != PT_MSDU_D_DATA))
	{
		/* unknown packet type */
		APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xC3);
#endif /* APP_USE_DEBUG */
		return;
	}

	/* WAKEUP packet received */
	if (params->Msdu[0] == PT_MSDU_D_WAKEUP)
	{
		if (APP_DevState[devnum] != APP_CST_DONE)
		{
			/* out of order */
			APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xC4);
#endif /* APP_USE_DEBUG */
			return;
		}

		if (params->MsduLength != 5)
		{
			/* wrong msdu length */
			APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xC5);
#endif /* APP_USE_DEBUG */
			return;
		}

		/* get device packet handle */
		serialnr =
		    (u32_t)((params->Msdu[4] << 24) + (params->Msdu[3] << 16) + (params->Msdu[2] << 8) + (params->Msdu[1]));

		if (serialnr == APP_DevHandle[devnum])
		{
/* packet has been re-sent */
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xC6);
#endif /* APP_USE_DEBUG */
			return;
		}

		APP_DevState[devnum]   = APP_CST_WAKEUP_RECEIVED;
		APP_DevHandle[devnum]  = serialnr;
		DeviceFAdd.AddressMode = MAC_MODE_SHORT_ADDR;
		PUTLE16(shortadd, DeviceFAdd.Address);
		PUTLE16(APP_PANId, DeviceFAdd.PANId);
		msdu[0] = PT_MSDU_C_DATA;

		/* send C_DATA packet direct to device */
		status = MCPS_DATA_request(MAC_MODE_LONG_ADDR,              /* SrcAddrMode */
		                           DeviceFAdd,                      /* DstAddr     */
		                           1,                               /* MsduLength  */
		                           msdu,                            /* *pMsdu      */
		                           LS0_BYTE(APP_DevHandle[devnum]), /* MsduHandle  */
		                           TXOPT_ACKREQ,                    /* TxOptions   */
		                           NULLP,                           /* *pSecurity  */
		                           pDeviceRef);

		if (status)
		{
			/* data request fail (highly unlikely) */
			APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xC7);
#endif /* APP_USE_DEBUG */
		}
		else
		{
			/* success */
			APP_DevState[devnum] = APP_CST_C_DATA_REQUESTED;
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xC8);
#endif /* APP_USE_DEBUG */
		}

	} /* WAKEUP packet */

	/* D_DATA packet received */
	if (params->Msdu[0] == PT_MSDU_D_DATA)
	{
		if (APP_DevState[devnum] != APP_CST_C_DATA_CONFIRMED)
		{
			/* out of order */
			APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xC9);
#endif /* APP_USE_DEBUG */
			return;
		}

		if (params->MsduLength != 6)
		{
			/* wrong msdu length */
			APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xCA);
#endif /* APP_USE_DEBUG */
			return;
		}

		/* success */
		APP_DevState[devnum] = APP_CST_DONE;
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0x0C);
#endif /* APP_USE_DEBUG */
		APP_DevTimeout[devnum] = TIME_ReadAbsoluteTime();
		if ((APP_DevHandle[devnum] % APP_COORD_REPORTN) == 0)
			TEMPSENSE_APP_Coordinator_DisplayData(devnum, edvallp, params, pDeviceRef);
		/* coordinator soft reinitialisation after data exchange */
		if (APP_COORD_SREINIT)
			TEMPSENSE_APP_Coordinator_SoftReinit(pDeviceRef);

	} /* D_DATA packet */

} // End of TEMPSENSE_APP_Coordinator_ProcessDataInd()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Coordinator Process incoming Data Confirmations
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	u8_t i;
	u8_t found;

	if (params->Status != MAC_SUCCESS)
	{
/* data request status fail */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xCB);
#endif /* APP_USE_DEBUG */
		return;
	}

	/* check if confirm corresponds to C_DATA packet sent */
	found = 0;
	for (i = 0; i < APP_MAX_DEVICES; ++i)
	{
		if ((APP_DevState[i] == APP_CST_C_DATA_REQUESTED) && (LS0_BYTE(APP_DevHandle[i]) == params->MsduHandle))
		{
			found           = 1;
			APP_DevState[i] = APP_CST_C_DATA_CONFIRMED;
			break;
		}
	}
	if (!found)
	{
/* confirm not matching for any device */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xCC);
#endif /* APP_USE_DEBUG */
	}

} // End of TEMPSENSE_APP_Coordinator_ProcessDataCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator check and display Data Packet
 *******************************************************************************
 * \param buf_ptr - Buffer containing data indication with data to display
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_DisplayData(u8_t                              device,
                                           u8_t                              edcoord,
                                           struct MCPS_DATA_indication_pset *params,
                                           struct ca821x_dev *               pDeviceRef)
{
	u8_t  temp;
	u16_t vbat;

	TEMPSENSE_APP_PrintSeconds();
	printf("TS: %u; N: %u", params->Src.Address[0], APP_DevHandle[device]);

	/* temperature */
	printf("; T: ");
	temp = params->Msdu[1];
	if (temp & 0x80) /* 1s complement */
	{
		printf("-");
		temp = ~temp + 1;
	}
	printf("%u'C", temp);

	/* vbat */
	vbat = (u16_t)(params->Msdu[3] << 8) + params->Msdu[2];
	TEMPSENSE_APP_Coordinator_CheckVbatt(vbat);

	/* cs / lqi */
	TEMPSENSE_APP_Coordinator_CheckLQI(params->Msdu[4], params->MpduLinkQuality);

	/* ed / rssi */
	TEMPSENSE_APP_Coordinator_CheckED(params->Msdu[5], edcoord);

	printf("\n");

} // End of TEMPSENSE_APP_Coordinator_DisplayData()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Timeout Check for all Devices
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckTimeouts(struct ca821x_dev *pDeviceRef)
{
	u8_t         i;
	u32_t        tnow;
	static u32_t tlast;
	i32_t        tdiff;
	u8_t         restart;

	tnow = TIME_ReadAbsoluteTime();

	restart = 0;
	for (i = 0; i < APP_MAX_DEVICES; ++i)
	{
		if (APP_DevAssociated[i] == 1)
		{
			/* avoids cases where tnow < timeout (u32_t) */
			/* this can happen when this routine gets interrupted by a packet reception */
			tdiff = tnow - APP_DevTimeout[i];
			if (tdiff > (1000 * APP_TIMEOUTINTERVALL))
			{
				TEMPSENSE_APP_PrintSeconds();
				printf("Sensor %u (0x%08X) Timeout, disconnected; Devices connected: %u\n",
				       i + 1,
				       APP_DevLongAddrLSBs[i],
				       (APP_NDEVICES - 1));
				APP_DevState[i]        = APP_CST_DONE;
				APP_DevAssociated[i]   = 0;
				APP_DevTimeout[i]      = 0;
				APP_DevLongAddrLSBs[i] = 0;
				APP_DevHandle[i]       = 0xFFFFFFFF;
				--APP_NDEVICES;
				/* apply restart if last device has been disconnected */
				if (APP_NDEVICES == 0)
				{
					restart = 1;
				}
			}
		}
	}

	if (restart)
	{
		EVBME_CAX_Restart(pDeviceRef);
	}

	/* coordinator soft reinitialisation if nothing is connected */
	if (APP_COORD_SREINIT)
	{
		if (APP_NDEVICES == 0)
		{
			if ((((tnow - tlast) > (1000 * APP_WAKEUPINTERVALL)) && (APP_WAKEUPINTERVALL != 0)) ||
			    (((tnow - tlast) > (100)) && (APP_WAKEUPINTERVALL == 0)))
			{
				TEMPSENSE_APP_Coordinator_SoftReinit(pDeviceRef);
				tlast = tnow;
			}
		}
	}

} // End of TEMPSENSE_APP_Coordinator_CheckTimeouts()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Checks and displays Battery Voltage
 *******************************************************************************
 * \param vbat - 12-bit Battery Voltage Reading from Device
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckVbatt(u16_t vbat)
{
	u8_t ones, tens;
	/* Vadc = adcval / 4096 * 3.3 = 0.6 * VBat */
	/* VBat = adcval / 4096 * 5.5 = adcval / 745 */
	ones = (u8_t)(vbat / 745);
	tens = (u8_t)(((vbat - (ones * 745)) * 10) / 745);
	if ((vbat < APP_VBAT_LIMIT) || APP_REPORT_VBATT)
	{
		printf("; VBat: %u.%uV", ones, tens);
	}
	if (vbat < APP_VBAT_LIMIT)
	{
		printf("; Warning: Requires Charging!");
	}
} // End of TEMPSENSE_APP_Coordinator_CheckVbatt()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Checks and displays LQI from both Sides
 *******************************************************************************
 * \param lqi_ts    - LQI received at Temperature Sensor (Device)
 * \param lqi_coord - LQI received locally (Coordinator)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckLQI(u8_t lqi_ts, u8_t lqi_coord)
{
	if (APP_REPORT_LQI)
	{
		printf("; LQI TS: ");
		if (lqi_ts == 0)
			printf("-");
		else
			printf("%u", lqi_ts);
		printf("; LQI C: %u", lqi_coord);
	}
	if (lqi_coord < APP_LQI_LIMIT)
		printf("; Warning: LQI low!");
} // End of TEMPSENSE_APP_Coordinator_CheckLQI()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Checks and displays ED from both Sides
 *******************************************************************************
 * \param ed_ts    - ED received at Temperature Sensor (Device)
 * \param ed_coord - ED received locally (Coordinator)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckED(u8_t ed_ts, u8_t ed_coord)
{
	if (APP_REPORT_ED)
	{
		printf("; ED TS: ");
		if (ed_ts == 0)
			printf("-");
		else
			printf("%u", ed_ts);
		printf("; ED C: %u", ed_coord);
	}
	if (ed_coord < APP_ED_LIMIT)
		printf("; Warning: ED low!");
} // End of TEMPSENSE_APP_Coordinator_CheckED()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Soft Reset (no change in PIB)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_SoftReinit(struct ca821x_dev *pDeviceRef)
{
	u8_t status;
	u8_t assocpermit;

	status = MLME_RESET_request_sync(0, pDeviceRef); /* SetDefaultPIB = FALSE */

	if (status)
	{
		BSP_LEDSigMode(LED_M_SETERROR);
		return;
	}

	status = MLME_START_request_sync(APP_PANId,   /* PANId */
	                                 APP_Channel, /* LogicalChannel */
	                                 15,          /* BeaconOrder */
	                                 15,          /* SuperframeOrder */
	                                 1,           /* PANCoordinator */
	                                 0,           /* BatteryLifeExtension */
	                                 0,           /* CoordRealignment */
	                                 NULLP,       /* *pCoordRealignSecurity */
	                                 NULLP,       /* *pBeaconSecurity */
	                                 pDeviceRef);

	if (status)
		BSP_LEDSigMode(LED_M_SETERROR);

	assocpermit = 1;
	MLME_SET_request_sync(macAssociationPermit, 0, 1, &assocpermit, pDeviceRef);

} // End of TEMPSENSE_APP_Coordinator_SoftReinit()
