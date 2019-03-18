/**
 * @file test15_4_phy_tests.c
 * @brief PHY Test Functions
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
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ca821x_api.h"
#include "test15_4_phy_tests.h"

#define M1P5(x) (x + (x >> 1)) /* multiply by 1.5 */

/******************************************************************************/
/****** Global Variables used commonly                                   ******/
/******************************************************************************/
uint8_t           PHY_TESTMODE;
struct PHYTestPar PHY_TESTPAR;
struct PHYTestRes PHY_TESTRES;

/******************************************************************************/
/****** Global Variables only used in this File                          ******/
/******************************************************************************/
uint8_t              PHY_TEST_INITIALISED; /* for tests that require initialisation */
static unsigned long start_ms;             /* Used for scheduling packet tx/rx */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Returns how many milliseconds have elapsed since start_ms
 *******************************************************************************
 * \return milliseconds since start_ms
 *******************************************************************************
 ******************************************************************************/
static unsigned long calculate_elapsed_ms(void)
{
	unsigned long current_ms;

	current_ms = test15_4_getms();
	return current_ms - start_ms;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch Branch for PHY Tests including Initialisation
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestModeHandler(struct ca821x_dev *pDeviceRef)
{
	if (PHY_TESTRES.TEST_RUNNING)
	{
		if (PHY_TESTMODE == PHY_TEST_TX_PKT)
		{
			if ((PHYTestTransmitPacket(pDeviceRef)))
				PHYTestExit("PHYTestTransmitPacket returned non-zero");
		}
		else if (PHY_TESTMODE == PHY_TEST_RX_PER)
		{
			PHYTestReceivePacketPER(pDeviceRef);
		}
		else if (PHY_TESTMODE == PHY_TEST_RX_PSN)
		{
			PHYTestReceivePacketPSN(pDeviceRef);
		}
		else if (PHY_TESTMODE == PHY_TEST_RX_EDSN)
		{
			PHYTestReceiveED(pDeviceRef);
		}
		else if (PHY_TESTMODE == PHY_TEST_LO_3)
		{
			if ((PHYTestLOLocking(pDeviceRef)))
				PHYTestExit("PHYTestLOLocking returned non-zero");
		}
	}

	/* Note: PHY_TEST_TX_CONT, PHY_TEST_LO_1 and PHY_TEST_LO_2 don't need
	 *       any interaction, only setup */

} // End of PHYTestModeHandler()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sequential Initialisation of TDME
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestInitialise(struct ca821x_dev *pDeviceRef)
{
	uint8_t status = 0;

	/* set up TDME PIB and synchronise with transceiver */
	if ((status = PHY_SET_request(TDME_CHANNEL, pDeviceRef)))
		return (status);

	/* set TXCONT according to test mode before TDME_SET_request */
	switch (PHY_TESTMODE)
	{
	case PHY_TEST_TX_CONT:
	case PHY_TEST_LO_1:
	case PHY_TEST_LO_2:
		PHY_TESTPAR.TXCONT = 1;
		break;
	default:
		PHY_TESTPAR.TXCONT = 0;
		break;
	}

	if ((status = PHY_SET_request(TDME_TX_CONFIG, pDeviceRef)))
		return (status);

	if ((status = PHY_SET_request(TDME_ED_CONFIG, pDeviceRef)))
		return (status);

	if ((status = PHY_SET_request(TDME_RX_CONFIG, pDeviceRef)))
		return (status);

	if ((status = PHY_SET_request(TDME_LO_1_CONFIG, pDeviceRef)))
		return (status);

	if ((status = PHY_SET_request(TDME_LO_2_CONFIG, pDeviceRef)))
		return (status);

	if ((status = PHY_SET_request(TDME_ATM_CONFIG, pDeviceRef)))
		return (status);

	/* start test */
	PHY_TEST_INITIALISED = 0;

	/* initialise variables */
	PHYTestInitTestResults();

	PHYTestReportTestMode();

	/* set test mode */
	if ((status = PHY_TESTMODE_request(PHY_TESTMODE, pDeviceRef)))
		return (status);

	if ((PHY_TESTMODE == PHY_TEST_TX_PKT) && PHY_TESTPAR.MACENABLED)
	{
		if ((status = PHYTestMACTxInitialise(pDeviceRef)))
			return (status);
	}

	if (((PHY_TESTMODE == PHY_TEST_RX_PSN) || (PHY_TESTMODE == PHY_TEST_RX_PER)) && PHY_TESTPAR.MACENABLED)
	{
		if ((status = PHYTestMACRxInitialise(pDeviceRef)))
			return (status);
	}

	start_ms = test15_4_getms();

	PHY_TESTRES.TEST_RUNNING = 1;

	return status;
} // End of PHYTestInitialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Deinitialisation of TDME
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestDeinitialise(struct ca821x_dev *pDeviceRef)
{
	if (PHY_TESTMODE == PHY_TEST_TX_PKT)
	{
		PHYTestReportTransmitPacketAnalysis();
	}
	else if ((PHY_TESTMODE == PHY_TEST_RX_PER) || (PHY_TESTMODE == PHY_TEST_RX_PSN))
	{
		PHYTestStatistics(TEST_STAT_FINAL, 0, 0, 0);
		PHYTestReportTestResult();
	}

	PHY_TEST_INITIALISED     = 0;
	PHY_TESTRES.TEST_RUNNING = 0;
	PHY_TESTMODE             = PHY_TEST_OFF;

	PHYTestReportTestMode();

	if ((PHY_TESTMODE_request(PHY_TEST_OFF, pDeviceRef)))
		printf("Could not exit Testmode\n");

	if (PHY_TESTPAR.MACENABLED)
	{
		if ((PHYTestMACDeinitialise(pDeviceRef)))
			printf("Could not deinitialise MAC Layer\n");
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Transmit Packet
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestTransmitPacket(struct ca821x_dev *pDeviceRef)
{
	uint8_t            status = 0;
	unsigned long      msElapsed;
	struct MAC_Message tx_msg;

	if (!PHY_TEST_INITIALISED)
	{
		PHYTestCalculateReportTime(1);
		PHY_TEST_INITIALISED = 1;
	}

	msElapsed = calculate_elapsed_ms();
	if (msElapsed <= PHY_TESTPAR.PACKETPERIOD)
		return status;

	start_ms = test15_4_getms();
	if (PHY_TESTPAR.MACENABLED)
	{
		status = PHY_TXPKT_MAC_request(&tx_msg, pDeviceRef);
	}
	else
	{
		status = PHY_TXPKT_request(&tx_msg, pDeviceRef);
	}

	++PHY_TESTRES.PACKET_COUNT;

	if ((PHY_TESTPAR.PACKETPERIOD >= 500) || ((PHY_TESTPAR.NUMBEROFPKTS <= 100) && (PHY_TESTPAR.NUMBEROFPKTS != 0)))
	{
		/* report detailed packet information if packet period 500 ms or more */
		PHYTestReportPacketTransmitted(&tx_msg, status);
	}
	else if (PHYTestCalculateReportTime(0))
	{
		PHYTestReportTransmitPacketAnalysis();
	}

	/* stop test if number of packets is 100 or less */
	if ((PHY_TESTRES.PACKET_COUNT >= PHY_TESTPAR.NUMBEROFPKTS) && (PHY_TESTPAR.NUMBEROFPKTS != 0))
	{
		PHYTestDeinitialise(pDeviceRef);
	}

	return (status);
} // End of PHYTestTransmitPacket()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Receive Packet in PER Mode (Packet Error Rate)
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestReceivePacketPER(struct ca821x_dev *pDeviceRef)
{
	unsigned long  msElapsed;
	uint8_t        missed_packet;
	static uint8_t missed_last = 0;

	if (PHY_TESTPAR.PACKETPERIOD == 0)
	{
		PHYTestExit("PHYTestReceivePacketPER Packet Period is 0");
		return;
	}

	/* wait for first received packet */
	if (!PHY_TEST_INITIALISED)
	{
		PHYTestCalculateReportTime(1);
		PHYTestStatistics(TEST_STAT_INIT, 0, 0, 0);
		missed_last = 0;
		if (PHY_TESTRES.PACKET_RECEIVED)
		{
			start_ms             = test15_4_getms();
			PHY_TEST_INITIALISED = 1;
		}
		else
		{
			return;
		}
	}

	msElapsed = calculate_elapsed_ms();

	/* 1.0 * PACKETPERIOD when previous packet was missed */
	/* 1.5 * PACKETPERIOD when previous packet was received */
	if (((msElapsed > PHY_TESTPAR.PACKETPERIOD) && missed_last) ||
	    ((msElapsed > M1P5(PHY_TESTPAR.PACKETPERIOD)) && !missed_last))
		missed_packet = 1;
	else
		missed_packet = 0;

	if ((!missed_packet && !PHY_TESTRES.PACKET_RECEIVED))
		return;

	start_ms = test15_4_getms();

	if (missed_packet)
	{
		++PHY_TESTRES.MISSED_COUNT;
		missed_last = 1; /* last packet missed */
	}
	else if (PHY_TESTRES.PACKET_RECEIVED)
	{
		missed_last = 0; /* last packet received */
	}

	missed_packet = 0;

	if (PHY_TESTRES.PACKET_COUNT >= PHY_TESTPAR.NUMBEROFPKTS)
	{
		PHYTestDeinitialise(pDeviceRef);
		return;
	}

	++PHY_TESTRES.PACKET_COUNT;

	if (PHYTestCalculateReportTime(0))
	{
		PHYTestStatistics(TEST_STAT_REPORT, 0, 0, 0);
		PHYTestReportReceivedPacketAnalysis();
	}

	PHY_TESTRES.PACKET_RECEIVED = 0;

} // End of PHYTestReceivePacketPER()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Receive Packet in in PSN Mode (Packet Sniffer)
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestReceivePacketPSN(struct ca821x_dev *pDeviceRef)
{
	unsigned long msElapsed;

	if (!PHY_TEST_INITIALISED)
	{
		PHYTestStatistics(TEST_STAT_INIT, 0, 0, 0);
		PHY_TEST_INITIALISED = 1;
		return;
	}

	if (PHY_TESTRES.PACKET_RECEIVED)
	{
		PHY_TESTRES.PACKET_RECEIVED = 0;
		++PHY_TESTRES.PACKET_COUNT;
		if (PHY_TESTRES.PACKET_COUNT >= PHY_TESTPAR.NUMBEROFPKTS)
		{
			PHYTestDeinitialise(pDeviceRef);
			return;
		}
	}
	else
	{
		msElapsed = calculate_elapsed_ms();
		if (msElapsed > PHY_TEST_REPORT_PERIOD)
		{
			start_ms = test15_4_getms();
			// PHYTestStatistics(TEST_STAT_REPORT, 0, 0, 0);
			// PHYTestReportReceivedPacketAnalysis();
		}
	}

} // End of PHYTestReceivePacketPSN()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Receive in ED Detect Mode (Energy Sniffer)
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void PHYTestReceiveED(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;

	if (PHY_TESTRES.PACKET_RECEIVED)
	{
		PHY_TESTRES.PACKET_RECEIVED = 0;
		++PHY_TESTRES.PACKET_COUNT;
	}
} // End of PHYTestReceiveED()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test LO_3 (Locking) Test
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestLOLocking(struct ca821x_dev *pDeviceRef)
{
	uint8_t        status = 0;
	unsigned long  msElapsed;
	static uint8_t ntest   = 0;
	static uint8_t channel = 11;
	static uint8_t rx_txb  = 0;

	if (!PHY_TEST_INITIALISED)
	{
		ntest                = 0;  // nr of locking tests
		channel              = 11; // channel
		rx_txb               = 0;  // rx_txb
		PHY_TEST_INITIALISED = 1;
	}

	msElapsed = calculate_elapsed_ms();

	if (msElapsed > PHY_TESTPAR.LO_3_PERIOD)
	{
		start_ms = test15_4_getms();
		status   = PHY_LOTLK_request(channel, rx_txb, ntest, pDeviceRef);

		if (ntest < PHY_TESTPAR.LO_3_LOCKS - 1)
		{
			++ntest;
		}
		else
		{
			ntest = 0;
			if (channel < 26)
			{
				++channel;
			}
			else
			{
				channel = 11;
				if (rx_txb == 0)
				{
					rx_txb = 1;
				}
				else
				{
					ntest   = 0;
					channel = 11;
					rx_txb  = 0;
					PHYTestDeinitialise(pDeviceRef);
				}
			}
		}
	}

	return (status);
} // End of PHYTestLOLocking()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Mode Exit
 *******************************************************************************
 * \param status - Status to be reported
 *******************************************************************************
 ******************************************************************************/
void PHYTestExit(char *errmsg)
{
	PHY_TESTMODE = PHY_TEST_OFF;
	printf("PHY Test Exit: %s.\n", errmsg);
} // End of PHYTestExit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Initialise Results and Run Parameters
 *******************************************************************************
 ******************************************************************************/
void PHYTestInitTestResults(void)
{
	PHY_TESTRES.SEQUENCENUMBER  = 0;
	PHY_TESTRES.PACKET_COUNT    = 0;
	PHY_TESTRES.PACKET_RECEIVED = 0;
	PHY_TESTRES.TEST_RUNNING    = 0;
	PHY_TESTRES.LOERR_COUNT     = 0;
	PHY_TESTRES.CRCERR_COUNT    = 0;
	PHY_TESTRES.PHRERR_COUNT    = 0;
	PHY_TESTRES.SHRERR_COUNT    = 0;
	PHY_TESTRES.PREERR_COUNT    = 0;
	PHY_TESTRES.MISSED_COUNT    = 0;
	PHY_TESTRES.FO_AVG          = 0;
	PHY_TESTRES.ED_AVG          = 0;
	PHY_TESTRES.CS_AVG          = 0;
	PHY_TESTRES.FO_AVG_TOTAL    = 0;
	PHY_TESTRES.ED_AVG_TOTAL    = 0;
	PHY_TESTRES.CS_AVG_TOTAL    = 0;
	PHY_TESTRES.ED_MAX          = 0;
	PHY_TESTRES.ED_MIN          = 255;
	PHY_TESTRES.CS_MAX          = 0;
	PHY_TESTRES.CS_MIN          = 255;

} // End of PHYTestInitTestResults()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Reset Testmode PIB
 *******************************************************************************
 ******************************************************************************/
void PHYTestReset(void)
{
	PHY_TESTMODE           = PHY_TEST_OFF;
	PHY_TESTPAR.MACENABLED = 0;

	PHY_TESTPAR.PACKETPERIOD   = PHY_TESTPARDEF_PACKETPERIOD;
	PHY_TESTPAR.PACKETLENGTH   = PHY_TESTPARDEF_PACKETLENGTH;
	PHY_TESTPAR.NUMBEROFPKTS   = PHY_TESTPARDEF_NUMBEROFPKTS;
	PHY_TESTPAR.PACKETDATATYPE = PHY_TESTPARDEF_PACKETDATATYPE;
	PHY_TESTPAR.CHANNEL        = PHY_TESTPARDEF_CHANNEL;
	PHY_TESTPAR.TXPOWER_IB     = PHY_TESTPARDEF_TXPOWER_IB;
	PHY_TESTPAR.TXPOWER_PB     = PHY_TESTPARDEF_TXPOWER_PB;
	PHY_TESTPAR.TXPOWER_BOOST  = PHY_TESTPARDEF_TXPOWER_BOOST;
	PHY_TESTPAR.TXCONT         = PHY_TESTPARDEF_TXCONT;
	PHY_TESTPAR.EDTHRESHOLD    = PHY_TESTPARDEF_EDTHRESHOLD;
	PHY_TESTPAR.RX_FFSYNC      = PHY_TESTPARDEF_RX_FFSYNC;
	PHY_TESTPAR.LO_1_RXTXB     = PHY_TESTPARDEF_LO_1_RXTXB;
	PHY_TESTPAR.LO_2_FDAC      = PHY_TESTPARDEF_LO_2_FDAC;
	PHY_TESTPAR.LO_3_LOCKS     = PHY_TESTPARDEF_LO_3_LOCKS;
	PHY_TESTPAR.LO_3_PERIOD    = PHY_TESTPARDEF_LO_3_PERIOD;
	PHY_TESTPAR.ATM            = PHY_TESTPARDEF_ATM;
	PHY_TESTPAR.MPW2_OVWR      = PHY_TESTPARDEF_MPW2_OVWR;
} // End of PHYTestReset()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Calculate Test Statistics
 *******************************************************************************
 * \param mode - mode: accumulation, initialisation, rollover report, final report
 * \param rollover - rollover for reporting when 1
 * \param final - final calculations for end of test reporting
 *******************************************************************************
 ******************************************************************************/
void PHYTestStatistics(uint8_t mode, uint8_t ed, uint8_t cs, uint8_t fo)
{
	int8_t          sv;
	static int32_t  acc_fo = 0;
	static uint32_t acc_ed = 0;
	static uint32_t acc_cs = 0;
	static uint32_t count1 = 0;
	static uint32_t count2 = 0;

	if (mode == TEST_STAT_INIT)
	{
		count1 = 0;
		count2 = 0;
		acc_fo = 0;
		acc_ed = 0;
		acc_cs = 0;
		return;
	}

	else if (mode == TEST_STAT_FINAL)
	{
		if (count2 != 0)
		{
			/* determine final averages for end of test */
			PHY_TESTRES.FO_AVG_TOTAL = PHYTest_divs32round((PHY_TESTRES.FO_AVG_TOTAL + acc_fo), (int32_t)count2);
			PHY_TESTRES.ED_AVG_TOTAL = PHYTest_divu32round((PHY_TESTRES.ED_AVG_TOTAL + acc_ed), (uint32_t)count2);
			PHY_TESTRES.CS_AVG_TOTAL = PHYTest_divu32round((PHY_TESTRES.CS_AVG_TOTAL + acc_cs), (uint32_t)count2);
		}
		else
		{
			PHY_TESTRES.ED_MIN = 0;
			PHY_TESTRES.CS_MIN = 0;
		}
	}
	else if (mode == TEST_STAT_REPORT)
	{
		/* determine temporary averages for interim reporting */
		if (count1 != 0)
		{
			PHY_TESTRES.FO_AVG = PHYTest_divs32round(acc_fo, (int32_t)count1);
			PHY_TESTRES.ED_AVG = PHYTest_divu32round(acc_ed, (uint32_t)count1);
			PHY_TESTRES.CS_AVG = PHYTest_divu32round(acc_cs, (uint32_t)count1);
		}
		else
		{
			PHY_TESTRES.FO_AVG = 0;
			PHY_TESTRES.ED_AVG = 0;
			PHY_TESTRES.CS_AVG = 0;
		}
		PHY_TESTRES.FO_AVG_TOTAL += acc_fo;
		PHY_TESTRES.ED_AVG_TOTAL += acc_ed;
		PHY_TESTRES.CS_AVG_TOTAL += acc_cs;
		acc_fo = 0;
		acc_ed = 0;
		acc_cs = 0;
		count1 = 0;
	}
	else /* mode == TEST_STAT_ACCUM */
	{
		if (PHY_TESTRES.PACKET_RECEIVED)
		{
			/* sums for averages */
			sv = (int8_t)fo;
			acc_fo += (int32_t)sv;
			acc_ed += (uint32_t)ed;
			acc_cs += (uint32_t)cs;
			++count1;
			++count2;
			/* determine min/max */
			if (ed > PHY_TESTRES.ED_MAX)
				PHY_TESTRES.ED_MAX = ed;
			if (ed < PHY_TESTRES.ED_MIN)
				PHY_TESTRES.ED_MIN = ed;
			if (cs > PHY_TESTRES.CS_MAX)
				PHY_TESTRES.CS_MAX = cs;
			if (cs < PHY_TESTRES.CS_MIN)
				PHY_TESTRES.CS_MIN = cs;
		}
	}

} // End of PHYTestStatistics()

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
uint32_t PHYTest_divu32round(uint32_t va, uint32_t vb)
{
	uint32_t vr;
	vr = va / vb;
	/* positive truncated down, check if needs correction */
	if ((va % vb) > (vb / 2))
		vr += 1;

	return vr;
} // End of PHYTest_divu32round()

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
int32_t PHYTest_divs32round(int32_t va, int32_t vb)
{
	int32_t vr;

	vr = va / vb;
	if (vr > 0)
	{
		/* positive truncated down, check if needs correction */
		if ((va % vb) > (vb / 2))
		{
			vr += 1;
		}
	}
	else
	{
		/* negative truncated up, check if needs correction */
		if ((va % vb) < (-vb / 2))
		{
			vr -= 1;
		}
	}
	return vr;
} // End of PHYTest_divs32round()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Calculate Time when to report Results
 *******************************************************************************
 * \param init - Initialise when 1
 *******************************************************************************
 * \return 1: report results
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestCalculateReportTime(uint8_t init)
{
	static uint16_t tcount = 0;

	if (init)
	{
		tcount = 0;
		return (0);
	}

	++tcount;

	/* report statistics every 5 to 12 seconds */
	if (((PHY_TESTPAR.PACKETPERIOD >= 1000) && (tcount >= 5)) ||
	    ((PHY_TESTPAR.PACKETPERIOD >= 250) && (tcount >= 20)) ||
	    ((PHY_TESTPAR.PACKETPERIOD >= 100) && (tcount >= 50)) ||
	    ((PHY_TESTPAR.PACKETPERIOD >= 50) && (tcount >= 100)) ||
	    ((PHY_TESTPAR.PACKETPERIOD >= 25) && (tcount >= 200)) || ((PHY_TESTPAR.PACKETPERIOD < 25) && (tcount >= 500)))
	{
		tcount = 0;
		return (1);
	}

	return (0);
} // End of PHYTestCalculateReportTime()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Test Mode after Initialisation is done
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTestMode(void)
{
	printf("PHY Testmode initialised for ");
	switch (PHY_TESTMODE)
	{
	case PHY_TEST_OFF:
		printf("Normal Mode");
		break;
	case PHY_TEST_TX_PKT:
		printf("Tx (Packets)");
		break;
	case PHY_TEST_TX_CONT:
		printf("Tx (Continuous)");
		break;
	case PHY_TEST_RX_PER:
		printf("Rx (PER Test)");
		break;
	case PHY_TEST_RX_PSN:
		printf("Rx (Packet Sniffer)");
		break;
	case PHY_TEST_RX_EDSN:
		printf("Rx (Energy Sniffer)");
		break;
	case PHY_TEST_LO_1:
		printf("LO1 (Closed Loop)");
		break;
	case PHY_TEST_LO_2:
		printf("LO2 (Open Loop)");
		break;
	case PHY_TEST_LO_3:
		printf("LO3 (Locking Test)");
		break;
	default:
		printf("Unknown Test Mode");
		break;
	}
	printf("\n");
} // End of PHYTestReportTestMode()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Test Parameter PIB
 *******************************************************************************
 * \param parameter - Specific Parameter or PHY_TESTPAR_ALL
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTestParameters(uint8_t parameter)
{
	if (parameter == PHY_TESTPAR_ALL)
		printf("PHY Test Parameter Set:\n");
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_PACKETPERIOD))
		printf("PACKETPERIOD   = %u ms\n", PHY_TESTPAR.PACKETPERIOD);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_PACKETLENGTH))
		printf("PACKETLENGTH   = %u Bytes\n", PHY_TESTPAR.PACKETLENGTH);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_NUMBEROFPKTS))
		printf("NUMBEROFPKTS   = %u\n", PHY_TESTPAR.NUMBEROFPKTS);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_PACKETDATATYPE))
	{
		printf("PACKETDATATYPE = ");
		switch (PHY_TESTPAR.PACKETDATATYPE)
		{
		case TDME_TXD_RANDOM:
			printf("Random");
			break;
		case TDME_TXD_SEQRANDOM:
			printf("Random with Sequence Number");
			break;
		case TDME_TXD_COUNT:
			printf("Count (Increment)");
			break;
		case TDME_TXD_APPENDED:
			printf("Appended");
			break;
		default:
			printf("???");
			break;
		}
		printf("\n");
	}
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_CHANNEL))
		printf("CHANNEL        = %u (%02X)\n", PHY_TESTPAR.CHANNEL, PHY_TESTPAR.CHANNEL);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_TXPOWER))
		printf("TXPOWER        = IB: %u; PB: %u; BOOST: %u\n",
		       PHY_TESTPAR.TXPOWER_IB,
		       PHY_TESTPAR.TXPOWER_PB,
		       PHY_TESTPAR.TXPOWER_BOOST);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_EDTHRESHOLD))
		printf("EDTHRESHOLD    = %u\n", PHY_TESTPAR.EDTHRESHOLD);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_RX_FFSYNC))
		printf("RX_FFSYNC      = %u\n", PHY_TESTPAR.RX_FFSYNC);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_LO_1_RXTXB))
		printf("LO_1_RXTXB     = %u\n", PHY_TESTPAR.LO_1_RXTXB);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_LO_2_FDAC))
		printf("LO_2_FDAC      = %u (%02X)\n", PHY_TESTPAR.LO_2_FDAC, PHY_TESTPAR.LO_2_FDAC);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_LO_3_LOCKS))
		printf("LO_3_LOCKS     = %u\n", PHY_TESTPAR.LO_3_LOCKS);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_LO_3_PERIOD))
		printf("LO_3_PERIOD    = %u\n", PHY_TESTPAR.LO_3_PERIOD);
	if ((parameter == PHY_TESTPAR_ALL) || (parameter == PHY_TESTPAR_ATM))
		printf("ATM            = %u (%02X)\n", PHY_TESTPAR.ATM, PHY_TESTPAR.ATM);
} // End of PHYTestReportTestParameters()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Successful Transmission of Packet
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportPacketTransmitted(struct MAC_Message *msg, uint8_t status)
{
	uint8_t i;

	if (status == TDME_LO_ERROR)
	{
		printf("Tx: LO Error");
	}
	else
	{
		printf("Tx: SN=%3u PL=%3u:",
		       msg->PData.TDMETxPktReq.TestPacketSequenceNumber,
		       msg->PData.TDMETxPktReq.TestPacketLength);
		for (i = 0; i < msg->PData.TDMETxPktReq.TestPacketLength - 2; ++i)
			printf(" %02X", msg->PData.TDMETxPktReq.TestPacketData[i]);
	}
	printf("\n");
} // End of PHYTestReportPacketTransmitted()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Packet Transmission Analysis
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTransmitPacketAnalysis(void)
{
	printf("Tx: %u Packets sent\n", PHY_TESTRES.PACKET_COUNT);
	if (PHY_TESTPAR.MACENABLED)
		printf("%u No-Acks; %u Channel Access Failures\n", PHY_TESTRES.SHRERR_COUNT, PHY_TESTRES.PHRERR_COUNT);
} // End of PHYTestReportTransmitPacketAnalysis()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Reception of Packet
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportPacketReceived(struct TDME_RXPKT_indication_pset *params)
{
	uint8_t i;
	int8_t  sv;

	if (params->Status == TDME_LO_ERROR)
	{
		printf("Rx: LO Error");
	}
	else
	{
		printf("Rx: PL=%3u:", params->TestPacketLength);
		for (i = 0; i < params->TestPacketLength - 2; ++i) printf(" %02X", params->TestPacketData[i]);
		if (params->Status == TDME_FCS_ERROR)
			printf("; CRCErr");
		else if (params->Status == TDME_SHR_ERROR)
			printf("; SHRErr");
		else if (params->Status == TDME_PHR_ERROR)
			printf("; PHRErr");
		sv = (int8_t)params->TestPacketFoffsValue;
		printf("; ED: %u; CS: %u; FO: %d", params->TestPacketEDValue, params->TestPacketCSValue, sv);
	}
	printf("\n");
} // End of PHYTestReportPacketReceived()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Packet Reception Analysis
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportReceivedPacketAnalysis(void)
{
	uint32_t errcount_total;

	errcount_total = PHY_TESTRES.CRCERR_COUNT + PHY_TESTRES.MISSED_COUNT;

	printf("Rx: %u Packets received, %u Errors\n", PHY_TESTRES.PACKET_COUNT, errcount_total);

	if (PHY_TESTPAR.PACKETPERIOD != 0)
	{
		if (PHY_TESTPAR.RX_FFSYNC)
		{
			printf("Rx Error  Analysis: CRC: %u; PHR: %u; SHR: %u; PRE: %u; Missed: %u\n",
			       PHY_TESTRES.CRCERR_COUNT,
			       PHY_TESTRES.PHRERR_COUNT,
			       PHY_TESTRES.SHRERR_COUNT,
			       PHY_TESTRES.PREERR_COUNT,
			       PHY_TESTRES.MISSED_COUNT);
		}
		else
		{
			printf("Rx Error  Analysis: CRC: %u; Missed: %u\n", PHY_TESTRES.CRCERR_COUNT, PHY_TESTRES.MISSED_COUNT);
		}
	}

	printf("Rx Signal Analysis: ED: %u; CS: %u; FO: %d\n", PHY_TESTRES.ED_AVG, PHY_TESTRES.CS_AVG, PHY_TESTRES.FO_AVG);

} // End of PHYTestReportReceivedPacketAnalysis()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Complete Test Result
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportTestResult(void)
{
	uint32_t errcount_total;

	errcount_total = PHY_TESTRES.CRCERR_COUNT + PHY_TESTRES.MISSED_COUNT;

	printf("Test Result: %u Packets received, %u Errors\n", PHY_TESTRES.PACKET_COUNT, errcount_total);

	if (PHY_TESTPAR.RX_FFSYNC)
	{
		printf("Error Analysis: CRC: %u; PHR: %u; SHR: %u; PRE: %u Missed: %u\n",
		       PHY_TESTRES.CRCERR_COUNT,
		       PHY_TESTRES.PHRERR_COUNT,
		       PHY_TESTRES.SHRERR_COUNT,
		       PHY_TESTRES.PREERR_COUNT,
		       PHY_TESTRES.MISSED_COUNT);
	}
	else
	{
		printf("Error Analysis: CRC: %u; Missed: %u\n", PHY_TESTRES.CRCERR_COUNT, PHY_TESTRES.MISSED_COUNT);
	}
	printf("Rx Overall Averages: ED: %u; CS: %u; FO: %d\n",
	       PHY_TESTRES.ED_AVG_TOTAL,
	       PHY_TESTRES.CS_AVG_TOTAL,
	       PHY_TESTRES.FO_AVG_TOTAL);
	printf("Rx Min/Max Analysis: ED Max: %u; ED Min: %u; CS Max: %u; CS Min: %u\n",
	       PHY_TESTRES.ED_MAX,
	       PHY_TESTRES.ED_MIN,
	       PHY_TESTRES.CS_MAX,
	       PHY_TESTRES.CS_MIN);
	if (PHY_TESTPAR.RX_FFSYNC)
	{
		printf("Rx Missed Pkt Analysis: SHR: %u; PHR: %u\n", PHY_TESTRES.SHRERR_COUNT, PHY_TESTRES.PHRERR_COUNT);
	}
	printf("Test completed\n");

} // End of PHYTestReportTestResult()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report Reception of ED above Threshold
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportEDReceived(struct TDME_EDDET_indication_pset *params)
{
	uint16_t tat;

	tat = ((uint16_t)(params->TestTimeAboveThreshold_us[1]) << 8) + (uint16_t)(params->TestTimeAboveThreshold_us[0]);

	printf("ED above %3u: ED=%3u; CS=%3u; THigh=%5u us; N=%u\n",
	       params->TestEDThreshold,
	       params->TestEDValue,
	       params->TestCSValue,
	       tat,
	       PHY_TESTRES.PACKET_COUNT);

} // End of PHYTestReportEDReceived()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Report LO Test 3 Result (Locking Test)
 *******************************************************************************
 * \param ntest - Number of Test
 *******************************************************************************
 ******************************************************************************/
void PHYTestReportLOLocking(struct TDME_LOTLK_confirm_pset *params, uint8_t ntest)
{
	if (params->TestRxTxb)
		printf("Rx");
	else
		printf("Tx");
	printf(
	    " Ch=%3u N=%3u FDAC=%3u AMP=%3u", params->TestChannel, ntest, params->TestLOFDACValue, params->TestLOAMPValue);
	if (!params->TestRxTxb)
		printf(" TXCAL=%3u", params->TestLOTXCALValue);
	if (params->Status == TDME_LO_ERROR)
		printf(" LOCK FAILURE");
	printf("\n");
} // End of PHYTestReportLOLocking()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Change Test Configuration
 *******************************************************************************
 * \param val - Value to set the Configurations to
 *******************************************************************************
 ******************************************************************************/
void PHYTestCfg(uint8_t val)
{
	if (val == 0)
	{
		printf("PHYTest configured to PHY only\n");
		PHY_TESTPAR.MACENABLED = 0;
	}
	else if (val == 1)
	{
		printf("PHYTest configured to MAC Usage\n");
		PHY_TESTPAR.MACENABLED = 1;
	}
	else
	{
		printf("PHYTest Configuration unknown\n");
	}
} // End of PHYTestCfg()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_TESTMODE_request_sync
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_TESTMODE_request(uint8_t testmode, struct ca821x_dev *pDeviceRef)
{
	uint8_t status;
	uint8_t tdme_testmode;

	/* translate between EVBME and TDME test mode */
	switch (testmode)
	{
	case PHY_TEST_OFF:
		tdme_testmode = TDME_TEST_OFF;
		break;
	case PHY_TEST_TX_PKT:
		tdme_testmode = TDME_TEST_TX;
		break;
	case PHY_TEST_TX_CONT:
		tdme_testmode = TDME_TEST_TX;
		break;
	case PHY_TEST_RX_PER:
		tdme_testmode = TDME_TEST_RX;
		break;
	case PHY_TEST_RX_PSN:
		tdme_testmode = TDME_TEST_RX;
		break;
	case PHY_TEST_RX_EDSN:
		tdme_testmode = TDME_TEST_ED;
		break;
	case PHY_TEST_LO_1:
		tdme_testmode = TDME_TEST_LO_1;
		break;
	case PHY_TEST_LO_2:
		tdme_testmode = TDME_TEST_LO_2;
		break;
	case PHY_TEST_LO_3:
		tdme_testmode = TDME_TEST_LO_3;
		break;
	default:
		tdme_testmode = TDME_TEST_OFF;
		break;
	}
	status = TDME_TESTMODE_request_sync(tdme_testmode, pDeviceRef);
	return status;
} // End of PHY_TESTMODE_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_SET_request_sync
 *******************************************************************************
 * \param attribute - TDME Attribute
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_SET_request(uint8_t attribute, struct ca821x_dev *pDeviceRef)
{
	uint8_t status;
	uint8_t param[4];
	uint8_t plength;

	switch (attribute)
	{
	case TDME_CHANNEL:
		plength  = 1;
		param[0] = PHY_TESTPAR.CHANNEL;
		break;
	case TDME_TX_CONFIG:
		plength  = 4;
		param[0] = PHY_TESTPAR.TXPOWER_IB;
		param[1] = PHY_TESTPAR.TXPOWER_PB;
		param[2] = PHY_TESTPAR.TXPOWER_BOOST;
		param[3] = PHY_TESTPAR.TXCONT;
		break;
	case TDME_ED_CONFIG:
		plength  = 1;
		param[0] = PHY_TESTPAR.EDTHRESHOLD;
		break;
	case TDME_RX_CONFIG:
		plength  = 1;
		param[0] = PHY_TESTPAR.RX_FFSYNC;
		break;
	case TDME_LO_1_CONFIG:
		plength  = 2;
		param[0] = PHY_TESTPAR.LO_1_RXTXB;
		param[1] = PHY_TESTPAR.CHANNEL;
		break;
	case TDME_LO_2_CONFIG:
		plength  = 1;
		param[0] = PHY_TESTPAR.LO_2_FDAC;
		break;
	case TDME_ATM_CONFIG:
		plength  = 1;
		param[0] = PHY_TESTPAR.ATM;
		break;
	default:
		plength  = 1;
		param[0] = 0x00;
		break;
	}
	status = TDME_SET_request_sync(attribute, plength, param, pDeviceRef);
	return status;
} // End of PHY_SET_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_TXPKT_request_sync
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_TXPKT_request(struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	uint8_t i;
	uint8_t status;

	msg->PData.TDMETxPktReq.TestPacketSequenceNumber = LS_BYTE(PHY_TESTRES.SEQUENCENUMBER);
	msg->PData.TDMETxPktReq.TestPacketDataType       = PHY_TESTPAR.PACKETDATATYPE;
	msg->PData.TDMETxPktReq.TestPacketLength         = PHY_TESTPAR.PACKETLENGTH;

	if (PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_APPENDED)
	{
		for (i = 0; i < PHY_TESTPAR.PACKETLENGTH; ++i)
			msg->PData.TDMETxPktReq.TestPacketData[i] = 0x00; /* currently filled with 0's */
	}

	status = TDME_TXPKT_request_sync(msg->PData.TDMETxPktReq.TestPacketDataType,
	                                 &msg->PData.TDMETxPktReq.TestPacketSequenceNumber,
	                                 &msg->PData.TDMETxPktReq.TestPacketLength,
	                                 msg->PData.TDMETxPktReq.TestPacketData,
	                                 pDeviceRef);

	++PHY_TESTRES.SEQUENCENUMBER;
	return status;
} // End of PHY_TXPKT_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_LOTLK_request_sync
 *******************************************************************************
 * \param ch - 802.15.4 Channel to be tested
 * \param rx_txb - rx Mode to be tested when 1, Tx Mode when 0
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_LOTLK_request(uint8_t ch, uint8_t rx_txb, uint8_t ntest, struct ca821x_dev *pDeviceRef)
{
	struct TDME_LOTLK_confirm_pset params;

	params.TestChannel = ch;
	params.TestRxTxb   = rx_txb;

	params.Status = TDME_LOTLK_request_sync(&params.TestChannel,
	                                        &params.TestRxTxb,
	                                        &params.TestLOFDACValue,
	                                        &params.TestLOAMPValue,
	                                        &params.TestLOTXCALValue,
	                                        pDeviceRef);

	PHYTestReportLOLocking(&params, ntest);

	if (params.Status == TDME_LO_ERROR)
		params.Status = MAC_SUCCESS;

	return params.Status;
} // End of PHY_LOTLK_request()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_RXPKT_indication
 *******************************************************************************
 * \param indication - rxpkt indication buffer
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
int PHY_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;

	switch (params->Status)
	{
	case TDME_LO_ERROR:
		++PHY_TESTRES.LOERR_COUNT;
		break;
	case TDME_FCS_ERROR:
		++PHY_TESTRES.CRCERR_COUNT;
		break;
	case TDME_SHR_ERROR:
		++PHY_TESTRES.SHRERR_COUNT;
		break;
	case TDME_PHR_ERROR:
		++PHY_TESTRES.PHRERR_COUNT;
		break;
	default:
		break;
	}

	PHY_TESTRES.PACKET_RECEIVED = 1; /* Flag indication */

	PHYTestStatistics(
	    TEST_STAT_ACCUM, params->TestPacketEDValue, params->TestPacketCSValue, params->TestPacketFoffsValue);

	if ((PHY_TESTPAR.PACKETPERIOD >= 500) || (PHY_TESTMODE == PHY_TEST_RX_PSN))
		PHYTestReportPacketReceived(params);

	return params->Status;
} // End of PHY_RXPKT_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief PHY Test Wrapper for TDME_EDDET_indication
 *******************************************************************************
 * \param indication - eddet indication buffer
 * \param pDeviceRef - Device reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
int PHY_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;

	PHY_TESTRES.PACKET_RECEIVED = 1; /* Flag indication */
	PHYTestReportEDReceived(params);

	return 0;
} // End of PHY_EDDET_indication()
