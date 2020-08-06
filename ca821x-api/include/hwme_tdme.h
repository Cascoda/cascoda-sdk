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
/**
 * @file
 * @brief Definitions relating to HWME and TDME API messages.
 */
/**
 * @ingroup ca821x-api-struct
 * @defgroup ca821x-api-hwme HWME and TWME Message definitions
 * @brief Data structures and definitions used for HWME and TDME Messages
 *
 * @{
 */

#ifndef HWME_TDME_H
#define HWME_TDME_H

#include "ca821x_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HWME_ATTRIBUTE_SIZE 16
#define MAX_TDME_ATTRIBUTE_SIZE 2

/** HWME Status Codes */
enum hwme_status
{
	HWME_SUCCESS   = 0x00, //!< The requested Primitive has been executed successfully
	HWME_UNKNOWN   = 0x01, //!< Unknown HWME Attribute or Parameter
	HWME_INVALID   = 0x02, //!< Invalid HWME Attribute Value or Parameter Value
	HWME_NO_ACCESS = 0x03, //!< The requested Attribute cannot currently be accessed
};

/** Potential values for HWME_WAKEUP_indication's WakeUpCondition parameter */
enum hwme_wakeup_condition
{
	HWME_WAKEUP_POWERUP    = 0x00, //!< Transceiver woken up from Power Up / System Reset
	HWME_WAKEUP_WDT        = 0x01, //!< Watchdog Timer Time-Out
	HWME_WAKEUP_POFF_SLT   = 0x02, //!< Transceiver woken up from Power-Off by Sleep Timer Time-Out
	HWME_WAKEUP_POFF_GPIO  = 0x03, //!< Transceiver woken up from Power-Off by GPIO Activity
	HWME_WAKEUP_STBY_SLT   = 0x04, //!< Transceiver woken up from Standby by Sleep Timer Time-Out
	HWME_WAKEUP_STBY_GPIO  = 0x05, //!< Transceiver woken up from Standby by GPIO Activity
	HWME_WAKEUP_ACTIVE_SLT = 0x06, //!< Sleep-Timer Time-Out in Active Mode
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
	HWME_POWERCON    = 0x00, //!< Power Saving Mode Control
	HWME_CHIPID      = 0x01, //!< Product ID and Version Number
	HWME_TXPOWER     = 0x02, //!< Transmit Power Setting
	HWME_CCAMODE     = 0x03, //!< Clear Channel Assessment Mode, according to 802.15.4 Section 6.9.9
	HWME_EDTHRESHOLD = 0x04, //!< Energy Detect (ED) Threshold for CCA
	HWME_CSTHRESHOLD = 0x05, //!< Carrier Sense (CS) Threshold for CCA
	HWME_EDVALUE     = 0x06, //!< Energy Detect (ED) Value of current channel
	HWME_CSVALUE     = 0x07, //!< Carrier Sense (CS) Value of current channel
	HWME_EDVALLP     = 0x08, //!< Energy Detect (ED) Value of last received Packet
	HWME_CSVALLP     = 0x09, //!< Carrier Sense (CS) Value of last received Packet
	HWME_FREQOFFS    = 0x0A, //!< Frequency Offset of last received Packet
	HWME_MACTIMER    = 0x0B, //!< MAC Symbol Timer Value
	HWME_RANDOMNUM   = 0x0C, //!< Random Number Value
	HWME_TEMPERATURE = 0x0D, //!< Temperature Sensor Measurement Value
	HWME_HSKEY       = 0x0E, //!< Host Security Key for AES-128 Encryption or Decryption
	HWME_SYSCLKOUT   = 0x0F, //!< System Clock Output to a specific GPIO
	HWME_LQIMODE     = 0x10, //!< LQI Reporting Mode for Data Indications and PanDescriptors
	HWME_LQILIMIT    = 0x11, //!< LQI minimal value to accept a received MAC frame
#if CASCODA_CA_VER == 8211
	HWME_RXMODE       = 0x12, //!< RX_MODE: 0 = MCPS, 1=PCPS, 2=PCPS except data polls
	HWME_POLLINDMODE  = 0x13, //!< Poll Ind Mode: 0=No indications, 1=Always indicate, 2=indicate if no data confirm
	HWME_ENHANCEDFP   = 0x14, //!< Use the 'Frame Pending' flag in data frames as informative for the child
	HWME_MAXDIRECTS   = 0x15, //!< Maximum number of direct frames queued
	HWME_MAXINDIRECTS = 0x16, //!< Maximum number of indirect frames that can be queued
	HWME_IMAGINARYINDIRECT = 0x17, //!< 0=Normal Indirect behaviour, 1=Imaginary mode, don't send indirect frames
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
	TDME_SUCCESS   = 0x00, //!< The requested Primitive has been executed successfully
	TDME_UNKNOWN   = 0x01, //!< Unknown TDME Value or Parameter
	TDME_INVALID   = 0x02, //!< Invalid TDME Value or Parameter Value
	TDME_NO_ACCESS = 0x03, //!< The requested Value cannot currently be accessed
	TDME_LO_ERROR  = 0x04, //!< LO Locking Error
	TDME_FCS_ERROR = 0x05, //!< Received Packet Frame Check Sequence (CRC) Error
	TDME_SHR_ERROR = 0x06, //!< Received Packet Synchronisation Header Error
	TDME_PHR_ERROR = 0x07, //!< Received Packet Packet Header Error
};

/** TDME Test Modes */
enum tdme_test_mode
{
	TDME_TEST_OFF  = 0x00, //!< All Test Modes are disabled (default)
	TDME_TEST_IDLE = 0x01, //!< Idle Test Mode. Test mode is enabled but transmitter and receiver are off
	TDME_TEST_TX   = 0x02, //!< Transmit Test Mode
	TDME_TEST_RX   = 0x03, //!< Receive Test Mode
	TDME_TEST_ED   = 0x04, //!< Energy Detect Test Mode (Energy Sniffer)
	TDME_TEST_LO_1 = 0x05, //!< LO Test Mode 1 (Tx/Rx with no Modulation on 802.15.4 Channel, PA enabled)
	TDME_TEST_LO_2 = 0x06, //!< LO Test Mode 2 (VCO Open Loop / Initialisation, PA enabled)
	TDME_TEST_LO_3 = 0x07, //!< LO Test Mode 3 (Locking Test, PA disabled)
};

/* Tx Packet PHY Payload Data Type */
enum tdme_payload_type
{
	TDME_TXD_RANDOM    = 0x00, //!< Random Data, internally generated
	TDME_TXD_SEQRANDOM = 0x01, //!< Sequence Number in 1st Byte and all other Data Random, internally generated
	TDME_TXD_COUNT     = 0x02, //!< Count (Increment) Data, internally generated
	TDME_TXD_APPENDED  = 0x03, //!< Data external and appended to TDME-TXPKT Request
};

/** TDME Attribute IDs */
enum tdme_attribute
{
	TDME_CHANNEL     = 0x00, //!< IEEE802.15.4 Channel Selection
	TDME_TX_CONFIG   = 0x01, //!< Transmit (Tx) Test Configuration
	TDME_ED_CONFIG   = 0x02, //!< Energy Detect Configuration
	TDME_RX_CONFIG   = 0x03, //!< Receiver Test Configuration
	TDME_LO_1_CONFIG = 0x04, //!< LO Test 1 Configuration (Tx/Rx with no Modulation on 802.15.4 Channel)
	TDME_LO_2_CONFIG = 0x05, //!< LO Test 2 Configuration (VCO Open Loop / Initialisation)
	TDME_ATM_CONFIG  = 0x06, //!< Analog Test Bus Configuration
	TDME_MPW2_OVWR   = 0x07,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // HWME_TDME_H
