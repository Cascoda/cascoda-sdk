/**
 * @file
 * @brief Chili temperature sensing EVBME declarations
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

#ifndef TEMPSENSE_EVBME_H
#define TEMPSENSE_EVBME_H

/******************************************************************************/
/****** EVBME TEMPSENSE Command ID Codes                                 ******/
/******************************************************************************/
#define EVBME_TSENSE_SETMODE (0xC0) /* set mode (NORMAL/COORDINATOR/DEVICE) */
#define EVBME_TSENSE_REPORT (0xC1)  /* Report Network Status (COORDINATOR) */

/******************************************************************************/
/****** EVBME API Functions                                              ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise Chili module for temperature sensing app
 *******************************************************************************
 * \param status - EVBME status
 * \param pDeviceRef - pointer to a CA-821x Device reference struct
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_Initialise(u8_t status, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Chili Event Handler in Main Polling Loop
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_Handler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE Dispatch Branch (UpStream, Serial)
 *******************************************************************************
 ******************************************************************************/
int TEMPSENSE_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
/* Callbacks */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dynamically Register Callbacks for TEMPSENSE
 *******************************************************************************
 ******************************************************************************/
void TEMPSENSE_RegisterCallbacks(struct ca821x_dev *pDeviceRef);

#endif // TEMPSENSE_EVBME_H
