/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2019                                                ******/
/******************************************************************************/
/******************************************************************************/
/****** Openthread standalone SED w/ basic CoAP server discovery & reporting **/
/******************************************************************************/
/******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#include "openthread/coap.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "cbor.h"

#define SuccessOrExit(aCondition) \
	do                            \
	{                             \
		if ((aCondition) != 0)    \
		{                         \
			goto exit;            \
		}                         \
	} while (0)

/******************************************************************************/
/****** Application name                                                 ******/
/******************************************************************************/
const char *APP_NAME = "OT SED";

const char *uriCascodaDiscover = "ca/di";
const char *uriCascodaSensor   = "ca/se";

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/
otInstance *       OT_INSTANCE;
struct ca821x_dev  dev;
struct ca821x_dev *CA_DEVICE;

static bool         isConnected  = false;
static int          timeoutCount = 0;
static otIp6Address serverIp;

/******************************************************************************/
/****** FreeRTOS-related globals                                         ******/
/******************************************************************************/
TaskHandle_t      CommsTaskHandle;
SemaphoreHandle_t CommsMutexHandle;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation function
 *******************************************************************************
 ******************************************************************************/
static void NANO120_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();
	/* register LED_G */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_GREEN, MODULE_PIN_TYPE_LED);
	/* register LED_R */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_RED, MODULE_PIN_TYPE_LED);

	if (status == CA_ERROR_FAIL)
	{
		BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_ON);
		return;
	}

	BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_OFF);
	BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_ON);

	EVBME_SwitchClock(pDeviceRef, 1);

	//NANO120_APP_SaveOrRestoreAddress();
} // End of NANO120_Initialise()

// Callback called by OpenThread when there is work to
void otTaskletsSignalPending(otInstance *aInstance)
{
	(void)aInstance;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handle the response to the server discover, and register the server
 * locally.
 *******************************************************************************
 ******************************************************************************/
static void handleServerDiscoverResponse(void *               aContext,
                                         otMessage *          aMessage,
                                         const otMessageInfo *aMessageInfo,
                                         otError              aError)
{
	if (aError != OT_ERROR_NONE)
		return;
	if (isConnected)
		return;

	uint16_t length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

	if (length < sizeof(otIp6Address))
		return;

	otMessageRead(aMessage, otMessageGetOffset(aMessage), &serverIp, sizeof(otIp6Address));
	isConnected  = true;
	timeoutCount = 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send a multicast cascoda 'server discover' coap message. This is a
 * non-confirmable get request, and the get responses will be handled by the
 * 'handleServerDiscoverResponse' function.
 *******************************************************************************
 ******************************************************************************/
static otError sendServerDiscover(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage *   message = NULL;
	otMessageInfo messageInfo;
	otIp6Address  coapDestinationIp;

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, NULL);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	//Build CoAP header
	//Realm local all-nodes multicast - this of course generates some traffic, so shouldn't be overused
	SuccessOrExit(error = otIp6AddressFromString("FF03::1", &coapDestinationIp));
	otCoapMessageInit(message, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_GET);
	otCoapMessageGenerateToken(message, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaDiscover));

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = coapDestinationIp;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

	//send
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleServerDiscoverResponse, NULL);

exit:
	if (error && message)
	{
		//error, we have to free
		otMessageFree(message);
	}
	return error;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handle the response to the sensor data post
 *******************************************************************************
 ******************************************************************************/
static void handleSensorConfirm(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
	if (aError == OT_ERROR_RESPONSE_TIMEOUT && timeoutCount++ > 3)
	{
		isConnected = false;
	}
	else if (aError == OT_ERROR_NONE)
	{
		timeoutCount = 0;
	}
}

static otError sendSensorData(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage *   message = NULL;
	otMessageInfo messageInfo;
	int32_t       temperature = BSP_GetTemperature();
	uint8_t       buffer[32];
	CborError     err;
	CborEncoder   encoder, mapEncoder;

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, NULL);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	//Build CoAP header
	otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
	otCoapMessageGenerateToken(message, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaSensor));
	otCoapMessageAppendContentFormatOption(message, OT_COAP_OPTION_CONTENT_FORMAT_CBOR);
	otCoapMessageSetPayloadMarker(message);

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = serverIp;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

	//Initialise the CBOR encoder

	cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);

	//Create and populate the CBOR map
	SuccessOrExit(err = cbor_encoder_create_map(&encoder, &mapEncoder, 1));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "t"));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, temperature));
	SuccessOrExit(err = cbor_encoder_close_container(&encoder, &mapEncoder));

	size_t length = cbor_encoder_get_buffer_size(&encoder, buffer);

	//Append and send the serialised message
	SuccessOrExit(error = otMessageAppend(message, buffer, length));
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleSensorConfirm, NULL);

exit:
	if ((err || error) && message)
	{
		//error, we have to free
		otMessageFree(message);
	}
	return error;
}

void SensorHandlerTask(void *unused)
{
	(void)unused;

	/* This task calls secure-side functions (namely, functions in the BSP)
	 * therefore it must allocate a secure context before doing so. */
	portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);

	for (;;)
	{
		if (isConnected)
		{
			xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);
			sendSensorData();
			xSemaphoreGive(CommsMutexHandle);

			// Block for 1 seconds after sending data
			const TickType_t delay = 10 * 1000 / portTICK_PERIOD_MS;
			vTaskDelay(delay);
		}
		else
		{
			xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);
			sendServerDiscover();
			xSemaphoreGive(CommsMutexHandle);

			// Block for 3 seconds after attempting to connect
			const TickType_t delay = 10 * 1000 / portTICK_PERIOD_MS;
			vTaskDelay(delay);
		}
	}
}

static void CommsTask(void *unused)
{
	(void)unused;

	/* This task calls secure-side functions (namely, functions in the BSP)
	 * therefore it must allocate a secure context before doing so. */
	portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);

	for (;;)
	{
		xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);

		cascoda_io_handler(CA_DEVICE);
		otTaskletsProcess(OT_INSTANCE);

		xSemaphoreGive(CommsMutexHandle);
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
	u8_t StartupStatus;
	ca821x_api_init(&dev);
	CA_DEVICE = &dev;

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(APP_NAME, &dev);

	// Insert Application-Specific Initialisation Routines here
	NANO120_Initialise(StartupStatus, &dev);

	PlatformRadioInitWithDev(&dev);

	OT_INSTANCE = otInstanceInitSingle();

	/* Setup Thread stack with hard coded demo parameters */
	otLinkModeConfig linkMode = {0};
	otMasterKey key = {0xca, 0x5c, 0x0d, 0xa5, 0x01, 0x07, 0xca, 0x5c, 0x0d, 0xaa, 0xca, 0xfe, 0xbe, 0xef, 0xde, 0xad};
	otLinkSetPollPeriod(OT_INSTANCE, 5000);
	otIp6SetEnabled(OT_INSTANCE, true);
	otLinkSetPanId(OT_INSTANCE, 0xc0da);
	linkMode.mSecureDataRequests = true;
	otThreadSetLinkMode(OT_INSTANCE, linkMode);
	otThreadSetMasterKey(OT_INSTANCE, &key);
	otLinkSetChannel(OT_INSTANCE, 22);
	otThreadSetEnabled(OT_INSTANCE, true);

	otCoapStart(OT_INSTANCE, OT_DEFAULT_COAP_PORT);

	otTaskletsProcess(OT_INSTANCE);

	// Create the mutex that controls access to the OpenThread API
	CommsMutexHandle = xSemaphoreCreateMutex();

	// Create the communications task. It controls the radio and the
	// Thread network stack.
	xTaskCreate(CommsTask, "Comms", 1024, NULL, 2, &CommsTaskHandle);

	xTaskCreate(SensorHandlerTask, "Sensor", 1024, NULL, 3, NULL);
	// Start the FreeRTOS Scheduler
	vTaskStartScheduler();

	// Should never get here
	for (;;)
	{
	}
}

int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0)
{
	(void)n32In_R0;
	(void)n32In_R1;
	(void)pn32Out_R0;

	return 0;
}

/* Stack overflow hook, required by FreeRTOS. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	/* Force an assert. */
	configASSERT(pcTaskName == 0);
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that
 * is used by the Idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t ** ppxIdleTaskStackBuffer,
                                   uint32_t *     pulIdleTaskStackSize)
{
	/* If the buffers to be provided to the Idle task are declared inside this
	 * function then they must be declared static - otherwise they will be
	 * allocated on the stack and so not exists after this function exits. */
	static StaticTask_t xIdleTaskTCB;
	static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE] __attribute__((aligned(32)));

	/* Pass out a pointer to the StaticTask_t structure in which the Idle
	 * task's state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	 * Note that, as the array is necessarily of type StackType_t,
	 * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 * application must provide an implementation of vApplicationGetTimerTaskMemory()
 * to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t ** ppxTimerTaskStackBuffer,
                                    uint32_t *     pulTimerTaskStackSize)
{
	/* If the buffers to be provided to the Timer task are declared inside this
	 * function then they must be declared static - otherwise they will be
	 * allocated on the stack and so not exists after this function exits. */
	static StaticTask_t xTimerTaskTCB;
	static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] __attribute__((aligned(32)));

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	 * task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	 * Note that, as the array is necessarily of type StackType_t,
	 * configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/
