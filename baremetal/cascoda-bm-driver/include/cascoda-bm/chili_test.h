/**
 * @file
 * @brief Chili Module Production Test Modes
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

#ifndef CHILI_TEST_H
#define CHILI_TEST_H

#include "cascoda-bm/cascoda_types.h"

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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Production Test Initialisation
 *******************************************************************************
 ******************************************************************************/
void CHILI_TEST_Initialise(u8_t status, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
  * \brief Chili Production Test Handler
  *******************************************************************************
  ******************************************************************************/
void CHILI_TEST_Handler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Test Dispatch Branch (UpStream, Serial)
 *******************************************************************************
 ******************************************************************************/
int CHILI_TEST_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks if Device is in Production Test Mode (yes if non-zero)
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_TEST_IsInTestMode(void);

#endif // CHILI_TEST_H
