/**
 * @file mac_messages.h
 * @brief Definitions relating to API messages.
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

#ifndef MAC_MESSAGES_H
#define MAC_MESSAGES_H

#include <stdint.h>

#include "ca821x_config.h"
#include "hwme_tdme.h"
#include "ieee_802_15_4.h"

#define MAX_HWME_ATTRIBUTE_SIZE 16 /**< Longest hwme attribute in octets */
#define MAX_TDME_ATTRIBUTE_SIZE 2  /**< Longest tdme attribute in octets */

/******************************************************************************/
/****** Structures used in the MAC MCPS/MLME Procedure Definitions       ******/
/******************************************************************************/

/** Contains full addressing information for a node */
struct FullAddr
{
	/** Clarifies the contents of \ref Address (empty, short, extended)*/
	uint8_t AddressMode;
	uint8_t PANId[2];
	/** Holds either short or extended address (little-endian) */
	uint8_t Address[8];
};

/** Holds either short or extended address (little-endian) */
union MacAddr
{
	uint16_t ShortAddress;
	uint8_t  IEEEAddress[8];
};

/** Security specification to be applied to MAC frames */
struct SecSpec
{
	/** Specifies level of authentication and encryption */
	uint8_t SecurityLevel;
	/** How the key is to be retrieved */
	uint8_t KeyIdMode;
	/** Source part of key lookup data (commonly addressing info) */
	uint8_t KeySource[8];
	/** Index part of key lookup data */
	uint8_t KeyIndex;
};

/** Describes a discovered PAN */
struct PanDescriptor
{
	struct FullAddr Coord;             /**< PAN coordinator addressing information */
	uint8_t         LogicalChannel;    /**< Current operating channel */
	uint8_t         SuperframeSpec[2]; /**< Superframe specification */
	/** True if the beacon originator is a PAN coordinator accepting
	  * guaranteed time slot requests*/
	uint8_t GTSPermit;
	uint8_t LinkQuality; /**< LQI of the received beacon */
	/** Time at which the beacon was received, in symbols*/
	uint8_t TimeStamp[4];
	/** Security processing status of the beacon frame */
	uint8_t SecurityFailure;
	/** Security specification of the beacon */
	struct SecSpec Security;
};

struct PendAddrSpec
{
	uint8_t ShortAddrCount : 3;
	uint8_t /* Reserved */ : 1;
	uint8_t ExtAddrCount : 3;
	uint8_t /* Reserved */ : 1;
};

/***************************************************************************/ /**
 * \defgroup DownPSets Parameter set definitions (Downstream)
 ************************************************************************** @{*/

// MCPS

/** MCPS_DATA_request parameter set */
struct MCPS_DATA_request_pset
{
	uint8_t         SrcAddrMode;         /**< Source addressing mode */
	struct FullAddr Dst;                 /**< Destination addressing information */
	uint8_t         MsduLength;          /**< Length of Data */
	uint8_t         MsduHandle;          /**< Handle of Data */
	uint8_t         TxOptions;           /**< Tx options bit field */
	uint8_t         Msdu[MAX_DATA_SIZE]; /**< Data */
};

// PCPS

/** PCPS_DATA_request parameter set */
struct PCPS_DATA_request_pset
{
	uint8_t PsduHandle;              /**< Handle to identify PCPS request */
	uint8_t TxOpts;                  /**< TxOpts for data request (such as indirect sending) */
	uint8_t PsduLength;              /**< Length of the PSDU */
	uint8_t Psdu[aMaxPHYPacketSize]; /**< PSDU data */
};

// MLME

/** MLME_ASSOCIATE_request parameter set */
struct MLME_ASSOCIATE_request_pset
{
	uint8_t         LogicalChannel; /**< Channel number */
	struct FullAddr Dst;            /**< Destination addressing information */
	/** Bitmap of operational capabilities */
	uint8_t        CapabilityInfo;
	struct SecSpec Security; /**< Security specification */
};

/** MLME_ASSOCIATE_response parameter set */
struct MLME_ASSOCIATE_response_pset
{
	/** IEEE address to give to associating device */
	uint8_t        DeviceAddress[8];
	uint8_t        AssocShortAddress[2];
	uint8_t        Status;
	struct SecSpec Security;
};

/** MLME_DISASSOCIATE_request parameter set */
struct MLME_DISASSOCIATE_request_pset
{
	struct FullAddr DevAddr;
	uint8_t         DisassociateReason;
	uint8_t         TxIndirect;
	struct SecSpec  Security;
};

/** MLME_GET_request parameter set */
struct MLME_GET_request_pset
{
	uint8_t PIBAttribute;
	uint8_t PIBAttributeIndex;
};

/** MLME_ORPHAN_response parameter set */
struct MLME_ORPHAN_response_pset
{
	uint8_t        OrphanAddress[8];
	uint8_t        ShortAddress[2];
	uint8_t        AssociatedMember;
	struct SecSpec Security;
};

/** MLME_POLL_request parameter set */
struct MLME_POLL_request_pset
{
	struct FullAddr CoordAddress;
#if CASCODA_CA_VER == 8210
	uint8_t Interval[2]; /* polling interval in 0.1 seconds res */
	                     /* 0 means poll once */
	                     /* 0xFFFF means stop polling */
#endif
	struct SecSpec Security;
};

/** MLME_RX_ENABLE_request parameter set */
struct MLME_RX_ENABLE_request_pset
{
	uint8_t DeferPermit;
	uint8_t RxOnTime[4];
	uint8_t RxOnDuration[4];
};

/** MLME_SCAN_request parameter set */
struct MLME_SCAN_request_pset
{
	uint8_t        ScanType;
	uint8_t        ScanChannels[4];
	uint8_t        ScanDuration;
	struct SecSpec Security;
};

/** MLME_SET_request parameter set */
struct MLME_SET_request_pset
{
	uint8_t PIBAttribute;
	uint8_t PIBAttributeIndex;
	uint8_t PIBAttributeLength;
	uint8_t PIBAttributeValue[MAX_ATTRIBUTE_SIZE];
};

/** MLME_START_request parameter set */
struct MLME_START_request_pset
{
	uint8_t        PANId[2];
	uint8_t        LogicalChannel;
	uint8_t        BeaconOrder;
	uint8_t        SuperframeOrder;
	uint8_t        PANCoordinator;
	uint8_t        BatteryLifeExtension;
	uint8_t        CoordRealignment;
	struct SecSpec CoordRealignSecurity;
	struct SecSpec BeaconSecurity;
};

// HWME

/** HWME_SET_request parameter set */
struct HWME_SET_request_pset
{
	uint8_t HWAttribute;
	uint8_t HWAttributeLength;
	uint8_t HWAttributeValue[MAX_HWME_ATTRIBUTE_SIZE];
};

/** HWME_GET_request parameter set */
struct HWME_GET_request_pset
{
	uint8_t HWAttribute;
};

/** HWME_HAES_request parameter set */
struct HWME_HAES_request_pset
{
	uint8_t HAESMode;
	uint8_t HAESData[16];
};

// TDME

/** TDME_SETSFR_request parameter set */
struct TDME_SETSFR_request_pset
{
	uint8_t SFRPage;
	uint8_t SFRAddress;
	uint8_t SFRValue;
};

/** TDME_GETSFR_request parameter set */
struct TDME_GETSFR_request_pset
{
	uint8_t SFRPage;
	uint8_t SFRAddress;
};

/** TDME_TESTMODE_request parameter set */
struct TDME_TESTMODE_request_pset
{
	uint8_t TestMode;
};

/** TDME_SET_request parameter set */
struct TDME_SET_request_pset
{
	uint8_t TDAttribute;
	uint8_t TDAttributeLength;
	uint8_t TDAttributeValue[MAX_TDME_ATTRIBUTE_SIZE];
};

/** TDME_TXPKT_request parameter set */
struct TDME_TXPKT_request_pset
{
	uint8_t TestPacketDataType;
	uint8_t TestPacketSequenceNumber;
	uint8_t TestPacketLength;
	uint8_t TestPacketData[128];
};

/** TDME_LOTLK_request parameter set */
struct TDME_LOTLK_request_pset
{
	uint8_t TestChannel;
	uint8_t TestRxTxb;
};

/**@}*/

/***************************************************************************/ /**
 * \defgroup UpPSets Parameter set definitions (Upstream)
 ***************************************************************************@{*/

// MCPS

/** MCPS_DATA_confirm parameter set */
struct MCPS_DATA_confirm_pset
{
	uint8_t MsduHandle;
	uint8_t Status;
	uint8_t TimeStamp[4];
#if CASCODA_CA_VER == 8211
	uint8_t FramePending;
#endif
};

/** MCPS_PURGE_confirm parameter set */
struct MCPS_PURGE_confirm_pset
{
	uint8_t MsduHandle;
	uint8_t Status;
};

/** MCPS_DATA_indication parameter set */
struct MCPS_DATA_indication_pset
{
	struct FullAddr Src;
	struct FullAddr Dst;
	uint8_t         MsduLength;
	uint8_t         MpduLinkQuality;
	uint8_t         DSN;
	uint8_t         TimeStamp[4];
#if CASCODA_CA_VER == 8211
	uint8_t FramePending;
#endif
	uint8_t Msdu[MAX_DATA_SIZE];
};

// PCPS - CA8211 only!
/** PCPS_DATA_indication parameter set */
struct PCPS_DATA_indication_pset
{
	uint8_t CS;                      /**< Carrier sense value of received frame*/
	uint8_t ED;                      /**< Energy detect value of received frame */
	uint8_t PsduLength;              /**< Length of received PSDU */
	uint8_t Psdu[aMaxPHYPacketSize]; /**< Received PSDU */
};

/** PCPS_DATA_confirm parameter set */
struct PCPS_DATA_confirm_pset
{
	uint8_t PsduHandle;   /**< PSDU handle identifying the PSDU request */
	uint8_t Status;       /**< Status of the PSDU Data Request */
	uint8_t FramePending; /**< Value of 'Frame Pending' on the ack that was received (if any) */
};

// MLME

/** MLME_ASSOCIATE_indication parameter set */
struct MLME_ASSOCIATE_indication_pset
{
	uint8_t        DeviceAddress[8];
	uint8_t        CapabilityInformation;
	struct SecSpec Security;
};

/** MLME_ASSOCIATE_confirm parameter set */
struct MLME_ASSOCIATE_confirm_pset
{
	uint8_t        AssocShortAddress[2];
	uint8_t        Status;
	struct SecSpec Security;
};

/** MLME_DISASSOCIATE_confirm parameter set */
struct MLME_DISASSOCIATE_confirm_pset
{
	uint8_t         Status;
	struct FullAddr Address;
};

/** MLME_DISASSOCIATE_indication parameter set */
struct MLME_DISASSOCIATE_indication_pset
{
	uint8_t        DevAddr[8];
	uint8_t        Reason;
	struct SecSpec Security;
};

/** MLME_BEACON_NOTIFY_indication parameter set */
struct MLME_BEACON_NOTIFY_indication_pset
{
	uint8_t              BSN;
	struct PanDescriptor PanDescriptor; /* variable size and so following
                                         fields have to be dealt with
                                         separately */
	                                    /* struct PendAddrSpec  PendAddrSpec */
	                                    /* variable             Address List  */
	                                    /* variable             Beacon payload */
};

/** MLME_GET_confirm parameter set */
struct MLME_GET_confirm_pset
{
	uint8_t Status;
	uint8_t PIBAttribute;
	uint8_t PIBAttributeIndex;
	uint8_t PIBAttributeLength;
	uint8_t PIBAttributeValue[MAX_ATTRIBUTE_SIZE];
};

#define MLME_GET_CONFIRM_BASE_SIZE (sizeof(struct MLME_GET_confirm_pset) - MAX_ATTRIBUTE_SIZE)

/** Default size of scan results list */
#define DEFAULT_RESULT_LIST_SIZE (16)
/** MLME_SCAN_confirm parameter set */
struct MLME_SCAN_confirm_pset
{
	uint8_t Status;
	uint8_t ScanType;
	uint8_t UnscannedChannels[4];
	uint8_t ResultListSize;
	uint8_t ResultList[DEFAULT_RESULT_LIST_SIZE];
};

/** MLME_COMM_STATUS_indication parameter set */
struct MLME_COMM_STATUS_indication_pset
{
	uint8_t        PANId[2];
	uint8_t        SrcAddrMode;
	uint8_t        SrcAddr[8];
	uint8_t        DstAddrMode;
	uint8_t        DstAddr[8];
	uint8_t        Status;
	struct SecSpec Security;
};

/** MLME_ORPHAN_indication parameter set */
struct MLME_ORPHAN_indication_pset
{
	uint8_t        OrphanAddr[8];
	struct SecSpec Security;
};

/** MLME_SYNC_LOSS_indication parameter set */
struct MLME_SYNC_LOSS_indication_pset
{
	uint8_t        LossReason;
	uint8_t        PANId[2];
	uint8_t        LogicalChannel;
	struct SecSpec Security;
};

struct MLME_POLL_indication_pset
{
	struct FullAddr Src;
	struct FullAddr Dst;
	uint8_t         LQI;
	uint8_t         DSN;
	struct SecSpec  Security;
};

// HWME

/** HWME_SET_confirm parameter set */
struct HWME_SET_confirm_pset
{
	uint8_t Status;
	uint8_t HWAttribute;
};

/** HWME_GET_confirm parameter set */
struct HWME_GET_confirm_pset
{
	uint8_t Status;
	uint8_t HWAttribute;
	uint8_t HWAttributeLength;
	uint8_t HWAttributeValue[MAX_HWME_ATTRIBUTE_SIZE];
};

/** HWME_HAES_confirm parameter set */
struct HWME_HAES_confirm_pset
{
	uint8_t Status;
	uint8_t HAESData[16];
};

/** HWME_WAKEUP_indication parameter set */
struct HWME_WAKEUP_indication_pset
{
	uint8_t WakeUpCondition;
};

// TDME

/** TDME_SETSFR_confirm parameter set */
struct TDME_SETSFR_confirm_pset
{
	uint8_t Status;
	uint8_t SFRPage;
	uint8_t SFRAddress;
};

/** TDME_GETSFR_confirm parameter set */
struct TDME_GETSFR_confirm_pset
{
	uint8_t Status;
	uint8_t SFRPage;
	uint8_t SFRAddress;
	uint8_t SFRValue;
};

/** TDME_TESTMODE_confirm parameter set */
struct TDME_TESTMODE_confirm_pset
{
	uint8_t Status;
	uint8_t TestMode;
};

/** TDME_SET_confirm parameter set */
struct TDME_SET_confirm_pset
{
	uint8_t Status;
	uint8_t TDAttribute;
};

/** TDME_TXPKT_confirm parameter set */
struct TDME_TXPKT_confirm_pset
{
	uint8_t Status;
	uint8_t TestPacketSequenceNumber;
	uint8_t TestPacketLength;
	uint8_t TestPacketData[128];
};

/** TDME_RXPKT_indication parameter set */
struct TDME_RXPKT_indication_pset
{
	uint8_t Status;
	uint8_t TestPacketEDValue;
	uint8_t TestPacketCSValue;
	uint8_t TestPacketFoffsValue;
	uint8_t TestPacketLength;
	uint8_t TestPacketData[128];
};

/** TDME_EDDET_indication parameter set */
struct TDME_EDDET_indication_pset
{
	uint8_t TestEDThreshold;
	uint8_t TestEDValue;
	uint8_t TestCSValue;
	uint8_t TestTimeAboveThreshold_us[2];
};

/** TDME_ERROR_indication parameter set */
struct TDME_ERROR_indication_pset
{
	uint8_t ErrorCode;
};

/** TDME_LOTLK_confirm parameter set */
struct TDME_LOTLK_confirm_pset
{
	uint8_t Status;
	uint8_t TestChannel;
	uint8_t TestRxTxb;
	uint8_t TestLOFDACValue;
	uint8_t TestLOAMPValue;
	uint8_t TestLOTXCALValue;
};

/**@}*/

/******************************************************************************/
/****** Security PIB Table Size Definitions                              ******/
/******************************************************************************/
/** Maximum value of macKeyTableEntries */
#define KEY_TABLE_SIZE (4)
/** Maximum value of KeyIdLookupListEntries */
#define LOOKUP_DESC_TABLE_SIZE (5)
/** Maximum value of KeyUsageListEntries */
#define KEY_USAGE_TABLE_SIZE (12)
/** Maximum value of macSecurityLevelTableEntries */
#define SECURITY_LEVEL_TABLE_SIZE (2)

#if CASCODA_CA_VER == 8210
/** Maximum value of KeyDeviceListEntries */
#define KEY_DEVICE_TABLE_SIZE (10)
/** Maximum value of macDeviceTableEntries */
#define DEVICE_TABLE_SIZE (10)
#elif CASCODA_CA_VER == 8211
/** Maximum value of KeyDeviceListEntries */
#define KEY_DEVICE_TABLE_SIZE (32)
/** Maximum value of macDeviceTableEntries */
#define DEVICE_TABLE_SIZE (32)
#elif
#error "Security table sizes undefined"
#endif

/***************************************************************************/ /**
 * \defgroup SecPIBStructs Security PIB attribute structures
 ***************************************************************************@{*/

struct M_KeyIdLookupDesc
{
	uint8_t LookupData[9];
	uint8_t LookupDataSizeCode;
};

struct M_DeviceDescriptor
{
	uint8_t PANId[2];
	uint8_t ShortAddress[2];
	uint8_t ExtAddress[8];
	uint8_t FrameCounter[4];
	uint8_t Exempt;
};

struct M_SecurityLevelDescriptor
{
	uint8_t FrameType;
	uint8_t CommandFrameIdentifier;
	uint8_t SecurityMinimum;
	uint8_t DeviceOverrideSecurityMinimum;
};

struct M_KeyDeviceDesc
{
	// uint8_t      DeviceDescriptorHandle : 6;
	// uint8_t      UniqueDevice : 1;
	// uint8_t      Blacklisted : 1;
	uint8_t Flags;
};
/* Masks for KeyDeviceDesc Flags*/
/** Key Device Descriptor handle mask */
#define KDD_DeviceDescHandleMask (0x3F)
#if CASCODA_CA_VER == 8211
/** Key Device Descriptor nonstandard 'is new' key-device pair */
#define KDD_NewMask (0x20)
#endif
/** Key Device Descriptor is unique device mask */
#define KDD_UniqueDeviceMask (0x40)
/** Key Device Descriptor is blacklisted mask */
#define KDD_BlacklistedMask (0x80)

struct M_KeyUsageDesc
{
	// uint8_t      FrameType : 2;
	// uint8_t      CommandFrameIdentifier : 4;
	uint8_t Flags;
};
/* Masks for KeyUsageDesc Flags*/
/** Key Usage Descriptor frame type mask */
#define KUD_FrameTypeMask (0x03)
/** Key Usage Descriptor command frame identifier mask */
#define KUD_CommandFrameIdentifierMask (0xF0)
/** Key Usage Descriptor command frame identifier offset shift */
#define KUD_CommandFrameIdentifierShift (4)

struct M_KeyTableEntryFixed
{
	uint8_t KeyIdLookupListEntries;
	uint8_t KeyDeviceListEntries;
	uint8_t KeyUsageListEntries;
	uint8_t Key[16];
};

struct M_KeyDescriptor
{
	struct M_KeyTableEntryFixed Fixed;
	struct M_KeyIdLookupDesc    KeyIdLookupList[LOOKUP_DESC_TABLE_SIZE];
	struct M_KeyDeviceDesc      KeyDeviceList[KEY_DEVICE_TABLE_SIZE];
	struct M_KeyUsageDesc       KeyUsageList[SECURITY_LEVEL_TABLE_SIZE];
};

/**@}*/

/** Message ID codes in SPI commands (Downstream) */
enum msg_id_code_down
{
	MCPS_DATA_REQUEST         = 0x00,
	MCPS_PURGE_REQUEST        = 0x01,
	MLME_ASSOCIATE_REQUEST    = 0x02,
	MLME_ASSOCIATE_RESPONSE   = 0x03,
	MLME_DISASSOCIATE_REQUEST = 0x04,
	MLME_GET_REQUEST          = 0x05,
	MLME_ORPHAN_RESPONSE      = 0x06,
	MLME_RESET_REQUEST        = 0x07,
	MLME_RX_ENABLE_REQUEST    = 0x08,
	MLME_SCAN_REQUEST         = 0x09,
	MLME_SET_REQUEST          = 0x0A,
	MLME_START_REQUEST        = 0x0B,
	MLME_SYNC_REQUEST         = 0x0C,
	MLME_POLL_REQUEST         = 0x0D,
	HWME_SET_REQUEST          = 0x0E,
	HWME_GET_REQUEST          = 0x0F,
	HWME_HAES_REQUEST         = 0x10,
	TDME_SETSFR_REQUEST       = 0x11,
	TDME_GETSFR_REQUEST       = 0x12,
	TDME_TESTMODE_REQUEST     = 0x13,
	TDME_SET_REQUEST          = 0x14,
	TDME_TXPKT_REQUEST        = 0x15,
	TDME_LOTLK_REQUEST        = 0x16
};

/** Message ID codes in SPI commands (Upstream) */
enum msg_id_code_up
{
	MCPS_DATA_INDICATION          = 0x00,
	MCPS_DATA_CONFIRM             = 0x01,
	MCPS_PURGE_CONFIRM            = 0x02,
	MLME_ASSOCIATE_INDICATION     = 0x03,
	MLME_ASSOCIATE_CONFIRM        = 0x04,
	MLME_DISASSOCIATE_INDICATION  = 0x05,
	MLME_DISASSOCIATE_CONFIRM     = 0x06,
	MLME_BEACON_NOTIFY_INDICATION = 0x07,
	MLME_GET_CONFIRM              = 0x08,
	MLME_ORPHAN_INDICATION        = 0x09,
	MLME_RESET_CONFIRM            = 0x0A,
	MLME_RX_ENABLE_CONFIRM        = 0x0B,
	MLME_SCAN_CONFIRM             = 0x0C,
	MLME_COMM_STATUS_INDICATION   = 0x0D,
	MLME_SET_CONFIRM              = 0x0E,
	MLME_START_CONFIRM            = 0x0F,
	MLME_SYNC_LOSS_INDICATION     = 0x10,
	MLME_POLL_CONFIRM             = 0x11,
	MLME_POLL_INDICATION          = 0x11,
	HWME_SET_CONFIRM              = 0x12,
	HWME_GET_CONFIRM              = 0x13,
	HWME_HAES_CONFIRM             = 0x14,
	HWME_WAKEUP_INDICATION        = 0x15,
	TDME_SETSFR_CONFIRM           = 0x17,
	TDME_GETSFR_CONFIRM           = 0x18,
	TDME_TESTMODE_CONFIRM         = 0x19,
	TDME_SET_CONFIRM              = 0x1A,
	TDME_TXPKT_CONFIRM            = 0x1B,
	TDME_RXPKT_INDICATION         = 0x1C,
	TDME_EDDET_INDICATION         = 0x1D,
	TDME_ERROR_INDICATION         = 0x1E,
	TDME_LOTLK_CONFIRM            = 0x1F
};

/***************************************************************************/ /**
 * SPI Message Format Typedef
 ******************************************************************************/
struct MAC_Message
{
	uint8_t CommandId;
	uint8_t Length;
	union
	{
		/* MAC MCPS / MLME */
		struct MCPS_DATA_request_pset             DataReq;
		struct MLME_ASSOCIATE_request_pset        AssocReq;
		struct MLME_ASSOCIATE_response_pset       AssocRsp;
		struct MLME_DISASSOCIATE_request_pset     DisassocReq;
		struct MLME_GET_request_pset              GetReq;
		struct MLME_ORPHAN_response_pset          OrphanRsp;
		struct MLME_POLL_request_pset             PollReq;
		struct MLME_RX_ENABLE_request_pset        RxEnableReq;
		struct MLME_SCAN_request_pset             ScanReq;
		struct MLME_SET_request_pset              SetReq;
		struct MLME_START_request_pset            StartReq;
		struct MCPS_DATA_confirm_pset             DataCnf;
		struct MCPS_PURGE_confirm_pset            PurgeCnf;
		struct MCPS_DATA_indication_pset          DataInd;
		struct MLME_ASSOCIATE_indication_pset     AssocInd;
		struct MLME_ASSOCIATE_confirm_pset        AssocCnf;
		struct MLME_DISASSOCIATE_indication_pset  DisassocInd;
		struct MLME_DISASSOCIATE_confirm_pset     DisassocCnf;
		struct MLME_BEACON_NOTIFY_indication_pset BeaconInd;
		struct MLME_GET_confirm_pset              GetCnf;
		struct MLME_SCAN_confirm_pset             ScanCnf;
		struct MLME_COMM_STATUS_indication_pset   CommStatusInd;
		struct MLME_SYNC_LOSS_indication_pset     SyncLossInd;
		struct MLME_ORPHAN_indication_pset        OrphanInd;
#if CASCODA_CA_VER >= 8211
		struct MLME_POLL_indication_pset PollInd;
		/* PCPS */
		struct PCPS_DATA_request_pset    PhyDataReq;
		struct PCPS_DATA_confirm_pset    PhyDataCnf;
		struct PCPS_DATA_indication_pset PhyDataInd;
#endif
		/* HWME */
		struct HWME_SET_request_pset       HWMESetReq;
		struct HWME_GET_request_pset       HWMEGetReq;
		struct HWME_HAES_request_pset      HWMEHAESReq;
		struct HWME_SET_confirm_pset       HWMESetCnf;
		struct HWME_GET_confirm_pset       HWMEGetCnf;
		struct HWME_HAES_confirm_pset      HWMEHAESCnf;
		struct HWME_WAKEUP_indication_pset HWMEWakeupInd;
		/* TDME */
		struct TDME_SETSFR_request_pset   TDMESetSFRReq;
		struct TDME_GETSFR_request_pset   TDMEGetSFRReq;
		struct TDME_TESTMODE_request_pset TDMETestModeReq;
		struct TDME_SET_request_pset      TDMESetReq;
		struct TDME_TXPKT_request_pset    TDMETxPktReq;
		struct TDME_LOTLK_request_pset    TDMELOTlkReq;
		struct TDME_SETSFR_confirm_pset   TDMESetSFRCnf;
		struct TDME_GETSFR_confirm_pset   TDMEGetSFRCnf;
		struct TDME_TESTMODE_confirm_pset TDMETestModeCnf;
		struct TDME_SET_confirm_pset      TDMESetCnf;
		struct TDME_TXPKT_confirm_pset    TDMETxPktCnf;
		struct TDME_RXPKT_indication_pset TDMERxPktInd;
		struct TDME_EDDET_indication_pset TDMEEDDetInd;
		struct TDME_ERROR_indication_pset TDMEErrorInd;
		struct TDME_LOTLK_confirm_pset    TDMELOTlkCnf;
		/* Common */
		uint8_t u8Param;
		uint8_t Status;
		uint8_t Payload[156];
	} PData;
};

/******************************************************************************/
/****** SPI Command IDs                                                  ******/
/******************************************************************************/
/** Mask to derive the Message ID Code from the Command ID */
#define SPI_MID_MASK (0x1F)
/** Bit indicating a Confirm or Indication from Slave to Master */
#define SPI_S2M (0x20)
/** Bit indicating a Synchronous Message */
#define SPI_SYN (0x40)

/** SPI Command IDs */
enum spi_command_ids
{
	/** Present on SPI when stream is idle - No Data */
	SPI_IDLE = 0xFF,
	/** Present on SPI when buffer full or busy - resend Request */
	SPI_NACK = 0xF0,
	// MAC MCPS
	SPI_MCPS_DATA_REQUEST    = MCPS_DATA_REQUEST,
	SPI_MCPS_PURGE_REQUEST   = MCPS_PURGE_REQUEST + SPI_SYN,
	SPI_MCPS_DATA_INDICATION = MCPS_DATA_INDICATION + SPI_S2M,
	SPI_MCPS_DATA_CONFIRM    = MCPS_DATA_CONFIRM + SPI_S2M,
	SPI_MCPS_PURGE_CONFIRM   = MCPS_PURGE_CONFIRM + SPI_S2M + SPI_SYN,
// MAC PCPS
#if CASCODA_CA_VER >= 8211
	SPI_PCPS_DATA_REQUEST    = 0x07,
	SPI_PCPS_DATA_CONFIRM    = 0x38,
	SPI_PCPS_DATA_INDICATION = 0x28,
#endif //CASCODA_CA_VER >= 8211
	// MAC MLME
	SPI_MLME_ASSOCIATE_REQUEST        = MLME_ASSOCIATE_REQUEST,
	SPI_MLME_ASSOCIATE_RESPONSE       = MLME_ASSOCIATE_RESPONSE,
	SPI_MLME_DISASSOCIATE_REQUEST     = MLME_DISASSOCIATE_REQUEST,
	SPI_MLME_GET_REQUEST              = MLME_GET_REQUEST + SPI_SYN,
	SPI_MLME_ORPHAN_RESPONSE          = MLME_ORPHAN_RESPONSE,
	SPI_MLME_RESET_REQUEST            = MLME_RESET_REQUEST + SPI_SYN,
	SPI_MLME_RX_ENABLE_REQUEST        = MLME_RX_ENABLE_REQUEST + SPI_SYN,
	SPI_MLME_SCAN_REQUEST             = MLME_SCAN_REQUEST,
	SPI_MLME_SET_REQUEST              = MLME_SET_REQUEST + SPI_SYN,
	SPI_MLME_START_REQUEST            = MLME_START_REQUEST + SPI_SYN,
	SPI_MLME_SYNC_REQUEST             = MLME_SYNC_REQUEST,
	SPI_MLME_POLL_REQUEST             = MLME_POLL_REQUEST + SPI_SYN,
	SPI_MLME_ASSOCIATE_INDICATION     = MLME_ASSOCIATE_INDICATION + SPI_S2M,
	SPI_MLME_ASSOCIATE_CONFIRM        = MLME_ASSOCIATE_CONFIRM + SPI_S2M,
	SPI_MLME_DISASSOCIATE_INDICATION  = MLME_DISASSOCIATE_INDICATION + SPI_S2M,
	SPI_MLME_DISASSOCIATE_CONFIRM     = MLME_DISASSOCIATE_CONFIRM + SPI_S2M,
	SPI_MLME_BEACON_NOTIFY_INDICATION = MLME_BEACON_NOTIFY_INDICATION + SPI_S2M,
	SPI_MLME_GET_CONFIRM              = MLME_GET_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_MLME_ORPHAN_INDICATION        = MLME_ORPHAN_INDICATION + SPI_S2M,
	SPI_MLME_RESET_CONFIRM            = MLME_RESET_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_MLME_RX_ENABLE_CONFIRM        = MLME_RX_ENABLE_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_MLME_SCAN_CONFIRM             = MLME_SCAN_CONFIRM + SPI_S2M,
	SPI_MLME_COMM_STATUS_INDICATION   = MLME_COMM_STATUS_INDICATION + SPI_S2M,
	SPI_MLME_SET_CONFIRM              = MLME_SET_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_MLME_START_CONFIRM            = MLME_START_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_MLME_SYNC_LOSS_INDICATION     = MLME_SYNC_LOSS_INDICATION + SPI_S2M,
	SPI_MLME_POLL_CONFIRM             = MLME_POLL_CONFIRM + SPI_S2M + SPI_SYN,
#if CASCODA_CA_VER >= 8211
	SPI_MLME_POLL_INDICATION = MLME_POLL_INDICATION + SPI_S2M,
#endif
	// HWME
	SPI_HWME_SET_REQUEST       = HWME_SET_REQUEST + SPI_SYN,
	SPI_HWME_GET_REQUEST       = HWME_GET_REQUEST + SPI_SYN,
	SPI_HWME_HAES_REQUEST      = HWME_HAES_REQUEST + SPI_SYN,
	SPI_HWME_SET_CONFIRM       = HWME_SET_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_HWME_GET_CONFIRM       = HWME_GET_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_HWME_HAES_CONFIRM      = HWME_HAES_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_HWME_WAKEUP_INDICATION = HWME_WAKEUP_INDICATION + SPI_S2M,
	// TDME
	SPI_TDME_SETSFR_REQUEST   = TDME_SETSFR_REQUEST + SPI_SYN,
	SPI_TDME_GETSFR_REQUEST   = TDME_GETSFR_REQUEST + SPI_SYN,
	SPI_TDME_TESTMODE_REQUEST = TDME_TESTMODE_REQUEST + SPI_SYN,
	SPI_TDME_SET_REQUEST      = TDME_SET_REQUEST + SPI_SYN,
	SPI_TDME_TXPKT_REQUEST    = TDME_TXPKT_REQUEST + SPI_SYN,
	SPI_TDME_LOTLK_REQUEST    = TDME_LOTLK_REQUEST + SPI_SYN,
	SPI_TDME_SETSFR_CONFIRM   = TDME_SETSFR_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_TDME_GETSFR_CONFIRM   = TDME_GETSFR_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_TDME_TESTMODE_CONFIRM = TDME_TESTMODE_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_TDME_SET_CONFIRM      = TDME_SET_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_TDME_TXPKT_CONFIRM    = TDME_TXPKT_CONFIRM + SPI_S2M + SPI_SYN,
	SPI_TDME_RXPKT_INDICATION = TDME_RXPKT_INDICATION + SPI_S2M,
	SPI_TDME_EDDET_INDICATION = TDME_EDDET_INDICATION + SPI_S2M,
	SPI_TDME_ERROR_INDICATION = TDME_ERROR_INDICATION + SPI_S2M,
	SPI_TDME_LOTLK_CONFIRM    = TDME_LOTLK_CONFIRM + SPI_S2M + SPI_SYN
};

#endif // MAC_MESSAGES_H
