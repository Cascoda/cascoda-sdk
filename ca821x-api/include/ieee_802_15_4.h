/**
 * @file
 * @brief 802.15.4 specific definitions (status codes, attributes etc).
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
/**
 * @file
 * @brief Definitions relating to the IEEE 802.15.4 Specification
 */
/**
 * @ingroup ca821x-api-struct
 * @defgroup ca821x-api-ieee IEEE 802.15.4 definitions
 * @brief Definitions used by the IEEE 802.15.4 Specification
 * @{
 */

#ifndef IEEE_802_15_4_H
#define IEEE_802_15_4_H

#ifdef __cplusplus
extern "C" {
#endif

enum mac_constants
{
	aMaxPHYPacketSize      = 127,
	aMaxMACSafePayloadSize = 102,
	aMaxMACPayloadSize     = aMaxPHYPacketSize - 9,
	aTurnaroundTime        = 12,
	aSymbolPeriod_us       = 16,
	aNumSuperframeSlots    = 16,

	aBaseSlotDuration       = 60,
	aBaseSuperframeDuration = aBaseSlotDuration * aNumSuperframeSlots,
	aMaxBeaconOverhead      = 75,
	aMaxBeaconPayloadLength = aMaxPHYPacketSize - aMaxBeaconOverhead,
	aUnitBackoffPeriod      = 20,

	MAX_ATTRIBUTE_SIZE = 250,
#if CASCODA_CA_VER >= 8212
	MAX_DATA_SIZE = 121,
#else
	MAX_DATA_SIZE                = 114,
#endif // CASCODA_CA_VER >= 8212

	M_MinimumChannel = 11,
	M_MaximumChannel = 26,
	M_ValidChannels  = 0x07FFF800,

	MAX_FRAME_DURATION    = 266,
	MAC_BROADCAST_ADDRESS = 0xFFFF
};

/** MAC Status Codes (see 802.15.4 2006 spec table 78) */
typedef enum mac_status
{
	MAC_SUCCESS                 = 0x00,
	MAC_ERROR                   = 0x01,
	MAC_CANCELLED               = 0x02,
	MAC_READY_FOR_POLL          = 0x03,
	MAC_COUNTER_ERROR           = 0xDB,
	MAC_IMPROPER_KEY_TYPE       = 0xDC,
	MAC_IMPROPER_SECURITY_LEVEL = 0xDD,
	MAC_UNSUPPORTED_LEGACY      = 0xDE,
	MAC_UNSUPPORTED_SECURITY    = 0xDF,
	MAC_BEACON_LOST             = 0xE0,
	MAC_CHANNEL_ACCESS_FAILURE  = 0xE1,
	MAC_DENIED                  = 0xE2,
	MAC_DISABLE_TRX_FAILURE     = 0xE3,
	MAC_SECURITY_ERROR          = 0xE4,
	MAC_FRAME_TOO_LONG          = 0xE5,
	MAC_INVALID_GTS             = 0xE6,
	MAC_INVALID_HANDLE          = 0xE7,
	MAC_INVALID_PARAMETER       = 0xE8,
	MAC_NO_ACK                  = 0xE9,
	MAC_NO_BEACON               = 0xEA,
	MAC_NO_DATA                 = 0xEB,
	MAC_NO_SHORT_ADDRESS        = 0xEC,
	MAC_OUT_OF_CAP              = 0xED,
	MAC_PAN_ID_CONFLICT         = 0xEE,
	MAC_REALIGNMENT             = 0xEF,
	MAC_TRANSACTION_EXPIRED     = 0xF0,
	MAC_TRANSACTION_OVERFLOW    = 0xF1,
	MAC_TX_ACTIVE               = 0xF2,
	MAC_UNAVAILABLE_KEY         = 0xF3,
	MAC_UNSUPPORTED_ATTRIBUTE   = 0xF4,
	MAC_INVALID_ADDRESS         = 0xF5,
	MAC_ON_TIME_TOO_LONG        = 0xF6,
	MAC_PAST_TIME               = 0xF7,
	MAC_TRACKING_OFF            = 0xF8,
	MAC_INVALID_INDEX           = 0xF9,
	MAC_LIMIT_REACHED           = 0xFA,
	MAC_READ_ONLY               = 0xFB,
	MAC_SCAN_IN_PROGRESS        = 0xFC,
	MAC_SUPERFRAME_OVERLAP      = 0xFD,
	MAC_SYSTEM_ERROR            = 0xFF,
#if CASCODA_CA_VER >= 8212
	MAC_UNAVAILABLE_DEVICE = 0xDA,
	MAC_SCHEDULING_FAILURE = 0xFE,
#endif // CASCODA_CA_VER >= 8212
} ca_mac_status;

/** MAC Address Mode Definitions */
enum mac_addr_mode
{
	MAC_MODE_NO_ADDR    = 0x00, //!< no address
	MAC_MODE_RESERVED   = 0x01, //!< reserved
	MAC_MODE_SHORT_ADDR = 0x02, //!< 16-bit short address
	MAC_MODE_LONG_ADDR  = 0x03  //!< 64-bit extended address
};

/** Enumeration of different MAC TxOptions */
enum MAC_TXOPT
{
#if CASCODA_CA_VER >= 8212
	TXOPT0_ACKREQ             = 0x01, //!< Set the AR bit in the mac header
	TXOPT0_GTS                = 0x02, //!< Unused, can be removed.
	TXOPT0_INDIRECT           = 0x04, //!< Send the frame indirectly
	TXOPT0_SCH                = 0x08, //!< Send a scheduled transmission at a specific timestamp
	TXOPT0_SPECIFIC_CHANNEL   = 0x10, //!< Transmit the frame on a specific channel
	TXOPT0_NS_SECURE_INDIRECT = 0x20, //!< Only allow this indirect frame to be extracted by a secured data poll
	TXOPT0_NS_FPEND           = 0x40, //!< Set the "Frame Pending" bit in the mac header
	TXOPT0_NS_THREADNONCE     = 0x80, //!< Use the Thread Key ID Mode 2 nonce for security (not secure)

	// Tx options for v2015 only:
	TXOPT1_2015_FRAME = 0x01, //!< Send a version 2015 frame
	TXOPT1_SN_SUPP    = 0x02, //!< Suppress the sequence number in the frame
	TXOPT1_PANID_SUPP = 0x04, //!< Suppress the PAN ID in the frame
	TXOPT1_SENDMPF    = 0x08, //!< Unused, can be removed
	TXOPT1_CSLIE      = 0x10, //!< Include a real-time updated CSL IE in the frame
#else
	TXOPT_ACKREQ                 = 0x01, //!< Request acknowledgement from receiving node
	TXOPT_GTS                    = 0x02, //!< Use guaranteed time slot (Not supported)
	TXOPT_INDIRECT               = 0x04, //!< Transmit indirectly
	TXOPT_NS_SECURE_INDIRECT     = 0x20, //!< Nonstandard, only send the indirect message in reply to a secure poll
	TXOPT_NS_FPEND               = 0x40, //!< Nonstandard, set the frame pending bit on the outgoing frame
	TXOPT_NS_THREADNONCE         = 0x80, //!< Nonstandard, use Thread-specific nonce for mode2 frames
#endif // CASCODA_CA_VER >= 8212
};

/**
 * MAC ScanType Parameter for MLME_SCAN Request and Confirm
 *
 * See 802.15.4 2006 spec sections 7.1.11.1 and 7.5.2.1
 */
enum mlme_scan_type
{
	ENERGY_DETECT = 0x00,
	ACTIVE_SCAN   = 0x01,
	PASSIVE_SCAN  = 0x02,
	ORPHAN_SCAN   = 0x03
};

/** Real time translations for MLME-SCAN ScanDuration (per channel) */
enum ca821x_scan_durations
{
	SCAN_DURATION_30MS  = 0,
	SCAN_DURATION_46MS  = 1,
	SCAN_DURATION_77MS  = 2,
	SCAN_DURATION_138MS = 3,
	SCAN_DURATION_261MS = 4,
	SCAN_DURATION_507MS = 5,
	SCAN_DURATION_998MS = 6,
	SCAN_DURATION_2S    = 7,
	SCAN_DURATION_4S    = 8,
	SCAN_DURATION_8S    = 9,
	SCAN_DURATION_16S   = 10,
	SCAN_DURATION_31S   = 11,
	SCAN_DURATION_63S   = 12,
	SCAN_DURATION_126S  = 13,
	SCAN_DURATION_252S  = 14
};

/** Enumeration of different MAC Frame Types */
enum mac_frame_type
{
	MAC_FRAME_TYPE_BEACON  = 0,
	MAC_FRAME_TYPE_DATA    = 1,
	MAC_FRAME_TYPE_ACK     = 2,
	MAC_FRAME_TYPE_COMMAND = 3
};

/** MAC Command Frame Identifiers */
enum mac_cmd_frame_id
{
	CMD_ASSOCIATION_REQ    = 1,
	CMD_ASSOCIATION_RSP    = 2,
	CMD_DISASSOCIATION_NFY = 3,
	CMD_DATA_REQ           = 4,
	CMD_PANID_CONFLICT_NFY = 5,
	CMD_ORPHAN_NFY         = 6,
	CMD_BEACON_REQ         = 7,
	CMD_COORD_REALIGN      = 8,
	CMD_GTS_REQ            = 9
};

/** Association status (see 802.15.4-2006 Table 83) */
enum mac_association_status
{
	ASSOC_STATUS_SUCCESS       = 0x00,
	ASSOC_STATUS_AT_CAPACITY   = 0x01,
	ASSOC_STATUS_ACCESS_DENIED = 0x02,
};

/** Reason for disassociation (see 802.15.4-2006 7.3.3.2) */
enum mac_disassociate_reason
{
	DISASSOC_REASON_EVICT = 1, //!< The coordinator wishes the device to leave the PAN
	DISASSOC_REASON_LEAVE = 2  //!< The device wishes to leave the PAN
};

/** PHY/MAC PIB Attribute Enumerations */
enum pib_attribute
{
	phyCurrentChannel   = 0x00,
	phyChannelsSupport  = 0x01,
	phyTransmitPower    = 0x02,
	phyCCAMode          = 0x03,
	phyCurrentPage      = 0x04,
	phyMaxFrameDuration = 0x05,
	phySHRDuration      = 0x06,
	phySymbolsPerOctet  = 0x07,

	phyPibFirst = phyCurrentChannel,
	phyPibLast  = phySymbolsPerOctet,

	macAckWaitDuration            = 0x40,
	macAssociationPermit          = 0x41,
	macAutoRequest                = 0x42,
	macBattLifeExt                = 0x43,
	macBattLifeExtPeriods         = 0x44,
	macBeaconPayload              = 0x45,
	macBeaconPayloadLength        = 0x46,
	macBeaconOrder                = 0x47,
	macBeaconTxTime               = 0x48,
	macBSN                        = 0x49,
	macCoordExtendedAddress       = 0x4a,
	macCoordShortAddress          = 0x4b,
	macDSN                        = 0x4c,
	macGTSPermit                  = 0x4d,
	macMaxCSMABackoffs            = 0x4e,
	macMinBE                      = 0x4f,
	macPANId                      = 0x50,
	macPromiscuousMode            = 0x51,
	macRxOnWhenIdle               = 0x52,
	macShortAddress               = 0x53,
	macSuperframeOrder            = 0x54,
	macTransactionPersistenceTime = 0x55,
	macAssociatedPANCoord         = 0x56,
	macMaxBE                      = 0x57,
	macMaxFrameTotalWaitTime      = 0x58,
	macMaxFrameRetries            = 0x59,
	macResponseWaitTime           = 0x5A,
	macSyncSymbolOffset           = 0x5B,
	macTimestampSupported         = 0x5C,
	macSecurityEnabled            = 0x5D,
#if CASCODA_CA_VER >= 8211
	macMinLIFSPeriod = 0x5E,
	macMinSIFSPeriod = 0x5F,
#endif // CASCODA_CA_VER >= 8211

#if CASCODA_CA_VER >= 8212
	macTimestamp        = 0x60,
	macCslPeriod        = 0x61,
	macCslMargin        = 0x62,
	macCslNextTimestamp = 0x63,
#endif // CASCODA_CA_VER >= 8212

	macPibFirst = macAckWaitDuration,
#if CASCODA_CA_VER >= 8212
	macPibLast = macCslNextTimestamp,
#elif CASCODA_CA_VER >= 8211
	macPibLast                   = macMinSIFSPeriod,
#endif // CASCODA_CA_VER >= 8212

	macKeyTable           = 0x71,
	macKeyTableEntries    = 0x72,
	macDeviceTable        = 0x73,
	macDeviceTableEntries = 0x74,

#if CASCODA_CA_VER >= 8212
	macKeyLookupTable                     = 0x75,
	macKeyLookupTableEntries              = 0x76,
	macCommandIdSecurityLevelTable        = 0x79,
	macCommandIdSecurityLevelTableEntries = 0x7a,
	macIESecurityLevelTable               = 0x7b,
	macIESecurityLevelTableEntries        = 0x7c,
	macSecurityLevelExemptionTable        = 0x7d,
	macSecurityLevelExemptionTableEntries = 0x7e,
	macFrameCounter                       = 0x7f,
	macAutoRequestSecurityLevel           = 0x80,
	macAutoRequestLookupDataIndex         = 0x81,
	macUseAutoReqForEnhAck                = 0x82,
	macEnhAckIeSec                        = 0x83,
	macIndicateSecurityDroppedFrames      = 0x84,

	macSecPibFirst = macKeyTable,
	macSecPibLast  = macIndicateSecurityDroppedFrames,

	macExtendedAddress = 0xFF /* Non-standard IEEE address */
#else
	macSecurityLevelTable        = 0x75,
	macSecurityLevelTableEntries = 0x76,
	macFrameCounter              = 0x77,
	macAutoRequestSecurityLevel  = 0x78,
	macAutoRequestKeyIdMode      = 0x79,
	macAutoRequestKeySource      = 0x7A,
	macAutoRequestKeyIndex       = 0x7B,
	macDefaultKeySource          = 0x7C,
	macPANCoordExtendedAddress   = 0x7D,
	macPANCoordShortAddress      = 0x7E,

	macSecPibFirst = macKeyTable,
	macSecPibLast  = macPANCoordShortAddress,

	nsIEEEAddress = 0xFF /* Non-standard IEEE address */
#endif // CASCODA_CA_VER >= 8212
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // IEEE_802_15_4_H
