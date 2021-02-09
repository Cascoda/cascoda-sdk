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
#include "sntp_helper.h"

#include "ca-ot-util/cascoda_dns.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "openthread/dns.h"
#include "openthread/sntp.h"
#include "openthread/thread.h"
#include "platform.h"

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

enum time_constants
{
	// The time between retries, if a SNTP or a DNS query fails
	REQUEST_TIMEOUT_MS = 60 * 1000, // one minute
	// After a successful SNTP query, the clock must be refreshed in order to mitigate clock drift.
	// This constant defines the time between such refreshes.
	REFRESH_MS = 48 * 60 * 60 * 1000, // fourty eight hours
};

static enum sntp_query_state state = NO_TIME;
// This tasklet is used for retrying  after failed queries, as well as refreshing the wall time
// after a significant amount of time passes
static ca_tasklet tasklet;

void            do_dns_query();
static void     do_sntp_query(const otIp6Address aAddress);
void            dns_response_handler(ca_error aError, const otIp6Address *aAddress, dns_index aIndex, void *aContext);
static void     sntp_response_handler(void *aContext, uint64_t aTime, otError aResult);
static ca_error dns_retry_handler(void *aContext);

void SNTP_Init()
{
	TASKLET_Init(&tasklet, dns_retry_handler);
}

ca_error SNTP_Update()
{
	if (state == NO_TIME || state == HAVE_TIME || state == RETRYING)
	{
		do_dns_query();
		return CA_ERROR_SUCCESS;
	}
	else
	{
		return CA_ERROR_ALREADY;
	}
}

void do_dns_query()
{
	enum ca_error error;
	otDeviceRole  role = otThreadGetDeviceRole(OT_INSTANCE);

	if (role == OT_DEVICE_ROLE_DETACHED || role == OT_DEVICE_ROLE_DISABLED)
		error = CA_ERROR_INVALID_STATE;
	else
		error = DNS_HostToIpv6(OT_INSTANCE, NTP_SERVER, dns_response_handler, NULL);

	if (error == CA_ERROR_SUCCESS)
	{
		state = WAITING_FOR_DNS;
	}
	else
	{
		state = RETRYING;
		TASKLET_Cancel(&tasklet);
		TASKLET_ScheduleDelta(&tasklet, REQUEST_TIMEOUT_MS, NULL);
	}
}

void dns_response_handler(ca_error aError, const otIp6Address *aAddress, dns_index aIndex, void *aContext)
{
	(void)aIndex;
	(void)aContext;
	if (aError == CA_ERROR_SUCCESS)
	{
		do_sntp_query(*aAddress);
		state = WAITING_FOR_SNTP;
	}
	else
	{
		state = RETRYING;
		TASKLET_Cancel(&tasklet);
		TASKLET_ScheduleDelta(&tasklet, REQUEST_TIMEOUT_MS, NULL);
	}
}

static void do_sntp_query(const otIp6Address aAddress)
{
	otMessageInfo info = {};
	otSntpQuery   query;
	otError       error;

	// Well-known NTP Port
	info.mPeerPort     = 123;
	info.mPeerAddr     = aAddress;
	query.mMessageInfo = &info;

	error = otSntpClientQuery(OT_INSTANCE, &query, sntp_response_handler, NULL);

	if (error != OT_ERROR_NONE)
	{
		state = RETRYING;
		TASKLET_Cancel(&tasklet);
		TASKLET_ScheduleDelta(&tasklet, REQUEST_TIMEOUT_MS, NULL);
	}
}

static void sntp_response_handler(void *aContext, uint64_t aTime, otError aResult)
{
	struct RTCDateAndTime dt;
	(void)aContext;
	if (aResult == OT_ERROR_NONE)
	{
		BSP_RTCConvertSecondsToDateAndTime(aTime, &dt);
		BSP_RTCSetDateAndTime(dt);

		TASKLET_Cancel(&tasklet);
		TASKLET_ScheduleDelta(&tasklet, REFRESH_MS, NULL);
		state = HAVE_TIME;
	}
	else
	{
		TASKLET_Cancel(&tasklet);
		TASKLET_ScheduleDelta(&tasklet, REQUEST_TIMEOUT_MS, NULL);
		state = RETRYING;
	}
}

static ca_error dns_retry_handler(void *aContext)
{
	(void)aContext;
	do_dns_query();
	return CA_ERROR_SUCCESS;
}

enum sntp_query_state SNTP_GetState()
{
	return state;
}
