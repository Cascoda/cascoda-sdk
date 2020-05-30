/**
 * @file
 * @brief test15_4 test integration defines
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
#include "cascoda-bm/cascoda_types.h"

#ifndef TEST15_4_EVBME_H
#define TEST15_4_EVBME_H

/******************************************************************************/
/****** EVBME Command ID Codes                                           ******/
/******************************************************************************/
#define EVBME_PHY_TESTMODE_REQUEST (0x83)
#define EVBME_PHY_SET_REQUEST (0x84)
#define EVBME_PHY_REPORT_REQUEST (0x85)
#define EVBME_FFD_AWAIT_ASSOC_REQUEST (0x8D)
#define EVBME_FFD_AWAIT_ORPHAN_REQUEST (0x8E)

/******************************************************************************/
/****** EVBME API Functions                                              ******/
/******************************************************************************/
/* TEST15_4 EVBME Functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEST15_4 Initialistion
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_Initialise(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEST15_4 Event Handler in Main Polling Loop
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_Handler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch Branch for EVBME Request (UpStream, Serial)
 *******************************************************************************
 ******************************************************************************/
int TEST15_4_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_PHY_TESTMODE_request according to EVBME Spec
 *******************************************************************************
 * \param TestMode - Test Mode
 * \param pDeviceRef - pointer to a CA-821x Device reference struct
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
u8_t EVBME_PHY_TESTMODE_request(u8_t TestMode, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_PHY_SET_request according to EVBME Spec
 *******************************************************************************
 * \param Parameter - Parameter Specifier
 * \param ParameterLength - Parameter Length
 * \param ParameterValue - Pointer to Parameter Value
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
u8_t EVBME_PHY_SET_request(u8_t Parameter, u8_t ParameterLength, u8_t *ParameterValue);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_PHY_REPORT_request according to EVBME Spec
 *******************************************************************************
 ******************************************************************************/
void EVBME_PHY_REPORT_request(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set up Association Response when waiting for it
 *******************************************************************************
 * \param pDeviceAddress - IEEE address of device expected
 * \param AssocShortAddress - Short address for end device
 * \param Status - EVBME Status
 *******************************************************************************
 * \return -
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_SetupAwaitAssoc(uint8_t *pDeviceAddress, uint16_t AssocShortAddress, uint8_t Status);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set up Orphan Response when waiting for it
 *******************************************************************************
 * \param pDeviceAddress - IEEE address of device expected
 * \param OrphanShortAddress - Short address of orphan
 *******************************************************************************
 * \return -
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_SetupAwaitOrphan(uint8_t *pDeviceAddress, uint16_t OrphanShortAddress);
/* Callbacks */
static ca_error TEST15_4_AssociateIndication(struct MLME_ASSOCIATE_indication_pset *params,
                                             struct ca821x_dev *                    pDeviceRef);
static ca_error TEST15_4_OrphanIndication(struct MLME_ORPHAN_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error TEST15_4_MAC_TXPKT_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error TEST15_4_PHY_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error TEST15_4_MAC_RXPKT_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error TEST15_4_PHY_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dynamically Register Callbacks for TEST15_4
 *******************************************************************************
 ******************************************************************************/
void TEST15_4_RegisterCallbacks(struct ca821x_dev *pDeviceRef);

#endif // TEST15_4_EVBME_H
