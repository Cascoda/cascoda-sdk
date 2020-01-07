/**
 * @file
 * @brief PHY Tests Definitions and Function Declarations
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

#ifndef TEST15_4_PHY_TESTS_H
#define TEST15_4_PHY_TESTS_H

#include <stddef.h>
#include <stdint.h>

/******************************************************************************/
/****** PHY Testmode Definitions                                         ******/
/******************************************************************************/
#define PHY_TEST_OFF 0x00     //!< Testmode Off
#define PHY_TEST_TX_PKT 0x01  //!< Transmit Periodic Packets
#define PHY_TEST_TX_CONT 0x02 //!< Transmit Continuous
#define PHY_TEST_RX_PER 0x03  //!< Receive PER Test
#define PHY_TEST_RX_PSN 0x04  //!< Receive Packet Sniffer
#define PHY_TEST_RX_EDSN 0x05 //!< Receive Energy Detect Sniffer
#define PHY_TEST_LO_1 0x06    //!< LO Test 1 (Closed Loop, Tx/Rx)
#define PHY_TEST_LO_2 0x07    //!< LO Test 2 (VCO Open Loop)
#define PHY_TEST_LO_3 0x08    //!< LO Test 3 (Locking Test)
#define PHY_TEST_MAX PHY_TEST_LO_3

/******************************************************************************/
/****** PHY Testmode Parameter Enumerations                              ******/
/******************************************************************************/
#define PHY_TESTPAR_PACKETPERIOD 0x00   //!< Packet Period [ms]
#define PHY_TESTPAR_PACKETLENGTH 0x01   //!< Packet Length [bytes]
#define PHY_TESTPAR_NUMBEROFPKTS 0x02   //!< Number of Packets for Test
#define PHY_TESTPAR_PACKETDATATYPE 0x03 //!< Packet Data Type (Payload)
#define PHY_TESTPAR_CHANNEL 0x04        //!< TDME-PIB 802.15.4 Channel
#define PHY_TESTPAR_TXPOWER 0x05        //!< TDME-PIB Transmit Power
#define PHY_TESTPAR_EDTHRESHOLD 0x06    //!< TDME-PIB ED Threshold
#define PHY_TESTPAR_RX_FFSYNC 0x07      //!< TDME-PIB Rx Flag False Sync
#define PHY_TESTPAR_LO_1_RXTXB 0x08     //!< TDME-PIB LO Test 1 Tx/Rx
#define PHY_TESTPAR_LO_2_FDAC 0x09      //!< TDME-PIB LO Test 2 FDAC Value
#define PHY_TESTPAR_LO_3_LOCKS 0x0A     //!< LO Test 3 Number of Locks per Tx/Rx Channel
#define PHY_TESTPAR_LO_3_PERIOD 0x0B    //!< LO Test 3 Locking Test Period [ms]
#define PHY_TESTPAR_ATM 0x0C            //!< TDME-PIB Analog Test Bus Configuration
#define PHY_TESTPAR_MPW2_OVWR 0x0D      //!< TDME-PIB MPW2 Overwrite
#define PHY_TESTPAR_MACENABLED 0x0E     //!< MAC Enabled for Test
#define PHY_TESTPAR_MAX PHY_TESTPAR_MACENABLED
#define PHY_TESTPAR_ALL (PHY_TESTPAR_MAX + 1) // for Reporting

#define PHY_TESTPARDEF_PACKETPERIOD 1000
#define PHY_TESTPARDEF_PACKETLENGTH 20
#define PHY_TESTPARDEF_NUMBEROFPKTS 10000
#define PHY_TESTPARDEF_PACKETDATATYPE TDME_TXD_RANDOM
#define PHY_TESTPARDEF_CHANNEL 0x12
#define PHY_TESTPARDEF_TXPOWER_IB 1
#define PHY_TESTPARDEF_TXPOWER_PB 3
#define PHY_TESTPARDEF_TXPOWER_BOOST 0
#define PHY_TESTPARDEF_TXCONT 0
#define PHY_TESTPARDEF_EDTHRESHOLD 0x80
#define PHY_TESTPARDEF_RX_FFSYNC 0
#define PHY_TESTPARDEF_LO_1_RXTXB 0
#define PHY_TESTPARDEF_LO_2_FDAC 32
#define PHY_TESTPARDEF_LO_3_LOCKS 20
#define PHY_TESTPARDEF_LO_3_PERIOD 10
#define PHY_TESTPARDEF_ATM 0x00
#define PHY_TESTPARDEF_MPW2_OVWR 0x03
#define PHY_TESTPARDEF_MACENABLED 0

/* PHY Testmode Setup Parameter Structure */
struct PHYTestPar
{
	uint16_t PACKETPERIOD;   //!< Packet Period [ms]
	uint8_t  PACKETLENGTH;   //!< Packet Length [bytes]
	uint32_t NUMBEROFPKTS;   //!< Number of Packets for Test
	uint8_t  PACKETDATATYPE; //!< Packet Data Type (Payload)
	uint8_t  CHANNEL;        //!< TDME-PIB 802.15.4 Channel
	uint8_t  TXPOWER_IB;     //!< TDME-PIB Transmit Power Amp Current
	uint8_t  TXPOWER_PB;     //!< TDME-PIB Transmit Power Amp Frequency Trim
	uint8_t  TXPOWER_BOOST;  //!< TDME-PIB Transmit Boost Mode
	uint8_t  TXCONT;
	uint8_t  EDTHRESHOLD; //!< TDME-PIB ED Threshold
	uint8_t  RX_FFSYNC;   //!< TDME-PIB Rx Flag False Sync
	uint8_t  LO_1_RXTXB;  //!< TDME-PIB LO Test 1 Tx/Rx
	uint8_t  LO_2_FDAC;   //!< TDME-PIB LO Test 2 FDAC Value
	uint8_t  LO_3_LOCKS;  //!< LO Test 3 Number of Locks per Tx/Rx Channel
	uint8_t  LO_3_PERIOD; //!< LO Test 3 Locking Test Period [ms]
	uint8_t  ATM;         //!< TDME-PIB Analog Test Bus Configuration
	uint8_t  MPW2_OVWR;   //!< TDME-PIB MPW2 Overwrite
	uint8_t  MACENABLED;  //!< MAC Enabled for Test
};

/* PHY Testmode Results/Runtime Parameter Structure */
struct PHYTestRes
{
	uint16_t SEQUENCENUMBER;  //!< Packet Sequence Number (SN)
	uint32_t PACKET_COUNT;    //!< Packet Count
	uint8_t  PACKET_RECEIVED; //!< Packet Received Flag
	uint8_t  TEST_RUNNING;    //!< Test Running Flag
	uint32_t LOERR_COUNT;     //!< Error Counter for LO Locking Failures
	uint32_t CRCERR_COUNT;    //!< Error Counter for CRC/FCS Errors
	uint32_t PHRERR_COUNT;    //!< Error Counter for PHR (Packet Header) Errors
	uint32_t SHRERR_COUNT;    //!< Error Counter for SHR (SFD) Errors
	uint32_t PREERR_COUNT;    //!< Error Counter for Preamble Errors
	uint32_t MISSED_COUNT;    //!< Error Counter for Missed Packets
	int32_t  FO_AVG;          //!< Averaged Frequency Offset (between Analysis Reports)
	uint32_t ED_AVG;          //!< Averaged ED Value         (between Analysis Reports)
	uint32_t CS_AVG;          //!< Averaged CS Value         (between Analysis Reports)
	int32_t  FO_AVG_TOTAL;    //!< Total Averaged Frequency Offset
	uint32_t ED_AVG_TOTAL;    //!< Total Averaged ED Value
	uint32_t CS_AVG_TOTAL;    //!< Total Averaged CS Value
	uint8_t  ED_MAX;          //!< Maximum ED Value
	uint8_t  ED_MIN;          //!< Minimum ED Value
	uint8_t  CS_MAX;          //!< Maximum CS Value
	uint8_t  CS_MIN;          //!< Minimum CS Value
};

/******************************************************************************/
/****** Locally used Definitions for PHY Tests                           ******/
/******************************************************************************/
#define PHY_TEST_REPORT_PERIOD 5000 //!< Report period in [ms]

/* Modes for PHYTestStatistics() */
#define TEST_STAT_ACCUM 0  /* accumulate */
#define TEST_STAT_INIT 1   /* initialise */
#define TEST_STAT_REPORT 2 /* averages for interim reporting */
#define TEST_STAT_FINAL 3  /* averages for final reporting */

/******************************************************************************/
/****** Global Variables defined in test15_4_phy_tests.c                  ******/
/******************************************************************************/
extern uint8_t           PHY_TESTMODE;
extern struct PHYTestPar PHY_TESTPAR;
extern struct PHYTestRes PHY_TESTRES;

/******************************************************************************/
/****** Function Declarations for Externally Defined Functions           ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
* \brief Returns the current time
*******************************************************************************
* \return ms since start of execution
*******************************************************************************
******************************************************************************/
unsigned long test15_4_getms(void);

/******************************************************************************/
/****** Function Declarations for test15_4_phy_tests.c                   ******/
/******************************************************************************/
/* PHY Test Functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch Branch for PHY Tests including Initialisation
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestModeHandler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sequential Initialisation of TDME
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestInitialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Deinitialisation of TDME
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestDeinitialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Transmit Packet
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestTransmitPacket(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Receive Packet in PER Mode (Packet Error Rate)
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestReceivePacketPER(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Receive Packet in in PSN Mode (Packet Sniffer)
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestReceivePacketPSN(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Receive in ED Detect Mode (Energy Sniffer)
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestReceiveED(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test LO_3 (Locking) Test
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestLOLocking(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Mode Exit
 *******************************************************************************
 * \param errmsg - Status to be reported
 *******************************************************************************
 ******************************************************************************/
void PHYTestExit(char *errmsg);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Initialise Results and Run Parameters
 *******************************************************************************
 ******************************************************************************/
void PHYTestInitTestResults(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Reset Testmode PIB
 *******************************************************************************
 ******************************************************************************/
void PHYTestReset(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Calculate Test Statistics
 *******************************************************************************
 * \param mode
 * \parblock
 * mode: accumulation, initialisation, rollover report, final report
 * rollover - rollover for reporting when 1
 * final - final calculations for end of test reporting
 * \endparblock
 * \param ed - Test Packet energy Detect Value
 * \param cs - Test Packet carrier sense value
 * \param fo - Test Packet count of falloffs
 *
 *******************************************************************************
 ******************************************************************************/
void PHYTestStatistics(uint8_t mode, uint8_t ed, uint8_t cs, uint8_t fo);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Division of Unsigned 32-Bit Values with Rounding
 *******************************************************************************
 * \param va - Dividend
 * \param vb - Divisor
 *******************************************************************************
 * \return Result
 *******************************************************************************
 ******************************************************************************/
uint32_t PHYTest_divu32round(uint32_t va, uint32_t vb);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Division of Signed 32-Bit Values with Rounding
 *******************************************************************************
 * \param va - Dividend
 * \param vb - Divisor
 *******************************************************************************
 * \return Result
 *******************************************************************************
 ******************************************************************************/
int32_t PHYTest_divs32round(int32_t va, int32_t vb);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Calculate Time when to report Results
 *******************************************************************************
 * \param init - Initialise when 1
 *******************************************************************************
 * \return 1: report results
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestCalculateReportTime(uint8_t init);
/* PHY Test Reporting Functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Test Mode after Initialisation is done
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTestMode(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Test Parameter PIB
 *******************************************************************************
 * \param parameter - Specific Parameter or PHY_TESTPAR_ALL
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTestParameters(uint8_t parameter);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Successful Transmission of Packet
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportPacketTransmitted(struct MAC_Message *msg, uint8_t status);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Packet Transmission Analysis
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTransmitPacketAnalysis(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Reception of Packet
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportPacketReceived(struct TDME_RXPKT_indication_pset *params);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Packet Reception Analysis
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportReceivedPacketAnalysis(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Complete Test Result
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTestResult(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Reception of ED above Threshold
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportEDReceived(struct TDME_EDDET_indication_pset *params);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report LO Test 3 Result (Locking Test)
 *******************************************************************************
 * \param ntest - Number of Test
 * \param params - TDME LOTLK confirm buffer
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportLOLocking(struct TDME_LOTLK_confirm_pset *params, uint8_t ntest);
/* PHY Functions in EVBME Attribute Control */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Change Test Configuration
 *******************************************************************************
 * \param val - Value to set the Configurations to
 *******************************************************************************
 ******************************************************************************/
void PHYTestCfg(uint8_t val);
/* PHY Test Wrappers for TDME Commands and Responses */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_TESTMODE_request_sync
 *******************************************************************************
 * \param testmode - Test mode
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_TESTMODE_request(uint8_t testmode, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_SET_request_sync
 *******************************************************************************
 * \param evbme_attribute - TDME Attribute
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_SET_request(uint8_t evbme_attribute, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_TXPKT_request_sync
 *******************************************************************************
 * \param msg - TDME TxPktReq MAC Message
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_TXPKT_request(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_LOTLK_request_sync
 *******************************************************************************
 * \param ch - 802.15.4 Channel to be tested
 * \param rx_txb - rx Mode to be tested when 1, Tx Mode when 0#
 * \param ntest - Number of Test
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_LOTLK_request(uint8_t ch, uint8_t rx_txb, uint8_t ntest, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_RXPKT_indication
 *******************************************************************************
 * \param params - rxpkt indication buffer
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
int PHY_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_EDDET_indication
 *******************************************************************************
 * \param params - eddet indication buffer
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
int PHY_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef);
/******************************************************************************/
/****** Function Declarations for test15_4_phy_tests_mac.c               ******/
/******************************************************************************/
/* PHY_MAC Test Functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Address Initialisation
 *******************************************************************************
 ******************************************************************************/
void PHYTestMACAddInit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation for using MAC layer in EVBME (Tx)
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACTxInitialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation for using MAC layer in EVBME (Rx)
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACRxInitialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Denitialisation for using MAC layer in EVBME
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACDeinitialise(struct ca821x_dev *pDeviceRef);
/* PHY_MAC Test Wrappers for MCPS Commands and Responses */
uint8_t PHY_TXPKT_MAC_request(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for MCPS_DATA_indication()
 *******************************************************************************
 * \param params - MCPS data indication buffer
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_RXPKT_MAC_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for MCPS_DATA_confirm()
 *******************************************************************************
 * \param params - MCPS data confirm buffer
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_TXPKT_MAC_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);

#endif /* TEST15_4_PHY_TESTS_H */
