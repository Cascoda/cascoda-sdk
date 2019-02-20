/**
 * @file tempsense_evbme.h
 * @brief Chili temperature sensing EVBME declarations
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

#ifndef TEMPSENSE_EVBME_H
#define TEMPSENSE_EVBME_H

/******************************************************************************/
/****** EVBME API Functions                                              ******/
/******************************************************************************/
void TEMPSENSE_Initialise(u8_t status, struct ca821x_dev *pDeviceRef);
void TEMPSENSE_Handler(struct ca821x_dev *pDeviceRef);
int  TEMPSENSE_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
/* Callbacks */
void TEMPSENSE_RegisterCallbacks(struct ca821x_dev *pDeviceRef);

#endif // TEMPSENSE_EVBME_H
