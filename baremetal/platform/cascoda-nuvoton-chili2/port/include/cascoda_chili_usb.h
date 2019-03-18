/*
 * Copyright (C) 2019  Cascoda, Ltd.
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
/*
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * USB Functions
*/
#ifndef CASCODA_CHILI_USB_H
#define CASCODA_CHILI_USB_H

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"

#if defined(USE_USB)

#define USB_TX_RETRIES (1000)

/* USB Functions */
void USB_Initialise(void);
void USB_Deinitialise(void);
void USB_FloatDetect(void);
void USB_BusEvent(void);
void USB_UsbEvent(uint32_t u32INTSTS);
u8_t USB_Transmit(u8_t *pBuffer);
u8_t USB_Receive(u8_t *pBuffer);
void USB_SetConnectedFlag(bool is_connected);

#endif /* USE_USB */

#endif /* CASCODA_CHILI_USB_H */
