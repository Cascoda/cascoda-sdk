/**
 * @file tempsense_debug.h
 * @brief Over-Air Debug Queue
 * @author Wolfgang Bruchner
 * @date 31/05/16
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
void APP_Debug_Send(struct ca821x_dev *pDeviceRef);
int  APP_Debug_Sent(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
void APP_Debug_Reset(void);
void APP_Debug_Error(u8_t code);
void APP_Debug_SetAppState(u8_t state);

#endif // TEMPSENSE_DEBUG_H
