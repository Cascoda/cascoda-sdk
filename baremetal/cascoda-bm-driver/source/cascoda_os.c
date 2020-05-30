/*
 * Copyright (c) 2020, Cascoda
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
/*
 * Default baremetal OS abstraction
 * Purely does runtime checking of locking/unlocking to make sure it's balanced
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "cascoda-bm/cascoda_os.h"
#include "ca821x_toolchain.h"

// Note to reader: These are only used for internal checking and should NOT be relied upon.
enum
{
	LOCK_INIT   = 0xCA,
	LOCK_LOCKED = 0xC0,
};

static int sched_counter  = 0;
static int ca_api_counter = 0;

CA_TOOL_WEAK void CA_OS_Init()
{
}

CA_TOOL_WEAK void CA_OS_LockAPI()
{
	assert(ca_api_counter >= 0);
	ca_api_counter++;
}

CA_TOOL_WEAK void CA_OS_UnlockAPI()
{
	ca_api_counter--;
	assert(ca_api_counter >= 0);
}

CA_TOOL_WEAK void CA_OS_Yield()
{
}

CA_TOOL_WEAK ca_mutex CA_OS_MutexInit()
{
	intptr_t rval = LOCK_INIT;
	return (ca_mutex)rval;
}

CA_TOOL_WEAK void CA_OS_MutexLock(ca_mutex *aMutex)
{
	assert((intptr_t)(*aMutex) == LOCK_INIT);
	*((intptr_t *)aMutex) = LOCK_LOCKED;
}

CA_TOOL_WEAK void CA_OS_MutexUnlock(ca_mutex *aMutex)
{
	assert((intptr_t)(*aMutex) == LOCK_LOCKED);
	*((intptr_t *)aMutex) = LOCK_INIT;
}

CA_TOOL_WEAK void CA_OS_SchedulerSuspend(void)
{
	assert(sched_counter >= 0);
	sched_counter++;
}

CA_TOOL_WEAK void CA_OS_SchedulerResume(void)
{
	sched_counter--;
	assert(sched_counter >= 0);
}
