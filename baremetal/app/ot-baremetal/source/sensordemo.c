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

// General
const char *uriCascodaDiscover         = "ca/di";
const char *uriCascodaTemperature      = "ca/te";
const char *uriCascodaDiscoverResponse = "ca/dr";

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
static otCoapResource sTempResource;
static otCoapResource sDiscoverResource;
static otCoapResource sDiscoverResponseResource;

/**
 * \brief Handle the response to the server discover, and register the server
 * locally.
 */
static void handleDiscoverResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_POST)
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
 * non-confirmable POST request, and the seperate POST responses will be handled
 * by the 'handleDiscoverResponse' function.
 */
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
	otCoapMessageInit(message, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
	otCoapMessageGenerateToken(message, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaDiscover));

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr    = coapDestinationIp;
	messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;

	//send
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, NULL, NULL);

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
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaTemperature));
	otCoapMessageSetPayloadMarker(message);

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr    = serverIp;
	messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;

	//Payload
	SuccessOrExit(error = otMessageAppend(message, &temperature, sizeof(temperature)));

	//send
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleSensorConfirm, NULL);

exit:
	if (error && message)
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

/** Server: Handle a temperature message by printing it */
static void handleTemperature(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage * responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;
	uint16_t    length          = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	int32_t     temperature;

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_POST)
		return;

	if (length != sizeof(temperature))
		return;

	otMessageRead(aMessage, otMessageGetOffset(aMessage), &temperature, sizeof(temperature));

	otCliOutputFormat("Server received temperature %d.%d*C from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
	                  (temperature / 10),
	                  (temperature % 10),
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

	otCoapMessageInit(responseMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_VALID);
	otCoapMessageSetMessageId(responseMessage, otCoapMessageGetMessageId(aMessage));
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

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_POST)
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

	otCoapMessageInit(responseMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
	otCoapMessageGenerateToken(responseMessage, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(responseMessage, uriCascodaDiscoverResponse));
	otCoapMessageSetPayloadMarker(responseMessage);

	SuccessOrExit(error = otMessageAppend(responseMessage, otThreadGetMeshLocalEid(OT_INSTANCE), sizeof(otIp6Address)));
	SuccessOrExit(error = otCoapSendRequest(OT_INSTANCE, responseMessage, aMessageInfo, NULL, NULL));

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
	otCoapAddResource(OT_INSTANCE, &sTempResource);
	otCoapAddResource(OT_INSTANCE, &sDiscoverResource);
}

/** Remove server coap resources to the coap stack */
static void unbind_server_resources()
{
	otCoapRemoveResource(OT_INSTANCE, &sTempResource);
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

	memset(&sTempResource, 0, sizeof(sTempResource));
	sTempResource.mUriPath = uriCascodaTemperature;
	sTempResource.mContext = aInstance;
	sTempResource.mHandler = &handleTemperature;
	memset(&sDiscoverResource, 0, sizeof(sDiscoverResource));
	sDiscoverResource.mUriPath = uriCascodaDiscover;
	sDiscoverResource.mContext = aInstance;
	sDiscoverResource.mHandler = &handleDiscover;
	memset(&sDiscoverResponseResource, 0, sizeof(sDiscoverResponseResource));
	sDiscoverResponseResource.mUriPath = uriCascodaDiscoverResponse;
	sDiscoverResponseResource.mContext = aInstance;
	sDiscoverResponseResource.mHandler = &handleDiscoverResponse;

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
