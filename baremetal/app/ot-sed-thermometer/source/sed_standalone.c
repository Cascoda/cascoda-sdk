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
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#include "openthread/coap.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "cbor.h"

static const char *uriCascodaDiscover            = "ca/di";
static const char *uriCascodaSensorDiscoverQuery = "t=sen";
static const char *uriCascodaSensor              = "ca/se";

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/
otInstance *OT_INSTANCE;

static bool         isConnected  = false;
static int          timeoutCount = 0;
static otIp6Address serverIp;
static ca_tasklet   dataRequestTasklet;
static ca_tasklet   sensorTasklet;

enum
{
	SENSORDATA_PERIOD    = 5000,
	SENSOR_POLL_DELAY    = 250,
	DISCOVERY_PERIOD     = 30000,
	DISCOVERY_POLL_DELAY = 2000,
	INITIAL_DELAY        = 5000
};

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks current device status and goes to sleep if nothing is happening
 *******************************************************************************
 ******************************************************************************/
static void sleep_if_possible(void)
{
	if (PlatformCanSleep(OT_INSTANCE))
	{
		uint32_t taskletTimeLeft = 600000;

		TASKLET_GetTimeToNext(&taskletTimeLeft);

		if (taskletTimeLeft > 100)
		{
			struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();
			BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_OFF);
			BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_OFF);
			PlatformSleep(taskletTimeLeft);
			BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_ON);
		}
	}
}

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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handler function that should be called repeatedly from main
 *******************************************************************************
 ******************************************************************************/
static void SED_Handler(struct ca821x_dev *pDeviceRef)
{
	cascoda_io_handler(pDeviceRef);
	sleep_if_possible();
	otTaskletsProcess(OT_INSTANCE);
} // End of SED_Handler()

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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send a sensor data coap message to the bound server.
 *******************************************************************************
 ******************************************************************************/
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

ca_error PollHandler(void *aContext)
{
	otLinkSendDataRequest(OT_INSTANCE);
	return CA_ERROR_SUCCESS;
}

ca_error SensorHandler(void *aContext)
{
	(void)aContext;

	if (isConnected)
	{
		sendSensorData();
		TASKLET_ScheduleDelta(&sensorTasklet, SENSORDATA_PERIOD, NULL);
		TASKLET_ScheduleDelta(&dataRequestTasklet, SENSOR_POLL_DELAY, NULL);
	}
	else
	{
		sendServerDiscover();
		TASKLET_ScheduleDelta(&sensorTasklet, DISCOVERY_PERIOD, NULL);
		TASKLET_ScheduleDelta(&dataRequestTasklet, DISCOVERY_POLL_DELAY, NULL);
	}
	return CA_ERROR_SUCCESS;
}

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
	u8_t              StartupStatus;
	struct ca821x_dev dev;
	otError           otErr    = OT_ERROR_NONE;
	otLinkModeConfig  linkMode = {0};

	ca821x_api_init(&dev);

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(CA_TARGET_NAME, &dev);

	// Insert Application-Specific Initialisation Routines here
	NANO120_Initialise(StartupStatus, &dev);

	PlatformRadioInitWithDev(&dev);
	OT_INSTANCE = otInstanceInitSingle();

	// Print the joiner credentials, delaying for up to 5 seconds
	PlatformPrintJoinerCredentials(&dev, OT_INSTANCE, 5000);
	// Try to join network
	do
	{
		otErr = PlatformTryJoin(&dev, OT_INSTANCE);
		if (otErr == OT_ERROR_NONE || otErr == OT_ERROR_ALREADY)
			break;

		PlatformSleep(30000);
	} while (1);

	otLinkSetPollPeriod(OT_INSTANCE, 5000);
	linkMode.mSecureDataRequests = true;
	otThreadSetLinkMode(OT_INSTANCE, linkMode);

	otThreadSetEnabled(OT_INSTANCE, true);

	otCoapStart(OT_INSTANCE, OT_DEFAULT_COAP_PORT);

	TASKLET_Init(&dataRequestTasklet, &PollHandler);
	TASKLET_Init(&sensorTasklet, &SensorHandler);

	TASKLET_ScheduleDelta(&sensorTasklet, INITIAL_DELAY, NULL);

	// Endless Polling Loop
	while (1)
	{
		cascoda_io_handler(&dev);
		SED_Handler(&dev);
	}
}
