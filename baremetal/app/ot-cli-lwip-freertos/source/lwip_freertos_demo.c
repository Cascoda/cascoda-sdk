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

/******************************************************************************/
/****** Openthread standalone SED w/ basic CoAP server discovery & reporting **/
/******************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_os.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#include "openthread/cli.h"
#include "openthread/coap.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip-port.h"

#include "lwip_freertos_demo.h"

#define SuccessOrExit(aCondition) \
	do                            \
	{                             \
		if ((aCondition) != 0)    \
		{                         \
			goto exit;            \
		}                         \
	} while (0)

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/
otInstance *             OT_INSTANCE;
static struct ca821x_dev sDeviceRef;

/******************************************************************************/
/****** FreeRTOS-related globals                                         ******/
/******************************************************************************/
TaskHandle_t      CommsTaskHandle;
SemaphoreHandle_t CommsMutexHandle;

// Callback called by OpenThread when there is work to
void otTaskletsSignalPending(otInstance *aInstance)
{
	(void)aInstance;
}

ca_error LockCommsThread()
{
	if (xSemaphoreTake(CommsMutexHandle, portMAX_DELAY))
		return CA_ERROR_SUCCESS;
	return CA_ERROR_BUSY;
}

ca_error UnlockCommsThread()
{
	if (xSemaphoreGive(CommsMutexHandle))
		return CA_ERROR_SUCCESS;
	return CA_ERROR_INVALID_STATE;
}

/**
 * Handle App-specific EVBME commands
 * @param buf EVBME Command buffer, [0] is id, [1] is len
 * @param len Length of the buffer
 * @param pDeviceRef The cascoda device ref
 * @return 0 for unhandled, 1 for handled
 */
static int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret = 0;

	if (buf[0] == OT_SERIAL_DOWNLINK)
	{
		PlatformUartReceive(buf + 2, buf[1]);
		ret = 1;
	}

	// switch clock otherwise chip is locking up as it loses external clock
	if (((buf[0] == EVBME_SET_REQUEST) && (buf[2] == EVBME_RESETRF)) || (buf[0] == EVBME_HOST_CONNECTED))
	{
		EVBME_SwitchClock(pDeviceRef, 0);
	}

	TEST15_4_SerialDispatch(buf, len, pDeviceRef);

	return ret;
}

/**
 * Initialise the core system modules (external to this app)
 */
static void System_Init()
{
	//Initialise ca821x-api
	ca821x_api_init(&sDeviceRef);
	cascoda_serial_dispatch = ot_serial_dispatch;

	// Initialisation of Chip and EVBME
	EVBMEInitialise(CA_TARGET_NAME, &sDeviceRef);

	//Openthread Init
	PlatformRadioInitWithDev(&sDeviceRef);
	OT_INSTANCE = otInstanceInitSingle();
	otCliUartInit(OT_INSTANCE);

	//LWIP init
	LWIP_NetifInit(OT_INSTANCE);
	tcpip_init(NULL, NULL);
}

/**
 * The communications task. This handles the openthread and cascoda stacks. Calls into the openthread
 * and cascoda APIs should be locked with LockCommsThread and UnlockCommsThread for Thread safety.
 * @param unused unused
 */
static void CommsTask(void *unused)
{
	(void)unused;

	/* This task calls secure-side functions (namely, functions in the BSP)
	 * therefore it must allocate a secure context before doing so. */
	portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);

	//Initialise the core system
	System_Init();

	// Initialise the lwip demo application
	init_lwipdemo(OT_INSTANCE, &sDeviceRef);

	for (;;)
	{
		xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);

		cascoda_io_handler(&sDeviceRef);
		otTaskletsProcess(OT_INSTANCE);
		LWIP_OtProcess();

		xSemaphoreGive(CommsMutexHandle);
		taskYIELD(); //TODO: Only do this if something is waiting on the commsmutexhandle
	}
} // End of CommsTask()

/******************************************************************************/
/******************************************************************************/
/****** main()                                                           ******/
/******************************************************************************/
/****** Brief:  Main Program Endless Loop                                ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: Does not return                                          ******/
/******************************************************************************/
/******************************************************************************/
int main(void)
{
	// Create the communications task. It controls the radio and the
	// Thread network stack.
	xTaskCreate(CommsTask, "Comms", 1024, NULL, 2, &CommsTaskHandle);

	// Create the mutex that controls access to the OpenThread API
	CommsMutexHandle = xSemaphoreCreateMutex();

	// Start the FreeRTOS Scheduler
	CA_OS_Init();
	vTaskStartScheduler();

	// Should never get here
	for (;;)
	{
	}
}

/* Stack overflow hook, required by FreeRTOS. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	/* Force an assert. */
	configASSERT(pcTaskName == NULL);
}
