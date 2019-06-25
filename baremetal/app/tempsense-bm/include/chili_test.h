/**
 * @file chili_test.h
 * @brief Chili Module Production Test Modes
 * @author Wolfgang Bruchner
 * @date 08/02/17
 *//*
 * Copyright (C) 2017  Cascoda, Ltd.
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

#ifndef CHILI_TEST_H
#define CHILI_TEST_H

/******************************************************************************/
/****** EVBME TEMPSENSE Command ID Codes                                 ******/
/******************************************************************************/
#define EVBME_TEST_START_TEST (0x9F)   /* start test (automatic by GUI) */
#define EVBME_TEST_START_TEST_2 (0x9E) /* start test (manual command) */
#define EVBME_TEST_SETUP_REF (0x9D)    /* configure reference device */

/******************************************************************************/
/****** EVBME Command ID Code for Starting Test (DUT)                    ******/
/******************************************************************************/
/* Test Command Packet Payload for extra Safety */
#define EVBME_TEST_LEN (3)
#define EVBME_TEST_DATA0 (0x5A)
#define EVBME_TEST_DATA1 (0xA5)
#define EVBME_TEST_DATA2 (0x81)

/******************************************************************************/
/****** Test Limits                                                      ******/
/******************************************************************************/
#define CHILI_TEST_CS_LIMIT 200    //!< CS (LQI)  Limit
#define CHILI_TEST_ED_LIMIT 100    //!< ED (RSSI) Limit
#define CHILI_TEST_DUT_TIMEOUT 400 //!< DUT Timeout [ms]

/******************************************************************************/
/****** Test Mode Definitions                                            ******/
/******************************************************************************/
#define CHILI_TEST_OFF 0 //!< No Test Mode
#define CHILI_TEST_DUT 1 //!< DUT
#define CHILI_TEST_REF 2 //!< Reference Device

/******************************************************************************/
/****** MAC PIB Values for Test                                          ******/
/******************************************************************************/
#define CHILI_TEST_DUT_SHORTADD 0xCAFE //!< Short Address for DUT
#define CHILI_TEST_REF_SHORTADD 0xCAFF //!< Short Address for Reference Device
#define CHILI_TEST_CHANNEL 11          //!< Channel
#define CHILI_TEST_PANID 0xCA5C        //!< PanID

/******************************************************************************/
/****** Test Communication State Definitions                             ******/
/******************************************************************************/
#define CHILI_TEST_CST_DONE 0x00            //!< communication completed
#define CHILI_TEST_CST_DUT_D_REQUESTED 0x01 //!< data pkt requested  (dut)
#define CHILI_TEST_CST_DUT_D_CONFIRMED 0x02 //!< data pkt confirmed  (dut)
#define CHILI_TEST_CST_DUT_DISPLAY 0x03     //!< display test result (dut)
#define CHILI_TEST_CST_DUT_FINISHED 0x04    //!< test complete       (dut)

/******************************************************************************/
/****** Test Packet Types, defined by MSDU[0]                            ******/
/******************************************************************************/
#define PT_MSDU_TEST_DUT 0xAA //!< packet from DUT
#define PT_MSDU_TEST_REF 0xBB //!< packet from Reference Device

/******************************************************************************/
/****** Test Result/Status Definitions                                   ******/
/******************************************************************************/
#define CHILI_TEST_ST_SUCCESS 0x00          //!< Success / Test Pass
#define CHILI_TEST_ST_NOCOMMS 0x01          //!< No Communication between Chips
#define CHILI_TEST_ST_DATA_CNF_UNEXP 0x02   //!< Data Confirm Unexpected
#define CHILI_TEST_ST_DATA_CNF_TIMEOUT 0x03 //!< Data Confirm Timeout
#define CHILI_TEST_ST_DATA_CNF_NO_ACK 0x04  //!< Data Confirm No Ack
#define CHILI_TEST_ST_DATA_CNF_CHACCF 0x05  //!< Data Confirm Ch. Access Fail
#define CHILI_TEST_ST_DATA_CNF_TROVFL 0x06  //!< Data Confirm Transaction Ovfl.
#define CHILI_TEST_ST_DATA_CNF_OTHERS 0x07  //!< Data Confirm Status Others
#define CHILI_TEST_ST_DATA_IND_UNEXP 0x08   //!< Data Indication Unexpected
#define CHILI_TEST_ST_DATA_IND_TIMEOUT 0x09 //!< Data Indication Timeout
#define CHILI_TEST_ST_DATA_IND_ID 0x0A      //!< Data Indication Id wrong
#define CHILI_TEST_ST_TIMEOUT 0x0B          //!< Overall Timeout
#define CHILI_TEST_ST_CS_REF_LOW 0x0C       //!< CS low on REF Side
#define CHILI_TEST_ST_CS_DUT_LOW 0x0D       //!< CS low on DUT Side
#define CHILI_TEST_ST_ED_REF_LOW 0x0E       //!< ED low on REF Side
#define CHILI_TEST_ST_ED_DUT_LOW 0x0F       //!< ED low on DUT Side

/******************************************************************************/
/****** TEST Functions                                                   ******/
/******************************************************************************/
void CHILI_TEST_Initialise(u8_t status, struct ca821x_dev *pDeviceRef);
void CHILI_TEST_Handler(struct ca821x_dev *pDeviceRef);
int  CHILI_TEST_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
void CHILI_TEST_TestInit(struct ca821x_dev *pDeviceRef);
void CHILI_TEST_InitPIB(struct ca821x_dev *pDeviceRef);
void CHILI_TEST_REF_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
void CHILI_TEST_REF_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
void CHILI_TEST_DUT_ExchangeData(struct ca821x_dev *pDeviceRef);
void CHILI_TEST_DUT_ProcessDataInd(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
void CHILI_TEST_DUT_ProcessDataCnf(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
void CHILI_TEST_DUT_CheckTimeout(struct ca821x_dev *pDeviceRef);
void CHILI_TEST_DUT_DisplayResult(struct ca821x_dev *pDeviceRef);
u8_t CHILI_TEST_IsInTestMode(void);
void CHILI_TEST_LED_Handler(void);
/* Callbacks */
void CHILI_TEST_RegisterCallbacks(struct ca821x_dev *pDeviceRef);

#endif // CHILI_TEST_H
