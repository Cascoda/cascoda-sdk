/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2019                                                ******/
/******************************************************************************/
/******************************************************************************/
/****** Openthread standalone SED w/ basic CoAP server discovery & reporting **/
/******************************************************************************/
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#include "openthread/cli.h"
#include "openthread/coap.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/platform/settings.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "cbor.h"

// General
const char *uriCascodaDiscover            = "ca/di";
const char *uriCascodaSensorDiscoverQuery = "t=sen";
const char *uriCascodaSensor              = "ca/se";

otInstance *          OT_INSTANCE;
static const uint16_t sensordemo_key = 0xCA5C;

enum sensordemo_state
{
	SENSORDEMO_STOPPED = 0,
	SENSORDEMO_SENSOR  = 1,
	SENSORDEMO_SERVER  = 2,
} static sensordemo_state = SENSORDEMO_STOPPED;

// Sensor
static bool         isConnected  = false;
static int          timeoutCount = 0;
static otIp6Address serverIp;
static uint32_t     appNextSendTime = 5000;

// Server
static otCoapResource sSensorResource;
static otCoapResource sDiscoverResource;
static otCoapResource sDiscoverResponseResource;

/**
 * \brief Handle the response to the server discover, and register the server
 * locally.
 */
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

/**
 * \brief Handle the response to the sensor data post
 */
static void handleSensorConfirm(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
	ca_error error = CA_ERROR_FAIL;

	if (aError == OT_ERROR_RESPONSE_TIMEOUT)
	{
		error = CA_ERROR_FAIL;
	}
	else if (aMessage != NULL && otCoapMessageGetCode(aMessage) != OT_COAP_CODE_VALID)
	{
		error = CA_ERROR_FAIL;
	}
	else if (aError == OT_ERROR_NONE)
	{
		timeoutCount = 0;
		error        = CA_ERROR_SUCCESS;
	}

	if (error != CA_ERROR_SUCCESS && timeoutCount++ > 3)
	{
		isConnected = false;
	}
}

/**
 * \brief Send a sensor data coap message to the bound server.
 */
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

/** Send sensor data if the time is right */
static void sensordemo_handler(void)
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

/** Server: Handle a sensor data message by printing it */
static void handleSensorData(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage * responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;
	uint16_t    length          = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	uint16_t    offset          = otMessageGetOffset(aMessage);
	int64_t     temperature;
	int64_t     humidity;
	int64_t     pir_counter;
	int64_t     light_level;

	CborParser parser;
	CborValue  value;
	CborValue  t_result;
	CborValue  h_result;
	CborValue  c_result;
	CborValue  l_result;

	unsigned char *temp_key     = "t";
	unsigned char *humidity_key = "h";
	unsigned char *counter_key  = "c";
	unsigned char *light_key    = "l";

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_POST)
		return;

	if (length > 64)
		return;

	//Allocate a buffer and store the received serialised CBOR message into it
	unsigned char buffer[64];
	otMessageRead(aMessage, offset, buffer, length);

	//Initialise the CBOR parser
	SuccessOrExit(cbor_parser_init(buffer, length, 0, &parser, &value));

	//Extract the values corresponding to the keys from the CBOR map
	SuccessOrExit(cbor_value_map_find_value(&value, temp_key, &t_result));
	SuccessOrExit(cbor_value_map_find_value(&value, humidity_key, &h_result));
	SuccessOrExit(cbor_value_map_find_value(&value, counter_key, &c_result));
	SuccessOrExit(cbor_value_map_find_value(&value, light_key, &l_result));

	//Retrieve and print the sensor values
	otCliOutputFormat("Server received");

	bool first_print = true;

	if (cbor_value_get_type(&t_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&t_result, &temperature));
		otCliOutputFormat(" temperature %d.%d*C", (int)(temperature / 10), (int)abs(temperature % 10));
		first_print = false;
	}
	if (cbor_value_get_type(&h_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&h_result, &humidity));
		if (!first_print)
			otCliOutputFormat(",");
		otCliOutputFormat(" humidity %d%%", (int)humidity);
		first_print = false;
	}
	if (cbor_value_get_type(&c_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&c_result, &pir_counter));
		if (!first_print)
			otCliOutputFormat(",");
		otCliOutputFormat(" PIR count %d", (int)pir_counter);
		first_print = false;
	}
	if (cbor_value_get_type(&l_result) != CborInvalidType)
	{
		if (!first_print)
			otCliOutputFormat(",");
		SuccessOrExit(cbor_value_get_int64(&l_result, &light_level));
		otCliOutputFormat(" light level %d", (int)light_level);
	}

	otCliOutputFormat(" from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 0),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 2),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 4),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 6),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 8),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 10),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 12),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 14));

	responseMessage = otCoapNewMessage(OT_INSTANCE, NULL);
	if (responseMessage == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_VALID);
	otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

	SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));

exit:
	if (error != OT_ERROR_NONE && responseMessage != NULL)
	{
		otCliOutputFormat("Temperature ack failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

/** Server: Handle a discover message by printing it and sending response */
static void handleDiscover(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage * responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_GET)
		return;

	// Find the URI Query
	const otCoapOption *option;
	char                uri_query[6];
	bool                valid_query = false;

	for (option = otCoapMessageGetFirstOption(aMessage); option != NULL; option = otCoapMessageGetNextOption(aMessage))
	{
		if (option->mNumber == OT_COAP_OPTION_URI_QUERY && option->mLength <= 6)
		{
			SuccessOrExit(otCoapMessageGetOptionValue(aMessage, uri_query));

			if (strncmp(uri_query, uriCascodaSensorDiscoverQuery, 5) == 0)
			{
				valid_query = true;
				break;
			}
		}
	}

	if (!valid_query)
		return;

	otCliOutputFormat("Server received discover from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 0),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 2),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 4),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 6),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 8),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 10),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 12),
	                  GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 14));

	responseMessage = otCoapNewMessage(OT_INSTANCE, NULL);
	if (responseMessage == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_CONTENT);
	otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

	otCoapMessageSetPayloadMarker(responseMessage);

	SuccessOrExit(error = otMessageAppend(responseMessage, otThreadGetMeshLocalEid(OT_INSTANCE), sizeof(otIp6Address)));
	SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));

exit:
	if (error != OT_ERROR_NONE && responseMessage != NULL)
	{
		otCliOutputFormat("Discover response failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

/** Add server coap resources to the coap stack */
static void bind_server_resources()
{
	otCoapAddResource(OT_INSTANCE, &sSensorResource);
	otCoapAddResource(OT_INSTANCE, &sDiscoverResource);
}

/** Remove server coap resources to the coap stack */
static void unbind_server_resources()
{
	otCoapRemoveResource(OT_INSTANCE, &sSensorResource);
	otCoapRemoveResource(OT_INSTANCE, &sDiscoverResource);
}

/** Add sensor coap resources to the coap stack */
static void bind_sensor_resources()
{
	otCoapAddResource(OT_INSTANCE, &sDiscoverResponseResource);
}

/** Remove sensor coap resources to the coap stack */
static void unbind_sensor_resources()
{
	otCoapRemoveResource(OT_INSTANCE, &sDiscoverResponseResource);
}

void handle_cli_sensordemo(int argc, char *argv[])
{
	enum sensordemo_state prevState = sensordemo_state;

	if (argc == 0)
	{
		switch (sensordemo_state)
		{
		case SENSORDEMO_SENSOR:
			otCliOutputFormat("sensor\n");
			break;
		case SENSORDEMO_SERVER:
			otCliOutputFormat("server\n");
			break;
		case SENSORDEMO_STOPPED:
		default:
			otCliOutputFormat("stopped\n");
			break;
		}
	}
	if (argc != 1)
		return;

	if (strcmp(argv[0], "sensor") == 0)
	{
		sensordemo_state = SENSORDEMO_SENSOR;
		otLinkSetPollPeriod(OT_INSTANCE, 2500);
		unbind_server_resources();
		bind_sensor_resources();
	}
	else if (strcmp(argv[0], "server") == 0)
	{
		sensordemo_state = SENSORDEMO_SERVER;
		bind_server_resources();
		unbind_sensor_resources();
	}
	else if (strcmp(argv[0], "stop") == 0)
	{
		sensordemo_state = SENSORDEMO_STOPPED;
		unbind_server_resources();
		unbind_sensor_resources();
	}
	if (prevState != sensordemo_state)
		otPlatSettingsSet(OT_INSTANCE, sensordemo_key, (uint8_t *)&sensordemo_state, sizeof(sensordemo_state));
}

ca_error init_sensordemo(otInstance *aInstance, struct ca821x_dev *pDeviceRef)
{
	otError  error = OT_ERROR_NONE;
	uint16_t stateLength;
	OT_INSTANCE = aInstance;

	otCoapStart(OT_INSTANCE, OT_DEFAULT_COAP_PORT);

	memset(&sSensorResource, 0, sizeof(sSensorResource));
	sSensorResource.mUriPath = uriCascodaSensor;
	sSensorResource.mContext = aInstance;
	sSensorResource.mHandler = &handleSensorData;
	memset(&sDiscoverResource, 0, sizeof(sDiscoverResource));
	sDiscoverResource.mUriPath = uriCascodaDiscover;
	sDiscoverResource.mContext = aInstance;
	sDiscoverResource.mHandler = &handleDiscover;

	stateLength = sizeof(sensordemo_state);
	error       = otPlatSettingsGet(OT_INSTANCE, sensordemo_key, 0, (uint8_t *)&sensordemo_state, &stateLength);
	if (error != OT_ERROR_NONE)
	{
		sensordemo_state = SENSORDEMO_STOPPED;
	}

	if (sensordemo_state == SENSORDEMO_SERVER)
	{
		bind_server_resources();
	}
	else if (sensordemo_state == SENSORDEMO_SENSOR)
	{
		otLinkSetPollPeriod(OT_INSTANCE, 2500);
		bind_sensor_resources();
	}

	return CA_ERROR_SUCCESS;
}

ca_error handle_sensordemo(struct ca821x_dev *pDeviceRef)
{
	switch (sensordemo_state)
	{
	case SENSORDEMO_SENSOR:
		sensordemo_handler();
		break;
	case SENSORDEMO_SERVER:
		break;
	case SENSORDEMO_STOPPED:
	default:
		break;
	}

	return CA_ERROR_SUCCESS;
}
