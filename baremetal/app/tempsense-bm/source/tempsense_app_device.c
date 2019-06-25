/**
 * @file tempsense_app.c
 * @brief Chili temperature sensing app functions for device
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
static u8_t APP_DEVICE_RETRIES = 0; //!< count number of retries to avoid immediate disconnection

static u32_t APP_Handle = 0;            /* device handle / sequence number */
static u8_t  APP_CState = APP_CST_DONE; /* device communications state */

static u8_t  APP_DevTemp    = 0; /* sensor temperature value */
static u16_t APP_DevVolts   = 0; /* sensor volts value */
static u32_t APP_DevTimeout = 0; /* device timeout */

static u32_t APP_FlashSaveAddr = 0x00000000; /* flash save address */
static void  TEMPSENSE_APP_Device_GetFlashSaveAddress(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Handler for Sensor Device
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_Handler(struct ca821x_dev *pDeviceRef)
{
	if (APP_CONNECTED)
	{
		/* start exchanging data with coordinator */
		if (APP_CState == APP_CST_DONE)
			TEMPSENSE_APP_Device_ExchangeData(pDeviceRef);
	}
	else
	{
		/* try to connect to coordinator */
		if (APP_CState == APP_CST_DONE)
			TEMPSENSE_APP_Device_Start(pDeviceRef);
	}
	TEMPSENSE_APP_Device_CheckTimeout(pDeviceRef);

	/* go into powerdown mode */
	if (APP_CState == APP_CST_GO_POWERDOWN)
		TEMPSENSE_APP_Device_GoPowerDown(pDeviceRef);

} // End of TEMPSENSE_APP_Device_Handler()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation for Sensor Device
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_Initialise(struct ca821x_dev *pDeviceRef)
{
	APP_Handle         = 0;
	APP_CState         = APP_CST_DONE;
	APP_CONNECTED      = 0;
	APP_DEVICE_RETRIES = 0;
	APP_DevTimeout     = TIME_ReadAbsoluteTime();

	/* initialise PIB */
	/* IEEE Address: upper 4 bytes are CA 5C 0D A0, lower 4 bytes are random */
	APP_PANId        = MAC_PANID;
	APP_ShortAddress = 0xFFFF;
	TEMPSENSE_APP_Device_GetLongAddress();

	/* initialise MAC PIB */
	TEMPSENSE_APP_InitPIB(pDeviceRef);

	/* enable watchdog timer for device */
	if (APP_USE_WATCHDOG)
		BSP_WatchdogEnable(1000 * APP_TIMEOUTINTERVALL);

	/* get address of first free flash entry */
	if (APP_DEFAULT_PDN_MODE == PDM_DPD)
	{
		TEMPSENSE_APP_Device_GetFlashSaveAddress();
	}

	/* try to connect */
	TEMPSENSE_APP_Device_Start(pDeviceRef);

} // End of TEMPSENSE_APP_Device_Initialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Get Device LongAddress from Dataflash
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_GetLongAddress(void)
{
	u32_t dsaved;
	memcpy(APP_LongAddress, (u8_t[])MAC_LONGADD, 8);
	/* read stored address */
	dsaved             = BSP_GetUniqueId() & 0xFFFFFFFF;
	APP_LongAddress[3] = LS3_BYTE(dsaved);
	APP_LongAddress[2] = LS2_BYTE(dsaved);
	APP_LongAddress[1] = LS1_BYTE(dsaved);
	APP_LongAddress[0] = LS0_BYTE(dsaved);
} // End of TEMPSENSE_APP_Device_GetLongAddress()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device Association Procedure
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_Start(struct ca821x_dev *pDeviceRef)
{
	u8_t status;

	APP_Handle = 0;
	APP_CState = APP_CST_DONE;

	/* perform active scan */
	status = MLME_SCAN_request(ACTIVE_SCAN, TEMPSENSE_APP_GetScanChannels(), 2, NULLP, pDeviceRef);

	if (status)
	{
/* scan request fail (highly unlikely) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xB1);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	/* success */
	APP_CState = APP_CST_SCANNING;
	return;

exit:
	APP_CState = APP_CST_GO_POWERDOWN;

} // End of TEMPSENSE_APP_Device_Start()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Scan Confirm
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessScanCnf(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct FullAddr       CoordAdd;
	u8_t                  status;
	u8_t                  i;
	u16_t                 panid;
	struct PanDescriptor *ppandesc;

	APP_ShortAddress = 0xFFFF;

	if (APP_CState != APP_CST_SCANNING)
	{
/* out of order, no scan confirm expected */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xB2);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	if ((params->ScanType != ACTIVE_SCAN) || (params->Status != MAC_SUCCESS) || (params->ResultListSize == 0))
	{
/* scan confirm fail or no result list */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xB3);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	/* initialise addressing */
	memcpy(CoordAdd.Address, (u8_t[])MAC_LONGADD, 8);
	PUTLE16(APP_PANId, CoordAdd.PANId);
	CoordAdd.AddressMode = MAC_MODE_LONG_ADDR;

	/* look for correct panid and select channel if possible */
	APP_Channel = 0;
	for (i = 0; i < params->ResultListSize; ++i)
	{
		ppandesc = ((struct PanDescriptor *)(params->ResultList + i * 22));
		panid    = (u16_t)(ppandesc->Coord.PANId[1] << 8) + ppandesc->Coord.PANId[0];

		if ((panid == APP_PANId) && (!memcmp(CoordAdd.Address, ppandesc->Coord.Address, 8)) &&
		    (ppandesc->LinkQuality > APP_LQI_LIMIT))
		{
			APP_Channel = ppandesc->LogicalChannel;
			break;
		}
	}

	if (APP_Channel == 0)
	{
/* no valid coordinator found */
/* (panid incorrect or coordinater address incorrect or lqi too low) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xB4);
#endif /* APP_USE_DEBUG */
		APP_Channel = 11;
		goto exit;
	}

	/* associate if scan has detected valid coordinator */
	/* association request */
	status = MLME_ASSOCIATE_request(APP_Channel, /* LogicalChannel */
	                                CoordAdd,    /* DstAddr */
	                                0x80,        /* CapabilityInfo  - only AllocateAddress set to 1 */
	                                NULLP,       /* pSecurity */
	                                pDeviceRef);

	if (status)
	{
/* associate request fail (highly unlikely) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xB5);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	/* wait and process associate confirm */
	APP_CState = APP_CST_ASSOC_REQUESTED;
	return;

exit:
	APP_CState = APP_CST_GO_POWERDOWN;

} // End of TEMPSENSE_APP_Device_ProcessScanCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Data Indications
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessAssociateCnf(struct MLME_ASSOCIATE_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	/* ignore if already connected */
	if ((!APP_CONNECTED) && (APP_CState == APP_CST_ASSOC_REQUESTED))

	{
		if (params->Status != MAC_SUCCESS)
		{
/* associate confirm fail */
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xB6);
#endif /* APP_USE_DEBUG */
			goto exit;
		}

/* success */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0x0B);
#endif /* APP_USE_DEBUG */
		APP_ShortAddress   = (u16_t)params->AssocShortAddress[0] + ((u16_t)params->AssocShortAddress[1] << 8);
		APP_CONNECTED      = 1;
		APP_DEVICE_RETRIES = 0;

	exit:
		APP_CState = APP_CST_GO_POWERDOWN;
	}

} // End of TEMPSENSE_APP_Device_ProcessAssociateCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Data Indications
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	struct FullAddr CoordFAdd;
	u8_t            msdu[6];
	u8_t            lqi_device;
	u8_t            ed_device;
	u8_t            len;
	u8_t            status;

	if (params->Msdu[0] != PT_MSDU_C_DATA)
	{
/* wrong packet type */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD1);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	if (APP_CState != APP_CST_WAKEUP_CONFIRMED)
	{
/* out of order (no data indication expected) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD2);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	if (params->MsduLength != 1)
	{
/* wrong msdu length */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD3);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	/* analyse C_DATA from coordinator */
	lqi_device = params->MpduLinkQuality; /* cs */
	ed_device  = 0;
	HWME_GET_request_sync(HWME_EDVALLP, &len, &ed_device, pDeviceRef); /* ed */

	/* coordinator address */
	CoordFAdd.AddressMode = MAC_MODE_LONG_ADDR;
	memcpy(CoordFAdd.Address, (u8_t[])MAC_LONGADD, 8);
	PUTLE16(APP_PANId, CoordFAdd.PANId);

	/* send D_DATA packet to coordinator */
	msdu[0] = PT_MSDU_D_DATA;        /* id */
	msdu[1] = APP_DevTemp;           /* temperature reading */
	msdu[2] = LS_BYTE(APP_DevVolts); /* vbatt reading */
	msdu[3] = MS_BYTE(APP_DevVolts);
	msdu[4] = lqi_device; /* received lqi at device */
	msdu[5] = ed_device;  /* received ed  at device */

	status = MCPS_DATA_request(MAC_MODE_SHORT_ADDR,  /* SrcAddrMode */
	                           CoordFAdd,            /* DstAddr */
	                           6,                    /* MsduLength */
	                           msdu,                 /* *pMsdu */
	                           LS0_BYTE(APP_Handle), /* MsduHandle */
	                           TXOPT_ACKREQ,         /* TxOptions */
	                           NULLP,                /* *pSecurity */
	                           pDeviceRef);

	if (status)
	{
/* data request fail (highly unlikely) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD4);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	APP_CState = APP_CST_D_DATA_REQUESTED;
	return;

exit:
	++APP_DEVICE_RETRIES;
	APP_CState = APP_CST_GO_POWERDOWN;

} // End of TEMPSENSE_APP_Device_ProcessDataInd()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Data Confirms
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (params->Status != MAC_SUCCESS)
	{
/* data request status fail */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD5);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	if ((APP_CState != APP_CST_WAKEUP_REQUESTED) && (APP_CState != APP_CST_D_DATA_REQUESTED))
	{
/* out of order (no data confirm expected) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD6);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	if (LS0_BYTE(APP_Handle) != params->MsduHandle)
	{
/* handle not matching */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD7);
#endif /* APP_USE_DEBUG */
		goto exit;
	}

	/* success */
	if (APP_CState == APP_CST_WAKEUP_REQUESTED)
	{
		APP_CState = APP_CST_WAKEUP_CONFIRMED;
		return;
	}

	++APP_Handle;
#if APP_USE_DEBUG
	APP_Debug_SetAppState(0x0D);
#endif /* APP_USE_DEBUG */
	APP_DEVICE_RETRIES = 0;
	APP_CState         = APP_CST_GO_POWERDOWN;
	return;

exit:
	++APP_DEVICE_RETRIES;
	APP_CState = APP_CST_GO_POWERDOWN;

} // End of TEMPSENSE_APP_Device_ProcessDataCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Start exchanging Data with Coordinator
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ExchangeData(struct ca821x_dev *pDeviceRef)
{
	u8_t            msdu[6];
	struct FullAddr CoordFAdd;
	u8_t            status;
	u8_t            rxidle;

	/* get sensing values (before turning receiver on to save power) */
	APP_DevVolts = TEMPSENSE_APP_GetVoltsVal();
	APP_DevTemp  = TEMPSENSE_APP_GetTempVal();

	/* coordinator address */
	CoordFAdd.AddressMode = MAC_MODE_LONG_ADDR;
	memcpy(CoordFAdd.Address, (u8_t[])MAC_LONGADD, 8);
	PUTLE16(APP_PANId, CoordFAdd.PANId);

	/* turn on receiver (set RxOnWhenIdle) */
	rxidle = 1;
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxidle, pDeviceRef);

	/* send WAKEUP packet to coordinator */
	msdu[0] = PT_MSDU_D_WAKEUP;     /* id */
	msdu[1] = LS0_BYTE(APP_Handle); /* sequence number */
	msdu[2] = LS1_BYTE(APP_Handle);
	msdu[3] = LS2_BYTE(APP_Handle);
	msdu[4] = LS3_BYTE(APP_Handle);

	status = MCPS_DATA_request(MAC_MODE_SHORT_ADDR,  /* SrcAddrMode */
	                           CoordFAdd,            /* DstAddr */
	                           5,                    /* MsduLength */
	                           msdu,                 /* *pMsdu */
	                           LS0_BYTE(APP_Handle), /* MsduHandle */
	                           TXOPT_ACKREQ,         /* TxOptions */
	                           NULLP,                /* *pSecurity */
	                           pDeviceRef);

	if (status)
	{
/* data request fail (highly unlikely) */
#if APP_USE_DEBUG
		APP_Debug_SetAppState(0xD8);
#endif /* APP_USE_DEBUG */
		APP_CState = APP_CST_GO_POWERDOWN;
	}
	else
	{
		APP_CState = APP_CST_WAKEUP_REQUESTED;
	}

} // End of TEMPSENSE_APP_Device_ExchangeData()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device Timeout Check
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_CheckTimeout(struct ca821x_dev *pDeviceRef)
{
	u32_t tnow;
	i32_t tdiff;

	tnow = TIME_ReadAbsoluteTime();

	/* avoids cases where tnow < timeout (u32_t) */
	/* this can happen when this routine gets interrupted by a packet reception */
	tdiff = tnow - APP_DevTimeout;

	if (tdiff > APP_DEVICETIMEOUT)
	{
#if APP_USE_DEBUG
		if (APP_CONNECTED)
			APP_Debug_SetAppState(0xD9);
		else
			APP_Debug_SetAppState(0xB9);
#endif /* APP_USE_DEBUG */
		APP_DevTimeout = 0;
		++APP_DEVICE_RETRIES;
		APP_CState = APP_CST_GO_POWERDOWN;
	}

} // End of TEMPSENSE_APP_Device_CheckTimeout()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device prepare and enter sleep mode (and wake-up)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_GoPowerDown(struct ca821x_dev *pDeviceRef)
{
	u8_t  rxidle;
	u32_t twakeup;

	/* before powerdown */

	/* turn off receiver (reset RxOnWhenIdle) */
	rxidle = 0;
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxidle, pDeviceRef);

	APP_CState = APP_CST_DONE;

	/* disconnect if high level retries exhausted */
	if (APP_DEVICE_RETRIES > APP_DEVICE_MAX_RETRIES)
		APP_CONNECTED = 0;

#if APP_USE_DEBUG
	APP_Debug_Send(pDeviceRef);
#endif /* APP_USE_DEBUG */

	/* power down for APP_WAKEUPINTERVALL */
	if (APP_WAKEUPINTERVALL == 0)
	{
		/* stress test mode - 100 ms wakeup intervall */
		twakeup = 100;
	}
	else
	{
		/* seconds to ms */
		twakeup = (u32_t)(APP_WAKEUPINTERVALL * 1000);
	}

	if (APP_DEFAULT_PDN_MODE == PDM_DPD)
	{
		TEMPSENSE_APP_Device_SaveStateToFlash();
	}

	EVBME_PowerDown(APP_DEFAULT_PDN_MODE, twakeup, pDeviceRef);

	/* after wakeup */
	APP_DevTimeout = TIME_ReadAbsoluteTime();

	/* watchdog used ? */
	if (APP_USE_WATCHDOG)
	{
		if (BSP_IsWatchdogTriggered())
		{
#if APP_USE_DEBUG
			APP_Debug_SetAppState(0xDA);
			APP_Debug_Error(0x08);
#endif /* APP_USE_DEBUG */
			EVBME_CAX_Restart(pDeviceRef);
		}
		BSP_WatchdogReset(1000 * APP_TIMEOUTINTERVALL);
	}

	/* re-initialise MAC PIB if necessary */
	if ((APP_DEFAULT_PDN_MODE == PDM_POWEROFF) || (APP_DEFAULT_PDN_MODE == PDM_POWERDOWN))
	{
		TEMPSENSE_APP_InitPIB(pDeviceRef);
	}

} // End of TEMPSENSE_APP_Device_GoPowerDown()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device get address of first free flash entry
 *******************************************************************************
 ******************************************************************************/
static void TEMPSENSE_APP_Device_GetFlashSaveAddress(void)
{
	u32_t add;
	u32_t buffer;

	/* look for first free entry */
	for (add = 0; add < (BSP_FlashInfo.numPages * BSP_FlashInfo.pageSize); add += 8)
	{
		BSP_ReadDataFlash(add, &buffer, 1);
		if (buffer & 0x80000000)
		{
			APP_FlashSaveAddr = add;
			break;
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device save state to flash
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_SaveStateToFlash(void)
{
	u32_t buffer[2];
	u32_t i;

	BSP_ReadDataFlash(0, &buffer[0], 1);

	/* erase next page if last address has been reached */
	for (i = 1; i <= BSP_FlashInfo.numPages; ++i)
	{
		if (APP_FlashSaveAddr == (i * BSP_FlashInfo.pageSize - 8))
		{
			if (i == BSP_FlashInfo.numPages)
			{
				/* erase page 0 */
				BSP_EraseDataFlashPage(0);
			}
			else
			{
				/* erase next page */
				BSP_EraseDataFlashPage(i * BSP_FlashInfo.pageSize);
			}
		}
	}

	/* Dataset
	word	bit								nr_bits
	0		31		used indicator			 1
			28-24	APP_Channel				 5
			23		APP_CONNECTED			 1
			22-20	APP_STATE				 3
			29-16	APP_DEVICE_RETRIES		 4
			15-0	APP_ShortAddress		16
	1		31-0	APP_Handle				32
	*/
	buffer[0] = ((APP_Channel & 0x1F) << 24);
	buffer[0] += ((APP_CONNECTED & 0x01) << 23);
	buffer[0] += ((APP_STATE & 0x07) << 20);
	buffer[0] += ((APP_DEVICE_RETRIES & 0x0F) << 16);
	buffer[0] += (APP_ShortAddress & 0xFFFF);
	buffer[1] = APP_Handle;

	BSP_WriteDataFlashInitial(APP_FlashSaveAddr, buffer, 2);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device restore state from flash
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_RestoreStateFromFlash(struct ca821x_dev *pDeviceRef)
{
	u32_t add;
	u32_t buffer[2];
	u32_t i;

	BSP_DisableUSB();

	/* get address of first free flash entry */
	TEMPSENSE_APP_Device_GetFlashSaveAddress();

	if (APP_FlashSaveAddr == 0)
		add = ((BSP_FlashInfo.numPages * BSP_FlashInfo.pageSize) - 8);
	else
		add = APP_FlashSaveAddr - 8;

	BSP_ReadDataFlash(add, buffer, 2);

	APP_Channel        = (buffer[0] >> 24) & 0x1F;
	APP_CONNECTED      = (buffer[0] >> 23) & 0x01;
	APP_STATE          = (buffer[0] >> 20) & 0x07;
	APP_DEVICE_RETRIES = (buffer[0] >> 16) & 0x0F;
	APP_ShortAddress   = buffer[0] & 0xFFFF;
	APP_Handle         = buffer[1];

	/* data integrity check */
	if ((APP_Channel < 11) || (APP_Channel > 26))
	{
		for (i = 0; i < BSP_FlashInfo.numPages; ++i)
		{
			/* erase all pages */
			BSP_EraseDataFlashPage(i * BSP_FlashInfo.pageSize);
		}
		TEMPSENSE_APP_Initialise(pDeviceRef);
		return;
	}

	TEMPSENSE_APP_Device_GetLongAddress();

	APP_DevTimeout = TIME_ReadAbsoluteTime();
	APP_PANId      = MAC_PANID;
	APP_STATE_new  = APP_STATE;
	APP_CState     = APP_CST_DONE;
	APP_INITIALISE = 0;
}
