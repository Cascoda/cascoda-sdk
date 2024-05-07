/*
 *  Copyright (c) 2024, Cascoda Ltd.
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
 * Aerial Adapter (printf over Air), interfaces to testref app.
*/

#ifndef CASCODA_AERIAL_ADAPTER_H
#define CASCODA_AERIAL_ADAPTER_H

#include "ca821x_api.h"

/* Note that this configuration has to match up with configuration of production test */
/* reference device testref (baremetal/app/testref/include/testref.h) */

/****** MAC PIB Setup for Test */
#define AERIAL_ADAPTER_DUT_SHORTADD 0xCAFE /* Short Address for DUT */
#define AERIAL_ADAPTER_REF_SHORTADD 0xCAFF /* Short Address for Reference Device */
#define AERIAL_ADAPTER_PANID 0xCA5C        /* PanID */
#define AERIAL_ADAPTER_MSDU_HANDLE 0x05    /* Fixed Msdu handle for reference device (only one data request) */

#define AERIAL_ADAPTER_EDTHRESHOLD 128 /* Increase ED treshold to get rid of interference (default 60) */
#define AERIAL_ADAPTER_CSTHRESHOLD 128 /* Increase CS treshold to get rid of interference (default 80) */

/* Positions in MAC Payload */
#define AERIAL_ADAPTER_MSDU_POS_ID 0      /* Byte  0 - Test Packet Identifier */
#define AERIAL_ADAPTER_MSDU_POS_TYPE 1    /* Byte  1 - Test Packet Type */
#define AERIAL_ADAPTER_MSDU_POS_ED 2      /* Byte  2 - ED of last received Packet */
#define AERIAL_ADAPTER_MSDU_POS_CS 3      /* Byte  3 - CS of last received Packet */
#define AERIAL_ADAPTER_MSDU_POS_STATUS 4  /* Byte  4 - Test Status */
#define AERIAL_ADAPTER_MSDU_POS_MESSAGE 5 /* from Byte  5: Message if attached */

/* Test Packet Identifier */
#define AERIAL_ADAPTER_PKT_ID 0xA5

/* Test Packet Types */
#define AERIAL_ADAPTER_TYPE_REQUEST 0x01  /* DUT -> REF: Test Initialisation Request */
#define AERIAL_ADAPTER_TYPE_CONFIRM 0x02  /* REF -> DUT: Test Initialisation Confirm */
#define AERIAL_ADAPTER_TYPE_MESSAGE 0x03  /* DUT -> REF: Test Message for Terminal   */
#define AERIAL_ADAPTER_TYPE_COMPLETE 0x04 /* DUT -> REF: Test Complete               */

/* maxium message size */
/* has to fit in 1 802.15.4 packet as no fragmentation supported */
#define AERIAL_ADAPTER_MAX_MESSAGE 100

/* Functions */

/**
 * \brief Initialise and start aerial adapter
 * \param channel - IEEE802.15.4 channel number (11-26)
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *
 */
ca_error aainitialise(uint8_t channel, struct ca821x_dev *pDeviceRef);

/**
 * \brief Print message over air (max. AERIAL_ADAPTER_MAX_MESSAGE characters)
 * \param format printf-style format string, followed by printf-style arguments
 *
 */
int aaprintf(const char *format, ...);

#endif // CASCODA_AERIAL_ADAPTER_H
