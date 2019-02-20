/**
 * @file tempsense_app.h
 * @brief Chili temperature sensing app declarations
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
#define APP_DEFAULT_PDN_MODE PDM_POWEROFF
//! use clock from cax8210 xtal oscillator (when 1) or nano120 internal RC oscillator as system clock (when 0)
#define APP_USE_EXTERNAL_CLOCK 1
//! use watchdog timeout
#define APP_USE_WATCHDOG 0
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

extern u16_t APP_PANId;
extern u16_t APP_ShortAddress;
extern u8_t  APP_LongAddress[8];
extern u8_t  APP_Channel;

/******************************************************************************/
/****** APP Functions                                                    ******/
/******************************************************************************/
/* tempsense_app */
void  TEMPSENSE_APP_Handler(struct ca821x_dev *pDeviceRef);
void  TEMPSENSE_APP_Initialise(struct ca821x_dev *pDeviceRef);
int   TEMPSENSE_APP_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
void  TEMPSENSE_APP_InitPIB(struct ca821x_dev *pDeviceRef);
void  TEMPSENSE_APP_SaveOrRestoreAddress(struct ca821x_dev *pDeviceRef);
u8_t  TEMPSENSE_APP_GetTempVal(void);
u16_t TEMPSENSE_APP_GetVoltsVal(void);
u32_t TEMPSENSE_APP_GetScanChannels(void);
void  TEMPSENSE_APP_PrintScanChannels(void);
void  TEMPSENSE_APP_PrintSeconds(void);
/* tempsense_app_coord */
void TEMPSENSE_APP_Coordinator_Handler(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Coordinator_Initialise(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Coordinator_Start(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Coordinator_ProcessScanCnf(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Coordinator_AssociateResponse(struct MLME_ASSOCIATE_indication_pset *params,
                                                 struct ca821x_dev *                    pDeviceRef);
void TEMPSENSE_APP_Coordinator_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Coordinator_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Coordinator_DisplayData(u8_t                              device,
                                           u8_t                              edcoord,
                                           struct MCPS_DATA_indication_pset *params,
                                           struct ca821x_dev *               pDeviceRef);
void TEMPSENSE_APP_Coordinator_CheckTimeouts(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Coordinator_CheckVbatt(u16_t vbat);
void TEMPSENSE_APP_Coordinator_CheckLQI(u8_t lqi_ts, u8_t lqi_coord);
void TEMPSENSE_APP_Coordinator_CheckED(u8_t ed_ts, u8_t ed_coord);
void TEMPSENSE_APP_Coordinator_SoftReinit(struct ca821x_dev *pDeviceRef);
/* tempsense_app_device */
void TEMPSENSE_APP_Device_Handler(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_Initialise(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_GetLongAddress(void);
void TEMPSENSE_APP_Device_Start(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_ProcessScanCnf(struct MLME_SCAN_confirm_pset *params, struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_ProcessAssociateCnf(struct MLME_ASSOCIATE_confirm_pset *params,
                                              struct ca821x_dev *                 pDeviceRef);
void TEMPSENSE_APP_Device_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_ExchangeData(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_CheckTimeout(struct ca821x_dev *pDeviceRef);
void TEMPSENSE_APP_Device_GoPowerDown(struct ca821x_dev *pDeviceRef);

#endif // TEMPSENSE_APP_H
