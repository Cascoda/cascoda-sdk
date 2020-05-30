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
/**
 * @file
 * @brief  OS abstraction functions - in baremetal (no rtos) these are just stubs!
 */

#ifndef BAREMETAL_CASCODA_BM_DRIVER_INCLUDE_CASCODA_BM_CASCODA_OS_H_
#define BAREMETAL_CASCODA_BM_DRIVER_INCLUDE_CASCODA_BM_CASCODA_OS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generic mutex type
 */
typedef void *ca_mutex;

/**
 * Initialise the CA_OS subsystem. This should be called before the scheduler
 * is started. In non-OS applications it is called by EVBME_Initialise.
 */
void CA_OS_Init(void);

/**
 * Yield the CPU to a different task.
 */
void CA_OS_Yield(void);

/**
 * Initialise a mutex for inter-thread control in an OS-independent manner. Non-recursive mutex.
 * @return An initialised mutex (Or null if no memory)
 */
ca_mutex CA_OS_MutexInit(void);

/**
 * Claim a mutex, blocking until it is taken
 * Mutex must be initialised with CA_OS_MutexInit
 * Counterpart is CA_OS_MutexUnlock
 * @param aMutex The mutex to lock
 */
void CA_OS_MutexLock(ca_mutex *aMutex);

/**
 * Unlock a claimed mutex
 * Counterpart is CA_OS_MutexLock
 * @param aMutex the mutex to unlock
 */
void CA_OS_MutexUnlock(ca_mutex *aMutex);

/**
 * Suspend the scheduler so that it will not pre-emptively context switch
 * to another task. Useful for system initialisation such as initialising a
 * mutex. Implemented with a counter, so can be called multiple times as
 * long as CA_OS_SchedulerResume is called the same number of times.
 * Counterpart is CA_OS_SchedulerResume.
 */
void CA_OS_SchedulerSuspend(void);

/**
 * Resume the scheduler so pre-emption begins again. Counterpart is
 * CA_OS_SchedulerSuspend.
 */
void CA_OS_SchedulerResume(void);

/**
 * Lock the Cascoda API thread, for safely calling Cascoda API functions.
 * Implemented with a counter, so can be called multiple times as
 * long as CA_OS_UnlockAPI is called the same number of times.
 * Counterpart is CA_OS_UnlockAPI
 */
void CA_OS_LockAPI(void);

/**
 * Release the Cascoda API lock.
 * Counterpart is CA_OS_LockAPI
 */
void CA_OS_UnlockAPI(void);

/**
 * Shorthand to perform an action with the API Lock taken.
 */
#define CA_OS_LOCKED(x)    \
	do                     \
	{                      \
		CA_OS_LockAPI();   \
		x;                 \
		CA_OS_UnlockAPI(); \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* BAREMETAL_CASCODA_BM_DRIVER_INCLUDE_CASCODA_BM_CASCODA_OS_H_ */
