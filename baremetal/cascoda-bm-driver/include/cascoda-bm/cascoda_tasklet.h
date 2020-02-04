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
/**
 * @file
 * @brief  Helper 'tasklet' framework for scheduling simple events for the future.
 */

#ifndef CASCODA_TASKLET_H
#define CASCODA_TASKLET_H

#include "ca821x_api.h"
#include "mac_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function pointer typedef for tasklet callbacks. Will be called when the tasklet is triggered.
 * @param context User-defined context to be passed to the callback when it is triggered.
 * @return ca_error status
 * @retval CA_ERROR_SUCCESS Callback successfully executed
 */
typedef ca_error (*ca_tasklet_callback)(void *context);

/**
 * Internal tasklet state structure. Must be allocated with a lifetime that exceeds the usage of
 * the tasklet, ideally statically. Do not modify this struct directly, and instead use the TASKLET_
 * functions to control it.
 */
typedef struct ca_tasklet
{
	ca_tasklet_callback callback; //!< Internal: The callback that will be called when the tasklet is triggered
	void *              context;  //!< Internal: The context that will be passed to the callback when it is called.
	struct ca_tasklet * next; //!< Internal: The next tasklet in the sorted tasklet linkedlist. NULL when not queued.
	uint32_t            fireTime;      //!< Internal: The next time at which the tasklet is due to trigger
	uint8_t             scheduled : 1; //!< Internal: Is this tasklet scheduled?
} ca_tasklet;

/**
 * Initialise a tasklet to a stable state, and register its associated callback function.
 * It is invalid to call this function on a currently scheduled target, and this produces undefined behaviour.
 * @param aTasklet A pointer to the uninitialised target
 * @param aCallback The callback to be called when this tasklet passes its scheduled time.
 * @return status
 * @retval CA_ERROR_SUCCESS Successfully initialised tasklet
 */
ca_error TASKLET_Init(ca_tasklet *aTasklet, ca_tasklet_callback aCallback);

/**
 * Schedule a tasklet to be called in the future, by aTimeDelta milliseconds.
 * @param aTasklet A pointer to an initalised ca_tasklet struct
 * @param aTimeDelta The number of milliseconds into the future that this tasklet should be called
 * @param aContext A user-defined context pointer which will be passed to the callback. Can be NULL.
 * @return status
 * @retval CA_ERROR_SUCCESS Successfully scheduled the tasklet
 * @retval CA_ERROR_ALREADY The given tasklet is already registered. Must first be excecuted or cancelled with TASKLET_Cancel.
 * @retval CA_ERROR_INVALID_ARGS Scheduled too far into the future (must be less than 0x7FFFFFFF ms into future)
 */
ca_error TASKLET_ScheduleDelta(ca_tasklet *aTasklet, uint32_t aTimeDelta, void *aContext);

/**
 * Schedule a tasklet to be called in the future, at aTimeAbs milliseconds.
 * @param aTasklet A pointer to an initalised ca_tasklet struct
 * @param aTimeNow The current time used to schedule the event. Used to prevent race condition.
 * @param aTimeAbs The absolute time to schedule this tasklet.
 * @param aContext A user-defined context pointer which will be passed to the callback. Can be NULL.
 * @return status
 * @retval CA_ERROR_SUCCESS Successfully scheduled the tasklet
 * @retval CA_ERROR_ALREADY The given tasklet is already registered. Must first be excecuted or cancelled with TASKLET_Cancel.
 * @retval CA_ERROR_INVALID_ARGS Scheduled too far into the future (must be less than 0x7FFFFFFF ms into future)
 */
ca_error TASKLET_ScheduleAbs(ca_tasklet *aTasklet, uint32_t aTimeNow, uint32_t aTimeAbs, void *aContext);

/**
 * Get the time that the tasklet is scheduled to be called
 * @param[in] aTasklet A pointer to an initalised ca_tasklet struct to query
 * @param[out] aTimeAbs Output parameter which will be filled with the absolute time that the tasklet is scheduled for.
 * @return status
 * @retval CA_ERROR_SUCCESS Tasklet is scheduled to be called at *aTimeAbs ms
 * @retval CA_ERROR_INVALID_STATE Tasklet is not currently scheduled. aTimeAbs has not been modified.
 */
ca_error TASKLET_GetScheduledTime(ca_tasklet *aTasklet, uint32_t *aTimeAbs);

/**
 * Cancel a scheduled tasklet if it is scheduled.
 * @param aTasklet A pointer to an initalised ca_tasklet struct to cancel
 * @return status
 * @retval CA_ERROR_SUCCESS Tasklet successfully cancelled
 * @retval CA_ERROR_ALREADY Tasklet is not currently scheduled
 */
ca_error TASKLET_Cancel(ca_tasklet *aTasklet);

/**
 * Query whether a tasklet is currently scheduled.
 * @param aTasklet A pointer to an initalised ca_tasklet struct to query
 * @return True if the tasklet is scheduled, false if not
 */
bool TASKLET_IsQueued(ca_tasklet *aTasklet);

/**
 * Get the time delta until the next tasklet is scheduled to occur.
 * @param[out] aTimeDelta Output parameter that will be filled with the time delta until the next tasklet to be scheduled.
 * @return status
 * @retval CA_ERROR_SUCCESS There is at least one tasklet scheduled and it is scheduled for *aTimeDelta milliseconds
 * @retval CA_ERROR_NOT_FOUND There are no tasklets currently scheduled. aTimeDelta has not been modified.
 */
ca_error TASKLET_GetTimeToNext(uint32_t *aTimeDelta);

/**
 * Process the callbacks for any tasklets that are scheduled to happen now or in the past.
 * @return CA_ERROR_SUCCESS Pending tasklets have been processed.
 * @return CA_ERROR_NOT_FOUND No tasklets currently scheduled for now or the past.
 */
ca_error TASKLET_Process(void);

#ifdef __cplusplus
}
#endif

#endif //CASCODA_TASKLET_H
