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
 * Cascoda FreeRTOS OS abstraction
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "cascoda-bm/cascoda_os.h"
#include "ca821x_toolchain.h"

static ca_mutex ca_api_mutex = NULL;

void CA_OS_freertos_bind()
{
	//Do nothing, this is just to kick the linker.
}

void CA_OS_Init()
{
	if (ca_api_mutex)
		return;

	ca_api_mutex = (ca_mutex)xSemaphoreCreateRecursiveMutex();
}

void CA_OS_Yield()
{
	taskYIELD();
}

void CA_OS_LockAPI()
{
	xSemaphoreTakeRecursive(((SemaphoreHandle_t)ca_api_mutex), portMAX_DELAY);
}

void CA_OS_UnlockAPI()
{
	xSemaphoreGiveRecursive(((SemaphoreHandle_t)ca_api_mutex));
}

ca_mutex CA_OS_MutexInit()
{
	return (ca_mutex)xSemaphoreCreateMutex();
}

void CA_OS_MutexLock(ca_mutex *aMutex)
{
	xSemaphoreTake(*((SemaphoreHandle_t *)aMutex), portMAX_DELAY);
}

void CA_OS_MutexUnlock(ca_mutex *aMutex)
{
	xSemaphoreGive(*((SemaphoreHandle_t *)aMutex));
}

void CA_OS_SchedulerSuspend(void)
{
	vTaskSuspendAll();
}

void CA_OS_SchedulerResume(void)
{
	xTaskResumeAll();
}
