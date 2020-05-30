/**
 * @file
 * @brief Chili temperature sensing app declarations
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
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"

#ifndef TEMPSENSE_APP_H
#define TEMPSENSE_APP_H

/******************************************************************************/
/****** System Definitions                                               ******/
/******************************************************************************/
//! wakeup interval for application [seconds]
#define APP_WAKEUPINTERVALL 5
//! timeout interval for end devices that havent sent anything for some time [seconds]
#define APP_TIMEOUTINTERVALL 15
//! device timeout in [ms] for max. time staying awake
#define APP_DEVICETIMEOUT 1000
//! maximum number of retries for device before disconnection and re-scanning
#define APP_DEVICE_MAX_RETRIES 1
//! maximum devices/sensors that can connect to coordinator
#define APP_MAX_DEVICES 32
//! default powerdown mode for device/sensor
#define APP_DEFAULT_PDN_MODE PDM_DPD
//! use clock from cax8210 xtal oscillator (when 1) or nano120 internal RC oscillator as system clock (when 0)
#define APP_USE_EXTERNAL_CLOCK 1
//! link quality limit for joining coordinator
#define APP_LQI_LIMIT 75
//! energy detect limit (triggers warning)
#define APP_ED_LIMIT 50
//! battery voltage limit when unplugged (triggers warning)
#define APP_VBAT_LIMIT 1862 /* = 2.5 V */
//! report battery voltage with every reading (otherwise just issue warning if it drops below APP_VBAT_LIMIT)
#define APP_REPORT_VBATT 1
//! report lqi values (both from device and coordinator)
#define APP_REPORT_LQI 1
//! report ed values (both from device and coordinator)
#define APP_REPORT_ED 1
//! report every nth data package
#define APP_COORD_REPORTN 1
//! soft re-initialisation of coordinator
#define APP_COORD_SREINIT 0
//! use over-air debug mode
#define APP_USE_DEBUG 0

/******************************************************************************/
/****** MAC PIB Default Values                                           ******/
/******************************************************************************/
#define MAC_PANID 0xCA5C
#define MAC_SHORTADD 0xCA00
#define MAC_LONGADD                                    \
	{                                                  \
		0x00, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA \
	}
#define MAC_CHANNELLIST \
	{                   \
		11, 18, 24      \
	}

/******************************************************************************/
/****** Application State Definitions                                    ******/
/******************************************************************************/
#define APP_ST_NORMAL 0      //!< Application disabled, EVBME
#define APP_ST_COORDINATOR 1 //!< Network Coordinator
#define APP_ST_DEVICE 2      //!< Sensing Device

/******************************************************************************/
/****** Communication State Definitions                                  ******/
/******************************************************************************/
#define APP_CST_DONE 0x00             //!< communication completed
#define APP_CST_WAKEUP_REQUESTED 0x01 //!< wakeup pkt  requested (device)
#define APP_CST_WAKEUP_CONFIRMED 0x02 //!< wakeup pkt  confirmed (device)
#define APP_CST_WAKEUP_RECEIVED 0x03  //!< wakeup pkt  received  (coord)
#define APP_CST_C_DATA_REQUESTED 0x04 //!< coord  data requested (coord)
#define APP_CST_C_DATA_CONFIRMED 0x05 //!< coord  data confirmed (coord)
#define APP_CST_C_DATA_RECEIVED 0x06  //!< coord  data received  (device)
#define APP_CST_D_DATA_REQUESTED 0x07 //!< device data requested (device)
#define APP_CST_GO_POWERDOWN 0x08     //!< go into powerdown     (device)
#define APP_CST_ASSOC_REQUESTED 0x09  //|< assoc. request sent   (device)
#define APP_CST_SCANNING 0x0A         //|< scanning in progress  (device)

/******************************************************************************/
/****** Data Packet Types, defined by MSDU[0]                            ******/
/******************************************************************************/
#define PT_MSDU_D_WAKEUP 0x01 //!< device wakeup indication
#define PT_MSDU_C_DATA 0x02   //!< coordinator data
#define PT_MSDU_D_DATA 0x03   //!< device data

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
extern u8_t APP_STATE;
extern u8_t APP_STATE_new;
extern u8_t APP_INITIALISE;
extern u8_t APP_CONNECTED;

extern u16_t APP_PANId;
extern u16_t APP_ShortAddress;
extern u8_t  APP_LongAddress[8];
extern u8_t  APP_Channel;

/******************************************************************************/
/****** APP Functions                                                    ******/
/******************************************************************************/
/* tempsense_app */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Handler
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Handler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation of MAC etc.
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Initialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Dispatch Branch (UpStream, Serial)
 *******************************************************************************
 ******************************************************************************/
int TEMPSENSE_APP_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Switch Mode
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_SwitchMode(u8_t mode);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation of MAC PIB
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_InitPIB(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Get Temperature Reading
 *******************************************************************************
 * \return Temperature, 8 bit signed (1s complement)
 *******************************************************************************
 ******************************************************************************/
u8_t TEMPSENSE_APP_GetTempVal(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Get Battery Voltage Reading
 *******************************************************************************
 * \return 8-Bit VBat, full range = 5.5V
 *******************************************************************************
 ******************************************************************************/
u16_t TEMPSENSE_APP_GetVoltsVal(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Get ScanChannels from MAC_CHANNELLIST
 *******************************************************************************
 * \return ScanChannels Parameter for Scan Requests
 *******************************************************************************
 ******************************************************************************/
u32_t TEMPSENSE_APP_GetScanChannels(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Print MAC_CHANNELLIST
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_PrintScanChannels(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Prints AboluteTime in Seconds
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_PrintSeconds(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief module LEDs signalling
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_LED_Handler(void);

/* tempsense_app_coord */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Handler for Coordinator
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_Handler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation for Coordinator
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_Initialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Start Procedure
 *******************************************************************************
 * \param pDeviceRef - pointer to a CA-821x Device reference struct
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_Start(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Coordinator Process incoming Scan Confirm
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_ProcessScanCnf(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Association Response
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_AssociateResponse(struct MLME_ASSOCIATE_indication_pset *params,
                                                 struct ca821x_dev *                    pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Coordinator Process incoming Data Indications
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Coordinator Process incoming Data Confirmations
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator check and display Data Packet
 *******************************************************************************
 * \param device - device number
 * \param edcoord - ED received locally (Coordinator)
 * \param params - Buffer containing data indication with data to display
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_DisplayData(u8_t                              device,
                                           u8_t                              edcoord,
                                           struct MCPS_DATA_indication_pset *params,
                                           struct ca821x_dev *               pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Timeout Check for all Devices
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckTimeouts(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Checks and displays Battery Voltage
 *******************************************************************************
 * \param vbat - 12-bit Battery Voltage Reading from Device
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckVbatt(u16_t vbat);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Checks and displays LQI from both Sides
 *******************************************************************************
 * \param lqi_ts    - LQI received at Temperature Sensor (Device)
 * \param lqi_coord - LQI received locally (Coordinator)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckLQI(u8_t lqi_ts, u8_t lqi_coord);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Checks and displays ED from both Sides
 *******************************************************************************
 * \param ed_ts    - ED received at Temperature Sensor (Device)
 * \param ed_coord - ED received locally (Coordinator)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_CheckED(u8_t ed_ts, u8_t ed_coord);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Soft Reset (no change in PIB)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_SoftReinit(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Coordinator Report Network Status
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Coordinator_ReportStatus(void);
/* tempsense_app_device */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Handler for Sensor Device
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_Handler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Application Initialisation for Sensor Device
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_Initialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Get Device LongAddress from Dataflash
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_GetLongAddress(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device Association Procedure
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_Start(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Scan Confirm
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessScanCnf(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Data Indications
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessAssociateCnf(struct MLME_ASSOCIATE_confirm_pset *params,
                                              struct ca821x_dev *                 pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Data Indications
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Device Process incoming Data Confirms
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Start exchanging Data with Coordinator
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_ExchangeData(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device Timeout Check
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_CheckTimeout(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device prepare and enter sleep mode (and wake-up)
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_GoPowerDown(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device restore state from flash
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_RestoreStateFromFlash(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE App. Device save state to flash
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_APP_Device_SaveStateToFlash(void);

#endif // TEMPSENSE_APP_H
