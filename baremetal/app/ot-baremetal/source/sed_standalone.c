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

const char *uriCascodaDiscover    = "ca/di";
const char *uriCascodaTemperature = "ca/te";

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/
otInstance *OT_INSTANCE;

static bool         isConnected = false;
static otIp6Address serverIp;
static uint32_t     appNextSendTime = 5000;

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
				BSP_LEDSigMode(LED_M_CLRALL);
				PlatformSleep(idleTimeLeft);
				BSP_LEDSigMode(1);
			}
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
	if (status == CA_ERROR_FAIL)
	{
		BSP_LEDSigMode(LED_M_SETERROR);
		return;
	}

	BSP_LEDSigMode(LED_M_CLRALL);
	BSP_LEDSigMode(LED_M_CONNECTED_BAT_FULL);

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
                                         otCoapHeader *       aHeader,
                                         otMessage *          aMessage,
                                         const otMessageInfo *aMessageInfo,
                                         otError              aError)
{
	if (aError != OT_ERROR_NONE)
		return;
	if (isConnected)
		return;

	uint16_t length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	otMessageRead(aMessage, otMessageGetOffset(aMessage), &serverIp, sizeof(otIp6Address));
	isConnected = true;
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
	otCoapHeader  header;
	otIp6Address  coapDestinationIp;

	//Build CoAP header
	//Realm local all-nodes multicast - this of course generates some traffic, so shouldn't be overused
	SuccessOrExit(error = otIp6AddressFromString("FF03::1", &coapDestinationIp));
	otCoapHeaderInit(&header, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);
	otCoapHeaderGenerateToken(&header, 2);
	SuccessOrExit(error = otCoapHeaderAppendUriPathOptions(&header, uriCascodaDiscover));

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, &header);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr    = coapDestinationIp;
	messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
	messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

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
static void handleSensorConfirm(void *               aContext,
                                otCoapHeader *       aHeader,
                                otMessage *          aMessage,
                                const otMessageInfo *aMessageInfo,
                                otError              aError)
{
	if (aError == OT_ERROR_RESPONSE_TIMEOUT)
	{
		isConnected = false;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send a multicast cascoda 'server discover' coap message. This is a
 * non-confirmable get request, and the get responses will be handled by the
 * 'handleServerDiscoverResponse' function.
 *******************************************************************************
 ******************************************************************************/
static otError sendSensorData(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage *   message = NULL;
	otMessageInfo messageInfo;
	otCoapHeader  header;
	int32_t       temperature = BSP_GetTemperature();

	//Build CoAP header
	otCoapHeaderInit(&header, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
	otCoapHeaderGenerateToken(&header, 2);
	SuccessOrExit(error = otCoapHeaderAppendUriPathOptions(&header, uriCascodaTemperature));
	otCoapHeaderSetPayloadMarker(&header);

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, &header);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr    = serverIp;
	messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
	messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

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

void SensorHandler(void)
{
	if (TIME_ReadAbsoluteTime() < appNextSendTime)
		return;

	appNextSendTime = TIME_ReadAbsoluteTime() + 10000;
	if (isConnected)
	{
		sendSensorData();
	}
	else
	{
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
	otThreadSetAutoStart(OT_INSTANCE, true);

	otCoapStart(OT_INSTANCE, OT_DEFAULT_COAP_PORT);

	// Endless Polling Loop
	while (1)
	{
		cascoda_io_handler(&dev);
		SensorHandler();
		SED_Handler(&dev);
	}
}
