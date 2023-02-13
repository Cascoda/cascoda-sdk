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
/*
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton Nano120
 * MODULE: Chili 1 (1.2, 1.3)
 * USB Functions
*/
#ifndef CASCODA_CHILI_USB_H
#define CASCODA_CHILI_USB_H

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"

#if defined(USE_USB)

#define USB_TX_RETRIES (100)

/* USB Functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise USB interface
 *******************************************************************************
 ******************************************************************************/
void USB_Initialise(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles USB plug/unplug events
 *******************************************************************************
 ******************************************************************************/
void USB_FloatDetect(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles USB state events (e.g. suspend/resume)
 *******************************************************************************
 ******************************************************************************/
void USB_BusEvent(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handles USB transfer events (descriptors or application data)
 *******************************************************************************
 * \param u32INTSTS - Interrupt status bitmap
 *******************************************************************************
 ******************************************************************************/
void USB_UsbEvent(uint32_t u32INTSTS);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Put HID fragment into transmit buffers and trigger transmission if
 *        stalled
 *******************************************************************************
 * \param pBuffer - Pointer to fragment to transmit
 *******************************************************************************
 * \return 1 if successful, 0 if buffers are full
 *******************************************************************************
 ******************************************************************************/
u8_t USB_Transmit(u8_t *pBuffer);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set USB Connected Flag
 *******************************************************************************
 ******************************************************************************/
void USB_SetConnectedFlag(bool is_connected);

#endif /* USE_USB */

#endif /* CASCODA_CHILI_USB_H */
