/*
 * Copyright (c) 2021, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Declarations of SNTP helper functions
 */
/**
 * @ingroup bm-thread
 * @defgroup sntp-helper Helper functions for updating the RTC using SNTP
 * @brief  SNTP helper functions
 *
 * @{
 */

#ifndef WALL_TIME_H
#define WALL_TIME_H

#include "ca821x_error.h"

enum sntp_query_state
{
	NO_TIME = 0,
	WAITING_FOR_DNS,
	WAITING_FOR_SNTP,
	HAVE_TIME,
	RETRYING,
};

/**
 * @brief Initialise the SNTP library.
 * 
 */
void SNTP_Init();

/** 
 * @brief Start the process for updating the RTC using an SNTP request.
 * 
 * This is an asynchronous (non-blocking) function, which makes progress as long
 * as otTaskletsProcess() and cascoda_io_handler() are being called regularly. The
 * RTC will be updated once both the SNTP and DNS requests succeed.
 * 
 * Once SNTP_Update is called, the library will keep retrying until all requests
 * succeed, and then it will refresh the RTC every 48 hours.
 * 
 * Calling this function while the library is in the HAVE_TIME or RETRYING states
 * will make it start an update immediately, without waiting for the timeouts
 * inherent in those states.
 * 
 * @return ca_error CA_ERROR_SUCCESS if the request to update the RTC is
 * successful. CA_ERROR_ALREADY if the library is already in the process of
 * refreshing the RTC.
 */
ca_error SNTP_Update();

/** 
 * @brief Get the status of the library, which indicates whether the RTC has
 * been updated using SNTP.
 * 
 * @return enum sntp_query_state NO_TIME if the time has never been updated,
 * WAITING_FOR_DNS or WAITING_FOR_SNTP if an update is in progress, RETRYING if
 * an unsuccessful update was attempted, and HAVE_TIME if the RTC has been
 * successfully updated.
 */
enum sntp_query_state SNTP_GetState();

#endif // WALL_TIME_H

/**
 * @}
 */
