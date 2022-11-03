/**
 * @file
 * @brief test15_4 test integration functions
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
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "cascoda-util/cascoda_time.h"
#include "mac_messages.h"
#include "test15_4_phy_tests.h"

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
u8_t MAC_AWAITING_ASSOC_INDICATION  = 0;
u8_t MAC_AWAITING_ORPHAN_INDICATION = 0;

uint8_t  EVBME_AssocDeviceAddress[8]  = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t  EVBME_OrphanDeviceAddress[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint16_t EVBME_StoredAssocShortAddress;
uint8_t  EVBME_StoredAssocStatus;
uint16_t EVBME_StoredOrphanShortAddress;

unsigned long test15_4_getms(void)
{
	return TIME_ReadAbsoluteTime();
}

void TEST15_4_Initialise(struct ca821x_dev *pDeviceRef)
{
	pDeviceRef->callbacks.MLME_ASSOCIATE_indication = &TEST15_4_AssociateIndication;
	pDeviceRef->callbacks.MLME_ORPHAN_indication    = &TEST15_4_OrphanIndication;
}

void TEST15_4_Handler(struct ca821x_dev *pDeviceRef)
{
	static u8_t oldval = 0;

	if (PHY_TESTMODE)
		PHYTestModeHandler(pDeviceRef);

	/* EVBME_SetVal[0] for PHYTestReset() */
	if (EVBME_HasReset > 0)
	{
		PHYTestReset(); // reset test modes
		EVBME_HasReset = 0;
	}

	/* EVBME_SetVal[1] for PHY_TEST_USE_MAC */
	if (EVBME_UseMAC != oldval)
	{
		PHYTestCfg(EVBME_UseMAC);
		oldval = EVBME_UseMAC;
	}

} // End of TEST15_4_Handler()

int TEST15_4_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	if ((SerialRxBuffer->CmdId >= EVBME_PHY_TESTMODE_REQUEST) &&
	    (SerialRxBuffer->CmdId <= EVBME_FFD_AWAIT_ORPHAN_REQUEST))
	{
		switch (SerialRxBuffer->CmdId)
		{
		case EVBME_PHY_TESTMODE_REQUEST:
			if (EVBME_PHY_TESTMODE_request(SerialRxBuffer->Data[0], pDeviceRef))
				printf("Invalid Test Mode\n");
			break;
		case EVBME_PHY_SET_REQUEST:
			if (EVBME_PHY_SET_request(SerialRxBuffer->Data[0], SerialRxBuffer->Data[1], SerialRxBuffer->Data + 2))
				printf("Invalid Parameter\n");
			break;
		case EVBME_PHY_REPORT_REQUEST:
			EVBME_PHY_REPORT_request();
			break;
		case EVBME_FFD_AWAIT_ASSOC_REQUEST:
			TEST15_4_SetupAwaitAssoc(SerialRxBuffer->Data,
			                         SerialRxBuffer->Data[8] + (SerialRxBuffer->Data[9] << 8),
			                         SerialRxBuffer->Data[10]);
			MAC_AWAITING_ASSOC_INDICATION = 1;
			break;
		case EVBME_FFD_AWAIT_ORPHAN_REQUEST:
			TEST15_4_SetupAwaitOrphan(SerialRxBuffer->Data, SerialRxBuffer->Data[8] + (SerialRxBuffer->Data[9] << 8));
			MAC_AWAITING_ORPHAN_INDICATION = 1;
			break;
		}
		return 1;
	}
	return 0;
} // End of TEST15_4_UpStreamDispatch()

u8_t EVBME_PHY_TESTMODE_request(u8_t TestMode, struct ca821x_dev *pDeviceRef)
{
	if (TestMode > PHY_TEST_MAX)
	{
		PHY_TESTMODE = PHY_TEST_OFF;
		return (CA_ERROR_INVALID);
	}

	if (TestMode)
	{
		PHY_TESTMODE = TestMode;
		if ((PHYTestInitialise(pDeviceRef)))
			PHYTestExit("Error Initialising PHYTest");
	}
	else
	{
		PHYTestDeinitialise(pDeviceRef);
	}

	TEST15_4_RegisterCallbacks(pDeviceRef);

	return (CA_ERROR_SUCCESS);
} // End of EVBME_PHY_TESTMODE_request()

u8_t EVBME_PHY_SET_request(u8_t Parameter, u8_t ParameterLength, u8_t *ParameterValue)
{
	u8_t status = CA_ERROR_INVALID;

	switch (Parameter)
	{
	case PHY_TESTPAR_PACKETPERIOD:
		if (ParameterLength == 2)
		{
			PHY_TESTPAR.PACKETPERIOD = ((u16_t)(ParameterValue[1]) << 8) + (u16_t)(ParameterValue[0]);
			status                   = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_PACKETLENGTH:
		if (ParameterValue[0] < 128)
		{
			PHY_TESTPAR.PACKETLENGTH = ParameterValue[0];
			status                   = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_NUMBEROFPKTS:
		if (ParameterLength == 4)
		{
			PHY_TESTPAR.NUMBEROFPKTS = ((u32_t)(ParameterValue[3]) << 24) + ((u32_t)(ParameterValue[2]) << 16) +
			                           ((u32_t)(ParameterValue[1]) << 8) + (u32_t)(ParameterValue[0]);
			status = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_PACKETDATATYPE:
		if (ParameterValue[0] <= TDME_TXD_APPENDED)
		{
			PHY_TESTPAR.PACKETDATATYPE = ParameterValue[0];
			status                     = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_CHANNEL:
		if ((ParameterValue[0] >= 11) && (ParameterValue[0] <= 26))
		{
			PHY_TESTPAR.CHANNEL = ParameterValue[0];
			status              = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_TXPOWER:
		if ((ParameterLength == 3) && (ParameterValue[0] <= 63) && (ParameterValue[1] <= 7) && (ParameterValue[2] <= 1))
		{
			PHY_TESTPAR.TXPOWER_IB    = ParameterValue[0];
			PHY_TESTPAR.TXPOWER_PB    = ParameterValue[1];
			PHY_TESTPAR.TXPOWER_BOOST = ParameterValue[2];
			status                    = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_EDTHRESHOLD:
		if (ParameterLength == 1)
		{
			PHY_TESTPAR.EDTHRESHOLD = ParameterValue[0];
			status                  = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_RX_FFSYNC:
		if (ParameterValue[0] <= 1)
		{
			PHY_TESTPAR.RX_FFSYNC = ParameterValue[0];
			status                = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_LO_1_RXTXB:
		if (ParameterValue[0] <= 1)
		{
			PHY_TESTPAR.LO_1_RXTXB = ParameterValue[0];
			status                 = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_LO_2_FDAC:
		if (ParameterValue[0] <= 48)
		{
			PHY_TESTPAR.LO_2_FDAC = ParameterValue[0];
			status                = CA_ERROR_SUCCESS;
		}
		break;

	case PHY_TESTPAR_LO_3_LOCKS:
		if (ParameterLength == 1)
		{
			PHY_TESTPAR.LO_3_LOCKS = ParameterValue[0];
			status                 = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_LO_3_PERIOD:
		if (ParameterLength == 1)
		{
			PHY_TESTPAR.LO_3_PERIOD = ParameterValue[0];
			status                  = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_ATM:
		if (ParameterValue[0] <= 31)
		{
			PHY_TESTPAR.ATM = ParameterValue[0];
			status          = CA_ERROR_SUCCESS;
		}
		break;
	case PHY_TESTPAR_MPW2_OVWR:
		if (ParameterValue[0] <= 3)
		{
			PHY_TESTPAR.MPW2_OVWR = ParameterValue[0];
			status                = CA_ERROR_SUCCESS;
		}
		break;
	default:
		status = CA_ERROR_INVALID;
		break;
	}

	if (status == CA_ERROR_SUCCESS)
		PHYTestReportTestParameters(Parameter);

	return (status);
} // End of EVBME_PHY_SET_request()

void EVBME_PHY_REPORT_request(void)
{
	PHYTestReportTestParameters(PHY_TESTPAR_ALL);
} // End of EVBME_PHY_REPORT_request()

void TEST15_4_SetupAwaitAssoc(uint8_t *pDeviceAddress, uint16_t AssocShortAddress, uint8_t Status)
{
	memcpy(EVBME_AssocDeviceAddress, pDeviceAddress, 8);
	EVBME_StoredAssocShortAddress = AssocShortAddress;
	EVBME_StoredAssocStatus       = Status;
} // End of EVBME_SetupAwaitAssoc()

void TEST15_4_SetupAwaitOrphan(uint8_t *pDeviceAddress, uint16_t OrphanShortAddress)
{
	memcpy(EVBME_OrphanDeviceAddress, pDeviceAddress, 8);
	EVBME_StoredOrphanShortAddress = OrphanShortAddress;
} // End of EVBME_SetupAwaitOrphan()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_ASSOCIATE_indication
 *******************************************************************************
 ******************************************************************************/
ca_error TEST15_4_AssociateIndication(struct MLME_ASSOCIATE_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (MAC_AWAITING_ASSOC_INDICATION)
	{
		if (memcmp(params->DeviceAddress, EVBME_AssocDeviceAddress, 8) != 0)
			return CA_ERROR_NOT_HANDLED;
		MLME_ASSOCIATE_response(
		    params->DeviceAddress, EVBME_StoredAssocShortAddress, EVBME_StoredAssocStatus, 0, pDeviceRef);
		MAC_AWAITING_ASSOC_INDICATION = 0;
	}
	return CA_ERROR_NOT_HANDLED;
} // End of TEST15_4_AssociateIndication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_ORPHAN_indication
 *******************************************************************************
 ******************************************************************************/
ca_error TEST15_4_OrphanIndication(struct MLME_ORPHAN_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (MAC_AWAITING_ORPHAN_INDICATION)
	{
		if (memcmp(params->OrphanAddr, EVBME_OrphanDeviceAddress, 8) != 0)
			return CA_ERROR_NOT_HANDLED;
		MLME_ORPHAN_response(params->OrphanAddr, EVBME_StoredOrphanShortAddress, 1, &params->Security, pDeviceRef);
		MAC_AWAITING_ORPHAN_INDICATION = 0;
	}
	return CA_ERROR_NOT_HANDLED;
} // End of TEST15_4_OrphanIndication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for TDME_RXPKT_indication
 *******************************************************************************
 ******************************************************************************/
ca_error TEST15_4_PHY_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (!PHY_TESTPAR.MACENABLED)
	{
		if ((PHY_TESTMODE == PHY_TEST_RX_PER) || (PHY_TESTMODE == PHY_TEST_RX_PSN))
		{
			PHY_RXPKT_indication(params, pDeviceRef);
			return CA_ERROR_SUCCESS;
		}
	}
	return CA_ERROR_NOT_HANDLED;
} // End of TEST15_4_PHY_RXPKT_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MCPS_DATA_indication
 *******************************************************************************
 ******************************************************************************/
ca_error TEST15_4_MAC_RXPKT_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (PHY_TESTPAR.MACENABLED)
	{
		if ((PHY_TESTMODE == PHY_TEST_RX_PER) || (PHY_TESTMODE == PHY_TEST_RX_PSN))
		{
			PHY_RXPKT_MAC_indication(params, pDeviceRef);
			return CA_ERROR_SUCCESS;
		}
	}
	return CA_ERROR_NOT_HANDLED;
} // End of TEST15_4_MAC_RXPKT_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MCPS_DATA_confirm
 *******************************************************************************
 ******************************************************************************/
ca_error TEST15_4_MAC_TXPKT_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (PHY_TESTPAR.MACENABLED)
	{
		if (PHY_TESTMODE == PHY_TEST_TX_PKT)
		{
			PHY_TXPKT_MAC_confirm(params, pDeviceRef);
			return CA_ERROR_SUCCESS;
		}
	}
	return CA_ERROR_NOT_HANDLED;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for TDME_EDDET_indication
 *******************************************************************************
 ******************************************************************************/
ca_error TEST15_4_PHY_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (PHY_TESTMODE == PHY_TEST_RX_EDSN)
	{
		PHY_EDDET_indication(params, pDeviceRef);
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_NOT_HANDLED;
} // End of TEST15_4_PHY_EDDET_indication()

void TEST15_4_RegisterCallbacks(struct ca821x_dev *pDeviceRef)
{
	if ((PHY_TESTMODE == PHY_TEST_TX_PKT) && (PHY_TESTPAR.MACENABLED))
		pDeviceRef->callbacks.MCPS_DATA_confirm = &TEST15_4_MAC_TXPKT_confirm;
	else
		pDeviceRef->callbacks.MCPS_DATA_confirm = NULL;

	if (((PHY_TESTMODE == PHY_TEST_RX_PSN) || (PHY_TESTMODE == PHY_TEST_RX_PER)) && (PHY_TESTPAR.MACENABLED))
		pDeviceRef->callbacks.MCPS_DATA_indication = &TEST15_4_MAC_RXPKT_indication;
	else
		pDeviceRef->callbacks.MCPS_DATA_indication = NULL;

	if (((PHY_TESTMODE == PHY_TEST_RX_PSN) || (PHY_TESTMODE == PHY_TEST_RX_PER)) && (!PHY_TESTPAR.MACENABLED))
		pDeviceRef->callbacks.TDME_RXPKT_indication = &TEST15_4_PHY_RXPKT_indication;
	else
		pDeviceRef->callbacks.TDME_RXPKT_indication = NULL;

	if (PHY_TESTMODE == PHY_TEST_RX_EDSN)
		pDeviceRef->callbacks.TDME_EDDET_indication = &TEST15_4_PHY_EDDET_indication;
	else
		pDeviceRef->callbacks.TDME_EDDET_indication = NULL;

} // End of TEST15_4_RegisterCallbacks()

int TEST15_4_SerialDispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	(void)len;
	int ret = 0;
	if ((ret = TEST15_4_UpStreamDispatch((struct SerialBuffer *)(buf), pDeviceRef)))
		return ret;
	/* Insert Application-Specific Dispatches here in the same style */
	return 0;
} // End of TEST15_4_SerialDispatch()
