/**
 * @file ieee_802_15_4.h
 * @brief 802.15.4 specific definitions (status codes, attributes etc).
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

#ifndef IEEE_802_15_4_H
#define IEEE_802_15_4_H

/******************************************************************************/
/****** MAC Frame Control Field Definitions                              ******/
/******************************************************************************/
/** Frame control frame type mask */
#define MAC_FC_FT_MASK (0x0003)
/** Frame control beacon frame type */
#define MAC_FC_FT_BEACON (0x0000)
/** Frame control data frame type */
#define MAC_FC_FT_DATA (0x0001)
/** Frame control acknowledgement frame type */
#define MAC_FC_FT_ACK (0x0002)
/** Frame control command frame type */
#define MAC_FC_FT_COMMAND (0x0003)

/** Frame control security enabled bit */
#define MAC_FC_SEC_ENA (0x0008)
/** Frame control frame pending bit */
#define MAC_FC_FP (0x0010)
/** Frame control acknowledgement requested bit */
#define MAC_FC_ACK_REQ (0x0020)
/** Frame control pan id compression bit */
#define MAC_FC_PAN_COMP (0x0040)
/** Offset for frame control short destination addressing mode */
#define MAC_FC_DAM_SHORT (MAC_MODE_SHORT_ADDR << 10)
/** Offset for frame control extended destination addressing mode */
#define MAC_FC_DAM_LONG (MAC_MODE_LONG_ADDR << 10)
/** Frame control 2006 version code */
#define MAC_FC_VER2006 (0x1000)
/** Frame control version code getter */
#define MAC_FC_VER(fc) ((fc >> 12) & 3)
/** Offset for frame control short source addressing mode */
#define MAC_FC_SAM_SHORT (MAC_MODE_SHORT_ADDR << 14)
/** Offset for frame control extended source addressing mode */
#define MAC_FC_SAM_LONG (MAC_MODE_LONG_ADDR << 14)
/** Frame control destination addressing mode getter */
#define MAC_FC_DAM(fc) ((fc >> 10) & 3)
/** Frame control source addressing mode getter */
#define MAC_FC_SAM(fc) ((fc >> 14) & 3)

/******************************************************************************/
/****** MAC TxOption Bits Definitions                                    ******/
/******************************************************************************/
#define TXOPT_ACKREQ (0x01)   //!< Request acknowledgement from receiving node
#define TXOPT_GTS (0x02)      //!< Use guaranteed time slot
#define TXOPT_INDIRECT (0x04) //!< Transmit indirectly

/***************************************************************************/ /**
 * \defgroup PMConstants PHY/MAC Constants
 ************************************************************************** @{*/
#define aMaxPHYPacketSize (127)
#define aMaxMACSafePayloadSize (102)
#define aTurnaroundTime (12)
#define aSymbolPeriod_us (16)
#define aNumSuperframeSlots (16)

#define aBaseSlotDuration (60)
#define aBaseSuperframeDuration (aBaseSlotDuration * aNumSuperframeSlots)
#define aMaxBeaconOverhead (75)
#define aMaxBeaconPayloadLength (aMaxPHYPacketSize - aMaxBeaconOverhead)
#define aUnitBackoffPeriod (20)

#define MAX_ATTRIBUTE_SIZE (122)
#define MAX_DATA_SIZE (aMaxPHYPacketSize)

#define M_MinimumChannel (11)
#define M_MaximumChannel (26)
#define M_ValidChannels (0x07FFF800)

#define MAX_FRAME_DURATION (266)
/**@}*/

#define MAC_BROADCAST_ADDRESS (0xFFFF)

/******************************************************************************/
/****** Length Definitions for different Commands                        ******/
/******************************************************************************/
// Data Request includes 1 phylen, 2 fc, 1 dsn, 12 address, 1 payload, 2 fcs
#define BASELEN_DATA_REQUEST (19)

/***************************************************************************/ /**
* \defgroup MACEnums MAC Enumerations
************************************************************************** @{*/
/** MAC Status Codes (see 802.15.4 2006 spec table 78) */
enum mac_status
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
	MAC_SYSTEM_ERROR            = 0xFF
};

/** MAC Address Mode Definitions */
enum mac_addr_mode
{
	MAC_MODE_NO_ADDR    = 0x00, //!< no address
	MAC_MODE_RESERVED   = 0x01, //!< reserved
	MAC_MODE_SHORT_ADDR = 0x02, //!< 16-bit short address
	MAC_MODE_LONG_ADDR  = 0x03  //!< 64-bit extended address
};

/**
 * MAC Address Code Definitions
 *
 * Made from Combinations of Source and Destination Addressing Modes
 */
enum mac_addr_code
{
	/** Invalid addressing parameter combination */
	AC_INVALID = 0xFF,
	/** No dst pan id, no dst address, src pan id, short src address */
	AC_DP0DA0SP2SA2 = 0,
	/** No dst pan id, no dst address, src pan id, extended src address */
	AC_DP0DA0SP2SA8 = 1,
	/** Dst pan id, short dst address, no src pan id, no src address */
	AC_DP2DA2SP0SA0 = 2,
	/** Dst pan id, extended dst address, no src pan id, no src address */
	AC_DP2DA8SP0SA0 = 3,
	/** Dst pan id, short dst address, no src pan id, short src address */
	AC_DP2DA2SP0SA2 = 4,
	/** Dst pan id, extended dst address, no src pan id, short src address */
	AC_DP2DA8SP0SA2 = 5,
	/** Dst pan id, short dst address, no src pan id, extended src address */
	AC_DP2DA2SP0SA8 = 6,
	/** Dst pan id, extended dst address, no src pan id, extended src address */
	AC_DP2DA8SP0SA8 = 7,
	/** Dst pan id, short dst address, src pan id, short src address */
	AC_DP2DA2SP2SA2 = 8,
	/** Dst pan id, extended dst address, src pan id, short src address */
	AC_DP2DA8SP2SA2 = 9,
	/** Dst pan id, short dst address, src pan id, extended src address */
	AC_DP2DA2SP2SA8 = 10,
	/** Dst pan id, extended dst address, src pan id, extended src address */
	AC_DP2DA8SP2SA8 = 11
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

/** Reason for disassociation (see 802.15.4-2006 7.3.3.2) */
enum mac_disassociate_reason
{
	DISASSOC_REASON_EVICT, //!< The coordinator wishes the device to leave the PAN
	DISASSOC_REASON_LEAVE  //!< The device wishes to leave the PAN
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

	macPibFirst = macAckWaitDuration,
	macPibLast  = macSecurityEnabled,

	macKeyTable                  = 0x71,
	macKeyTableEntries           = 0x72,
	macDeviceTable               = 0x73,
	macDeviceTableEntries        = 0x74,
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
};
/**@}*/

#endif // IEEE_802_15_4_H
