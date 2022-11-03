/**
 * @file
 * @brief test15_4 main program loop and supporting functions
 */
/*
 *  Copyright (c) 2022, Cascoda Ltd.
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
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "cascoda-bm/test15_4_evbme.h"
#include "cascoda_devboard_click.h"

/* measurement period in [ms] */
#define MEASUREMENT_PERIOD 5000
/* delta in [ms] around measurement time */
/* needs to be increased of more devices are tested */
#define MEASUREMENT_DELTA 100
/* report measurement times flag */
#define SENSORIF_REPORT_TMEAS 1

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Main Program Endless Loop
 *******************************************************************************
 * \return Does not return
 *******************************************************************************
 ******************************************************************************/
int main(void)
{
	u8_t               handled = 0;
	u32_t              status  = 0;
	uint32_t           portnum = 1, tnum = 1;
	uint32_t           dev_num[] = {10};
	mikrosdk_callbacks sensor_callbacks[tnum];
	struct ca821x_dev  dev;
	ca821x_api_init(&dev);

	cascoda_serial_dispatch = TEST15_4_SerialDispatch;

	/* Initialisation of Chip and EVBME */
	/* Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action */
	/* in case there is no UpStream Communications Channel available */
	EVBMEInitialise(CA_TARGET_NAME, &dev);
	// /* Insert Application-Specific Initialisation Routines here */
	for (int i = 0; i < tnum; ++i)
	{
		if (dev_num[i] >= 10)
		{
			select_sensor_gpio(&sensor_callbacks[i], dev_num[i], 6, 5, &MIKROSDK_Handler_MOTION);
		}
		else
		{
			select_sensor_comm(&sensor_callbacks[i], dev_num[i], portnum, &MIKROSDK_Handler_THERMO);
		}
		status = sensor_callbacks[i].sensor_initialise();
		if (status)
			printf("Sensor Number : %d, Status = %02X\n", sensor_callbacks[i].dev_number, status);
	}

	// /* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);

		/* Insert Application-Specific Event Handlers here */
		/* Note:
		* This is a tick based handler for polling only
		* In applications this should be based on timer-interrupts.
		*/
		if (((TIME_ReadAbsoluteTime() % MEASUREMENT_PERIOD) < MEASUREMENT_DELTA) && (!handled))
		{
			for (int i = 0; i < tnum; ++i) sensor_callbacks[i].sensor_handler();
			BSP_WaitTicks(200);
			printf("\n");
		}
		if ((TIME_ReadAbsoluteTime() % MEASUREMENT_PERIOD) > (MEASUREMENT_PERIOD - MEASUREMENT_DELTA))
		{
			handled = 0;
		}

	} /* while(1) */
}