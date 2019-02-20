/*
 * cascoda-chili-usb.h
 *
 *  Created on: 22 Nov 2018
 *      Author: ciaran
 */

#ifndef PORT_INCLUDE_CASCODA_CHILI_USB_H_
#define PORT_INCLUDE_CASCODA_CHILI_USB_H_

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"

#if defined(USE_USB)
void  USB_Initialise(void);
void  USB_FloatDetect(void);
void  USB_BusEvent(void);
void  USB_UsbEvent(uint32_t u32INTSTS);
void  HID_Initialise(void);
void  HID_ClassRequest(void);
u32_t HID_Ep2Handler(void);
u32_t HID_Ep3Handler(void);
u8_t  HID_Transmit(u8_t *pBuffer);
u8_t  HID_Receive(u8_t *pBuffer);
void  HID_SetConnectedFlag(bool is_connected);
#endif // USE_USB

#endif /* PORT_INCLUDE_CASCODA_CHILI_USB_H_ */
