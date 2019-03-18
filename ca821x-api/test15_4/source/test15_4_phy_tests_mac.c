/**
 * @file test15_4_phy_tests_mac.c
 * @brief PHY Test Functions using MAC Functions for Data Reliablity
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

#include "ca821x_api.h"
#include "test15_4_phy_tests.h"

/******************************************************************************/
/****** Address/PID Definitions                                          ******/
/******************************************************************************/
#define PHY_PANID 0xCA5C
#define PHY_TX_SHORTADD 0xCA51
#define PHY_RX_SHORTADD 0xCA52
#define PHY_TX_LONGADD                                 \
	{                                                  \
		0x01, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA \
	}
#define PHY_RX_LONGADD                                 \
	{                                                  \
		0x02, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA \
	}

/******************************************************************************/
/****** Global Variables only used in Phy Test Files                     ******/
/******************************************************************************/
uint16_t PHYPANId;
uint16_t PHYTxShortAddress;
uint16_t PHYRxShortAddress;
uint8_t  PHYTxLongAddress[8];
uint8_t  PHYRxLongAddress[8];

uint8_t DSN_OLD;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Address Initialisation
 *******************************************************************************
 ******************************************************************************/
void PHYTestMACAddInit(void)
{
	PHYPANId          = PHY_PANID;
	PHYTxShortAddress = PHY_TX_SHORTADD;
	PHYRxShortAddress = PHY_RX_SHORTADD;
	memcpy(PHYTxLongAddress, (uint8_t[])PHY_TX_LONGADD, 8);
	memcpy(PHYRxLongAddress, (uint8_t[])PHY_RX_LONGADD, 8);
} // End of PHYTestMACAddInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation for using MAC layer in EVBME (Tx)
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACTxInitialise(struct ca821x_dev *pDeviceRef)
{
	uint8_t status;
	uint8_t param;

	PHYTestMACAddInit();

	if ((status = TDME_TESTMODE_request_sync(TDME_TEST_OFF,
	                                         pDeviceRef))) // turn off test mode in order to be able to use MAC
		return status;

	if ((status = TDME_SETSFR_request_sync(0, 0xB2, 0x00, pDeviceRef))) // turn off AUTED
		return status;

	if ((status = MLME_RESET_request_sync(0, pDeviceRef)))
		return status;

	param = CCAM_EDORCS;
	if ((status = HWME_SET_request_sync(HWME_CCAMODE, 1, &param, pDeviceRef))) // set CCA mode to ED OR CS
		return status;

	param = PHY_TESTPAR.EDTHRESHOLD;
	if ((status = HWME_SET_request_sync(HWME_EDTHRESHOLD,
	                                    1,
	                                    &param,
	                                    pDeviceRef))) // set ED threshold to PHY_TESTPAR.EDTHRESHOLD
		return status;

	if ((status = MLME_SET_request_sync(macPANId, 0, 2, &PHYPANId, pDeviceRef))) // set local PANId
		return status;

	if ((status = MLME_SET_request_sync(nsIEEEAddress, 0, 8, PHYTxLongAddress, pDeviceRef))) // set local long address
		return status;

	if ((status =
	         MLME_SET_request_sync(macShortAddress, 0, 2, &PHYTxShortAddress, pDeviceRef))) // set local short address
		return status;

	param = 0;
	if ((status = MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &param, pDeviceRef))) /* turn receiver off */
		return status;

	return status;
} // End of PHYTestMACTxInitialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation for using MAC layer in EVBME (Rx)
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACRxInitialise(struct ca821x_dev *pDeviceRef)
{
	uint8_t status;
	uint8_t param;

	PHYTestMACAddInit();

	if ((status = TDME_TESTMODE_request_sync(TDME_TEST_OFF,
	                                         pDeviceRef))) // turn off test mode in order to be able to use MAC
		return status;

	if ((status = TDME_SETSFR_request_sync(0, 0xB2, 0x08, pDeviceRef))) // turn on AUTED
		return status;

	if ((status = MLME_RESET_request_sync(0, pDeviceRef)))
		return status;

	if ((status = MLME_SET_request_sync(macPANId, 0, 2, &PHYPANId, pDeviceRef))) // set local PANId
		return status;

	if ((status = MLME_SET_request_sync(nsIEEEAddress, 0, 8, PHYRxLongAddress, pDeviceRef))) // set local long address
		return status;

	if ((status =
	         MLME_SET_request_sync(macShortAddress, 0, 2, &PHYRxShortAddress, pDeviceRef))) // set local short address
		return status;

	param = 1;
	if ((status = MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &param, pDeviceRef))) // turn receiver on
		return status;

	DSN_OLD = 0;

	return status;
} // End of PHYTestMACRxInitialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Denitialisation for using MAC layer in EVBME
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACDeinitialise(struct ca821x_dev *pDeviceRef)
{
	uint8_t status;
	status = MLME_RESET_request_sync(1, pDeviceRef);
	return status;
} // End of PHYTestMACDeinitialise()

/******************************************************************************/
/****** PHY_TXPKT_MAC_request()                                          ******/
/******************************************************************************/
/****** Brief:  PHY Test Wrapper for MCPS_DATA_request()                 ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: Status                                                   ******/
/******************************************************************************/
/******************************************************************************/
uint8_t PHY_TXPKT_MAC_request(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	uint8_t            i;
	uint8_t            status;
	uint8_t            randnum[2];
	uint8_t            attlen;
	struct FullAddr    DstFAdd;
	struct MAC_Message rx_msg;

	msg->PData.TDMETxPktReq.TestPacketSequenceNumber = LS_BYTE(PHY_TESTRES.SEQUENCENUMBER);
	msg->PData.TDMETxPktReq.TestPacketDataType       = PHY_TESTPAR.PACKETDATATYPE;
	msg->PData.TDMETxPktReq.TestPacketLength         = PHY_TESTPAR.PACKETLENGTH;

	if (PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_APPENDED)
	{
		for (i = 0; i < PHY_TESTPAR.PACKETLENGTH; ++i)
			msg->PData.TDMETxPktReq.TestPacketData[i] = 0x00; /* currently filled with 0's */
	}
	else if (PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_RANDOM)
	{
		for (i = 0; i < PHY_TESTPAR.PACKETLENGTH; ++i)
		{
			if ((status = HWME_GET_request_sync(HWME_RANDOMNUM, &attlen, randnum, pDeviceRef)))
				return status;
			else
				msg->PData.TDMETxPktReq.TestPacketData[i] = randnum[0];
		}
	}
	else if (PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_SEQRANDOM)
	{
		msg->PData.TDMETxPktReq.TestPacketData[0] = PHY_TESTRES.SEQUENCENUMBER;
		for (i = 1; i < PHY_TESTPAR.PACKETLENGTH; ++i)
		{
			if ((status = HWME_GET_request_sync(HWME_RANDOMNUM, &attlen, randnum, pDeviceRef)))
				return (status);
			else
				msg->PData.TDMETxPktReq.TestPacketData[i] = randnum[0];
		}
	}
	else /* PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_COUNT) */
	{
		for (i = 0; i < PHY_TESTPAR.PACKETLENGTH; ++i) msg->PData.TDMETxPktReq.TestPacketData[i] = i + 1;
	}

	DstFAdd.AddressMode = MAC_MODE_SHORT_ADDR;
	PUTLE16(PHYPANId, DstFAdd.PANId);
	PUTLE16(PHYRxShortAddress, DstFAdd.Address);

	/* convert TDME to MLME */
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR,                              /* SrcAddrMode */
	                  DstFAdd,                                          /* DstAddr */
	                  msg->PData.TDMETxPktReq.TestPacketLength,         /* MsduLength */
	                  msg->PData.TDMETxPktReq.TestPacketData,           /* *pMsdu */
	                  msg->PData.TDMETxPktReq.TestPacketSequenceNumber, /* MsduHandle */
	                  TXOPT_ACKREQ,                                     /* TxOptions */
	                  NULL,                                             /* *pSecurity */
	                  pDeviceRef);

	if ((status = ca821x_wait_for_message(SPI_MCPS_DATA_CONFIRM, 5000, &rx_msg.CommandId, pDeviceRef)))
		return (status);

	status = rx_msg.PData.DataCnf.Status;

	if (status == MAC_NO_ACK)
	{
		++PHY_TESTRES.SHRERR_COUNT; // SHRERR_COUNT used for counting NO_ACK
		status = MAC_SUCCESS;
	}
	else if (status == MAC_CHANNEL_ACCESS_FAILURE)
	{
		++PHY_TESTRES.PHRERR_COUNT; // PHRERR_COUNT used for counting CHANNEL_ACCESS_FAILURE
		status = MAC_SUCCESS;
	}

	++PHY_TESTRES.SEQUENCENUMBER;

	return (status);
} // End of PHY_TXPKT_MAC_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for MCPS_DATA_indication()
 *******************************************************************************
 * \param data_ind - MCPS data indication buffer
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_RXPKT_MAC_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	uint8_t                           status = 0;
	uint8_t                           DSN;
	uint8_t                           edvallp, freqoffs;
	uint8_t                           len;
	struct TDME_RXPKT_indication_pset tdmeind;

	DSN = params->DSN;

	/* check if same sequence number - discard if this is the case */
	if (DSN == DSN_OLD)
		status = MAC_INVALID_HANDLE;
	else
		PHY_TESTRES.PACKET_RECEIVED = 1; /* Flag indication */

	DSN_OLD = DSN;

	if (!status)
		status = HWME_GET_request_sync(HWME_EDVALLP, &len, &edvallp, pDeviceRef);

	if (!status)
		status = HWME_GET_request_sync(HWME_FREQOFFS, &len, &freqoffs, pDeviceRef);

	/* convert MLME to TDME */
	tdmeind.Status               = status;
	tdmeind.TestPacketEDValue    = edvallp;
	tdmeind.TestPacketCSValue    = params->MpduLinkQuality;
	tdmeind.TestPacketFoffsValue = freqoffs;
	tdmeind.TestPacketLength     = params->MsduLength;
	memcpy(tdmeind.TestPacketData, params->Msdu, params->MsduLength);

	PHYTestStatistics(
	    TEST_STAT_ACCUM, tdmeind.TestPacketEDValue, tdmeind.TestPacketCSValue, tdmeind.TestPacketFoffsValue);

	if ((PHY_TESTPAR.PACKETPERIOD >= 500) || (PHY_TESTMODE == PHY_TEST_RX_PSN))
		PHYTestReportPacketReceived(&tdmeind);

	return status;
} // End of PHY_RXPKT_MAC_indication()
