/**
 * @file
 * @brief Over-Air Debug Queue
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

#ifndef TEMPSENSE_DEBUG_H
#define TEMPSENSE_DEBUG_H

/******************************************************************************/
/****** Debug Definitions                                               ******/
/******************************************************************************/
#define DEBUG_INTERVALL 4 //!< active state send interval [seconds]
#define DEBUG_CHANNEL 26  //!< 15.4 channel for debug messages

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/

/**
 * Debug_App_State Conventions:\n
 * 0x00: reset after sending Debug packet or not in Application Mode\n
 * 0xA*: Coordinator Pairing States\n
 * 0xB*: Device      Pairing States\n
 * 0xC*: Coordinator Data Transfer States\n
 * 0xD*: Device      Data Transfer States\n
 */
extern u8_t Debug_App_State;
extern u8_t Debug_App_Error;
extern u8_t Debug_Sent;

/******************************************************************************/
/****** Debug Functions                                                  ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send Debug Frame
 *******************************************************************************
 ******************************************************************************/
void APP_Debug_Send(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Debug Frame has been sent
 *******************************************************************************
 ******************************************************************************/
int APP_Debug_Sent(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset all APP Level Debug Codes
 *******************************************************************************
 ******************************************************************************/
void APP_Debug_Reset(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Assign Error Code
 *******************************************************************************
 * \param code - debug code
 *******************************************************************************
 ******************************************************************************/
void APP_Debug_Error(u8_t code);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set Application Debug State
 *******************************************************************************
 * \param state- debug state
 *******************************************************************************
 ******************************************************************************/
void APP_Debug_SetAppState(u8_t state);

#endif // TEMPSENSE_DEBUG_H
