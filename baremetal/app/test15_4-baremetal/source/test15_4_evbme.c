/**
 * @file test15_4_evbme.c
 * @brief test15_4 test integration functions
 * @author Wolfgang Bruchner
 * @date 19/07/14
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
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_debug.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#include "test15_4_evbme.h"
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

/******************************************************************************/
/***************************************************************************/ /**
* \brief Returns the current time
*******************************************************************************
* \return ms since start of execution
*******************************************************************************
******************************************************************************/
unsigned long test15_4_getms(void)
{
	return TIME_ReadAbsoluteTime();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEST15_4 Initialistion
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_Initialise(struct ca821x_dev *pDeviceRef)
{
	struct ca821x_api_callbacks callbacks = {0};
	callbacks.MLME_ASSOCIATE_indication   = &TEST15_4_AssociateIndication;
	callbacks.MLME_ORPHAN_indication      = &TEST15_4_OrphanIndication;
	ca821x_register_callbacks(&callbacks, pDeviceRef);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEST15_4 Event Handler in Main Polling Loop
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch Branch for EVBME Request (UpStream, Serial)
 *******************************************************************************
 ******************************************************************************/
int TEST15_4_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	if ((SERIAL_RX_CMD_ID >= EVBME_PHY_TESTMODE_REQUEST) && (SERIAL_RX_CMD_ID <= EVBME_FFD_AWAIT_ORPHAN_REQUEST))
	{
		switch (SERIAL_RX_CMD_ID)
		{
		case EVBME_PHY_TESTMODE_REQUEST:
			if (EVBME_PHY_TESTMODE_request(SERIAL_RX_DATA[0], pDeviceRef))
				printf("Invalid Test Mode\n");
			break;
		case EVBME_PHY_SET_REQUEST:
			if (EVBME_PHY_SET_request(SERIAL_RX_DATA[0], SERIAL_RX_DATA[1], SERIAL_RX_DATA + 2))
				printf("Invalid Parameter\n");
			break;
		case EVBME_PHY_REPORT_REQUEST:
			EVBME_PHY_REPORT_request();
			break;
		case EVBME_FFD_AWAIT_ASSOC_REQUEST:
			TEST15_4_SetupAwaitAssoc(SERIAL_RX_DATA, SERIAL_RX_DATA[8] + (SERIAL_RX_DATA[9] << 8), SERIAL_RX_DATA[10]);
			MAC_AWAITING_ASSOC_INDICATION = 1;
			break;
		case EVBME_FFD_AWAIT_ORPHAN_REQUEST:
			TEST15_4_SetupAwaitOrphan(SERIAL_RX_DATA, SERIAL_RX_DATA[8] + (SERIAL_RX_DATA[9] << 8));
			MAC_AWAITING_ORPHAN_INDICATION = 1;
			break;
		}
		return 1;
	}
	return 0;
} // End of TEST15_4_UpStreamDispatch()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_PHY_TESTMODE_request according to EVBME Spec
 *******************************************************************************
 * \param TestMode - Test Mode
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
u8_t EVBME_PHY_TESTMODE_request(u8_t TestMode, struct ca821x_dev *pDeviceRef)
{
	if (TestMode > PHY_TEST_MAX)
	{
		PHY_TESTMODE = PHY_TEST_OFF;
		return (EVBME_INVALID);
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

	return (EVBME_SUCCESS);
} // End of EVBME_PHY_TESTMODE_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_PHY_SET_request according to EVBME Spec
 *******************************************************************************
 * \param Parameter - Parameter Specifier
 * \param ParameterLength - Parameter Length
 * \param ParameterValue - Pointer to Parameter Value
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
u8_t EVBME_PHY_SET_request(u8_t Parameter, u8_t ParameterLength, u8_t *ParameterValue)
{
	u8_t status = EVBME_INVALID;

	switch (Parameter)
	{
	case PHY_TESTPAR_PACKETPERIOD:
		if (ParameterLength == 2)
		{
			PHY_TESTPAR.PACKETPERIOD = ((u16_t)(ParameterValue[1]) << 8) + (u16_t)(ParameterValue[0]);
			status                   = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_PACKETLENGTH:
		if (ParameterValue[0] < 128)
		{
			PHY_TESTPAR.PACKETLENGTH = ParameterValue[0];
			status                   = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_NUMBEROFPKTS:
		if (ParameterLength == 4)
		{
			PHY_TESTPAR.NUMBEROFPKTS = ((u32_t)(ParameterValue[3]) << 24) + ((u32_t)(ParameterValue[2]) << 16) +
			                           ((u32_t)(ParameterValue[1]) << 8) + (u32_t)(ParameterValue[0]);
			status = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_PACKETDATATYPE:
		if (ParameterValue[0] <= TDME_MAX_TXD)
		{
			PHY_TESTPAR.PACKETDATATYPE = ParameterValue[0];
			status                     = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_CHANNEL:
		if ((ParameterValue[0] >= 11) && (ParameterValue[0] <= 26))
		{
			PHY_TESTPAR.CHANNEL = ParameterValue[0];
			status              = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_TXPOWER:
		if ((ParameterLength == 3) && (ParameterValue[0] <= 63) && (ParameterValue[1] <= 7) && (ParameterValue[2] <= 1))
		{
			PHY_TESTPAR.TXPOWER_IB    = ParameterValue[0];
			PHY_TESTPAR.TXPOWER_PB    = ParameterValue[1];
			PHY_TESTPAR.TXPOWER_BOOST = ParameterValue[2];
			status                    = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_EDTHRESHOLD:
		if (ParameterLength == 1)
		{
			PHY_TESTPAR.EDTHRESHOLD = ParameterValue[0];
			status                  = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_RX_FFSYNC:
		if (ParameterValue[0] <= 1)
		{
			PHY_TESTPAR.RX_FFSYNC = ParameterValue[0];
			status                = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_LO_1_RXTXB:
		if (ParameterValue[0] <= 1)
		{
			PHY_TESTPAR.LO_1_RXTXB = ParameterValue[0];
			status                 = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_LO_2_FDAC:
		if (ParameterValue[0] <= 48)
		{
			PHY_TESTPAR.LO_2_FDAC = ParameterValue[0];
			status                = EVBME_SUCCESS;
		}
		break;

	case PHY_TESTPAR_LO_3_LOCKS:
		if (ParameterLength == 1)
		{
			PHY_TESTPAR.LO_3_LOCKS = ParameterValue[0];
			status                 = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_LO_3_PERIOD:
		if (ParameterLength == 1)
		{
			PHY_TESTPAR.LO_3_PERIOD = ParameterValue[0];
			status                  = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_ATM:
		if (ParameterValue[0] <= 31)
		{
			PHY_TESTPAR.ATM = ParameterValue[0];
			status          = EVBME_SUCCESS;
		}
		break;
	case PHY_TESTPAR_MPW2_OVWR:
		if (ParameterValue[0] <= 3)
		{
			PHY_TESTPAR.MPW2_OVWR = ParameterValue[0];
			status                = EVBME_SUCCESS;
		}
		break;
	default:
		status = EVBME_INVALID;
		break;
	}

	if (status == EVBME_SUCCESS)
		PHYTestReportTestParameters(Parameter);

	return (status);
} // End of EVBME_PHY_SET_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_PHY_REPORT_request according to EVBME Spec
 *******************************************************************************
 ******************************************************************************/
void EVBME_PHY_REPORT_request(void)
{
	PHYTestReportTestParameters(PHY_TESTPAR_ALL);
} // End of EVBME_PHY_REPORT_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set up Association Response when waiting for it
 *******************************************************************************
 * \param pDeviceAddress - IEEE address of device expected
 * \param AssocShortAddress - Short address for end device
 *******************************************************************************
 * \return -
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_SetupAwaitAssoc(uint8_t *pDeviceAddress, uint16_t AssocShortAddress, uint8_t Status)
{
	memcpy(EVBME_AssocDeviceAddress, pDeviceAddress, 8);
	EVBME_StoredAssocShortAddress = AssocShortAddress;
	EVBME_StoredAssocStatus       = Status;
} // End of EVBME_SetupAwaitAssoc()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set up Orphan Response when waiting for it
 *******************************************************************************
 * \param pDeviceAddress - IEEE address of device expected
 * \param OrphanShortAddress - Short address of orphan
 *******************************************************************************
 * \return -
 *******************************************************************************
 ******************************************************************************/
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
static int TEST15_4_AssociateIndication(struct MLME_ASSOCIATE_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (MAC_AWAITING_ASSOC_INDICATION)
	{
		if (memcmp(params->DeviceAddress, EVBME_AssocDeviceAddress, 8))
			return 0;
		MLME_ASSOCIATE_response(params->DeviceAddress, EVBME_StoredAssocShortAddress, EVBME_StoredAssocStatus, 0,
		                        pDeviceRef);
		MAC_AWAITING_ASSOC_INDICATION = 0;
	}
	return 0;
} // End of TEST15_4_AssociateIndication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_ORPHAN_indication
 *******************************************************************************
 ******************************************************************************/
static int TEST15_4_OrphanIndication(struct MLME_ORPHAN_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (MAC_AWAITING_ORPHAN_INDICATION)
	{
		if (memcmp(params->OrphanAddr, EVBME_OrphanDeviceAddress, 8))
			return 0;
		MLME_ORPHAN_response(params->OrphanAddr, EVBME_StoredOrphanShortAddress, 1, &params->Security, pDeviceRef);
		MAC_AWAITING_ORPHAN_INDICATION = 0;
	}
	return 0;
} // End of TEST15_4_OrphanIndication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for TDME_RXPKT_indication
 *******************************************************************************
 ******************************************************************************/
static int TEST15_4_PHY_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (!PHY_TESTPAR.MACENABLED)
	{
		if ((PHY_TESTMODE == PHY_TEST_RX_PER) || (PHY_TESTMODE == PHY_TEST_RX_PSN))
		{
			PHY_RXPKT_indication(params, pDeviceRef);
			return 1;
		}
	}
	return 0;
} // End of TEST15_4_PHY_RXPKT_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MCPS_DATA_indication
 *******************************************************************************
 ******************************************************************************/
static int TEST15_4_MAC_RXPKT_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (PHY_TESTPAR.MACENABLED)
	{
		if ((PHY_TESTMODE == PHY_TEST_RX_PER) || (PHY_TESTMODE == PHY_TEST_RX_PSN))
		{
			PHY_RXPKT_MAC_indication(params, pDeviceRef);
			return 1;
		}
	}
	return 0;
} // End of TEST15_4_MAC_RXPKT_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for TDME_EDDET_indication
 *******************************************************************************
 ******************************************************************************/
static int TEST15_4_PHY_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (PHY_TESTMODE == PHY_TEST_RX_EDSN)
	{
		PHY_EDDET_indication(params, pDeviceRef);
		return 1;
	}
	return 0;
} // End of TEST15_4_PHY_EDDET_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dynamically Register Callbacks for TEST15_4
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_RegisterCallbacks(struct ca821x_dev *pDeviceRef)
{
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
