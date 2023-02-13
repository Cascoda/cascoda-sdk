/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2019                                                ******/
/******************************************************************************/
/******************************************************************************/
/****** Openthread standalone ED w/ basic CoAP server discovery & reporting  **/
/******************************************************************************/
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "M2351.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#include "cascoda_secure.h"
#include "gpio.h"

#include "openthread/cli.h"
#include "openthread/coap.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/platform/settings.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "cbor.h"

/* Uncomment only one of the three "#define"s below. */
#define ACTUATOR_BY_DEFAULT
//#define CONTROLLER_BY_DEFAULT
//#define STOP_BY_DEFAULT

#define MAX_ACTUATORS 6
#define ACTUATOR_UART_BAUDRATE 115200

// General
const char *uriCascodaDiscover              = "ca/di";
const char *uriCascodaActuatorDiscoverQuery = "t=act";
const char *uriCascodaActuator              = "ca/ac";
const char *uriCascodaKeepAlive             = "ca/ka";

enum actuatordemo_state
{
	ACTUATORDEMO_STOPPED    = 0,
	ACTUATORDEMO_ACTUATOR   = 1,
	ACTUATORDEMO_CONTROLLER = 2,
} static actuatordemo_state = ACTUATORDEMO_STOPPED;

enum value_to_send
{
	ACTUATOR_INFO       = 0,
	ACTUATOR_BRIGHTNESS = 1,
	ACTUATOR_COLOUR_MIX = 2,
	ACTUATOR_BOTH       = 3,
};

//Controller
struct actuator_details
{
	otIp6Address ip;
	int          timeoutCount;
	bool         isConnected;
} actuators[MAX_ACTUATORS];
static otCoapResource sKeepAliveResource;
static uint32_t       updateConnectionStatusTime = 5000;
static uint32_t       sendDiscoverTime           = 5000;

// Actuator
static otCoapResource sActuatorResource;
static otCoapResource sDiscoverResource;
static otIp6Address   controllerIp;
static bool           controllerIpKnown             = false;
static bool           previousKeepAliveAcknowledged = true;
int64_t               brightness                    = 0;
int64_t               colour_mix                    = 0;
static uint32_t       nextKeepAliveTime             = 1000;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise UART for communications.
 *******************************************************************************
 ******************************************************************************/
static void ACTUATOR_UARTInit(void)
{
	// 4 MHz
	if (CHILI_GetUseExternalClock())
		CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HXT, CLK_CLKDIV0_UART1(1));
	else
		CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HIRC, CLK_CLKDIV0_UART1(3));

	CLK_EnableModuleClock(UART1_MODULE);

	/* Initialise UART */
	SYS_ResetModule(UART1_RST);
	UART_SetLineConfig(UART1, ACTUATOR_UART_BAUDRATE, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);
	UART_Open(UART1, ACTUATOR_UART_BAUDRATE);

	CHILI_ModuleSetMFP(PN_B, 3, PMFP_UART);

	//Set PB.4 (Pin 32) to logic low
	BSP_ModuleRegisterGPIOOutput(32, MODULE_PIN_TYPE_GENERIC);
	BSP_ModuleSetGPIOPin(32, 0);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief De-Initialise UART.
 *******************************************************************************
 ******************************************************************************/
static void ACTUATOR_UARTDeinit(void)
{
	CLK_DisableModuleClock(UART1_MODULE);
	UART_Close(UART1);

	CHILI_ModuleSetMFP(PN_B, 3, PMFP_GPIO);
	GPIO_SetPullCtl(PB, BITMASK(3), GPIO_PUSEL_PULL_UP);

	//Deregister GPIO on PB.4 (Pin 32)
	BSP_ModuleDeregisterGPIOPin(32);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Complete buffer write using FIFO. BufferSize shouldn't exceed 16.
 *******************************************************************************
 * \param pBuffer - Pointer to data buffer.
 * \param BufferSize - Maximum number of characters to read.
 *******************************************************************************
 ******************************************************************************/
static void ACTUATOR_UARTFIFOWrite(u8_t *pBuffer, u32_t BufferSize)
{
	//Return if BufferSize > transmit FIFO Bytes
	if (BufferSize > 16)
		return;

	u8_t i;
	/* Wait until Tx FIFO is empty */
	while (!(UART1->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
		;
	/* fill FIFO */
	for (i = 0; i < BufferSize; ++i) UART1->DAT = pBuffer[i];
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Determines whether two IP addresses of type otIp6Address are the same.
 *******************************************************************************
 * \param address1 - The first IP address.
 * \param address2 - The second IP address.
 *******************************************************************************
 * \return 1: IP addresses are the same.
 *         0: IP addresses are different.
 *******************************************************************************
 ******************************************************************************/
static bool ipAddressesEqual(otIp6Address address1, otIp6Address address2)
{
	bool theSame;

	if (memcmp(address1.mFields.m8, address2.mFields.m8, 16) == 0)
		theSame = true;
	else
		theSame = false;

	return theSame;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Gets a pointer to the actuator structure that has the given IP address.
 *******************************************************************************
 * \param actuatorIp - The IP address of the actuator.
 *******************************************************************************
 * \return a pointer to the actuator structure.
 *******************************************************************************
 ******************************************************************************/
static struct actuator_details *getActuator(otIp6Address actuatorIp)
{
	for (int i = 0; i < MAX_ACTUATORS; i++)
	{
		if (ipAddressesEqual(actuatorIp, actuators[i].ip))
			return &actuators[i];
	}
	return NULL;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Gets the ID of the actuator that is passed as an argument.
 *******************************************************************************
 * \param actuator - A pointer to the actuator structure.
 *******************************************************************************
 * \return the ID of the actuator.
 *******************************************************************************
 ******************************************************************************/
static int getActuatorID(struct actuator_details *actuator)
{
	return actuator - actuators;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Displays help text
 *******************************************************************************
 ******************************************************************************/
static void displayCommandHelpText(void)
{
	otCliOutputFormat("\r\nBelow is a description of the available commands and what they do.\r\n");
	otCliOutputFormat("All of the commands shown below should be preceded with the word 'actuatordemo'.\r\n");
	otCliOutputFormat("Typing the command 'actuatordemo' by itself will show the current state of the device.\r\n");
	otCliOutputFormat(
	    "The device can be configured to be in one of three states: stopped, actuator and controller.\r\n");

	otCliOutputFormat("\r\n-------------------------------------\r\n");
	otCliOutputFormat("--- Device configuration commands ---\r\n");
	otCliOutputFormat("-------------------------------------");
	otCliOutputFormat("\r\n  actuator");
	otCliOutputFormat("\r\n\tConfigures the device as an actuator.\r\n");
	otCliOutputFormat("\r\n  controller");
	otCliOutputFormat("\r\n\tConfigures the device as a controller.\r\n");
	otCliOutputFormat("\r\n  stop");
	otCliOutputFormat("\r\n\tStops the device's operation.\r\n");

	otCliOutputFormat("\r\n-----------------------------------------------------------------------\r\n");
	otCliOutputFormat("--- Commands that only a device configured as 'controller' can send ---\r\n");
	otCliOutputFormat("-----------------------------------------------------------------------");
	otCliOutputFormat("\r\n  discover");
	otCliOutputFormat("\r\n\tImmediately sends a discover for actuators.\r\n");
	otCliOutputFormat("\r\n  children");
	otCliOutputFormat("\r\n\tDisplays a list of all connected actuators, their ID and their IPv6 address.\r\n");
	otCliOutputFormat("\r\n  <actuator ID> actuator_info (or i)");
	otCliOutputFormat("\r\n\tRequests and displays the brightness and colour_mix information from the actuator with "
	                  "the given ID.\r\n");
	otCliOutputFormat("\r\n  <actuator ID> brightness (or b) <value 0-255>");
	otCliOutputFormat("\r\n\tSets the brightness of the actuator with the given ID to the provided value.\r\n");
	otCliOutputFormat("\r\n  <actuator ID> colour_mix (or c) <value 0-255>");
	otCliOutputFormat("\r\n\tSets the colour_mix of the actuator with the given ID to the provided value.\r\n");
	otCliOutputFormat("\r\n  <actuator ID> brightness (or b) <value 0-255> colour_mix (or c) <value 0-255>");
	otCliOutputFormat(
	    "\r\n\tSets both the brightness and colour_mix of the actuator with the given ID to the provided values.\r\n");
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Displays a list of all connected actuators.
 *******************************************************************************
 ******************************************************************************/
static void showConnectedActuators(void)
{
	bool none_connected = true;

	otCliOutputFormat("\r\n");
	for (int i = 0; i < MAX_ACTUATORS; i++)
	{
		if (actuators[i].isConnected)
		{
			none_connected = false;
			otCliOutputFormat("Actuator %d: ID %d\t", i + 1, i);
			otCliOutputFormat("IP address [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
			                  GETBE16(actuators[i].ip.mFields.m8 + 0),
			                  GETBE16(actuators[i].ip.mFields.m8 + 2),
			                  GETBE16(actuators[i].ip.mFields.m8 + 4),
			                  GETBE16(actuators[i].ip.mFields.m8 + 6),
			                  GETBE16(actuators[i].ip.mFields.m8 + 8),
			                  GETBE16(actuators[i].ip.mFields.m8 + 10),
			                  GETBE16(actuators[i].ip.mFields.m8 + 12),
			                  GETBE16(actuators[i].ip.mFields.m8 + 14));
		}
	}
	if (none_connected)
		otCliOutputFormat("No actuators connected.\r\n");
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Controller: Handle a "KeepAlive".
 *******************************************************************************
 * \param aContext - A pointer to arbitrary context information.
 * \param aMessage - A pointer to the message.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 *******************************************************************************
 ******************************************************************************/
static void handleKeepAlive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage  *responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_GET)
		return;

	//Get the actuator that sent the "KeepAlive"
	struct actuator_details *actuatorPtr = getActuator(aMessageInfo->mPeerAddr);

	if (actuatorPtr == NULL)
		return;

	//Get the ID of the actuator
	int id = getActuatorID(actuatorPtr);

	//Reset the actuator's timeoutCount to 0
	actuatorPtr->timeoutCount = 0;

	ca_log_info("Received 'keepAlive' from actuator id %d\r\n", id);

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
		otCliOutputFormat("Keep Alive ack failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Controller: Handle the response to an actuator discover, and
 * register the actuator locally.
 *******************************************************************************
 * \param aContext - A pointer to application-specific context.
 * \param aMessage - A pointer to the message buffer containing the response.
 *        NULL if no response was received.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 *        NULL if no response was received.
 * \param aError - A result of the CoAP transaction.
 *******************************************************************************
 ******************************************************************************/
static void handleActuatorDiscoverResponse(void                *aContext,
                                           otMessage           *aMessage,
                                           const otMessageInfo *aMessageInfo,
                                           otError              aError)
{
	if (aError != OT_ERROR_NONE)
		return;

	uint16_t length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	if (length < sizeof(otIp6Address))
		return;

	//Return if the discover response is from an actuator that is already connected.
	otIp6Address temp_ip;
	otMessageRead(aMessage, otMessageGetOffset(aMessage), &temp_ip, sizeof(otIp6Address));
	for (int i = 0; i < MAX_ACTUATORS; i++)
	{
		if (ipAddressesEqual(temp_ip, actuators[i].ip) && actuators[i].isConnected)
			return;
	}

	//Connect the actuator if the maximum allowed number of connected actuators hasn't been reached yet.
	int i;
	for (i = 0; i < MAX_ACTUATORS; i++)
	{
		if (!actuators[i].isConnected)
			break;
		if (i == MAX_ACTUATORS - 1)
			return;
	}

	actuators[i].ip           = temp_ip;
	actuators[i].isConnected  = true;
	actuators[i].timeoutCount = 0;

	otCliOutputFormat("Actuator ID %d connected.\r\n", i);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Controller: Send a multicast 'actuator discover' coap
 * message. This is a non-confirmable get request, and the get responses
 * will be handled by the 'handleActuatorDiscoverResponse' function.
 *******************************************************************************
 * \return an error code used in OpenThread.
 *******************************************************************************
 ******************************************************************************/
static otError sendActuatorDiscover(void)
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
	SuccessOrExit(error = otCoapMessageAppendUriQueryOption(message, uriCascodaActuatorDiscoverQuery));

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = coapDestinationIp;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

	//send
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleActuatorDiscoverResponse, NULL);

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
 * \brief Controller: Handle the response to the controller data post.
 *******************************************************************************
 * \param aContext - A pointer to application-specific context.
 * \param aMessage - A pointer to the message buffer containing the response.
 * 		  NULL if no response was received.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 * 		  NULL if no response was received.
 * \param aError - A result of the CoAP transaction.
 *******************************************************************************
 ******************************************************************************/
static void handleControllerPostConfirm(void                *aContext,
                                        otMessage           *aMessage,
                                        const otMessageInfo *aMessageInfo,
                                        otError              aError)
{
	if (aError == OT_ERROR_NONE)
	{
		//Get the actuator that sent the response to the data post
		struct actuator_details *actuatorPtr = getActuator(aMessageInfo->mPeerAddr);

		if (actuatorPtr == NULL)
			return;

		//Reset the actuator's timeoutCount to 0
		actuatorPtr->timeoutCount = 0;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Controller: Handle the response to the controller data get.
 *******************************************************************************
 * \param aContext - A pointer to application-specific context.
 * \param aMessage - A pointer to the message buffer containing the response.
 * 		  NULL if no response was received.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 * 	  	  NULL if no response was received.
 * \param aError - A result of the CoAP transaction.
 *******************************************************************************
 ******************************************************************************/
static void handleControllerGetConfirm(void                *aContext,
                                       otMessage           *aMessage,
                                       const otMessageInfo *aMessageInfo,
                                       otError              aError)
{
	//Handle the received payload
	otMessage  *responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;
	uint16_t    length          = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	uint16_t    offset          = otMessageGetOffset(aMessage);

	CborParser parser;
	CborValue  value;
	CborValue  brightness_result;
	CborValue  colour_mix_result;

	unsigned char *brightness_key = "b";
	unsigned char *colour_mix_key = "c";

	//Get the actuator that sent the response to the data post
	struct actuator_details *actuatorPtr = getActuator(aMessageInfo->mPeerAddr);

	if (actuatorPtr == NULL)
		return;

	//Check for errors
	ca_error error = CA_ERROR_FAIL;

	if (aError == OT_ERROR_RESPONSE_TIMEOUT)
	{
		error = CA_ERROR_FAIL;
	}
	else if (aMessage != NULL && otCoapMessageGetCode(aMessage) != OT_COAP_CODE_CONTENT)
	{
		error = CA_ERROR_FAIL;
	}
	else if (aError == OT_ERROR_NONE)
	{
		//Reset the actuator's timeoutCount to 0
		actuatorPtr->timeoutCount = 0;
		error                     = CA_ERROR_SUCCESS;
	}

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_CONTENT)
		return;

	if (length > 64)
		return;

	//Allocate a buffer and store the received serialised CBOR message into it
	unsigned char buffer[64];
	otMessageRead(aMessage, offset, buffer, length);

	//Initialise the CBOR parser
	SuccessOrExit(cbor_parser_init(buffer, length, 0, &parser, &value));

	//Extract the values corresponding to the keys from the CBOR map
	SuccessOrExit(cbor_value_map_find_value(&value, brightness_key, &brightness_result));
	SuccessOrExit(cbor_value_map_find_value(&value, colour_mix_key, &colour_mix_result));

	//Get the ID of the actuator
	int id = getActuatorID(actuatorPtr);

	//Retrieve and print the actuator values
	otCliOutputFormat("Actuator ID %d info: ", id);

	if (cbor_value_get_type(&brightness_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&brightness_result, &brightness));
		otCliOutputFormat("-- brightness %d/255 -- ", (int)brightness);
	}
	if (cbor_value_get_type(&colour_mix_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&colour_mix_result, &colour_mix));
		otCliOutputFormat("colour_mix %d/255 --\r\n", (int)colour_mix);
	}

exit:
	if (error != CA_ERROR_SUCCESS)
	{
		otCliOutputFormat("Failed to receive info...\r\n", error, otThreadErrorToString(error));
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Controller: Send coap message to the actuator, either requesting
 * information from the actuator or posting data.
 *******************************************************************************
 * \param actuatorID - The ID of the actuator to which the message will be sent.
 * \param type - The type of message to send:
 *               ACTUATOR_INFO: Request information from the actuator.
 *               ACTUATOR_BRIGHTNESS: Set the "brightness" of the actuator.
 *               ACTUATOR_COLOUR_MIX: Set the "colour_mix" of the actuator.
 *               ACTUATOR_BOTH: Simultaneously set the "brightness" and "colour_mix".
 * \param input_value1 - This value is interpreted based on the type of message:
 * 						 ACTUATOR_INFO: This input is ignored.
 * 						 ACTUATOR_BRIGHTNESS: This input is the "brightness" value.
 * 						 ACTUATOR_COLOUR_MIX: This input is the "colour_mix" value.
 * 						 ACTUATOR_BOTH: This input is the "brightness" value.
 * \param input_value2 - This value is ignored except when type = ACTUATOR_BOTH.
 * 						 ACTUATOR_BOTH: This input is the "colour_mix" value.
 *******************************************************************************
 * \return an error code used in OpenThread.
 *******************************************************************************
 ******************************************************************************/
static otError sendControllerRequest(int                actuatorID,
                                     enum value_to_send type,
                                     uint8_t            input_value1,
                                     uint8_t            input_value2)
{
	otError       error   = OT_ERROR_NONE;
	otMessage    *message = NULL;
	otMessageInfo messageInfo;
	uint8_t       brightness = 0;
	uint8_t       colour_mix = 0;
	uint8_t       buffer[16];

	CborError   err = CborNoError;
	CborEncoder encoder, mapEncoder;

	unsigned char *brightness_key = "b";
	unsigned char *colour_mix_key = "c";

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, NULL);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	//Build CoAP header
	if (type == ACTUATOR_INFO)
		otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);
	else
		otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);

	otCoapMessageGenerateToken(message, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaActuator));
	if (type != ACTUATOR_INFO)
	{
		otCoapMessageAppendContentFormatOption(message, OT_COAP_OPTION_CONTENT_FORMAT_CBOR);
		otCoapMessageSetPayloadMarker(message);
	}

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = actuators[actuatorID].ip;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

	if (type == ACTUATOR_INFO)
	{
		error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleControllerGetConfirm, NULL);
	}
	else if (type == ACTUATOR_BOTH || type == ACTUATOR_BRIGHTNESS || type == ACTUATOR_COLOUR_MIX)
	{
		//Initialise the CBOR encoder
		cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);

		//Create and populate the CBOR map
		SuccessOrExit(err = cbor_encoder_create_map(&encoder, &mapEncoder, (type == ACTUATOR_BOTH) ? 2 : 1));

		if (type == ACTUATOR_BOTH || type == ACTUATOR_BRIGHTNESS)
		{
			brightness = input_value1;
			SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, brightness_key));
			SuccessOrExit(err = cbor_encode_int(&mapEncoder, brightness));
		}
		if (type == ACTUATOR_BOTH || type == ACTUATOR_COLOUR_MIX)
		{
			if (type == ACTUATOR_BOTH)
				colour_mix = input_value2;
			else
				colour_mix = input_value1;

			SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, colour_mix_key));
			SuccessOrExit(err = cbor_encode_int(&mapEncoder, colour_mix));
		}

		SuccessOrExit(err = cbor_encoder_close_container(&encoder, &mapEncoder));

		size_t length = cbor_encoder_get_buffer_size(&encoder, buffer);

		//Append and send the serialised message
		SuccessOrExit(error = otMessageAppend(message, buffer, length));
		error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleControllerPostConfirm, NULL);
	}

exit:
	if ((err || error) && message)
	{
		//error, we have to free
		otMessageFree(message);
	}
	return error;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends actuator discover if the conditions are right, and manages
 * the actuators that are already connected in order to determine when they
 * have been disconnected.
 *******************************************************************************
 ******************************************************************************/
static void actuatordemo_handler(void)
{
	if (TIME_ReadAbsoluteTime() > updateConnectionStatusTime)
	{
		updateConnectionStatusTime = TIME_ReadAbsoluteTime() + 1000;
		//If there is room for more actuators to connect, send a discover.
		bool allConnected = true;
		for (uint8_t i = 0; i < MAX_ACTUATORS; i++)
		{
			//If actuator is connected, increase timeoutCount
			if (actuators[i].isConnected)
			{
				if (actuators[i].timeoutCount++ > 60)
				{
					actuators[i].isConnected = false;
					otCliOutputFormat("Actuator ID %d has disconnected\r\n", i);
				}
				ca_log_info("Actuator ID %d timeoutCount %d\r\n", i, actuators[i].timeoutCount);
			}
			allConnected = allConnected && actuators[i].isConnected;
		}
		if (TIME_ReadAbsoluteTime() > sendDiscoverTime && !allConnected)
		{
			sendDiscoverTime = TIME_ReadAbsoluteTime() + 30000;
			sendActuatorDiscover();
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Actuator: Handle the response to a "sendKeepAlive".
 *******************************************************************************
 * \param aContext - A pointer to application-specific context.
 * \param aMessage - A pointer to the message buffer containing the response.
 *        NULL if no response was received.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 *        NULL if no response was received.
 * \param aError - A result of the CoAP transaction.
 *******************************************************************************
 ******************************************************************************/
static void handleKeepAliveResponse(void                *aContext,
                                    otMessage           *aMessage,
                                    const otMessageInfo *aMessageInfo,
                                    otError              aError)
{
	ca_log_debg("In response handler\r\n");

	//Set controllerIpKnown to false on timeout.
	if (aError != OT_ERROR_NONE)
	{
		ca_log_info("TIMEOUT\r\n");
		controllerIpKnown = false;
	}
	else
	{
		previousKeepAliveAcknowledged = true;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Actuator: Send a "KeepAlive" coap message to the controller to notify it
 * that it is connected.
 *******************************************************************************
 * \return an error code used in OpenThread.
 *******************************************************************************
 ******************************************************************************/
static otError sendKeepAlive(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage    *message = NULL;
	otMessageInfo messageInfo;

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, NULL);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	//Build CoAP header
	otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);
	otCoapMessageGenerateToken(message, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaKeepAlive));

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = controllerIp;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

	//send
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleKeepAliveResponse, NULL);
	ca_log_info("Sent keep alive, error: %s\r\n", otThreadErrorToString(error));
	previousKeepAliveAcknowledged = false;

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
 * \brief Actuator: Send a coap message with "brightness" and "colour_mix"
 * data information about itself.
 *******************************************************************************
 * \param aMessage - A pointer to the message.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 *******************************************************************************
 * \return an error code used in OpenThread.
 *******************************************************************************
 ******************************************************************************/
static otError sendActuatorData(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError       error   = OT_ERROR_NONE;
	otMessage    *message = NULL;
	otMessageInfo messageInfo;

	uint8_t     buffer[32];
	CborError   err;
	CborEncoder encoder, mapEncoder;

	unsigned char *brightness_key = "b";
	unsigned char *colour_mix_key = "c";

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, NULL);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	//Build CoAP header
	otCoapMessageInitResponse(message, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_CONTENT);
	otCoapMessageSetToken(message, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));
	otCoapMessageAppendContentFormatOption(message, OT_COAP_OPTION_CONTENT_FORMAT_CBOR);
	otCoapMessageSetPayloadMarker(message);

	//Initialise the CBOR encoder
	cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);

	//Create and populate the CBOR map
	SuccessOrExit(err = cbor_encoder_create_map(&encoder, &mapEncoder, 2));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, brightness_key));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, brightness));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, colour_mix_key));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, colour_mix));
	SuccessOrExit(err = cbor_encoder_close_container(&encoder, &mapEncoder));

	size_t length = cbor_encoder_get_buffer_size(&encoder, buffer);

	//Append and send the serialised message
	SuccessOrExit(error = otMessageAppend(message, buffer, length));
	SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, message, aMessageInfo));

exit:
	if ((err || error) && message)
	{
		//error, we have to free
		otMessageFree(message);
	}
	return error;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Actuator: Handle a controller get/post request.
 *******************************************************************************
 * \param aContext - A pointer to arbitrary context information.
 * \param aMessage - A pointer to the message.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 *******************************************************************************
 ******************************************************************************/
static void handleControllerRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage  *responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;
	uint16_t    length          = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	uint16_t    offset          = otMessageGetOffset(aMessage);

	CborParser parser;
	CborValue  value;
	CborValue  brightness_result;
	CborValue  colour_mix_result;

	unsigned char *brightness_key = "b";
	unsigned char *colour_mix_key = "c";

	if (length > 64)
		return;

	if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
	{
		SuccessOrExit(error = sendActuatorData(aMessage, aMessageInfo));
		return;
	}
	else if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_POST)
	{
		//Allocate a buffer and store the received serialised CBOR message into it
		unsigned char buffer[64];
		otMessageRead(aMessage, offset, buffer, length);

		//Initialise the CBOR parser
		SuccessOrExit(cbor_parser_init(buffer, length, 0, &parser, &value));

		//Extract the values corresponding to the keys from the CBOR map
		SuccessOrExit(cbor_value_map_find_value(&value, brightness_key, &brightness_result));
		SuccessOrExit(cbor_value_map_find_value(&value, colour_mix_key, &colour_mix_result));

		//Retrieve and print the controller-requested value
		printf("Actuator received");

		if (cbor_value_get_type(&brightness_result) != CborInvalidType)
		{
			SuccessOrExit(cbor_value_get_int64(&brightness_result, &brightness));
			printf(" brightness %d", (int)brightness);
		}
		if (cbor_value_get_type(&colour_mix_result) != CborInvalidType)
		{
			SuccessOrExit(cbor_value_get_int64(&colour_mix_result, &colour_mix));
			printf(" colour_mix %d", (int)colour_mix);
		}

		printf(" from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 0),
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 2),
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 4),
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 6),
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 8),
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 10),
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 12),
		       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 14));

		//Send brightness and colour_mix via UART
		uint8_t data_buffer[2] = {(uint8_t)brightness, (uint8_t)colour_mix};
		uint8_t buffer_size    = 2;
		ACTUATOR_UARTFIFOWrite(data_buffer, buffer_size);

		responseMessage = otCoapNewMessage(OT_INSTANCE, NULL);
		if (responseMessage == NULL)
		{
			error = OT_ERROR_NO_BUFS;
			goto exit;
		}

		otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_VALID);
		otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

		SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));
	}
	else
	{
		return;
	}

exit:
	if (error != OT_ERROR_NONE && error != OT_ERROR_NO_BUFS && responseMessage == NULL)
	{
		responseMessage = otCoapNewMessage(OT_INSTANCE, NULL);
		if (responseMessage == NULL)
		{
			error = OT_ERROR_NO_BUFS;
			goto exit;
		}

		otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_BAD_REQUEST);
		otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

		SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));
	}
	else if (error != OT_ERROR_NONE && responseMessage != NULL)
	{
		printf("Controller request ack failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Actuator: Handle a discover message.
 *******************************************************************************
 * \param aContext - A pointer to arbitrary context information.
 * \param aMessage - A pointer to the message.
 * \param aMessageInfo - A pointer to the message info for aMessage.
 *******************************************************************************
 ******************************************************************************/
static void handleDiscover(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError               error           = OT_ERROR_NONE;
	otMessage            *responseMessage = NULL;
	otInstance           *OT_INSTANCE     = aContext;
	otCoapOptionIterator *iterator;
	const otCoapOption   *option;
	char                  uri_query[6];
	bool                  valid_query = false;

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_GET)
		return;

	if (otCoapOptionIteratorInit(iterator, aMessage) != OT_ERROR_NONE)
		return;

	// Find the URI Query
	for (option = otCoapOptionIteratorGetFirstOption(iterator); option != NULL;
	     option = otCoapOptionIteratorGetNextOption(iterator))
	{
		if (option->mNumber == OT_COAP_OPTION_URI_QUERY && option->mLength <= 6)
		{
			SuccessOrExit(otCoapOptionIteratorGetOptionValue(iterator, uri_query));

			if (strncmp(uri_query, uriCascodaActuatorDiscoverQuery, 5) == 0)
			{
				valid_query = true;
				break;
			}
		}
	}

	if (!valid_query)
		return;

	printf("Actuator received discover from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 0),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 2),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 4),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 6),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 8),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 10),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 12),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 14));

	controllerIp                  = aMessageInfo->mPeerAddr;
	controllerIpKnown             = true;
	previousKeepAliveAcknowledged = true;

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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Add controller coap resources to the coap stack.
 *******************************************************************************
 ******************************************************************************/
static void bind_controller_resources()
{
	otCoapAddResource(OT_INSTANCE, &sKeepAliveResource);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Remove controller coap resources from the coap stack.
 *******************************************************************************
 ******************************************************************************/
static void unbind_controller_resources()
{
	otCoapRemoveResource(OT_INSTANCE, &sKeepAliveResource);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Add actuator coap resources to the coap stack.
 *******************************************************************************
 ******************************************************************************/
static void bind_actuator_resources()
{
	otCoapAddResource(OT_INSTANCE, &sActuatorResource);
	otCoapAddResource(OT_INSTANCE, &sDiscoverResource);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Remove actuator coap resources from the coap stack.
 *******************************************************************************
 ******************************************************************************/
static void unbind_actuator_resources()
{
	otCoapRemoveResource(OT_INSTANCE, &sActuatorResource);
	otCoapRemoveResource(OT_INSTANCE, &sDiscoverResource);
}

void handle_cli_actuatordemo(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	(void)aContext;

	enum actuatordemo_state prevState = actuatordemo_state;

	if (aArgsLength == 0)
	{
		switch (actuatordemo_state)
		{
		case ACTUATORDEMO_ACTUATOR:
			otCliOutputFormat("actuator\n");
			break;
		case ACTUATORDEMO_CONTROLLER:
			otCliOutputFormat("controller\n");
			break;
		case ACTUATORDEMO_STOPPED:
		default:
			otCliOutputFormat("stopped\n");
			break;
		}
	}
	if (aArgsLength == 1)
	{
		if (strcmp(aArgs[0], "actuator") == 0)
		{
			actuatordemo_state = ACTUATORDEMO_ACTUATOR;
			bind_actuator_resources();
			unbind_controller_resources();
			ACTUATOR_UARTInit();
		}
		else if (strcmp(aArgs[0], "controller") == 0)
		{
			actuatordemo_state = ACTUATORDEMO_CONTROLLER;
			bind_controller_resources();
			unbind_actuator_resources();
			if (prevState == ACTUATORDEMO_ACTUATOR)
				ACTUATOR_UARTDeinit();
		}
		else if (strcmp(aArgs[0], "stop") == 0)
		{
			actuatordemo_state = ACTUATORDEMO_STOPPED;
			unbind_actuator_resources();
			unbind_controller_resources();
			if (prevState == ACTUATORDEMO_ACTUATOR)
				ACTUATOR_UARTDeinit();
		}
		else if (strcmp(aArgs[0], "help") == 0)
		{
			displayCommandHelpText();
		}
		else if (strcmp(aArgs[0], "children") == 0 && actuatordemo_state == ACTUATORDEMO_CONTROLLER)
		{
			showConnectedActuators();
		}
		else if (strcmp(aArgs[0], "discover") == 0 && actuatordemo_state == ACTUATORDEMO_CONTROLLER)
		{
			sendActuatorDiscover();
		}
		if (prevState != actuatordemo_state)
			otPlatSettingsSet(
			    OT_INSTANCE, actuatordemo_key, (uint8_t *)&actuatordemo_state, sizeof(actuatordemo_state));
	}
	else if (aArgsLength == 2 && actuatordemo_state == ACTUATORDEMO_CONTROLLER)
	{
		bool actuatorIDWithinLimits   = atoi(aArgs[0]) < MAX_ACTUATORS;
		bool validActuatorInfoCommand = (strcmp(aArgs[1], "actuator_info") == 0 || strcmp(aArgs[1], "i") == 0);

		if (actuatorIDWithinLimits && validActuatorInfoCommand)
		{
			sendControllerRequest(atoi(aArgs[0]), ACTUATOR_INFO, -1, -1);
		}
	}
	else if (aArgsLength == 3 && actuatordemo_state == ACTUATORDEMO_CONTROLLER)
	{
		bool actuatorIDWithinLimits         = atoi(aArgs[0]) < MAX_ACTUATORS;
		bool validActuatorBrightnessCommand = (strcmp(aArgs[1], "brightness") == 0 || strcmp(aArgs[1], "b") == 0);
		bool validActuatorColourMixCommand  = (strcmp(aArgs[1], "colour_mix") == 0 || strcmp(aArgs[1], "c") == 0);
		bool valueWithinLimits              = atoi(aArgs[2]) <= 255;

		if (actuatorIDWithinLimits && validActuatorBrightnessCommand && valueWithinLimits)
		{
			sendControllerRequest(atoi(aArgs[0]), ACTUATOR_BRIGHTNESS, (uint8_t)atoi(aArgs[2]), -1);
		}
		else if (actuatorIDWithinLimits && validActuatorColourMixCommand && valueWithinLimits)
		{
			sendControllerRequest(atoi(aArgs[0]), ACTUATOR_COLOUR_MIX, (uint8_t)atoi(aArgs[2]), -1);
		}
	}
	else if (aArgsLength == 5 && actuatordemo_state == ACTUATORDEMO_CONTROLLER)
	{
		bool actuatorIDWithinLimits         = atoi(aArgs[0]) < MAX_ACTUATORS;
		bool validActuatorBrightnessCommand = (strcmp(aArgs[1], "brightness") == 0 || strcmp(aArgs[1], "b") == 0);
		bool brightnessWithinLimits         = atoi(aArgs[2]) <= 255;
		bool validActuatorColourMixCommand  = (strcmp(aArgs[3], "colour_mix") == 0 || strcmp(aArgs[3], "c") == 0);
		bool colourMixWithinLimits          = atoi(aArgs[4]) <= 255;

		if (actuatorIDWithinLimits && validActuatorBrightnessCommand && brightnessWithinLimits &&
		    validActuatorColourMixCommand && colourMixWithinLimits)
		{
			sendControllerRequest(atoi(aArgs[0]), ACTUATOR_BOTH, (uint8_t)atoi(aArgs[2]), (uint8_t)atoi(aArgs[4]));
		}
	}
}

ca_error init_actuatordemo(otInstance *aInstance, struct ca821x_dev *pDeviceRef)
{
	otError  error = OT_ERROR_NONE;
	uint16_t stateLength;
	OT_INSTANCE = aInstance;

	otCoapStart(OT_INSTANCE, OT_DEFAULT_COAP_PORT);

	memset(&sActuatorResource, 0, sizeof(sActuatorResource));
	sActuatorResource.mUriPath = uriCascodaActuator;
	sActuatorResource.mContext = aInstance;
	sActuatorResource.mHandler = &handleControllerRequest;
	memset(&sDiscoverResource, 0, sizeof(sDiscoverResource));
	sDiscoverResource.mUriPath = uriCascodaDiscover;
	sDiscoverResource.mContext = aInstance;
	sDiscoverResource.mHandler = &handleDiscover;
	memset(&sKeepAliveResource, 0, sizeof(sKeepAliveResource));
	sKeepAliveResource.mUriPath = uriCascodaKeepAlive;
	sKeepAliveResource.mContext = aInstance;
	sKeepAliveResource.mHandler = &handleKeepAlive;

	stateLength = sizeof(actuatordemo_state);
	error       = otPlatSettingsGet(OT_INSTANCE, actuatordemo_key, 0, (uint8_t *)&actuatordemo_state, &stateLength);
	if (error != OT_ERROR_NONE)
	{
#ifdef ACTUATOR_BY_DEFAULT
		actuatordemo_state = ACTUATORDEMO_ACTUATOR;
#endif /* ACTUATOR_BY_DEFAULT */

#ifdef CONTROLLER_BY_DEFAULT
		actuatordemo_state = ACTUATORDEMO_CONTROLLER;
#endif /* CONTROLLER_BY_DEFAULT */

#ifdef STOP_BY_DEFAULT
		actuatordemo_state = ACTUATORDEMO_STOPPED;
#endif /* STOP_BY_DEFAULT */
	}
	if (actuatordemo_state == ACTUATORDEMO_ACTUATOR)
	{
		bind_actuator_resources();
		ACTUATOR_UARTInit();
	}
	if (actuatordemo_state == ACTUATORDEMO_CONTROLLER)
	{
		bind_controller_resources();
	}

	return CA_ERROR_SUCCESS;
}

ca_error handle_actuatordemo(struct ca821x_dev *pDeviceRef)
{
	switch (actuatordemo_state)
	{
	case ACTUATORDEMO_CONTROLLER:
		actuatordemo_handler();
		break;
	case ACTUATORDEMO_ACTUATOR:
		if (TIME_ReadAbsoluteTime() > nextKeepAliveTime && controllerIpKnown && previousKeepAliveAcknowledged)
		{
			nextKeepAliveTime = TIME_ReadAbsoluteTime() + 15000;
			sendKeepAlive();
		}
		break;
	case ACTUATORDEMO_STOPPED:
	default:
		break;
	}

	return CA_ERROR_SUCCESS;
}
