/**
 * @file hwme_tdme.h
 * @brief Definitions for Cascoda's HWME and TDME.
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

#ifndef HWME_TDME_H
#define HWME_TDME_H

#include "ca821x_config.h"

#define MAX_HWME_ATTRIBUTE_SIZE 16
#define MAX_TDME_ATTRIBUTE_SIZE 2

/***************************************************************************/ /**
* \defgroup HWMEEnums HWME Enumerations
************************************************************************** @{*/

/** HWME Status Codes */
enum hwme_status
{
	/** The requested Primitive has been executed successfully */
	HWME_SUCCESS = 0x00,
	/** Unknown HWME Attribute or Parameter */
	HWME_UNKNOWN = 0x01,
	/** Invalid HWME Attribute Value or Parameter Value */
	HWME_INVALID = 0x02,
	/** The requested Attribute cannot currently be accessed */
	HWME_NO_ACCESS  = 0x03,
	HWME_MIN_STATUS = HWME_SUCCESS,
	HWME_MAX_STATUS = HWME_NO_ACCESS
};

/** Potential values for HWME_WAKEUP_indication's WakeUpCondition parameter */
enum hwme_wakeup_condition
{
	/** Transceiver woken up from Power Up / System Reset */
	HWME_WAKEUP_POWERUP = 0x00,
	/** Watchdog Timer Time-Out */
	HWME_WAKEUP_WDT = 0x01,
	/** Transceiver woken up from Power-Off by Sleep Timer Time-Out */
	HWME_WAKEUP_POFF_SLT = 0x02,
	/** Transceiver woken up from Power-Off by GPIO Activity */
	HWME_WAKEUP_POFF_GPIO = 0x03,
	/** Transceiver woken up from Standby by Sleep Timer Time-Out */
	HWME_WAKEUP_STBY_SLT = 0x04,
	/** Transceiver woken up from Standby by GPIO Activity */
	HWME_WAKEUP_STBY_GPIO = 0x05,
	/** Sleep-Timer Time-Out in Active Mode */
	HWME_WAKEUP_ACTIVE_SLT = 0x06,
	HWME_MIN_WAKEUP        = HWME_WAKEUP_POWERUP,
	HWME_MAX_WAKEUP        = HWME_WAKEUP_ACTIVE_SLT
};

/** Potential values of HWME_LQIMODE */
enum lqi_mode
{
	HWME_LQIMODE_CS = 0x00, //!< CS (Carrier Sense) is reported as LQI
	HWME_LQIMODE_ED = 0x01  //!< ED (Energy Detect) is reported as LQI
};

/** HWME Attribute IDs */
enum hwme_attribute
{
	HWME_POWERCON = 0x00, //!< Power Saving Mode Control
	HWME_CHIPID   = 0x01, //!< Product ID and Version Number
	HWME_TXPOWER  = 0x02, //!< Transmit Power Setting
	/** Clear Channel Assessment Mode, according to 802.15.4 Section 6.9.9 */
	HWME_CCAMODE     = 0x03,
	HWME_EDTHRESHOLD = 0x04, //!< Energy Detect (ED) Threshold for CCA
	HWME_CSTHRESHOLD = 0x05, //!< Carrier Sense (CS) Threshold for CCA
	/** Energy Detect (ED) Value of current channel */
	HWME_EDVALUE = 0x06,
	/** Carrier Sense (CS) Value of current channel */
	HWME_CSVALUE = 0x07,
	/** Energy Detect (ED) Value of last received Packet */
	HWME_EDVALLP = 0x08,
	/** Carrier Sense (CS) Value of last received Packet */
	HWME_CSVALLP = 0x09,
	/** Frequency Offset of last received Packet */
	HWME_FREQOFFS    = 0x0A,
	HWME_MACTIMER    = 0x0B, //!< MAC Symbol Timer Value
	HWME_RANDOMNUM   = 0x0C, //!< Random Number Value
	HWME_TEMPERATURE = 0x0D, //!< Temperature Sensor Measurement Value
	/** Host Security Key for AES-128 Encryption or Decryption */
	HWME_HSKEY = 0x0E,
	/** System Clock Output to a specific GPIO */
	HWME_SYSCLKOUT = 0x0F,
	/** LQI Reporting Mode for Data Indications and PanDescriptors */
	HWME_LQIMODE  = 0x10,
	HWME_LQILIMIT = 0x11,
#if CASCODA_CA_VER == 8211
	HWME_RXMODE            = 0x12,
	HWME_POLLINDMODE       = 0x13,
	HWME_ENHANCEDFP        = 0x14,
	HWME_MAXDIRECTS        = 0x15,
	HWME_MAXINDIRECTS      = 0x16,
	HWME_IMAGINARYINDIRECT = 0x17,
#endif
};

/** Hardware AES mode values */
enum haes_mode
{
	HAES_MODE_ENCRYPT = 0, //!< Encrypt given buffer
	HAES_MODE_DECRYPT = 1  //!< Decrypt given buffer
};

/** CCA Mode */
enum cca_mode
{
	CCAM_EDORCS  = 0, //!< CCA Mode: Either ED or CS level exceeded
	CCAM_ED      = 1, //!< CCA Mode: ED level exceeded
	CCAM_CS      = 2, //!< CCA Mode: CS level exceeded
	CCAM_EDANDCS = 3  //!< CCA Mode: Both ED and CS level exceeded
};
/**@}*/

/***************************************************************************/ /**
* \defgroup TDMEEnums TDME Enumerations
************************************************************************** @{*/

/** Hardware Error Messages from TDME_ERROR_indication */
enum tdme_hw_err
{
	TDME_ERR_NO_ERROR     = 0x00, //!< Everything OK
	TDME_ERR_LO_UNLOCK    = 0x10, //!< LO Unlocked
	TDME_ERR_SPI_INVALID  = 0x20, //!< Invalid SPI Message
	TDME_ERR_EX_STOF      = 0x30, //!< Exception: Stack Overflow
	TDME_ERR_EX_STUF      = 0x40, //!< Exception: Stack Underflow
	TDME_ERR_EX_DIVBY0    = 0x50, //!< Exception: Divide by 0
	TDME_ERR_DMA_ACCESS   = 0x60, //!< DMA Access Overflow/Underflow (+0x0F)
	TDME_ERR_UNEXP_IRQ    = 0x70, //!< Unexpected Interrupt (+0x0F)
	TDME_ERR_MALLOC_FAIL  = 0x80, //!< Memory Allocation Failure
	TDME_ERR_SYSTEM_ERROR = 0x90  //!< System Error
};

/** TDME Status Codes */
enum tdme_status
{
	/** The requested Primitive has been executed successfully */
	TDME_SUCCESS = 0x00,
	TDME_UNKNOWN = 0x01, //!< Unknown TDME Value or Parameter
	TDME_INVALID = 0x02, //!< Invalid TDME Value or Parameter Value
	/** The requested Value cannot currently be accessed */
	TDME_NO_ACCESS = 0x03,
	TDME_LO_ERROR  = 0x04, //!< LO Locking Error
	/** Received Packet Frame Check Sequence (CRC) Error */
	TDME_FCS_ERROR = 0x05,
	/** Received Packet Synchronisation Header Error */
	TDME_SHR_ERROR  = 0x06,
	TDME_PHR_ERROR  = 0x07, //!< Received Packet Packet Header Error
	TDME_MIN_STATUS = TDME_SUCCESS,
	TDME_MAX_STATUS = TDME_PHR_ERROR
};

/** TDME Test Modes */
enum tdme_test_mode
{
	TDME_TEST_OFF = 0x00, //!< All Test Modes are disabled (default)
	/** Idle Test Mode. Test mode is enabled but transmitter and receiver
	 *  are off */
	TDME_TEST_IDLE = 0x01,
	TDME_TEST_TX   = 0x02, //!< Transmit Test Mode
	TDME_TEST_RX   = 0x03, //!< Receive Test Mode
	/** Energy Detect Test Mode (Energy Sniffer) */
	TDME_TEST_ED = 0x04,
	/** LO Test Mode 1 (Tx/Rx with no Modulation on 802.15.4 Channel, PA
	 *  enabled) */
	TDME_TEST_LO_1 = 0x05,
	/** LO Test Mode 2 (VCO Open Loop / Initialisation, PA enabled) */
	TDME_TEST_LO_2 = 0x06,
	/** LO Test Mode 3 (Locking Test, PA disabled) */
	TDME_TEST_LO_3    = 0x07,
	TDME_MIN_TESTMODE = TDME_TEST_OFF,
	TDME_MAX_TESTMODE = TDME_TEST_LO_3
};

/* Tx Packet PHY Payload Data Type */
enum tdme_payload_type
{
	TDME_TXD_RANDOM = 0x00, //!< Random Data, internally generated
	/** Sequence Number in 1st Byte and all other Data Random, internally
	 *  generated */
	TDME_TXD_SEQRANDOM = 0x01,
	/** Count (Increment) Data, internally generated */
	TDME_TXD_COUNT = 0x02,
	/** Data external and appended to TDME-TXPKT Request */
	TDME_TXD_APPENDED = 0x03,
	TDME_MIN_TXD      = TDME_TXD_RANDOM,
	TDME_MAX_TXD      = TDME_TXD_APPENDED
};

/** TDME Attribute IDs */
enum tdme_attribute
{
	TDME_CHANNEL   = 0x00, //!< IEEE802.15.4 Channel Selection
	TDME_TX_CONFIG = 0x01, //!< Transmit (Tx) Test Configuration
	TDME_ED_CONFIG = 0x02, //!< Energy Detect Configuration
	TDME_RX_CONFIG = 0x03, //!< Receiver Test Configuration
	/** LO Test 1 Configuration (Tx/Rx with no Modulation on 802.15.4 Channel) */
	TDME_LO_1_CONFIG = 0x04,
	/** LO Test 2 Configuration (VCO Open Loop / Initialisation) */
	TDME_LO_2_CONFIG   = 0x05,
	TDME_ATM_CONFIG    = 0x06, //!< Analog Test Bus Configuration
	TDME_MPW2_OVWR     = 0x07,
	TDME_MIN_ATTRIBUTE = TDME_CHANNEL,
	TDME_MAX_ATTRIBUTE = TDME_MPW2_OVWR
};
/**@}*/

#endif // HWME_TDME_H
