/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2019                                                ******/
/******************************************************************************/
/******************************************************************************/
/****** Openthread standalone SED w/ basic CoAP server discovery & reporting **/
/*******Includes sensorif sensor reading using the i2c sensor interface *******/
/******************************************************************************/
/******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
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

#include "sif_ltr303als.h"
#include "sif_si7021.h"

#include "cbor.h"

/******************************************************************************/
/****** Application name                                                 ******/
/******************************************************************************/
static const char *APP_NAME = "SENSORIF SED";

static const char *  uriCascodaDiscover = "ca/di";
static const char *  uriCascodaSensor   = "ca/se";
static const uint8_t ledPin             = 34;

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/
otInstance *OT_INSTANCE;

static bool         isConnected  = false;
static int          timeoutCount = 0;
static otIp6Address serverIp;
static uint32_t     appNextSendTime = 5000;

/******************************************************************************/
/****** Counter variable of how many times GPIO ISR executed             ******/
/******************************************************************************/
static volatile uint32_t isr_counter = 0;

/******************************************************************************/
/****** Minimum time required (in ms) between counter incrementations    ******/
/******************************************************************************/
static const uint32_t MIN_TIME           = 500;
static uint32_t       min_counter_period = 500;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR triggered by rising edge of GPIO pin.
 *******************************************************************************
 ******************************************************************************/
static int counter_ISR(void)
{
	if (TIME_ReadAbsoluteTime() < min_counter_period)
		return 0;

	isr_counter++;
	min_counter_period = TIME_ReadAbsoluteTime() + MIN_TIME;
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets up a GPIO pin as an input
 *******************************************************************************
 ******************************************************************************/
static void set_up_GPIO_input(u8_t mpin)
{
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
	    mpin, MODULE_PIN_PULLUP_OFF, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_RISE, &counter_ISR});
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks current device status and goes to sleep if nothing is happening
 *******************************************************************************
 ******************************************************************************/
static void sleep_if_possible(void)
{
	//Application check
	uint32_t appTimeLeft = appNextSendTime - TIME_ReadAbsoluteTime();

	if (!otTaskletsArePending(OT_INSTANCE))
	{
		otLinkModeConfig linkMode = otThreadGetLinkMode(OT_INSTANCE);

		if (linkMode.mDeviceType == 0 && linkMode.mRxOnWhenIdle == 0 &&
		    otThreadGetDeviceRole(OT_INSTANCE) == OT_DEVICE_ROLE_CHILD && !otLinkIsInTransmitState(OT_INSTANCE) &&
		    !PlatformIsExpectingIndication())
		{
			uint32_t idleTimeLeft = PlatformGetAlarmMilliTimeout();
			if (idleTimeLeft > appTimeLeft)
				idleTimeLeft = appTimeLeft;

			if (idleTimeLeft > 5)
			{
				BSP_ModuleSetGPIOPin(ledPin, LED_OFF);
				//PlatformSleep(idleTimeLeft);
			}
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation function
 *******************************************************************************
 ******************************************************************************/
static void SED_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	/* register LED */
	BSP_ModuleRegisterGPIOOutput(ledPin, MODULE_PIN_TYPE_LED);

	if (status == CA_ERROR_FAIL)
	{
		return;
	}

	BSP_ModuleSetGPIOPin(ledPin, LED_ON);

	EVBME_SwitchClock(pDeviceRef, 1);

	SENSORIF_I2C_Init();
	SIF_LTR303ALS_Initialise();
	SENSORIF_I2C_Deinit();
} // End of SED_Initialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handler function that should be called repeatedly from main
 *******************************************************************************
 ******************************************************************************/
static void SED_Handler(struct ca821x_dev *pDeviceRef)
{
	PlatformAlarmProcess(OT_INSTANCE);
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
	int32_t       temperature;
	uint32_t      humidity;
	uint16_t      lightLevel0, lightLevel1;
	uint32_t      counter_copy = isr_counter;
	uint8_t       buffer[32];
	CborError     err;
	CborEncoder   encoder, mapEncoder;

	BSP_ModuleSetGPIOPin(ledPin, LED_ON);

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

	SENSORIF_I2C_Init();
	temperature = 10 * SIF_SI7021_ReadTemperature();
	humidity    = SIF_SI7021_ReadHumidity();
	SIF_LTR303ALS_ReadLight(&lightLevel0, &lightLevel1);
	SENSORIF_I2C_Deinit();

	//Initialise the CBOR encoder
	cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);

	//Create and populate the CBOR map
	SuccessOrExit(err = cbor_encoder_create_map(&encoder, &mapEncoder, 4));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "t"));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, temperature));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "h"));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, humidity));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "c"));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, counter_copy));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "l"));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, lightLevel0));
	SuccessOrExit(err = cbor_encoder_close_container(&encoder, &mapEncoder));

	size_t length = cbor_encoder_get_buffer_size(&encoder, buffer);

	//Append and send the serialised message
	SuccessOrExit(error = otMessageAppend(message, buffer, length));
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleSensorConfirm, NULL);

exit:
	BSP_ModuleSetGPIOPin(ledPin, LED_OFF);
	if ((err || error) && message)
	{
		//error, we have to free
		otMessageFree(message);
	}
	return error;
}

void SensorHandler(void)
{
	if (TIME_ReadAbsoluteTime() < appNextSendTime)
		return;

	if (isConnected)
	{
		appNextSendTime = TIME_ReadAbsoluteTime() + 10000;
		sendSensorData();
	}
	else
	{
		appNextSendTime = TIME_ReadAbsoluteTime() + 30000;
		sendServerDiscover();
	}
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
	ca821x_api_init(&dev);

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(APP_NAME, &dev);

	// Insert Application-Specific Initialisation Routines here
	SED_Initialise(StartupStatus, &dev);

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

	// Sets up GPIO module pin 31 (PB.5) as an input for the interrupt.
	set_up_GPIO_input(31);

	// Endless Polling Loop
	while (1)
	{
		cascoda_io_handler(&dev);
		SensorHandler();
		SED_Handler(&dev);
	}
}
