/**
 * @file test15_4_main.c
 * @brief test15_4 main program loop and supporting functions
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
#include <stdlib.h>

#include "cascoda-bm/cascoda_debug.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

/* Insert Application-Specific Includes here */
#include "test15_4_evbme.h"

/******************************************************************************/
/****** Function and Version                                             ******/
/******************************************************************************/
#define FUNCTION_VERSION "TEST15_4 V2.0"

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch function to process received serial messages
 *******************************************************************************
 * \param buf - serial buffer to dispatch
 * \param len - length of buf
 *******************************************************************************
 * \return 1: consumed by driver 0: command to be sent downstream to spi
 *******************************************************************************
 ******************************************************************************/
int test15_4_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret = 0;
	if ((ret = TEST15_4_UpStreamDispatch((struct SerialBuffer *)(buf - 1), pDeviceRef)))
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
	/* Returns a Status of EVBME_SUCCESS/EVBME_FAIL for further Action */
	/* in case there is no UpStream Communications Channel available */
	EVBMEInitialise((uint8_t *)FUNCTION_VERSION, &dev);

	/* Insert Application-Specific Initialisation Routines here */
	TEST15_4_Initialise(&dev);

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&dev);

		/* Insert Application-Specific Event Handlers here */
		TEST15_4_Handler(&dev);

	} /* while(1) */
}
