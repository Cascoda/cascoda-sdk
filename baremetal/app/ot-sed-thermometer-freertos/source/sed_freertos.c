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
#include "cascoda-bm/cascoda_os.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "cascoda-util/cascoda_time.h"
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

const char        *uriCascodaDiscover            = "ca/di";
static const char *uriCascodaSensorDiscoverQuery = "t=sen";
const char        *uriCascodaSensor              = "ca/se";

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/
otInstance       *OT_INSTANCE;
struct ca821x_dev sDeviceRef;

static bool         isConnected  = false;
static int          timeoutCount = 0;
static otIp6Address serverIp;

/******************************************************************************/
/****** FreeRTOS-related globals                                         ******/
/******************************************************************************/
TaskHandle_t      CommsTaskHandle;
SemaphoreHandle_t CommsMutexHandle;

/**
 * Radio reinitialisation after sleep
 **/
int ot_reinitialise(struct ca821x_dev *pDeviceRef)
{
	otLinkSyncExternalMac(OT_INSTANCE);
}

/**
 * Initialise app-specific systems
 */
static void App_Initialise(ca_error status)
{
	otLinkModeConfig         linkMode     = {0};
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

	// Print the joiner credentials, delaying for up to 5 seconds
	PlatformPrintJoinerCredentials(&sDeviceRef, OT_INSTANCE, 5000);
	// Try to join network
	do
	{
		otError otErr = PlatformTryJoin(&sDeviceRef, OT_INSTANCE);
		if (otErr == OT_ERROR_NONE || otErr == OT_ERROR_ALREADY)
			break;

		PlatformSleep(30000);
	} while (1);

	otThreadSetLinkMode(OT_INSTANCE, linkMode);
	otThreadSetEnabled(OT_INSTANCE, true);

	otCoapStart(OT_INSTANCE, OT_DEFAULT_COAP_PORT);
}

/**
 * Initialise the system
 */
static void System_Init()
{
	u8_t StartupStatus;
	ca821x_api_init(&sDeviceRef);
	cascoda_reinitialise = ot_reinitialise;

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(CA_TARGET_NAME, &sDeviceRef);

	PlatformRadioInitWithDev(&sDeviceRef);

	OT_INSTANCE = otInstanceInitSingle();

	App_Initialise(StartupStatus);
}

// Callback called by OpenThread when there is work to
void otTaskletsSignalPending(otInstance *aInstance)
{
	(void)aInstance;
}

/**
 * \brief Handle the response to the server discover, and register the server
 * locally.
 */
static void handleServerDiscoverResponse(void                *aContext,
                                         otMessage           *aMessage,
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

/**
 * \brief Send a multicast cascoda 'server discover' coap message. This is a
 * non-confirmable get request, and the get responses will be handled by the
 * 'handleServerDiscoverResponse' function.
 */
static otError sendServerDiscover(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage    *message = NULL;
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
	SuccessOrExit(error = otCoapMessageAppendUriQueryOption(message, uriCascodaSensorDiscoverQuery));

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

/**
 * \brief Handle the response to the sensor data post
 */
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

/**
 * Get sensor data from sensors and send it to the server.
 */
static otError sendSensorData(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage    *message = NULL;
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

/**
 * Thread to handle maintaining connection to server and periodically sending data.
 */
void SensorHandlerTask(void *unused)
{
	(void)unused;

	/* This task calls secure-side functions (namely, functions in the BSP)
	 * therefore it must allocate a secure context before doing so. */
	portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);

	vTaskDelay((100 / portTICK_PERIOD_MS) + 1);

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

/**
 * Core Cascoda communications thread
 */
static void CommsTask(void *unused)
{
	(void)unused;

	/* This task calls secure-side functions (namely, functions in the BSP)
	 * therefore it must allocate a secure context before doing so. */
	portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);

	System_Init();

	xTaskCreate(SensorHandlerTask, "Sensor", 1024, NULL, 3, NULL);

	for (;;)
	{
		xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);

		cascoda_io_handler(&sDeviceRef);
		otTaskletsProcess(OT_INSTANCE);

		xSemaphoreGive(CommsMutexHandle);
	}
}

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
	configASSERT(pcTaskName == 0);
}
