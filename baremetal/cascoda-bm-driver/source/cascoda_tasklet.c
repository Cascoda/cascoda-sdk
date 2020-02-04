/*
 *  Copyright (c) 2020, Cascoda Ltd.
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
 * @brief  Helper 'tasklet' framework for scheduling simple events for the future.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_tasklet.h"
#include "cascoda-bm/cascoda_time.h"

static ca_tasklet *sTaskletHead = NULL;

/**
 * Get how far into the future 'future' is from 'start'.
 * @param start  The start time
 * @param future  The future event
 * @return The delta time between 'start' and 'future', or 0 if 'future' is before 'start'.
 */
static uint32_t GetTimeToEvent(uint32_t start, uint32_t future)
{
	uint32_t delta = future - start;

	//If the delta is too high, we assume that it looped around and is negative.
	if (delta >= 0x80000000UL)
		delta = 0;

	return delta;
}

ca_error TASKLET_Init(ca_tasklet *aTasklet, ca_tasklet_callback aCallback)
{
	/* Note we do not check for the invalid usage of initialising a scheduled
	 * callback, as this is quite indistinguishable from uninitialised memory.
	 */
	assert(aTasklet);
	memset(aTasklet, 0, sizeof(*aTasklet));
	aTasklet->callback = aCallback;
	return CA_ERROR_SUCCESS;
}

ca_error TASKLET_ScheduleDelta(ca_tasklet *aTasklet, uint32_t aTimeDelta, void *aContext)
{
	uint32_t now = TIME_ReadAbsoluteTime();
	return TASKLET_ScheduleAbs(aTasklet, now, aTimeDelta + now, aContext);
}

ca_error TASKLET_ScheduleAbs(ca_tasklet *aTasklet, uint32_t aTimeNow, uint32_t aTimeAbs, void *aContext)
{
	ca_tasklet * cur   = sTaskletHead;  //Current tasklet being processed
	ca_tasklet **prevn = &sTaskletHead; //'Previous next', the pointer that points to the current tasklet

	if (TASKLET_IsQueued(aTasklet))
		return CA_ERROR_ALREADY;

	if (!GetTimeToEvent(aTimeNow, aTimeAbs) && (aTimeNow != aTimeAbs))
		return CA_ERROR_INVALID_ARGS;

	//Set the constant tasklet parameters
	aTasklet->fireTime = aTimeAbs;
	aTasklet->context  = aContext;

	//Loop through the linked list until we find the correct place for the new tasklet
	while (cur)
	{
		uint32_t delta = GetTimeToEvent(cur->fireTime, aTimeAbs);
		if (!delta)
			break; //Current tasklet is scheduled for after new one

		//current tasklet is before new one, move through the list and update state.
		prevn = &cur->next;
		cur   = cur->next;
	}

	*prevn              = aTasklet; //Update the 'previous next' pointer to point to the new tasklet.
	aTasklet->next      = cur;      //Set the new tasklet's 'next' pointer to point to the next item in the list
	aTasklet->scheduled = 1;

	return CA_ERROR_SUCCESS;
}

ca_error TASKLET_GetScheduledTime(ca_tasklet *aTasklet, uint32_t *aTimeAbs)
{
	if (!TASKLET_IsQueued(aTasklet))
		return CA_ERROR_INVALID_STATE;

	*aTimeAbs = aTasklet->fireTime;
	return CA_ERROR_SUCCESS;
}

ca_error TASKLET_Cancel(ca_tasklet *aTasklet)
{
	ca_tasklet * cur   = sTaskletHead;  //Current tasklet being processed
	ca_tasklet **prevn = &sTaskletHead; //'Previous next', the pointer that points to the current tasklet

	if (!TASKLET_IsQueued(aTasklet))
		return CA_ERROR_ALREADY;

	//Loop through the linked list until we find the tasklet
	while (cur)
	{
		if (cur == aTasklet)
		{
			//We found the tasklet, remove it by pointing the 'previous next' to the following node
			*prevn         = cur->next;
			cur->next      = NULL;
			cur->scheduled = 0;
			break;
		}

		//move through the list and update state.
		prevn = &cur->next;
		cur   = cur->next;
	}
	assert(cur); //Invalid internal state - hit end of list before locating

	return CA_ERROR_SUCCESS;
}

bool TASKLET_IsQueued(ca_tasklet *aTasklet)
{
	return aTasklet->scheduled;
}

ca_error TASKLET_GetTimeToNext(uint32_t *aTimeDelta)
{
	uint32_t delta;
	if (!sTaskletHead)
		return CA_ERROR_NOT_FOUND;

	delta = GetTimeToEvent(TIME_ReadAbsoluteTime(), sTaskletHead->fireTime);

	*aTimeDelta = delta;

	return CA_ERROR_SUCCESS;
}

ca_error TASKLET_Process(void)
{
	ca_error error = CA_ERROR_NOT_FOUND;

	while (sTaskletHead)
	{
		ca_tasklet *tasklet = sTaskletHead;
		uint32_t    delta   = 1;

		TASKLET_GetTimeToNext(&delta);
		if (delta)
		{
			//event not due
			break;
		}

		error = CA_ERROR_SUCCESS;
		TASKLET_Cancel(tasklet); //Cancel before calling callback so that tasklet can self-reschedule
		tasklet->callback(tasklet->context);
	}

	return error;
}
