/**
 * @file test15_4_phy_tests.h
 * @brief PHY Tests Definitions and Function Declarations
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

/***************************************************************************/ /**
 * \defgroup PHY Testmode Parameter Defaults
 ************************************************************************** @{*/
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
/**@}*/

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
unsigned long test15_4_getms(void);

/******************************************************************************/
/****** Function Declarations for test15_4_phy_tests.c                   ******/
/******************************************************************************/
/* PHY Test Functions */
void     PHYTestModeHandler(struct ca821x_dev *pDeviceRef);
uint8_t  PHYTestInitialise(struct ca821x_dev *pDeviceRef);
void     PHYTestDeinitialise(struct ca821x_dev *pDeviceRef);
uint8_t  PHYTestTransmitPacket(struct ca821x_dev *pDeviceRef);
void     PHYTestReceivePacketPER(struct ca821x_dev *pDeviceRef);
void     PHYTestReceivePacketPSN(struct ca821x_dev *pDeviceRef);
void     PHYTestReceiveED(struct ca821x_dev *pDeviceRef);
uint8_t  PHYTestLOLocking(struct ca821x_dev *pDeviceRef);
void     PHYTestExit(char *errmsg);
void     PHYTestInitTestResults(void);
void     PHYTestReset(void);
void     PHYTestStatistics(uint8_t mode, uint8_t ed, uint8_t cs, uint8_t fo);
uint32_t PHYTest_divu32round(uint32_t va, uint32_t vb);
int32_t  PHYTest_divs32round(int32_t va, int32_t vb);
uint8_t  PHYTestCalculateReportTime(uint8_t init);
/* PHY Test Reporting Functions */
void PHYTestReportTestMode(void);
void PHYTestReportTestParameters(uint8_t parameter);
void PHYTestReportPacketTransmitted(struct MAC_Message *msg, uint8_t status);
void PHYTestReportTransmitPacketAnalysis(void);
void PHYTestReportPacketReceived(struct TDME_RXPKT_indication_pset *params);
void PHYTestReportReceivedPacketAnalysis(void);
void PHYTestReportTestResult(void);
void PHYTestReportEDReceived(struct TDME_EDDET_indication_pset *params);
void PHYTestReportLOLocking(struct TDME_LOTLK_confirm_pset *params, uint8_t ntest);
/* PHY Functions in EVBME Attribute Control */
void PHYTestCfg(uint8_t val);
/* PHY Test Wrappers for TDME Commands and Responses */
uint8_t PHY_TESTMODE_request(uint8_t testmode, struct ca821x_dev *pDeviceRef);
uint8_t PHY_SET_request(uint8_t evbme_attribute, struct ca821x_dev *pDeviceRef);
uint8_t PHY_TXPKT_request(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef);
uint8_t PHY_LOTLK_request(uint8_t ch, uint8_t rx_txb, uint8_t ntest, struct ca821x_dev *pDeviceRef);
int     PHY_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef);
int     PHY_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef);
/******************************************************************************/
/****** Function Declarations for test15_4_phy_tests_mac.c               ******/
/******************************************************************************/
/* PHY_MAC Test Functions */
void    PHYTestMACAddInit(void);
uint8_t PHYTestMACTxInitialise(struct ca821x_dev *pDeviceRef);
uint8_t PHYTestMACRxInitialise(struct ca821x_dev *pDeviceRef);
uint8_t PHYTestMACDeinitialise(struct ca821x_dev *pDeviceRef);
/* PHY_MAC Test Wrappers for MCPS Commands and Responses */
uint8_t PHY_TXPKT_MAC_request(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef);
uint8_t PHY_RXPKT_MAC_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
uint8_t PHY_TXPKT_MAC_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);

#endif /* TEST15_4_PHY_TESTS_H */
