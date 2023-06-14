/**
 * @file
 * @brief Chili Module Production Test Modes
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
#include "cascoda-bm/chili_test.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "cascoda_chili_config.h"
#if (CASCODA_CHILI2_CONFIG == 2)
#include "devboard_btn.h"
#endif

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
static u8_t  CHILI_TEST_MODE    = CHILI_TEST_OFF;      /* test mode */
static u8_t  CHILI_TEST_RESULT  = 0xFF;                /* test result */
static u8_t  CHILI_TEST_CState  = CHILI_TEST_CST_DONE; /* communication state */
static u32_t CHILI_TEST_Timeout = 0;                   /* device timeout */

/* static functions */
static void     CHILI_TEST_TestInit(struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_InitPIB(struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_REF_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_REF_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_DUT_ExchangeData(struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_DUT_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_DUT_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_DUT_CheckTimeout(struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_DUT_DisplayResult(struct ca821x_dev *pDeviceRef);
static ca_error CHILI_TEST_REF_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                    struct ca821x_dev                *pDeviceRef);
static ca_error CHILI_TEST_DUT_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                    struct ca821x_dev                *pDeviceRef);
static ca_error CHILI_TEST_REF_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error CHILI_TEST_DUT_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error CHILI_TEST_MLME_COMM_STATUS_indication(struct MLME_COMM_STATUS_indication_pset *params,
                                                       struct ca821x_dev                       *pDeviceRef);
static void     CHILI_TEST_RegisterCallbacks(struct ca821x_dev *pDeviceRef);
static void     CHILI_TEST_LEDComplete(uint8_t status);

#if (CASCODA_CHILI2_CONFIG == 2)
static void CHILI_TEST_LEDInit(void);
static void CHILI_TEST_LEDResult(uint8_t status);
#endif

void CHILI_TEST_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;

	/* store no-comms status */
	if (status == CA_ERROR_FAIL)
	{
		CHILI_TEST_RESULT = CHILI_TEST_ST_NOCOMMS;
		return;
	}
} // End of CHILI_TEST_Initialise()

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

u8_t CHILI_TEST_IsInTestMode(void)
{
	return (CHILI_TEST_MODE);
} // End of CHILI_TEST_IsInTestMode()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Production Test Start of Test Device Initialisation
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_TestInit(struct ca821x_dev *pDeviceRef)
{
	CHILI_TEST_CState  = CHILI_TEST_CST_DONE;
	CHILI_TEST_Timeout = 0;
	/* Reference Device */
	CHILI_TEST_InitPIB(pDeviceRef);
	CHILI_TEST_RESULT = 0xFF;
	/* dynamically register callbacks */
	CHILI_TEST_RegisterCallbacks(pDeviceRef);
#if (CASCODA_CHILI2_CONFIG == 2)
	if (CHILI_TEST_MODE != CHILI_TEST_REF)
		CHILI_TEST_LEDInit();
#endif
} // End of CHILI_TEST_TestInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Production Test Initialisation of MAC PIB
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_InitPIB(struct ca821x_dev *pDeviceRef)
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
static void CHILI_TEST_REF_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	(void)params;
	/* reset test */
	CHILI_TEST_TestInit(pDeviceRef);
} // End of CHILI_TEST_REF_ProcessDataCnf()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reference Device Process incoming Data Indication
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_REF_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
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

#if CASCODA_CA_VER >= 8212
	//MSDU located after Header IEs and Payload IEs.
	uint8_t msdu_shift = params->HeaderIELength + params->PayloadIELength;
	if ((panid == CHILI_TEST_PANID) && (shortadd == CHILI_TEST_DUT_SHORTADD) &&
	    (params->Data[msdu_shift] == PT_MSDU_TEST_DUT))
#else
	if ((panid == CHILI_TEST_PANID) && (shortadd == CHILI_TEST_DUT_SHORTADD) && (params->Msdu[0] == PT_MSDU_TEST_DUT))
#endif // CASCODA_CA_VER >= 8212
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

		WAIT_ms(20);

#if CASCODA_CA_VER >= 8212
		uint8_t tx_op[2] = {0x00, 0x00};
		tx_op[0] |= TXOPT0_ACKREQ;
		MCPS_DATA_request(MAC_MODE_SHORT_ADDR, /* SrcAddrMode */
		                  DUTAdd,              /* DstAddr */
		                  0,                   /* HeaderIELength */
		                  0,                   /* PayloadIELength */
		                  3,                   /* MsduLength */
		                  msdu,                /* pMsdu */
		                  0,                   /* MsduHandle */
		                  tx_op,               /* pTxOptions */
		                  0,                   /* SchTimestamp */
		                  0,                   /* SchPeriod */
		                  0,                   /* TxChannel */
		                  NULLP,               /* pHeaderIEList */
		                  NULLP,               /* pPayloadIEList */
		                  NULLP,               /* pSecurity */
		                  pDeviceRef);         /* pDeviceRef */
#else
		MCPS_DATA_request(MAC_MODE_SHORT_ADDR, /* SrcAddrMode */
		                  DUTAdd,              /* DstAddr */
		                  3,                   /* MsduLength */
		                  msdu,                /* *pMsdu */
		                  0,                   /* MsduHandle */
		                  TXOPT_ACKREQ,        /* TxOptions */
		                  NULLP,               /* *pSecurity */
		                  pDeviceRef);
#endif // CASCODA_CA_VER >= 8212
	}

} // End of CHILI_TEST_REF_ProcessDataInd()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Data Exchange
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_DUT_ExchangeData(struct ca821x_dev *pDeviceRef)
{
	struct FullAddr REFAdd;
	u8_t            msdu[1];

	WAIT_ms(100);

	CHILI_TEST_Timeout = TIME_ReadAbsoluteTime();

	/* send data packet to REF */
	msdu[0] = PT_MSDU_TEST_DUT; /* id */

	PUTLE16(CHILI_TEST_REF_SHORTADD, REFAdd.Address);
	PUTLE16(CHILI_TEST_PANID, REFAdd.PANId);
	REFAdd.AddressMode = MAC_MODE_SHORT_ADDR;

#if CASCODA_CA_VER >= 8212
	uint8_t tx_op[2] = {0x00, 0x00};
	tx_op[0] |= TXOPT0_ACKREQ;
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, /* SrcAddrMode */
	                  REFAdd,              /* DstAddr */
	                  0,                   /* HeaderIELength */
	                  0,                   /* PayloadIELength */
	                  1,                   /* MsduLength */
	                  msdu,                /* pMsdu */
	                  0,                   /* MsduHandle */
	                  tx_op,               /* pTxOptions */
	                  0,                   /* SchTimestamp */
	                  0,                   /* SchPeriod */
	                  0,                   /* TxChannel */
	                  NULLP,               /* pHeaderIEList */
	                  NULLP,               /* pPayloadIEList */
	                  NULLP,               /* pSecurity */
	                  pDeviceRef);         /* pDeviceRef */
#else
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, /* SrcAddrMode */
	                  REFAdd,              /* DstAddr */
	                  1,                   /* MsduLength */
	                  msdu,                /* *pMsdu */
	                  0,                   /* MsduHandle */
	                  TXOPT_ACKREQ,        /* TxOptions */
	                  NULLP,               /* *pSecurity */
	                  pDeviceRef);
#endif // CASCODA_CA_VER >= 8212

	CHILI_TEST_CState = CHILI_TEST_CST_DUT_D_REQUESTED;
	return;

} // End of CHILI_TEST_DUT_ExchangeData()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Process incoming Data Indication
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_DUT_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
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

#if CASCODA_CA_VER >= 8212
	// MSDU located after Header IEs and Payload IEs.
	uint8_t msdu_shift = params->HeaderIELength + params->PayloadIELength;
	if ((panid != CHILI_TEST_PANID) || (shortadd != CHILI_TEST_REF_SHORTADD) ||
	    (params->Data[msdu_shift] != PT_MSDU_TEST_REF))
#else
	if ((panid != CHILI_TEST_PANID) || (shortadd != CHILI_TEST_REF_SHORTADD) || (params->Msdu[0] != PT_MSDU_TEST_REF))
#endif // CASCODA_CA_VER >= 8212
	{
		/* wrong packet id */
		CHILI_TEST_RESULT = CHILI_TEST_ST_DATA_IND_ID;
		goto exit;
	}

/* packet received ok */
/* analyse cs and ed values of both ref and dut side */
#if CASCODA_CA_VER >= 8212
	cs_ref = params->Data[1 + msdu_shift];
	ed_ref = params->Data[2 + msdu_shift];
	cs_dut = params->MpduLinkQuality;
#else
	cs_ref = params->Msdu[1];
	ed_ref = params->Msdu[2];
	cs_dut = params->MpduLinkQuality;
#endif // CASCODA_CA_VER >= 8212
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
	WAIT_ms(100);
	CHILI_TEST_CState = CHILI_TEST_CST_DUT_DISPLAY;

} // End of CHILI_TEST_DUT_ProcessDataInd()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief DUT Process incoming Data Confirm
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_DUT_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
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
static void CHILI_TEST_DUT_CheckTimeout(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
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
static void CHILI_TEST_DUT_DisplayResult(struct ca821x_dev *pDeviceRef)
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

	CHILI_TEST_LEDComplete(CHILI_TEST_RESULT);

} // End of CHILI_TEST_DUT_DisplayResult()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reference Device Callback for MCPS_DATA_indication in CHILI_TEST_MODE
 *******************************************************************************
 ******************************************************************************/
static ca_error CHILI_TEST_REF_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                    struct ca821x_dev                *pDeviceRef)
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
                                                    struct ca821x_dev                *pDeviceRef)
{
	/* turn receiver off */
	u8_t rxidle = 0;
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxidle, pDeviceRef);

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
                                                       struct ca821x_dev                       *pDeviceRef)
{
	(void)params;
	(void)pDeviceRef;
	/* only supress message */
	return CA_ERROR_SUCCESS;
} // End of CHILI_TEST_MLME_COMM_STATUS_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dynamically Register Callbacks for CHILI_TEST_MODE
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_RegisterCallbacks(struct ca821x_dev *pDeviceRef)
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Test complete, avoid to jump into application code
 *******************************************************************************
 ******************************************************************************/
static void CHILI_TEST_LEDComplete(uint8_t status)
{
	/* this is hanging up intentionally - nothing to do after test */
	while (1)
	{
#if (CASCODA_CHILI2_CONFIG == 2)
		CHILI_TEST_LEDResult(status);
#else
		(void)status;
#endif
		/* allowed here, as nothing more to do but potentially blink */
		BSP_WaitTicks(200);
	}
}

// Cascoda Devboard specific
#if (CASCODA_CHILI2_CONFIG == 2)
static void CHILI_TEST_LEDInit(void)
{
	/* All LEDs on */
	DVBD_DeRegister(DEV_SWITCH_1, JUMPER_POS_1);
	DVBD_DeRegister(DEV_SWITCH_2, JUMPER_POS_1);
	DVBD_DeRegister(DEV_SWITCH_3, JUMPER_POS_1);
	DVBD_DeRegister(DEV_SWITCH_4, JUMPER_POS_1);
	DVBD_RegisterLEDOutput(DEV_SWITCH_1, JUMPER_POS_1);
	DVBD_RegisterLEDOutput(DEV_SWITCH_2, JUMPER_POS_1);
	DVBD_RegisterLEDOutput(DEV_SWITCH_3, JUMPER_POS_1);
	DVBD_RegisterLEDOutput(DEV_SWITCH_4, JUMPER_POS_1);
	DVBD_SetLED(DEV_SWITCH_1, LED_ON);
	DVBD_SetLED(DEV_SWITCH_2, LED_ON);
	DVBD_SetLED(DEV_SWITCH_3, LED_ON);
	DVBD_SetLED(DEV_SWITCH_4, LED_ON);
}

// devboard specific test result
static void CHILI_TEST_LEDResult(uint8_t status)
{
	u8_t ledstate;

	/* this is hanging up intentionally - nothing to do after test */
	if (status)
	{
		ledstate = LED_OFF;
	}
	else
	{
		DVBD_Sense(DEV_SWITCH_1, &ledstate);
		ledstate = 1 - ledstate;
	}
	DVBD_SetLED(DEV_SWITCH_1, ledstate);
	DVBD_SetLED(DEV_SWITCH_2, ledstate);
	DVBD_SetLED(DEV_SWITCH_3, ledstate);
	DVBD_SetLED(DEV_SWITCH_4, ledstate);
}
#endif
