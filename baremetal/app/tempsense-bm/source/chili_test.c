/**
 * @file chili_test.c
 * @brief Chili Module Production Test Modes
 * @author Wolfgang Bruchner
 * @date 08/02/17
 *//*
 * Copyright (C) 2017  Cascoda, Ltd.
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
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "chili_test.h"

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
static u8_t  CHILI_TEST_MODE    = CHILI_TEST_OFF;      /* test mode */
static u8_t  CHILI_TEST_RESULT  = 0xFF;                /* test result */
static u8_t  CHILI_TEST_CState  = CHILI_TEST_CST_DONE; /* communication state */
static u32_t CHILI_TEST_Timeout = 0;                   /* device timeout */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Production Test Initialisation
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	/* store no-comms status */
	if (status == CA_ERROR_FAIL)
	{
		CHILI_TEST_RESULT = CHILI_TEST_ST_NOCOMMS;
	}
} // End of CHILI_TEST_Initialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Production Test Handler
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_Handler(struct ca821x_dev *pDeviceRef)
{
	if (CHILI_TEST_MODE)
	{
		/* reset test mode when unplugged */
		if (!BSP_IsUSBPresent())
			CHILI_TEST_MODE = CHILI_TEST_OFF;
		/* check device timeout */
		if (CHILI_TEST_MODE == CHILI_TEST_DUT)
		{
			if (CHILI_TEST_CState == CHILI_TEST_CST_DUT_DISPLAY)
				CHILI_TEST_DUT_DisplayResult(pDeviceRef);
			if (CHILI_TEST_CState != CHILI_TEST_CST_DUT_FINISHED)
				CHILI_TEST_DUT_CheckTimeout(pDeviceRef);
		}
	}
} // End of CHILI_TEST_Handler()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Test Dispatch Branch (UpStream, Serial)
 *******************************************************************************
 ******************************************************************************/
int CHILI_TEST_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	/* check test command sequence validity */
	if ((SerialRxBuffer->Data[0] == EVBME_TEST_DATA0) && (SerialRxBuffer->Data[1] == EVBME_TEST_DATA1) &&
	    (SerialRxBuffer->Data[2] == EVBME_TEST_DATA2) && (SerialRxBuffer->CmdLen == EVBME_TEST_LEN))
	{
		/* start test (DUT) */
		if ((SerialRxBuffer->CmdId == EVBME_TEST_START_TEST) || (SerialRxBuffer->CmdId == EVBME_TEST_START_TEST_2))
		{
			CHILI_TEST_MODE = CHILI_TEST_DUT;
			if (CHILI_TEST_RESULT == CHILI_TEST_ST_NOCOMMS)
			{
				CHILI_TEST_CState = CHILI_TEST_CST_DUT_DISPLAY;
				return 1;
			}
			CHILI_TEST_TestInit(pDeviceRef);
			CHILI_TEST_DUT_ExchangeData(pDeviceRef);
			return 1;
		}
		/* setup REF device (REF) */
		if (SerialRxBuffer->CmdId == EVBME_TEST_SETUP_REF)
		{
			CHILI_TEST_MODE = CHILI_TEST_REF;
			CHILI_TEST_TestInit(pDeviceRef);
			printf("Reference Device initialised\n");
			return 1;
		}
	}
	return 0;
} // End of CHILI_TEST_UpStreamDispatch()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Production Test Start of Test Device Initialisation
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_TestInit(struct ca821x_dev *pDeviceRef)
{
	CHILI_TEST_CState  = CHILI_TEST_CST_DONE;
	CHILI_TEST_Timeout = 0;
	/* Reference Device */
	CHILI_TEST_InitPIB(pDeviceRef);
	CHILI_TEST_RESULT = 0xFF;
	/* dynamically register callbacks */
	CHILI_TEST_RegisterCallbacks(pDeviceRef);
} // End of CHILI_TEST_TestInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Production Test Initialisation of MAC PIB
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_InitPIB(struct ca821x_dev *pDeviceRef)
{
	u8_t  param8;
	u16_t param16;
	/* reset PIB */
	MLME_RESET_request_sync(1, pDeviceRef);
	/* initialise basic pib */
	param8 = CHILI_TEST_CHANNEL;
	MLME_SET_request_sync(phyCurrentChannel, 0, 1, &param8, pDeviceRef); // set 15.4 Channel
	param16 = CHILI_TEST_PANID;
	MLME_SET_request_sync(macPANId, 0, 2, &param16, pDeviceRef); // set local PANId
	if (CHILI_TEST_MODE == CHILI_TEST_REF)
		param16 = CHILI_TEST_REF_SHORTADD;
	else
		param16 = CHILI_TEST_DUT_SHORTADD;
	MLME_SET_request_sync(macShortAddress, 0, 2, &param16, pDeviceRef); // set local short address
	/* change PIB defaults */
	param8 = 3;
	MLME_SET_request_sync(macMinBE, 0, 1, &param8, pDeviceRef); // default = 3
	param8 = 5;
	MLME_SET_request_sync(macMaxBE, 0, 1, &param8, pDeviceRef); // default = 5
	param8 = 5;
	MLME_SET_request_sync(macMaxCSMABackoffs, 0, 1, &param8, pDeviceRef); // default = 4
	param8 = 7;
	MLME_SET_request_sync(macMaxFrameRetries, 0, 1, &param8, pDeviceRef); // default = 3
	/* set RxOnWhenIdle */
	param8 = 1;
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &param8, pDeviceRef);
} // End of CHILI_TEST_InitPIB()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reference Device Process incoming Data Confirm
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_REF_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	/* reset test */
	CHILI_TEST_TestInit(pDeviceRef);
} // End of CHILI_TEST_REF_ProcessDataCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reference Device Process incoming Data Indication
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_REF_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	u16_t           shortadd, panid;
	u8_t            cs_ref = 0;
	u8_t            ed_ref = 0;
	u8_t            len;
	struct FullAddr DUTAdd;
	u8_t            msdu[3];

	/* check received packet */
	panid    = (u16_t)(params->Src.PANId[1] << 8) + params->Src.PANId[0];
	shortadd = (u16_t)(params->Src.Address[1] << 8) + params->Src.Address[0];

	if ((panid == CHILI_TEST_PANID) && (shortadd == CHILI_TEST_DUT_SHORTADD) && (params->Msdu[0] == PT_MSDU_TEST_DUT))
	{
		/* correct device identifiers and packet id */
		/* get ed and cs values on reference device side */
		cs_ref = params->MpduLinkQuality;
		HWME_GET_request_sync(HWME_EDVALLP, &len, &ed_ref, pDeviceRef);

		PUTLE16(CHILI_TEST_DUT_SHORTADD, DUTAdd.Address);
		PUTLE16(CHILI_TEST_PANID, DUTAdd.PANId);
		DUTAdd.AddressMode = MAC_MODE_SHORT_ADDR;

		/* send data packet back to DUT */
		msdu[0] = PT_MSDU_TEST_REF; /* id */
		msdu[1] = cs_ref;           /* received cs at reference device */
		msdu[2] = ed_ref;           /* received ed at reference device */

		TIME_WaitTicks(20);

		MCPS_DATA_request(MAC_MODE_SHORT_ADDR, /* SrcAddrMode */
		                  DUTAdd,              /* DstAddr */
		                  3,                   /* MsduLength */
		                  msdu,                /* *pMsdu */
		                  0,                   /* MsduHandle */
		                  TXOPT_ACKREQ,        /* TxOptions */
		                  NULLP,               /* *pSecurity */
		                  pDeviceRef);
	}

} // End of CHILI_TEST_REF_ProcessDataInd()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Data Exchange
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_DUT_ExchangeData(struct ca821x_dev *pDeviceRef)
{
	struct FullAddr REFAdd;
	u8_t            msdu[1];

	TIME_WaitTicks(100);

	CHILI_TEST_Timeout = TIME_ReadAbsoluteTime();

	/* send data packet to REF */
	msdu[0] = PT_MSDU_TEST_DUT; /* id */

	PUTLE16(CHILI_TEST_REF_SHORTADD, REFAdd.Address);
	PUTLE16(CHILI_TEST_PANID, REFAdd.PANId);
	REFAdd.AddressMode = MAC_MODE_SHORT_ADDR;

	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, /* SrcAddrMode */
	                  REFAdd,              /* DstAddr */
	                  1,                   /* MsduLength */
	                  msdu,                /* *pMsdu */
	                  0,                   /* MsduHandle */
	                  TXOPT_ACKREQ,        /* TxOptions */
	                  NULLP,               /* *pSecurity */
	                  pDeviceRef);

	CHILI_TEST_CState = CHILI_TEST_CST_DUT_D_REQUESTED;
	return;

} // End of CHILI_TEST_DUT_ExchangeData()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Process incoming Data Indication
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_DUT_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	u16_t shortadd, panid;
	u8_t  cs_ref = 0;
	u8_t  ed_ref = 0;
	u8_t  cs_dut = 0;
	u8_t  ed_dut = 0;
	u8_t  len;

	if (CHILI_TEST_CState != CHILI_TEST_CST_DUT_D_CONFIRMED)
	{
		/* out of order, no indication expected */
		CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_IND_UNEXP;
		goto exit;
	}

	/* check received packet */
	panid    = (u16_t)(params->Src.PANId[1] << 8) + params->Src.PANId[0];
	shortadd = (u16_t)(params->Src.Address[1] << 8) + params->Src.Address[0];

	if ((panid != CHILI_TEST_PANID) || (shortadd != CHILI_TEST_REF_SHORTADD) || (params->Msdu[0] != PT_MSDU_TEST_REF))
	{
		/* wrong packet id */
		CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_IND_ID;
		goto exit;
	}

	/* packet received ok */
	/* analyse cs and ed values of both ref and dut side */
	cs_ref = params->Msdu[1];
	ed_ref = params->Msdu[2];
	cs_dut = params->MpduLinkQuality;
	HWME_GET_request_sync(HWME_EDVALLP, &len, &ed_dut, pDeviceRef);

	if (cs_ref < CHILI_TEST_CS_LIMIT)
		CHILI_TEST_RESULT = CHILI_TEST_ST_CS_REF_LOW;
	else if (cs_dut < CHILI_TEST_CS_LIMIT)
		CHILI_TEST_RESULT = CHILI_TEST_ST_CS_DUT_LOW;
	else if (ed_ref < CHILI_TEST_ED_LIMIT)
		CHILI_TEST_RESULT = CHILI_TEST_ST_ED_REF_LOW;
	else if (ed_dut < CHILI_TEST_ED_LIMIT)
		CHILI_TEST_RESULT = CHILI_TEST_ST_ED_DUT_LOW;
	else
		CHILI_TEST_RESULT = CHILI_TEST_ST_SUCCESS;

exit:
	TIME_WaitTicks(100);
	CHILI_TEST_CState = CHILI_TEST_CST_DUT_DISPLAY;

} // End of CHILI_TEST_DUT_ProcessDataInd()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Process incoming Data Confirm
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_DUT_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	u8_t status;

	if (CHILI_TEST_CState != CHILI_TEST_CST_DUT_D_REQUESTED)
	{
		/* out of order, no confirm expected */
		CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_CNF_UNEXP;
		CHILI_TEST_CState = CHILI_TEST_CST_DUT_DISPLAY;
		return;
	}

	status = params->Status;
	if (status != MAC_SUCCESS)
	{
		/* bad data confirm status */
		if (status == MAC_NO_ACK)
			CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_CNF_NO_ACK;
		else if (status == MAC_CHANNEL_ACCESS_FAILURE)
			CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_CNF_CHACCF;
		else if (status == MAC_TRANSACTION_OVERFLOW)
			CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_CNF_TROVFL;
		else
			CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_CNF_OTHERS;
		CHILI_TEST_CState = CHILI_TEST_CST_DUT_DISPLAY;
	}

	CHILI_TEST_CState = CHILI_TEST_CST_DUT_D_CONFIRMED;

} // End of CHILI_TEST_DUT_ProcessDataCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Timeout Check
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_DUT_CheckTimeout(struct ca821x_dev *pDeviceRef)
{
	u32_t tnow;
	i32_t tdiff;

	tnow = TIME_ReadAbsoluteTime();

	/* avoids cases where tnow < timeout (u32_t) */
	/* this can happen when this routine gets interrupted by a packet reception */
	tdiff = tnow - CHILI_TEST_Timeout;

	if (tdiff > CHILI_TEST_DUT_TIMEOUT)
	{
		if (CHILI_TEST_CState == CHILI_TEST_CST_DUT_D_REQUESTED)
			CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_CNF_TIMEOUT;
		else if (CHILI_TEST_CState == CHILI_TEST_CST_DUT_D_CONFIRMED)
			CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_IND_TIMEOUT;
		else
			CHILI_TEST_RESULT = CHILI_TEST_ST_TIMEOUT;
		CHILI_TEST_CState = CHILI_TEST_CST_DUT_DISPLAY;
	}

} // End of CHILI_TEST_DUT_CheckTimeout()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Display Test Result
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_DUT_DisplayResult(struct ca821x_dev *pDeviceRef)
{
	if (CHILI_TEST_RESULT == CHILI_TEST_ST_SUCCESS)
	{
		printf("Test Result: PASS\n");
		//	printf("Test Result: PASS; Time=%ums\n", (TIME_ReadAbsoluteTime() - CHILI_TEST_Timeout));
	}
	else
	{
		printf("Test Result: FAIL\n");
		//	printf("Test Result: FAIL; Time=%ums\n", (TIME_ReadAbsoluteTime() - CHILI_TEST_Timeout));
		printf("Fail Status = 0x%02X\n", CHILI_TEST_RESULT);
	}

	CHILI_TEST_CState = CHILI_TEST_CST_DUT_FINISHED;

	/* reset transceiver MAC */
	MLME_RESET_request_sync(1, pDeviceRef);

} // End of CHILI_TEST_DUT_DisplayResult()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks if Device is in Production Test Mode (yes if non-zero)
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_TEST_IsInTestMode(void)
{
	return (CHILI_TEST_MODE);
} // End of CHILI_TEST_IsInTestMode()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reference Device Callback for MCPS_DATA_indication in CHILI_TEST_MODE
 *******************************************************************************
 ******************************************************************************/
static ca_error CHILI_TEST_REF_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                    struct ca821x_dev *               pDeviceRef)
{
	CHILI_TEST_REF_ProcessDataInd(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of CHILI_TEST_REF_MCPS_DATA_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Callback for MCPS_DATA_indication in CHILI_TEST_MODE
 *******************************************************************************
 ******************************************************************************/
static ca_error CHILI_TEST_DUT_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                    struct ca821x_dev *               pDeviceRef)
{
	CHILI_TEST_DUT_ProcessDataInd(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of CHILI_TEST_DUT_MCPS_DATA_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reference Device Callback for MCPS_DATA_confirm in CHILI_TEST_MODE
 *******************************************************************************
 ******************************************************************************/
static ca_error CHILI_TEST_REF_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	CHILI_TEST_REF_ProcessDataCnf(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of CHILI_TEST_REF_MCPS_DATA_confirm()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Callback for MCPS_DATA_confirm in CHILI_TEST_MODE
 *******************************************************************************
 ******************************************************************************/
static ca_error CHILI_TEST_DUT_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	CHILI_TEST_DUT_ProcessDataCnf(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of CHILI_TEST_DUT_MCPS_DATA_confirm()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_COMM_STATUS_indication
 *******************************************************************************
 ******************************************************************************/
static ca_error CHILI_TEST_MLME_COMM_STATUS_indication(struct MLME_COMM_STATUS_indication_pset *params,
                                                       struct ca821x_dev *                      pDeviceRef)
{
	/* only supress message */
	return CA_ERROR_SUCCESS;
} // End of CHILI_TEST_MLME_COMM_STATUS_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dynamically Register Callbacks for CHILI_TEST_MODE
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_RegisterCallbacks(struct ca821x_dev *pDeviceRef)
{
	if (CHILI_TEST_MODE == CHILI_TEST_REF)
	{
		pDeviceRef->callbacks.MCPS_DATA_indication        = &CHILI_TEST_REF_MCPS_DATA_indication;
		pDeviceRef->callbacks.MCPS_DATA_confirm           = &CHILI_TEST_REF_MCPS_DATA_confirm;
		pDeviceRef->callbacks.MLME_COMM_STATUS_indication = &CHILI_TEST_MLME_COMM_STATUS_indication;
	}
	else if (CHILI_TEST_MODE == CHILI_TEST_DUT)
	{
		pDeviceRef->callbacks.MCPS_DATA_indication        = &CHILI_TEST_DUT_MCPS_DATA_indication;
		pDeviceRef->callbacks.MCPS_DATA_confirm           = &CHILI_TEST_DUT_MCPS_DATA_confirm;
		pDeviceRef->callbacks.MLME_COMM_STATUS_indication = &CHILI_TEST_MLME_COMM_STATUS_indication;
	}
	else
	{
		pDeviceRef->callbacks.MCPS_DATA_indication        = NULL;
		pDeviceRef->callbacks.MCPS_DATA_confirm           = NULL;
		pDeviceRef->callbacks.MLME_COMM_STATUS_indication = NULL;
	}
} // End of CHILI_TEST_RegisterCallbacks()
