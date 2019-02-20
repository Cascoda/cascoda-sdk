/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"

#include "openthread/platform/alarm-milli.h"

static bool  s_IsRunning = false;
static u32_t s_Expires;

uint32_t otPlatAlarmMilliGetNow(void)
{
	return TIME_ReadAbsoluteTime();
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
	s_Expires   = aT0 + aDt;
	s_IsRunning = true;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
	s_IsRunning = false;
}

void PlatformAlarmProcess(otInstance *aInstance)
{
	if (s_IsRunning)
	{
		int32_t remaining = (int32_t)(s_Expires - otPlatAlarmMilliGetNow());

		if (remaining <= 0)
		{
			s_IsRunning = false;
			otPlatAlarmMilliFired(aInstance);
		}
	}
}

uint32_t PlatformGetAlarmMilliTimeout()
{
	int32_t remaining;

	if (s_IsRunning == false)
	{
		return 0;
	}
	remaining = s_Expires - otPlatAlarmMilliGetNow();
	return (remaining < 0) ? 0 : remaining;
}
