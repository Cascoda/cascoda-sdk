/*
 *  Copyright (c) 2016, Nest Labs, Inc.
 *  Modifications copyright (c) 2020, Cascoda Ltd.
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

#define _DEFAULT_SOURCE 1
#define _POSIX_SOURCE 1

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ca821x-posix-thread/posix-platform.h"
#include "openthread/platform/alarm-milli.h"

static bool            s_is_running = false;
static uint32_t        s_alarm      = 0;
static struct timespec s_start;

void posixPlatformAlarmInit(void)
{
	clock_gettime(CLOCK_MONOTONIC, &s_start);
}

uint32_t otPlatAlarmMilliGetNow(void)
{
	struct timespec tv;

	clock_gettime(CLOCK_MONOTONIC, &tv);

	tv.tv_sec  = tv.tv_sec - s_start.tv_sec;
	tv.tv_nsec = tv.tv_nsec - s_start.tv_nsec;

	if (tv.tv_nsec < 0)
	{
		--tv.tv_sec;
		tv.tv_nsec += 1000000000;
	}

	return (uint32_t)((tv.tv_sec * 1000) + (tv.tv_nsec / 1000000));
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
	(void)aInstance;
	s_alarm      = t0 + dt;
	s_is_running = true;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
	(void)aInstance;
	s_is_running = false;
}

void posixPlatformAlarmUpdateTimeout(struct timeval *aTimeout)
{
	int32_t remaining;

	if (aTimeout == NULL)
	{
		return;
	}

	if (s_is_running)
	{
		remaining = (int32_t)(s_alarm - otPlatAlarmMilliGetNow());

		if (remaining > 0)
		{
			aTimeout->tv_sec  = remaining / 1000;
			aTimeout->tv_usec = (remaining % 1000) * 1000;
		}
		else
		{
			aTimeout->tv_sec  = 0;
			aTimeout->tv_usec = 0;
		}
	}
	else
	{
		aTimeout->tv_sec  = 10;
		aTimeout->tv_usec = 0;
	}
}

void posixPlatformAlarmProcess(otInstance *aInstance)
{
	int32_t remaining;

	if (s_is_running)
	{
		remaining = (int32_t)(s_alarm - otPlatAlarmMilliGetNow());

		if (remaining <= 0)
		{
			s_is_running = false;
			otPlatAlarmMilliFired(aInstance);
		}
	}
}
