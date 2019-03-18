/**
 * @file test15_4_evbme.h
 * @brief test15_4 test integration defines
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
void TEST15_4_Initialise(struct ca821x_dev *pDeviceRef);
void TEST15_4_Handler(struct ca821x_dev *pDeviceRef);
int  TEST15_4_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
u8_t EVBME_PHY_TESTMODE_request(u8_t TestMode, struct ca821x_dev *pDeviceRef);
u8_t EVBME_PHY_SET_request(u8_t Parameter, u8_t ParameterLength, u8_t *ParameterValue);
void EVBME_PHY_REPORT_request(void);
void TEST15_4_SetupAwaitAssoc(uint8_t *pDeviceAddress, uint16_t AssocShortAddress, uint8_t Status);
void TEST15_4_SetupAwaitOrphan(uint8_t *pDeviceAddress, uint16_t OrphanShortAddress);
/* Callbacks */
static ca_error TEST15_4_AssociateIndication(struct MLME_ASSOCIATE_indication_pset *params,
                                             struct ca821x_dev *                    pDeviceRef);
static ca_error TEST15_4_OrphanIndication(struct MLME_ORPHAN_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error TEST15_4_PHY_RXPKT_indication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error TEST15_4_MAC_RXPKT_indication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error TEST15_4_PHY_EDDET_indication(struct TDME_EDDET_indication_pset *params, struct ca821x_dev *pDeviceRef);
void            TEST15_4_RegisterCallbacks(struct ca821x_dev *pDeviceRef);

#endif // TEST15_4_EVBME_H
