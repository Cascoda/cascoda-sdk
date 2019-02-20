/**
 * @file ca821x_api.h
 * @brief API Access Function Declarations for MCPS, MLME, HWME and TDME.
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
/***************************************************************************/ /**
 * \def LS_BYTE(x)
 * Extract the least significant octet of a 16-bit value
 * \def MS_BYTE(x)
 * Extract the most significant octet of a 16-bit value
 * \def LS0_BYTE(x)
 * Extract the first (little-endian) octet of a 32-bit value
 * \def LS1_BYTE(x)
 * Extract the second (little-endian) octet of a 32-bit value
 * \def LS2_BYTE(x)
 * Extract the third (little-endian) octet of a 32-bit value
 * \def LS3_BYTE(x)
 * Extract the fourth (little-endian) octet of a 32-bit value
 * \def GETLE16(x)
 * Extract a 16-bit value from a little-endian octet array
 * \def GETLE32(x)
 * Extract a 32-bit value from a little-endian octet array
 * \def PUTLE16(x,y)
 * Put a 16-bit value x into a little-endian octet array y
 * \def PUTLE32(x,y)
 * Put a 32-bit value x into a little-endian octet array y
 ******************************************************************************/
#ifndef CA821X_API_H
#define CA821X_API_H

#include <stddef.h>
#include <stdint.h>

#include "ca821x_config.h"
#include "mac_messages.h"

#if CASCODA_CA_VER != 8210 && CASCODA_CA_VER != 8211
#error "UNSUPPORTED CASCODA_CA_VER VERSION (or build incorrectly configured - use cmake)"
#endif

#ifndef _CASCODA_MACROS
#define _CASCODA_MACROS

#define LS_BYTE(x) ((uint8_t)((x)&0xFF))
#define MS_BYTE(x) ((uint8_t)(((x) >> 8) & 0xFF))
#define LS0_BYTE(x) ((uint8_t)((x)&0xFF))
#define LS1_BYTE(x) ((uint8_t)(((x) >> 8) & 0xFF))
#define LS2_BYTE(x) ((uint8_t)(((x) >> 16) & 0xFF))
#define LS3_BYTE(x) ((uint8_t)(((x) >> 24) & 0xFF))

#define GETLE16(x) (((uint16_t)(x)[1] << 8) + (x)[0])
#define GETLE32(x) (((uint32_t)(x)[3] << 24) + ((uint32_t)(x)[2] << 16) + ((uint32_t)(x)[1] << 8) + (x)[0])
#define PUTLE16(x, y)        \
	{                        \
		(y)[0] = ((x)&0xff); \
		(y)[1] = ((x) >> 8); \
	}
#define PUTLE32(x, y)                  \
	{                                  \
		(y)[0] = ((x)&0xff);           \
		(y)[1] = (((x) >> 8) & 0xff);  \
		(y)[2] = (((x) >> 16) & 0xff); \
		(y)[3] = (((x) >> 24) & 0xff); \
	}

#endif

struct ca821x_dev;

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

/******************************************************************************/
/****** External function pointers                                       ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Function pointer for downstream api interface.
 *******************************************************************************
 * This function pointer is called by all api functions when it comes to
 * transmitting constructed commands to the transceiver. The user must implement
 * their own downstream exchange function conforming to this prototype and
 * assign that function to this pointer.
 *******************************************************************************
 * \param buf - The buffer containing the command to send downstream
 * \param len - The length of the command in octets
 * \param response - The buffer to populate with a received synchronous
 *                    response
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return Effectively a bool as far as API is concerned, 0 means exchange was
 *         successful, nonzero otherwise
 ******************************************************************************/
typedef int (*ca821x_api_downstream_t)(const uint8_t *    buf,
                                       size_t             len,
                                       uint8_t *          response,
                                       struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Function pointer for waiting for messages
 *******************************************************************************
 * This function pointer can be called by applications to wait for a specific
 * primitive to be received by the interface driver underlying the api. The
 * function should block until the primitive has been received, then copy this
 * message into buf.
 *******************************************************************************
 * \param cmdid - The id of the command to wait for
 * \param timeout_ms - Timeout for the wait in milliseconds
 * \param buf - The buffer to populate with the received message
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return 0: message successfully received
 ******************************************************************************/
extern int (*ca821x_wait_for_message)(uint8_t cmdid, int timeout_ms, uint8_t *buf, struct ca821x_dev *pDeviceRef);

extern const uint8_t sync_pairings[23];

/***************************************************************************/ /**
 * \brief API user callbacks structure
 *
 * Contains a set of function pointers that can (and should) be populated by the
 * user for processing asynchronous messages received from the hardware. If the
 * pointer for the specific command type is populated that will be called,
 * otherwise the generic_dispatch function will be. If neither are populated the
 * message is discarded.
 *
 * Every callback should return:
 * - 0 if the command was not handled, ie the command was unexpected/generated
 *   by another application etc.
 * - >0 if the command was successfully handled by the application.
 * - The appropriate negative error code if encountered.
 ******************************************************************************/
typedef int (*MCPS_DATA_indication_callback)(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*MCPS_DATA_confirm_callback)(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*PCPS_DATA_indication_callback)(struct PCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*PCPS_DATA_confirm_callback)(struct PCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*MLME_ASSOCIATE_indication_callback)(struct MLME_ASSOCIATE_indication_pset *params,
                                                  struct ca821x_dev *                    pDeviceRef);
typedef int (*MLME_ASSOCIATE_confirm_callback)(struct MLME_ASSOCIATE_confirm_pset *params,
                                               struct ca821x_dev *                 pDeviceRef);
typedef int (*MLME_DISASSOCIATE_indication_callback)(struct MLME_DISASSOCIATE_indication_pset *params,
                                                     struct ca821x_dev *                       pDeviceRef);
typedef int (*MLME_DISASSOCIATE_confirm_callback)(struct MLME_DISASSOCIATE_confirm_pset *params,
                                                  struct ca821x_dev *                    pDeviceRef);
typedef int (*MLME_BEACON_NOTIFY_indication_callback)(struct MLME_BEACON_NOTIFY_indication_pset *params,
                                                      struct ca821x_dev *                        pDeviceRef);
typedef int (*MLME_ORPHAN_indication_callback)(struct MLME_ORPHAN_indication_pset *params,
                                               struct ca821x_dev *                 pDeviceRef);
typedef int (*MLME_SCAN_confirm_callback)(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*MLME_COMM_STATUS_indication_callback)(struct MLME_COMM_STATUS_indication_pset *params,
                                                    struct ca821x_dev *                      pDeviceRef);
typedef int (*MLME_POLL_indication_callback)(struct MLME_POLL_indication_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*MLME_SYNC_LOSS_indication_callback)(struct MLME_SYNC_LOSS_indication_pset *params,
                                                  struct ca821x_dev *                    pDeviceRef);
typedef int (*HWME_WAKEUP_indication_callback)(struct HWME_WAKEUP_indication_pset *params,
                                               struct ca821x_dev *                 pDeviceRef);
typedef int (*TDME_RXPKT_indication_callback)(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*TDME_EDDET_indication_callback)(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef);
typedef int (*TDME_ERROR_indication_callback)(struct TDME_ERROR_indication_pset *params, struct ca821x_dev *pDeviceRef);

union ca821x_api_callback
{
	MCPS_DATA_indication_callback          MCPS_DATA_indication;
	MCPS_DATA_confirm_callback             MCPS_DATA_confirm;
	PCPS_DATA_indication_callback          PCPS_DATA_indication;
	PCPS_DATA_confirm_callback             PCPS_DATA_confirm;
	MLME_ASSOCIATE_indication_callback     MLME_ASSOCIATE_indication;
	MLME_ASSOCIATE_confirm_callback        MLME_ASSOCIATE_confirm;
	MLME_DISASSOCIATE_indication_callback  MLME_DISASSOCIATE_indication;
	MLME_DISASSOCIATE_confirm_callback     MLME_DISASSOCIATE_confirm;
	MLME_BEACON_NOTIFY_indication_callback MLME_BEACON_NOTIFY_indication;
	MLME_ORPHAN_indication_callback        MLME_ORPHAN_indication;
	MLME_SCAN_confirm_callback             MLME_SCAN_confirm;
	MLME_COMM_STATUS_indication_callback   MLME_COMM_STATUS_indication;
	MLME_POLL_indication_callback          MLME_POLL_indication;
	MLME_SYNC_LOSS_indication_callback     MLME_SYNC_LOSS_indication;
	HWME_WAKEUP_indication_callback        HWME_WAKEUP_indication;
	TDME_RXPKT_indication_callback         TDME_RXPKT_indication;
	TDME_EDDET_indication_callback         TDME_EDDET_indication;
	TDME_ERROR_indication_callback         TDME_ERROR_indication;
	int (*generic_callback)(void *params, struct ca821x_dev *pDeviceRef);
};

struct ca821x_api_callbacks
{
	MCPS_DATA_indication_callback MCPS_DATA_indication;
	MCPS_DATA_confirm_callback    MCPS_DATA_confirm;
#if CASCODA_CA_VER >= 8211
	PCPS_DATA_indication_callback PCPS_DATA_indication;
	PCPS_DATA_confirm_callback    PCPS_DATA_confirm;
#endif
	MLME_ASSOCIATE_indication_callback     MLME_ASSOCIATE_indication;
	MLME_ASSOCIATE_confirm_callback        MLME_ASSOCIATE_confirm;
	MLME_DISASSOCIATE_indication_callback  MLME_DISASSOCIATE_indication;
	MLME_DISASSOCIATE_confirm_callback     MLME_DISASSOCIATE_confirm;
	MLME_BEACON_NOTIFY_indication_callback MLME_BEACON_NOTIFY_indication;
	MLME_ORPHAN_indication_callback        MLME_ORPHAN_indication;
	MLME_SCAN_confirm_callback             MLME_SCAN_confirm;
	MLME_COMM_STATUS_indication_callback   MLME_COMM_STATUS_indication;
#if CASCODA_CA_VER >= 8211
	MLME_POLL_indication_callback MLME_POLL_indication;
#endif
	MLME_SYNC_LOSS_indication_callback MLME_SYNC_LOSS_indication;
	HWME_WAKEUP_indication_callback    HWME_WAKEUP_indication;
	TDME_RXPKT_indication_callback     TDME_RXPKT_indication;
	TDME_EDDET_indication_callback     TDME_EDDET_indication;
	TDME_ERROR_indication_callback     TDME_ERROR_indication;
	int (*generic_dispatch)(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);
};

/******************************************************************************/
/****** device_ref struct for all internal state                         ******/
/******************************************************************************/
struct ca821x_dev
{
	void *context;          //For the application
	void *exchange_context; //For the exchange

	//For the API:
	ca821x_api_downstream_t ca821x_api_downstream;

	/** Variable for storing callback routines registered by the user */
	struct ca821x_api_callbacks callbacks;

	uint8_t  extaddr[8]; /**< Mirrors nsIEEEAddress in the PIB */
	uint16_t shortaddr;  /**< Mirrors macShortAddress in the PIB */

	uint8_t lqi_mode;

	//MAC Workarounds for V1.1 and MPW silicon (V0.x)
	uint8_t MAC_Workarounds; /**< Flag to enable workarounds for ca8210 v1.1 */
	uint8_t MAC_MPW;         /**< Flag to enable workarounds for ca8210 v0.x */
};

/******************************************************************************/
/****** MAC MCPS/MLME Downlink                                           ******/
/******************************************************************************/

uint8_t MCPS_DATA_request(uint8_t            SrcAddrMode,
                          struct FullAddr    DstAddr,
                          uint8_t            MsduLength,
                          uint8_t *          pMsdu,
                          uint8_t            MsduHandle,
                          uint8_t            TxOptions,
                          struct SecSpec *   pSecurity,
                          struct ca821x_dev *pDeviceRef);

uint8_t MCPS_PURGE_request_sync(uint8_t *MsduHandle, struct ca821x_dev *pDeviceRef);

uint8_t MLME_ASSOCIATE_request(uint8_t            LogicalChannel,
                               struct FullAddr    DstAddr,
                               uint8_t            CapabilityInfo,
                               struct SecSpec *   pSecurity,
                               struct ca821x_dev *pDeviceRef);

uint8_t MLME_ASSOCIATE_response(uint8_t *          pDeviceAddress,
                                uint16_t           AssocShortAddress,
                                uint8_t            Status,
                                struct SecSpec *   pSecurity,
                                struct ca821x_dev *pDeviceRef);

uint8_t MLME_DISASSOCIATE_request(struct FullAddr    DevAddr,
                                  uint8_t            DisassociateReason,
                                  uint8_t            TxIndirect,
                                  struct SecSpec *   pSecurity,
                                  struct ca821x_dev *pDeviceRef);

uint8_t MLME_GET_request_sync(uint8_t            PIBAttribute,
                              uint8_t            PIBAttributeIndex,
                              uint8_t *          pPIBAttributeLength,
                              void *             pPIBAttributeValue,
                              struct ca821x_dev *pDeviceRef);

uint8_t MLME_ORPHAN_response(uint8_t *          pOrphanAddress,
                             uint16_t           ShortAddress,
                             uint8_t            AssociatedMember,
                             struct SecSpec *   pSecurity,
                             struct ca821x_dev *pDeviceRef);

uint8_t MLME_RESET_request_sync(uint8_t SetDefaultPIB, struct ca821x_dev *pDeviceRef);

uint8_t MLME_RX_ENABLE_request_sync(uint8_t            DeferPermit,
                                    uint32_t           RxOnTime,
                                    uint32_t           RxOnDuration,
                                    struct ca821x_dev *pDeviceRef);

uint8_t MLME_SCAN_request(uint8_t            ScanType,
                          uint32_t           ScanChannels,
                          uint8_t            ScanDuration,
                          struct SecSpec *   pSecurity,
                          struct ca821x_dev *pDeviceRef);

uint8_t MLME_SET_request_sync(uint8_t            PIBAttribute,
                              uint8_t            PIBAttributeIndex,
                              uint8_t            PIBAttributeLength,
                              const void *       pPIBAttributeValue,
                              struct ca821x_dev *pDeviceRef);

uint8_t MLME_START_request_sync(uint16_t           PANId,
                                uint8_t            LogicalChannel,
                                uint8_t            BeaconOrder,
                                uint8_t            SuperframeOrder,
                                uint8_t            PANCoordinator,
                                uint8_t            BatteryLifeExtension,
                                uint8_t            CoordRealignment,
                                struct SecSpec *   pCoordRealignSecurity,
                                struct SecSpec *   pBeaconSecurity,
                                struct ca821x_dev *pDeviceRef);

uint8_t MLME_POLL_request_sync(struct FullAddr CoordAddress,
#if CASCODA_CA_VER == 8210
                               uint8_t Interval[2], /* polling interval in 0.1 seconds res */
                                                    /* 0 means poll once */
                                                    /* 0xFFFF means stop polling */
#endif
                               struct SecSpec *   pSecurity,
                               struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** HWME Downlink                                                    ******/
/******************************************************************************/

uint8_t HWME_SET_request_sync(uint8_t            HWAttribute,
                              uint8_t            HWAttributeLength,
                              uint8_t *          pHWAttributeValue,
                              struct ca821x_dev *pDeviceRef);

uint8_t HWME_GET_request_sync(uint8_t            HWAttribute,
                              uint8_t *          HWAttributeLength,
                              uint8_t *          pHWAttributeValue,
                              struct ca821x_dev *pDeviceRef);

uint8_t HWME_HAES_request_sync(uint8_t HAESMode, uint8_t *pHAESData, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** TDME Downlink                                                    ******/
/******************************************************************************/

uint8_t TDME_SETSFR_request_sync(uint8_t SFRPage, uint8_t SFRAddress, uint8_t SFRValue, struct ca821x_dev *pDeviceRef);

uint8_t TDME_GETSFR_request_sync(uint8_t SFRPage, uint8_t SFRAddress, uint8_t *SFRValue, struct ca821x_dev *pDeviceRef);

uint8_t TDME_TESTMODE_request_sync(uint8_t TestMode, struct ca821x_dev *pDeviceRef);

uint8_t TDME_SET_request_sync(uint8_t            TestAttribute,
                              uint8_t            TestAttributeLength,
                              void *             pTestAttributeValue,
                              struct ca821x_dev *pDeviceRef);

uint8_t TDME_TXPKT_request_sync(uint8_t            TestPacketDataType,
                                uint8_t *          TestPacketSequenceNumber,
                                uint8_t *          TestPacketLength,
                                void *             pTestPacketData,
                                struct ca821x_dev *pDeviceRef);

uint8_t TDME_LOTLK_request_sync(uint8_t *          TestChannel,
                                uint8_t *          TestRxTxb,
                                uint8_t *          TestLOFDACValue,
                                uint8_t *          TestLOAMPValue,
                                uint8_t *          TestLOTXCALValue,
                                struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** TDME Register Default Initialisation and Checking Functions      ******/
/******************************************************************************/
uint8_t TDME_ChipInit(struct ca821x_dev *pDeviceRef);
uint8_t TDME_ChannelInit(uint8_t channel, struct ca821x_dev *pDeviceRef);
uint8_t TDME_CheckPIBAttribute(uint8_t PIBAttribute, uint8_t PIBAttributeLength, const void *pPIBAttributeValue);

uint8_t TDME_SetTxPower(uint8_t txp, struct ca821x_dev *pDeviceRef);
uint8_t TDME_GetTxPower(uint8_t *txp, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** API meta functions                                           ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation function for initialising a ca821x_dev data structure
 *        for use with the API.
 *******************************************************************************
 * This function should be called to initialise a pDeviceRef. This should be
 * done prior to any other use of the structure.
 *******************************************************************************
 * \param pDeviceRef - Pointer to ca821x_device_ref struct to be initialised.
 *******************************************************************************
 * \return 0: Structure successfully initialised
 * \return -1: Structure initialisation failed (pDeviceRef cannot be NULL)
 ******************************************************************************/
int ca821x_api_init(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Function for registering a callback struct for this device.
 *******************************************************************************
 * Depending on the exchange used, these callbacks may be called from a different
 * processing context, and should be written accordingly.
 *******************************************************************************
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 ******************************************************************************/
void ca821x_register_callbacks(struct ca821x_api_callbacks *in_callbacks, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Function to get a reference to the callback for a certain command ID
 *******************************************************************************
 * This is mainly used internally, and is probably not useful for most user
 * applications.
 *******************************************************************************
 * \param cmdid - The command ID of the desired callback
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 * \retval  A reference to the relevant callback, or NULL if the cmdid is not recognised
 ******************************************************************************/
union ca821x_api_callback *ca821x_get_callback(uint8_t cmdid, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Function called by the exchange to dispatch indications from the ca821x
 *******************************************************************************
 * FOR USE IN THE EXCHANGE ONLY. If the application needs to process incoming
 * indications, this should be by connecting callbacks using
 * ca821x_register_callbacks and a ca821x_api_callbacks struct.
 *******************************************************************************
 * \param buf - entire buffer of message, including command and length bytes
 * \param len - length of buffer
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 ******************************************************************************/
int ca821x_downstream_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

#endif // CA821X_API_H
