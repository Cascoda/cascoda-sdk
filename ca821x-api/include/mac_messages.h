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
 * @brief Definitions relating to MLME and MCPS API messages.
 */
/**
 * @ingroup ca821x-api-struct
 * @defgroup ca821x-api-macm MLME and MCPS Message definitions
 * @brief Data structures and definitions used for MLME and MCPS Messages
 *
 * @{
 */

#ifndef MAC_MESSAGES_H
#define MAC_MESSAGES_H

#include <stdint.h>

#include "ca821x_config.h"
#include "ca821x_error.h"
#include "hwme_tdme.h"
#include "ieee_802_15_4.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HWME_ATTRIBUTE_SIZE 16 /**< Longest hwme attribute in octets */
#define MAX_TDME_ATTRIBUTE_SIZE 2  /**< Longest tdme attribute in octets */

/******************************************************************************/
/****** Structures used in the MAC MCPS/MLME Procedure Definitions       ******/
/******************************************************************************/

/** Contains full addressing information for a node */
struct FullAddr
{
	uint8_t AddressMode; /**< Clarifies the contents of \ref Address (empty, short, extended). See \ref mac_addr_mode.*/
	uint8_t PANId[2];    /**< PanId (little-endian) */
	uint8_t Address[8];  /**< Short or Extended Address, based on AddressMode (little-endian) */
};

/** Contains raw little endian short address */
struct ShortAddr
{
	uint8_t Address[2]; /**< Short Address (little-endian) */
};

/** Contains raw extended address */
struct ExtAddr
{
	uint8_t Address[8]; /**< Extended Address */
};

/** Holds either short or extended address */
struct MacAddr
{
	uint8_t AddressMode; /**< Clarifies the contents of \ref Address (empty, short, extended). See \ref mac_addr_mode.*/
	uint8_t Address[8];  /**< Short or Extended Address, based on AddressMode (little-endian) */
};

/** Security specification to be applied to MAC frames */
struct SecSpec
{
	uint8_t SecurityLevel; /**< Specifies level of authentication and encryption */
	uint8_t KeyIdMode;     /**< How the key is to be retrieved */
	uint8_t KeySource[8];  /**< Source part of key lookup data (commonly addressing info) */
	uint8_t KeyIndex;      /**< Index part of key lookup data */
};

/** Describes a discovered PAN */
struct PanDescriptor
{
	struct FullAddr Coord;             /**< PAN coordinator addressing information */
	uint8_t         LogicalChannel;    /**< Current operating channel */
	uint8_t         SuperframeSpec[2]; /**< Superframe specification */
	uint8_t         GTSPermit;         /**< True if the beacon originator is a PAN coordinator accepting GTS requests*/
	uint8_t         LinkQuality;       /**< LQI of the received beacon */
	uint8_t         TimeStamp[4];      /**< Time at which the beacon was received, in symbols*/
	uint8_t         SecurityFailure;   /**< Security processing status of the beacon frame */
	struct SecSpec  Security;          /**< Security specification of the beacon */
};

// MCPS
#if CASCODA_CA_VER >= 8212
/** MCPS_DATA_request parameter set */
struct MCPS_DATA_request_pset
{
	uint8_t         SrcAddrMode;     /**< Source addressing mode */
	struct FullAddr Dst;             /**< Destination addressing information */
	uint8_t         HeaderIELength;  /**< Length of the Header IE List */
	uint8_t         PayloadIELength; /**< Length of the Payload IE List */
	uint8_t         MsduLength;      /**< Length of Data */
	uint8_t         MsduHandle;      /**< Handle of Data */
	uint8_t         TxOptions[2];    /**< Tx options bit field */
	uint8_t Data[MAX_DATA_SIZE];     /**< SCH params, tx channel, IEs, MSDU and Security are concatenated into Data */
};
#else
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
#endif // CASCODA_CA_VER >= 8212

// PCPS
#if CASCODA_CA_VER >= 8212
/** PCPS_DATA_request parameter set */
struct PCPS_DATA_request_pset
{
	uint8_t PsduHandle; /**< Handle to identify PCPS request */
	uint8_t TxOpts;     /**< TxOpts for data request (such as indirect sending) */
	/* 6 bytes (Optional: SCH timestamp and period) + 1 byte PsduLength + aMaxPHYPacketSize bytes Psdu */
	uint8_t VariableData[7 + aMaxPHYPacketSize];
};
#elif CASCODA_CA_VER == 8211
/** PCPS_DATA_request parameter set */
struct PCPS_DATA_request_pset
{
	uint8_t PsduHandle;              /**< Handle to identify PCPS request */
	uint8_t TxOpts;                  /**< TxOpts for data request (such as indirect sending) */
	uint8_t PsduLength;              /**< Length of the PSDU */
	uint8_t Psdu[aMaxPHYPacketSize]; /**< PSDU data */
};
#endif // CASCODA_CA_VER >= 8212

// MLME

/** MLME_ASSOCIATE_request parameter set */
struct MLME_ASSOCIATE_request_pset
{
	uint8_t         LogicalChannel; /**< Channel number */
	struct FullAddr Dst;            /**< Destination addressing information */
	uint8_t         CapabilityInfo; /**< Bitmap of operational capabilities */
	struct SecSpec  Security;       /**< Security specification */
};

/** MLME_ASSOCIATE_response parameter set */
struct MLME_ASSOCIATE_response_pset
{
	uint8_t        DeviceAddress[8]; /**< IEEE address to give to associating device */
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

/** MLME_GET_confirm parameter set */
struct MLME_GET_confirm_pset
{
	uint8_t Status;
	uint8_t PIBAttribute;
	uint8_t PIBAttributeIndex;
	uint8_t PIBAttributeLength;
	uint8_t PIBAttributeValue[MAX_ATTRIBUTE_SIZE];
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
#endif                   // CASCODA_CA_VER == 8210
#if CASCODA_CA_VER >= 8212
	uint8_t FrameVersion; /* Determines if the Data request frame to be sent is a 2015 frame (FrameVersion set to 1) */
#endif                    // CASCODA_CA_VER >= 8212
	struct SecSpec Security;
};

#if CASCODA_CA_VER >= 8212
struct MLME_POLL_confirm_pset
{
	uint8_t Status;
};
#endif // CASCODA_CA_VER >= 8212

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

/** MLME_SET_confirm parameter set */
struct MLME_SET_confirm_pset
{
	uint8_t Status;
	uint8_t PIBAttribute;
	uint8_t PIBAttributeIndex;
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

// MCPS

/** MCPS_DATA_confirm parameter set */
struct MCPS_DATA_confirm_pset
{
	uint8_t MsduHandle;
	uint8_t Status;
	uint8_t TimeStamp[4];
#if CASCODA_CA_VER >= 8211
	uint8_t FramePending;
#endif // CASCODA_CA_VER >= 8211
#if CASCODA_CA_VER >= 8212
	uint8_t FailCount_NoAck;
	uint8_t FailCount_CsmaCa;
	uint8_t Channel;
#endif // CASCODA_CA_VER >= 8212
};

/** MCPS_PURGE_confirm parameter set */
struct MCPS_PURGE_confirm_pset
{
	uint8_t MsduHandle;
	uint8_t Status;
};

/**
 * MCPS_DATA_indication parameter set
 *
 * See ca821x_api_helper.h to extract the struct SecSpec.
 */
struct MCPS_DATA_indication_pset
{
	struct FullAddr Src;
	struct FullAddr Dst;
#if CASCODA_CA_VER >= 8212
	uint8_t HeaderIELength;
	uint8_t PayloadIELength;
#endif // CASCODA_CA_VER >= 8212
	uint8_t MsduLength;
	uint8_t MpduLinkQuality;
	uint8_t DSN;
	uint8_t TimeStamp[4];
#if CASCODA_CA_VER >= 8211
	uint8_t FramePending;
#endif // CASCODA_CA_VER >= 8212
#if CASCODA_CA_VER >= 8212
	uint8_t Data[MAX_DATA_SIZE]; /* IEs and MSDU and Security are concatenated into Data */
#else
	uint8_t Msdu[MAX_DATA_SIZE]; /* Security is concatenated here */
#endif // CASCODA_CA_VER >= 8212
};

#if CASCODA_CA_VER >= 8212
/** PCPS_DATA_indication parameter set */
struct PCPS_DATA_indication_pset
{
	uint8_t CS;                      /**< Carrier sense value of received frame*/
	uint8_t ED;                      /**< Energy detect value of received frame */
	uint8_t Timestamp[4];            /**< Timestamp the frame was received at */
	uint8_t PsduLength;              /**< Length of received PSDU */
	uint8_t Psdu[aMaxPHYPacketSize]; /**< Received PSDU */
};

/** PCPS_DATA_confirm parameter set */
struct PCPS_DATA_confirm_pset
{
	uint8_t PsduHandle;       /**< PSDU handle identifying the PSDU request */
	uint8_t Status;           /**< Status of the PSDU Data Request */
	uint8_t TimeStamp[4];     /**< Timestamp the frame was transmitted at */
	uint8_t FramePending;     /**< Value of 'Frame Pending' on the ack that was received (if any) */
	uint8_t FailCount_NoAck;  /**< Number of unsuccessful tx attempts due to no reception of acknowledgements */
	uint8_t FailCount_CsmaCa; /**< Number of unsuccessful tx attempts due to back-offs from a busy channel */
};

/** MLME_IE_NOTIFY_indication parameter set */
struct MLME_IE_NOTIFY_indication_pset
{
	struct FullAddr Src;                         /**< Source addressing information */
	struct FullAddr Dst;                         /**< Destination addressing information */
	uint8_t         HeaderIELength;              /**< Length of the Header IE List */
	uint8_t         PayloadIELength;             /**< Length of the Payload IE List */
	uint8_t         FrameType;                   /**< Received frame type (ACK, COMMAND) */
	uint8_t         CommandID;                   /**< Command ID if FrameType is a command */
	uint8_t         VariableData[MAX_DATA_SIZE]; /**< IEs and Security are concatenated here */
};
#elif CASCODA_CA_VER == 8211
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
#endif // CASCODA_CA_VER >= 8212

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
	struct PanDescriptor PanDescriptor; /**< Variable length and so following
	                                         fields have to be dealt with separately,
											  see ca821x_api_helper.h */
	                                    /* u8_t            PendAddrSpec; */
	                                    /* variable        Address List  */
	                                    /* variable        Beacon payload */
};

/** Default size of scan results list */
#define DEFAULT_RESULT_LIST_SIZE (128)

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
#if CASCODA_CA_VER >= 8212
	uint8_t Timestamp[4];
#endif // CASCODA_CA_VER >= 8212
	struct SecSpec Security;
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

/******************************************************************************/
/****** Security PIB Table Size Definitions                              ******/
/******************************************************************************/
enum SecurityPibSize
{
#if CASCODA_CA_VER >= 8212
	KEY_TABLE_SIZE                       = 6,   /** Maximum value of macKeyTableEntries */
	LOOKUP_DESC_TABLE_SIZE               = 10,  /** Maximum value of macKeyLookupTableEntries */
	DEVICE_TABLE_SIZE                    = 150, /** Maximum value of macDeviceTableEntries */
	SECURITY_LEVEL_TABLE_SIZE            = 10,  /** Maximum value of macSecurityLevelTableEntries */
	COMMAND_ID_SECURITY_LEVEL_TABLE_SIZE = 10,  /** Maximum value of macCommandIdSecurityLevelTableEntries */
	IE_SECURITY_LEVEL_TABLE_SIZE         = 10,  /** Maximum value of macIESecurityLevelTableEntries */
	SECURITY_LEVEL_EXEMPTION_TABLE_SIZE  = 4,   /** Maximum value of macSecurityLevelExemptionTableEntries */
#elif CASCODA_CA_VER == 8211
	KEY_TABLE_SIZE            = 4,  /** Maximum value of macKeyTableEntries */
	LOOKUP_DESC_TABLE_SIZE    = 5,  /** Maximum value of KeyIdLookupListEntries */
	KEY_DEVICE_TABLE_SIZE     = 32, /** Maximum value of KeyDeviceListEntries */
	KEY_USAGE_TABLE_SIZE      = 12, /** Maximum value of KeyUsageListEntries */
	SECURITY_LEVEL_TABLE_SIZE = 2,  /** Maximum value of macSecurityLevelTableEntries */
	DEVICE_TABLE_SIZE         = 32, /** Maximum value of macDeviceTableEntries */
#elif CASCODA_CA_VER == 8210
	KEY_TABLE_SIZE            = 4,  /** Maximum value of macKeyTableEntries */
	LOOKUP_DESC_TABLE_SIZE    = 5,  /** Maximum value of KeyIdLookupListEntries */
	KEY_DEVICE_TABLE_SIZE     = 10, /** Maximum value of KeyDeviceListEntries */
	KEY_USAGE_TABLE_SIZE      = 12, /** Maximum value of KeyUsageListEntries */
	SECURITY_LEVEL_TABLE_SIZE = 2,  /** Maximum value of macSecurityLevelTableEntries */
	DEVICE_TABLE_SIZE         = 10, /** Maximum value of macDeviceTableEntries */
#else
#error "Security table sizes undefined"
#endif // CASCODA_CA_VER >= 8212
};

/*******************************************************************************
 * Structures used by security PIB attributes
 ******************************************************************************/

#if CASCODA_CA_VER >= 8212
struct DeviceTable
{
	uint8_t NumEntries;
	struct DeviceTableShortPan
	{
		uint8_t ShortAddr[2];
		uint8_t PANId[2];
	} ShortPan[DEVICE_TABLE_SIZE];
	struct DeviceTableExtAddr
	{
		uint8_t ExtAddr[8];
	} ExtAddr[DEVICE_TABLE_SIZE];
	struct
	{
		uint8_t FrameCounter[4];
		uint8_t EnhAckFCOffset;
		uint8_t KeyMask : 7;
		uint8_t ThreadKeyIndexEnabled : 1;
	} Entry[DEVICE_TABLE_SIZE];
};

struct KeyTable
{
	uint8_t NumEntries;
	struct
	{
		uint8_t Key[16];
		uint8_t ThreadKeyIDMode2 : 1;
	} Entry[KEY_TABLE_SIZE];
};

struct KeyLookupTable
{
	uint8_t NumEntries;
	struct LookupTableData
	{
		uint8_t KeyIdMode;
		uint8_t LookupData[9];
	} LookupData[LOOKUP_DESC_TABLE_SIZE];
	struct
	{
		uint8_t KeyTableIndex;
	} Entry[LOOKUP_DESC_TABLE_SIZE];
};

struct SecurityLevelTable
{
	uint8_t NumEntries;
	struct
	{
		uint8_t FrameTypeMask;
		uint8_t SecLevelMask;
		uint8_t KeyMask;
	} Entry[SECURITY_LEVEL_TABLE_SIZE];
};

struct CommandIdSecurityLevelTable
{
	uint8_t NumEntries;
	uint8_t CommandId[COMMAND_ID_SECURITY_LEVEL_TABLE_SIZE];
	struct
	{
		uint8_t SecLevelMask;
		uint8_t KeyMask;
	} Entry[COMMAND_ID_SECURITY_LEVEL_TABLE_SIZE];
};

struct IESecurityLevelTable
{
	uint8_t NumEntries;
	uint8_t IEId[IE_SECURITY_LEVEL_TABLE_SIZE];
	struct
	{
		uint8_t FrameTypeMask;
		uint8_t SecLevelMask;
		uint8_t KeyMask;
	} Entry[IE_SECURITY_LEVEL_TABLE_SIZE];
};

struct SecurityLevelExemptionTable
{
	uint8_t NumEntries;
	struct
	{
		uint8_t FrameTypeMask;
		uint8_t CommandIdMask;
		uint8_t SecLevelMask;
		uint8_t KeyMask;
		uint8_t DeviceTableIndex;
	} Entry[SECURITY_LEVEL_EXEMPTION_TABLE_SIZE];
};

// MLME Get/Set representations of security table entries
struct DeviceTablePib
{
	uint8_t ShortAddr[2];
	uint8_t PANId[2];
	uint8_t ExtAddr[8];
	uint8_t FrameCounter[4];
	uint8_t EnhAckFCOffset;
	uint8_t KeyMask;
	uint8_t ThreadKeyIndexEnabled;
};

struct KeyTablePib
{
	uint8_t Key[16];
	uint8_t ThreadKeyIDMode2;
};

struct KeyLookupTablePib
{
	uint8_t KeyIdMode;
	uint8_t LookupData[9];
	uint8_t KeyTableIndex;
};

struct SecurityLevelTablePib
{
	uint8_t FrameTypeMask;
	uint8_t SecLevelMask;
	uint8_t KeyMask;
};

struct CommandIdSecurityLevelTablePib
{
	uint8_t CommandId;
	uint8_t SecLevelMask;
	uint8_t KeyMask;
};

struct IESecurityLevelTablePib
{
	uint8_t IEId;
	uint8_t FrameTypeMask;
	uint8_t SecLevelMask;
	uint8_t KeyMask;
};

struct SecurityLevelExemptionTablePib
{
	uint8_t FrameTypeMask;
	uint8_t CommandIdMask;
	uint8_t SecLevelMask;
	uint8_t KeyMask;
	uint8_t DeviceTableIndex;
};
#else
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

/** Masks for \ref M_KeyDeviceDesc Flags*/
enum kdd_mask
{
	KDD_DeviceDescHandleMask = 0x3F, /** Key Device Descriptor handle mask */
	KDD_BlacklistedMask      = 0x80, /** Key Device Descriptor is blacklisted mask */
	KDD_UniqueDeviceMask     = 0x40, /** Key Device Descriptor is unique device mask */
#if CASCODA_CA_VER == 8211
	KDD_NewMask              = 0x20, /** Key Device Descriptor nonstandard 'is new' key-device pair */
#endif // CASCODA_CA_VER == 8211
};

struct M_KeyUsageDesc
{
	// uint8_t      FrameType : 2;
	// uint8_t      CommandFrameIdentifier : 4;
	uint8_t Flags;
};

/** Masks for \ref M_KeyUsageDesc Flags*/
enum kud_mask
{
	KUD_FrameTypeMask               = 0x03, /** Key Usage Descriptor frame type mask */
	KUD_CommandFrameIdentifierMask  = 0xF0, /** Key Usage Descriptor command frame identifier mask */
	KUD_CommandFrameIdentifierShift = 4,    /** Key Usage Descriptor command frame identifier offset shift */
};

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
	struct M_KeyUsageDesc       KeyUsageList[KEY_USAGE_TABLE_SIZE];
};
#endif // CASCODA_CA_VER >= 8212

/***************************************************************************/ /**
 * SPI Callback templates
 ******************************************************************************/
struct ca821x_dev;

typedef ca_error (*HWME_WAKEUP_indication_callback)(struct HWME_WAKEUP_indication_pset *params,
                                                    struct ca821x_dev                  *pDeviceRef);
typedef ca_error (*MLME_ASSOCIATE_indication_callback)(struct MLME_ASSOCIATE_indication_pset *params,
                                                       struct ca821x_dev                     *pDeviceRef);
typedef ca_error (*MLME_ASSOCIATE_confirm_callback)(struct MLME_ASSOCIATE_confirm_pset *params,
                                                    struct ca821x_dev                  *pDeviceRef);
typedef ca_error (*MLME_DISASSOCIATE_indication_callback)(struct MLME_DISASSOCIATE_indication_pset *params,
                                                          struct ca821x_dev                        *pDeviceRef);
typedef ca_error (*MLME_DISASSOCIATE_confirm_callback)(struct MLME_DISASSOCIATE_confirm_pset *params,
                                                       struct ca821x_dev                     *pDeviceRef);
typedef ca_error (*MLME_BEACON_NOTIFY_indication_callback)(struct MLME_BEACON_NOTIFY_indication_pset *params,
                                                           struct ca821x_dev                         *pDeviceRef);
typedef ca_error (*MLME_ORPHAN_indication_callback)(struct MLME_ORPHAN_indication_pset *params,
                                                    struct ca821x_dev                  *pDeviceRef);
typedef ca_error (*MLME_COMM_STATUS_indication_callback)(struct MLME_COMM_STATUS_indication_pset *params,
                                                         struct ca821x_dev                       *pDeviceRef);
typedef ca_error (*MLME_SYNC_LOSS_indication_callback)(struct MLME_SYNC_LOSS_indication_pset *params,
                                                       struct ca821x_dev                     *pDeviceRef);
typedef ca_error (*MLME_POLL_indication_callback)(struct MLME_POLL_indication_pset *params,
                                                  struct ca821x_dev                *pDeviceRef);
#if CASCODA_CA_VER >= 8212
typedef ca_error (*MLME_POLL_confirm_callback)(struct MLME_POLL_confirm_pset *params, struct ca821x_dev *pDeviceRef);
typedef ca_error (*MLME_IE_NOTIFY_indication_callback)(struct MLME_IE_NOTIFY_indication_pset *params,
                                                       struct ca821x_dev                     *pDeviceRef);
#endif // CASCODA_CA_VER >= 8212
typedef ca_error (*MLME_SCAN_confirm_callback)(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef);
typedef ca_error (*MCPS_DATA_indication_callback)(struct MCPS_DATA_indication_pset *params,
                                                  struct ca821x_dev                *pDeviceRef);
typedef ca_error (*MCPS_DATA_confirm_callback)(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
#if CASCODA_CA_VER >= 8211
typedef ca_error (*PCPS_DATA_indication_callback)(struct PCPS_DATA_indication_pset *params,
                                                  struct ca821x_dev                *pDeviceRef);
typedef ca_error (*PCPS_DATA_confirm_callback)(struct PCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
#endif // CASCODA_CA_VER >= 8211
typedef ca_error (*TDME_RXPKT_indication_callback)(struct TDME_RXPKT_indication_pset *params,
                                                   struct ca821x_dev                 *pDeviceRef);
typedef ca_error (*TDME_EDDET_indication_callback)(struct TDME_EDDET_indication_pset *params,
                                                   struct ca821x_dev                 *pDeviceRef);
typedef ca_error (*TDME_ERROR_indication_callback)(struct TDME_ERROR_indication_pset *params,
                                                   struct ca821x_dev                 *pDeviceRef);
typedef ca_error (*ca821x_generic_callback)(void *params, struct ca821x_dev *pDeviceRef);

/** Union of all compatible callback types */
union ca821x_api_callback
{
	MCPS_DATA_indication_callback MCPS_DATA_indication;
	MCPS_DATA_confirm_callback    MCPS_DATA_confirm;
#if CASCODA_CA_VER >= 8211
	PCPS_DATA_indication_callback PCPS_DATA_indication;
	PCPS_DATA_confirm_callback    PCPS_DATA_confirm;
#endif // CASCODA_CA_VER >= 8211
	MLME_ASSOCIATE_indication_callback     MLME_ASSOCIATE_indication;
	MLME_ASSOCIATE_confirm_callback        MLME_ASSOCIATE_confirm;
	MLME_DISASSOCIATE_indication_callback  MLME_DISASSOCIATE_indication;
	MLME_DISASSOCIATE_confirm_callback     MLME_DISASSOCIATE_confirm;
	MLME_BEACON_NOTIFY_indication_callback MLME_BEACON_NOTIFY_indication;
	MLME_ORPHAN_indication_callback        MLME_ORPHAN_indication;
	MLME_SCAN_confirm_callback             MLME_SCAN_confirm;
	MLME_COMM_STATUS_indication_callback   MLME_COMM_STATUS_indication;
	MLME_POLL_indication_callback          MLME_POLL_indication;
#if CASCODA_CA_VER >= 8212
	MLME_POLL_confirm_callback         MLME_POLL_confirm;
	MLME_IE_NOTIFY_indication_callback MLME_IE_NOTIFY_indication;
#endif // CASCODA_CA_VER >= 8212
	MLME_SYNC_LOSS_indication_callback MLME_SYNC_LOSS_indication;
	HWME_WAKEUP_indication_callback    HWME_WAKEUP_indication;
	TDME_RXPKT_indication_callback     TDME_RXPKT_indication;
	TDME_EDDET_indication_callback     TDME_EDDET_indication;
	TDME_ERROR_indication_callback     TDME_ERROR_indication;
	ca821x_generic_callback            generic_callback;
};

/***************************************************************************/ /**
 * SPI Message Format Typedef
 * 
 * IMPORTANT: Make sure all of the structs in the PData union have no alignment
 * requirements, i.e. all the members of any of the structs should be of type
 * uint8_t (that includes uint8_t arrays, or other structs which themselves
 * only have uint8_t members.).
 * An example of what NOT to do:
 * struct BAD_EXAMPLE_request_set 
 * {
 * 		uint8_t var1;
 * 		uint8_t var2;
 * 		uint32_t bad_var; // DON'T DO THIS, instead do uint8_t good_var[4];
 * };
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
		struct MLME_SET_confirm_pset              SetCnf;
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
#if CASCODA_CA_VER >= 8212
		struct MLME_POLL_confirm_pset         PollCnf;
		struct MLME_IE_NOTIFY_indication_pset IENotifyInd;
#endif // CASCODA_CA_VER >= 8212
		/* PCPS */
		struct PCPS_DATA_request_pset    PhyDataReq;
		struct PCPS_DATA_confirm_pset    PhyDataCnf;
		struct PCPS_DATA_indication_pset PhyDataInd;
#endif // CASCODA_CA_VER >= 8211
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
		uint8_t Payload[254];
	} PData;
};

/******************************************************************************/
/****** SPI Command IDs                                                  ******/
/******************************************************************************/
enum spi_command_masks
{
	SPI_MID_MASK = 0x1F, /** Mask to derive the Message ID Code from the Command ID */
	SPI_S2M      = 0x20, /** Bit indicating a Confirm or Indication from Slave to Master */
	SPI_SYN      = 0x40, /** Bit indicating a Synchronous Message */
	SPI_INVALID  = 0x80, /** Bit indicating an IDLE or invalid command ID */
};

/** SPI Command IDs */
enum spi_command_ids
{
	//CA821x control bytes
	SPI_IDLE = 0xFF, ///< Present on SPI when stream is idle - No Data
	SPI_NACK = 0xF0, ///< Present on SPI when buffer full or busy - resend Request
	// MAC MCPS
	SPI_MCPS_DATA_REQUEST    = 0x00,
	SPI_MCPS_PURGE_REQUEST   = 0x41,
	SPI_MCPS_DATA_INDICATION = 0x20,
	SPI_MCPS_DATA_CONFIRM    = 0x21,
	SPI_MCPS_PURGE_CONFIRM   = 0x62,
// MAC PCPS
#if CASCODA_CA_VER >= 8211
	SPI_PCPS_DATA_REQUEST    = 0x07,
	SPI_PCPS_DATA_CONFIRM    = 0x38,
	SPI_PCPS_DATA_INDICATION = 0x28,
#endif // CASCODA_CA_VER >= 8211
	// MAC MLME
	SPI_MLME_ASSOCIATE_REQUEST    = 0x02,
	SPI_MLME_ASSOCIATE_RESPONSE   = 0x03,
	SPI_MLME_DISASSOCIATE_REQUEST = 0x04,
	SPI_MLME_GET_REQUEST          = 0x45,
	SPI_MLME_ORPHAN_RESPONSE      = 0x06,
	SPI_MLME_RESET_REQUEST        = 0x47,
	SPI_MLME_RX_ENABLE_REQUEST    = 0x48,
	SPI_MLME_SCAN_REQUEST         = 0x09,
	SPI_MLME_SET_REQUEST          = 0x4A,
	SPI_MLME_START_REQUEST        = 0x4B,
	SPI_MLME_SYNC_REQUEST         = 0x0C,
#if CASCODA_CA_VER >= 8212
	SPI_MLME_POLL_REQUEST = 0x32,
#else
	SPI_MLME_POLL_REQUEST = 0x4D,
#endif // CASCODA_CA_VER >= 8212
	SPI_MLME_ASSOCIATE_INDICATION     = 0x23,
	SPI_MLME_ASSOCIATE_CONFIRM        = 0x24,
	SPI_MLME_DISASSOCIATE_INDICATION  = 0x25,
	SPI_MLME_DISASSOCIATE_CONFIRM     = 0x26,
	SPI_MLME_BEACON_NOTIFY_INDICATION = 0x27,
	SPI_MLME_GET_CONFIRM              = 0x68,
	SPI_MLME_ORPHAN_INDICATION        = 0x29,
	SPI_MLME_RESET_CONFIRM            = 0x6A,
	SPI_MLME_RX_ENABLE_CONFIRM        = 0x6B,
	SPI_MLME_SCAN_CONFIRM             = 0x2C,
	SPI_MLME_COMM_STATUS_INDICATION   = 0x2D,
	SPI_MLME_SET_CONFIRM              = 0x6E,
	SPI_MLME_START_CONFIRM            = 0x6F,
	SPI_MLME_SYNC_LOSS_INDICATION     = 0x30,
#if CASCODA_CA_VER >= 8212
	SPI_MLME_POLL_CONFIRM         = 0x33,
	SPI_MLME_IE_NOTIFY_INDICATION = 0x22,
#else
	SPI_MLME_POLL_CONFIRM = 0x71,
#endif // CASCODA_CA_VER >= 8212
#if CASCODA_CA_VER >= 8211
	SPI_MLME_POLL_INDICATION = 0x31,
#endif // CASCODA_CA_VER >= 8211
	// HWME
	SPI_HWME_SET_REQUEST       = 0x4E,
	SPI_HWME_GET_REQUEST       = 0x4F,
	SPI_HWME_HAES_REQUEST      = 0x50,
	SPI_HWME_SET_CONFIRM       = 0x72,
	SPI_HWME_GET_CONFIRM       = 0x73,
	SPI_HWME_HAES_CONFIRM      = 0x74,
	SPI_HWME_WAKEUP_INDICATION = 0x35,
	// TDME
	SPI_TDME_SETSFR_REQUEST   = 0x51,
	SPI_TDME_GETSFR_REQUEST   = 0x52,
	SPI_TDME_TESTMODE_REQUEST = 0x53,
	SPI_TDME_SET_REQUEST      = 0x54,
	SPI_TDME_TXPKT_REQUEST    = 0x55,
	SPI_TDME_LOTLK_REQUEST    = 0x56,
	SPI_TDME_SETSFR_CONFIRM   = 0x77,
	SPI_TDME_GETSFR_CONFIRM   = 0x78,
	SPI_TDME_TESTMODE_CONFIRM = 0x79,
	SPI_TDME_SET_CONFIRM      = 0x7A,
	SPI_TDME_TXPKT_CONFIRM    = 0x7B,
	SPI_TDME_RXPKT_INDICATION = 0x3C,
	SPI_TDME_EDDET_INDICATION = 0x3D,
	SPI_TDME_ERROR_INDICATION = 0x3E,
	SPI_TDME_LOTLK_CONFIRM    = 0x7F,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // MAC_MESSAGES_H
