
/*
 *  Copyright (c) 2023, Cascoda Ltd.
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
/**
 * @file
 *
 * @brief Demo application which uses PWM to dim an LED to various brightness levels
 */

#include "buzz2_click.h"
#include "buzz2_click_test.h"

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_ota_upgrade.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "ca821x_api.h"

#include <stdlib.h>

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

	/* Initialisation of Chip and EVBME */
	/* Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action */
	/* in case there is no UpStream Communications Channel available */
	EVBMEInitialise(CA_TARGET_NAME, &dev);

	/* Insert Application-Specific Initialisation Routines here */
	MIKROSDK_BUZZ2_Initialise();

#if CASCODA_OTA_UPGRADE_ENABLED
	/* Initialises handling of OTA Firmware Upgrade */
	ota_upgrade_init();
#endif

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);
		buzz2_ascending_scale();
	} /* while(1) */
}
