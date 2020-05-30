/**
 * @file
 * @brief test15_4 main program loop and supporting functions
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
/*
 * Example application for external sensor interfaces
*/
#include <stdio.h>
#include <stdlib.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "sensorif_app.h"
#include "test15_4_evbme.h"

/******************************************************************************/
/****** Application name                                                 ******/
/******************************************************************************/
#define APP_NAME "SENSORIF"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch function to process received serial messages
 *******************************************************************************
 * \param buf - serial buffer to dispatch
 * \param len - length of buf
 * \param pDeviceRef - pointer to a CA-821x Device reference struct
 *******************************************************************************
 * \return 1: consumed by driver 0: command to be sent downstream to spi
 *******************************************************************************
 ******************************************************************************/
static int test15_4_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret = 0;
	if ((ret = TEST15_4_UpStreamDispatch((struct SerialBuffer *)(buf), pDeviceRef)))
		return ret;
	/* Insert Application-Specific Dispatches here in the same style */
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Main Program Endless Loop
 *******************************************************************************
 * \return Does not return
 *******************************************************************************
 ******************************************************************************/
int main(void)
{
	struct ca821x_dev dev;
	ca821x_api_init(&dev);
	cascoda_serial_dispatch = test15_4_serial_dispatch;

	/* Initialisation of Chip and EVBME */
	/* Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action */
	/* in case there is no UpStream Communications Channel available */
	EVBMEInitialise(APP_NAME, &dev);
	/* Insert Application-Specific Initialisation Routines here */
	TEST15_4_Initialise(&dev);
	SENSORIF_Initialise(&dev);

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);

		/* Insert Application-Specific Event Handlers here */
		TEST15_4_Handler(&dev);
		SENSORIF_Handler(&dev);

	} /* while(1) */
}
