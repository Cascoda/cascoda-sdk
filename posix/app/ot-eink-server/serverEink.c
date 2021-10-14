/*
 *  Copyright (c) 2019, Cascoda Ltd.
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

#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "openthread/cli.h"
#include "openthread/coap.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"

#include "cbor.h"

#include "ca821x-posix-thread/posix-platform.h"

#define SuccessOrExit(aCondition) \
	do                            \
	{                             \
		if ((aCondition) != 0)    \
		{                         \
			goto exit;            \
		}                         \
	} while (0)
#define FILENAME_SIZE (255)

static int            isRunning;
static otCoapResource sDiscoverResource;
static otCoapResource sImageResource;
static const char *   sDiscoverUri = "ca/di";
static const char *   sImageUri    = "ca/img";

static char         imageBuffer[1024] = {0};
static otCliCommand sCliCommands[2];

struct connected_device
{
	uint8_t        ip[16];
	otExtAddress   eui64;
	int            deviceID;
	bool           isConnected;
	struct timeval last_wakeup;
	char *         fileName;
};

#define MAX_CONNECTED_DEVICES_LIST 50
static time_t                  TIMEOUT_S                           = 600;
static int                     num_of_connected_devices            = 0;
static struct connected_device devices[MAX_CONNECTED_DEVICES_LIST] = {};

void printf_time(const char *format, ...)
{
	struct timeval tv;
	char           timeString[40];
	va_list        args;

	gettimeofday(&tv, NULL);
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
	printf("%s - ", timeString);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

static bool ipAddressEqual(otIp6Address address1, uint8_t deviceAddress[])
{
	bool ipValue;
	if (memcmp(address1.mFields.m8, deviceAddress, 16) == 0)
		ipValue = true;
	else
		ipValue = false;

	return ipValue;
}

static struct connected_device *getDevice(otIp6Address deviceIP)
{
	for (int i = 0; i < MAX_CONNECTED_DEVICES_LIST; i++)
	{
		if (ipAddressEqual(deviceIP, devices[i].ip))
			return &devices[i];
	}
	return NULL;
}

static int getDeviceID(struct connected_device *device)
{
	return device - devices;
}

static void handleDiscover(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError      error           = OT_ERROR_NONE;
	otMessage *  responseMessage = NULL;
	otInstance * OT_INSTANCE     = aContext;
	otExtAddress eui64;
	//including CBOR variables
	uint8_t    buffer[64];
	uint16_t   messageLength = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	uint16_t   messageOffset = otMessageGetOffset(aMessage);
	CborParser parser;
	CborValue  value;
	CborValue  eui64_result;
	CborValue  result;

	CborError   err;
	CborEncoder encoder, mapEncoder;
	//Cbor Parser Variables
	uint16_t length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	uint16_t offset = otMessageGetOffset(aMessage);

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_GET)
		return;

	if (messageLength > sizeof(buffer))
		return;

	otMessageRead(aMessage, messageOffset, buffer, messageLength);

	//Initialise the CBOR parser
	SuccessOrExit(cbor_parser_init(buffer, messageLength, 0, &parser, &value));
	//Extract the values corresponding to the keys from the CBOR map
	SuccessOrExit(cbor_value_map_find_value(&value, "eui", &eui64_result));

	//Retrieve and store the device's Eui64 value
	if (cbor_value_get_type(&eui64_result) != CborInvalidType)
	{
		size_t len = sizeof(otExtAddress);
		SuccessOrExit(cbor_value_copy_byte_string(&eui64_result, devices->eui64.m8, &len, NULL));
	}

	printf_time("Server received discover from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 0),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 2),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 4),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 6),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 8),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 10),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 12),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 14));

	bool           is_already_connected = false;
	int            connected_device_id;
	struct timeval time_now = {0};
	gettimeofday(&time_now, NULL);

	// Check if the device that sent the request is already connected
	for (int i = 0; i < MAX_CONNECTED_DEVICES_LIST; ++i)
	{
		if (ipAddressEqual(aMessageInfo->mPeerAddr, devices[i].ip) && devices[i].isConnected)
		{
			is_already_connected   = true;
			connected_device_id    = i;
			devices[i].last_wakeup = time_now;
			break;
		}
	}

	// Check if any device has timed out
	for (int i = 0; i < MAX_CONNECTED_DEVICES_LIST; ++i)
	{
		time_t last_wakeup_s = devices[i].last_wakeup.tv_sec;

		if ((time_now.tv_sec > last_wakeup_s + TIMEOUT_S) && last_wakeup_s != 0)
		{
			devices[i].isConnected = false;
			devices[i].fileName    = "none";
			num_of_connected_devices--;

			printf_time("[%x:%x:%x:%x:%x:%x:%x:%x] timed out!\r\n",
			            GETBE16(devices[i].ip + 0),
			            GETBE16(devices[i].ip + 2),
			            GETBE16(devices[i].ip + 4),
			            GETBE16(devices[i].ip + 6),
			            GETBE16(devices[i].ip + 8),
			            GETBE16(devices[i].ip + 10),
			            GETBE16(devices[i].ip + 12),
			            GETBE16(devices[i].ip + 14));
			printf_time("Last seen %ds ago.\r\n", time_now.tv_sec - last_wakeup_s);
		}
	}

	if (!is_already_connected)
	{
		// Add the new device to the list of connected devices if possible
		int i;
		for (i = 0; i < MAX_CONNECTED_DEVICES_LIST; i++)
		{
			if (!devices[i].isConnected)
				break;
			if (i == MAX_CONNECTED_DEVICES_LIST - 1)
			{
				//TODO: Respond to the client with a coap error 5.03 Service Unavailable instead of ignoring
				printf_time("Capacity full, couldn't connect the new device.\r\n");
				return;
			}
		}

		devices[i].isConnected = true;
		memcpy(devices[i].ip, aMessageInfo->mPeerAddr.mFields.m8, 16);
		devices[i].fileName = "none";
		num_of_connected_devices++;
		printf_time("New device connected!\r\n");

		printf_time("Connected devices: %d\r\n", num_of_connected_devices);
		gettimeofday(&time_now, NULL);
		devices[i].last_wakeup = time_now;
	}
	else
	{
		memcpy(devices[connected_device_id].ip, aMessageInfo->mPeerAddr.mFields.m8, 16);

		printf_time("Connected devices: %d\r\n", num_of_connected_devices);
		gettimeofday(&time_now, NULL);
		devices[connected_device_id].last_wakeup = time_now;
	}

	responseMessage = otCoapNewMessage(OT_INSTANCE, NULL);
	if (responseMessage == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	//Initialise CBOR parser
	SuccessOrExit(cbor_parser_init(buffer, length, 0, &parser, &value));

	//Extract values corresponding to the keys from the CBOR map
	SuccessOrExit(cbor_value_map_find_value(&value, "eui", &result));
	if (cbor_value_get_type(&result) != CborInvalidType)
	{
		size_t len = sizeof(otExtAddress);
		SuccessOrExit(cbor_value_copy_byte_string(&result, devices->eui64.m8, &len, NULL));
	}

	otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_CONTENT);
	otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

	otCoapMessageSetPayloadMarker(responseMessage);

	//get and send Device ID back to the client.
	struct connected_device *devicePtr = getDevice(aMessageInfo->mPeerAddr);

	if (devicePtr == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	devicePtr->deviceID = getDeviceID(devicePtr);

	//Sending back IP and Device ID
	//Initialise CBOR encoder
	cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);

	SuccessOrExit(err = cbor_encoder_create_map(&encoder, &mapEncoder, 2));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "ip"));
	SuccessOrExit(err = cbor_encode_byte_string(
	                  &mapEncoder, otThreadGetMeshLocalEid(OT_INSTANCE)->mFields.m8, sizeof(otIp6Address)));

	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "devID"));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, devices->deviceID));

	SuccessOrExit(err = cbor_encoder_close_container(&encoder, &mapEncoder));
	size_t len = cbor_encoder_get_buffer_size(&encoder, buffer);
	SuccessOrExit(error = otMessageAppend(responseMessage, buffer, len));

	SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));

exit:
	if ((err || error) && responseMessage != NULL)
	{
		printf("Discover response failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

static void handleImageRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage * responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;
	//including CBOR variables
	uint8_t     buffer[1024] = {0};
	CborError   err;
	CborEncoder encoder, mapEncoder;

	char                filename[FILENAME_SIZE] = {0};
	char                deviceIdString[10];
	char                uri_query[10];
	bool                valid_query = false;
	const otCoapOption *option;

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_GET)
		return;

	printf_time("Server received GET Request from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 0),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 2),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 4),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 6),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 8),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 10),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 12),
	            GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 14));

	for (option = otCoapMessageGetFirstOption(aMessage); option != NULL; option = otCoapMessageGetNextOption(aMessage))
	{
		if (option->mNumber == OT_COAP_OPTION_URI_QUERY && option->mLength < sizeof(uri_query))
		{
			SuccessOrExit(otCoapMessageGetOptionValue(aMessage, uri_query));
			valid_query = true;
			break;
		}
	}

	if (!valid_query)
		return;

	responseMessage = otCoapNewMessage(OT_INSTANCE, NULL);
	if (responseMessage == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	int i;
	for (i = 0; i < MAX_CONNECTED_DEVICES_LIST; i++)
	{
		if (!devices[i].isConnected)
			continue;

		snprintf(deviceIdString, sizeof(deviceIdString), "id=%d", devices[i].deviceID);
		break;
	}

	if (strncmp(uri_query, deviceIdString, sizeof(uri_query)) != 0)
		return;

	if (strncmp(devices[i].fileName, "none", sizeof(devices[i].fileName)) != 0)
	{
		strcpy(filename, devices[i].fileName);
		otCliOutputFormat("File Added: %s\r\n", filename);
	}
	else
	{
		/*send response that the request was received but there are no images available.*/
		otCliOutputFormat("No Image Available.\r\n");
		// CoAP header
		otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_NOT_FOUND);
		otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));
		SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));
		return;
	}

	FILE *fin;
	int   filesize = 0;

	if ((fin = fopen(filename, "rb")) == NULL)
	{
		printf_time("Could not open \"%s\"!\r\n", filename);
		error = OT_ERROR_FAILED;
		goto exit;
	}

	fseek(fin, 0, SEEK_END);
	filesize = ftell(fin);
	fseek(fin, 0, SEEK_SET);

	if (fread(imageBuffer, 1, sizeof(imageBuffer), fin) != filesize)
	{
		// TODO: handle this error more gracefully, by sending a server error
		printf_time("Archive \"%s\" too large to transmit!\r\n", filename);
		error = OT_ERROR_FAILED;
		fclose(fin);
		goto exit;
	}

	fclose(fin);

	// CoAP header
	otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_CONTENT);
	otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

	otCoapMessageSetPayloadMarker(responseMessage);

	//Initialise CBOR encoder
	cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);

	//use temp buffer instead of image buffer
	SuccessOrExit(err = cbor_encoder_create_map(&encoder, &mapEncoder, 2));

	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "img"));
	SuccessOrExit(err = cbor_encode_byte_string(&mapEncoder, imageBuffer, filesize));

	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "devID"));
	SuccessOrExit(err = cbor_encode_int(&mapEncoder, devices->deviceID));

	SuccessOrExit(err = cbor_encoder_close_container(&encoder, &mapEncoder));
	size_t length = cbor_encoder_get_buffer_size(&encoder, buffer);

	// CoAP Payload: contents of the image buffer
	SuccessOrExit(error = otMessageAppend(responseMessage, buffer, length));
	SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));

	printf_time("Sent image titled \"%s\"\r\n", filename);

exit:
	if (error != OT_ERROR_NONE && responseMessage != NULL)
	{
		printf_time("Image response failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

static void registerCoapResources(otInstance *aInstance)
{
	otError error = OT_ERROR_NONE;

	SuccessOrExit(error = otCoapStart(aInstance, OT_DEFAULT_COAP_PORT));

	memset(&sDiscoverResource, 0, sizeof(sDiscoverResource));
	sDiscoverResource.mUriPath = sDiscoverUri;
	sDiscoverResource.mContext = aInstance;
	sDiscoverResource.mHandler = &handleDiscover;

	memset(&sImageResource, 0, sizeof(sImageResource));
	sImageResource.mUriPath = sImageUri;
	sImageResource.mContext = aInstance;
	sImageResource.mHandler = &handleImageRequest;

	SuccessOrExit(error = otCoapAddResource(aInstance, &sDiscoverResource));
	SuccessOrExit(error = otCoapAddResource(aInstance, &sImageResource));

exit:
	assert(error == OT_ERROR_NONE);
	return;
}

static void listConnectedDevice(void)
{
	bool noneConnected = true;

	otCliOutputFormat("\r\n");
	for (int i = 0; i < MAX_CONNECTED_DEVICES_LIST; i++)
	{
		if (devices[i].isConnected)
		{
			noneConnected = false;
			otCliOutputFormat("Device %d: ID %d \t", i + 1, devices[i].deviceID);
			otCliOutputFormat("Eui64: %llx, ", GETBE64(devices[i].eui64.m8));
			otCliOutputFormat("IP:[%x:%x:%x:%x:%x:%x:%x:%x] \r\n ",
			                  GETBE16(devices[i].ip + 0),
			                  GETBE16(devices[i].ip + 2),
			                  GETBE16(devices[i].ip + 4),
			                  GETBE16(devices[i].ip + 6),
			                  GETBE16(devices[i].ip + 8),
			                  GETBE16(devices[i].ip + 10),
			                  GETBE16(devices[i].ip + 12),
			                  GETBE16(devices[i].ip + 14));
			otCliOutputFormat("Assigned Image: %s\r\n", devices[i].fileName);
		}
	}
	if (noneConnected)
		otCliOutputFormat("No Devices Visible. \r\n");
}

static void list_error(void)
{
	otCliOutputFormat("Parse error, usage: list devices\r\n");
}

static void handle_cli_list(int argc, char *argv[])
{
	if (argc == 1 && strcmp(argv[0], "devices") == 0)
	{
		listConnectedDevice();
	}
	else
	{
		list_error();
		return;
	}
}

static void assign_error(void)
{
	otCliOutputFormat("Parse error, usage: assign [Device ID] [image.gz]\r\n");
}

static void handle_cli_assign(int argc, char *argv[])
{
	if (argc != 2)
	{
		assign_error();
		return;
	}

	else
	{
		//assign <ID> <filename>
		char *      fileLabel = argv[1];
		int         len       = strlen(fileLabel);
		const char *gzip      = &fileLabel[len - 3];
		int         inputId   = atoi(argv[0]);

		bool deviceIDWithinLimits = (inputId >= 0 && inputId <= MAX_CONNECTED_DEVICES_LIST);

		if (!deviceIDWithinLimits)
		{
			otCliOutputFormat("Error: Invalid Device ID. Please enter an ID between 0 and 50.\r\n");
			return;
		}
		if (!devices[inputId].isConnected)
		{
			otCliOutputFormat("Device not connected. \r\n");
			return;
		}
		else
		{
			if (memcmp(gzip, ".gz", 3) == 0)
			{
				devices[inputId].fileName = fileLabel;
				otCliOutputFormat(
				    "File Name: %s, assigned to device with ID: %d\r\n", fileLabel, devices[inputId].deviceID);
			}
			else
			{
				otCliOutputFormat("Error: Invalid Image type. Please try again.\r\n");
				return;
			}
		}
	}
}

ca_error init_sed_eink_commands(otInstance *aInstance)
{
	otError  error = OT_ERROR_NONE;
	uint16_t stateLength;

	//CLI Commands
	sCliCommands[0].mCommand = &handle_cli_list;
	sCliCommands[0].mName    = "list";
	sCliCommands[1].mCommand = &handle_cli_assign;
	sCliCommands[1].mName    = "assign";

	otCliSetUserCommands(sCliCommands, 2);
}

static void quit(int sig)
{
	isRunning = 0;
}

int main(int argc, char *argv[])
{
	otInstance *OT_INSTANCE;

	printf_time("Thread Sensor Server %s initialising...\r\n", ca821x_get_version());

	if (argc > 1)
		NODE_ID = atoi(argv[1]);
	else
		NODE_ID = 597;

	posixPlatformSetOrigArgs(argc, argv);
	while (posixPlatformInit() < 0) sleep(1);
	OT_INSTANCE = otInstanceInitSingle();
	otCliUartInit(OT_INSTANCE);

	isRunning = 1;
	signal(SIGINT, quit);

	otIp6SetEnabled(OT_INSTANCE, true);
	registerCoapResources(OT_INSTANCE);

	printf_time("Initialisation complete.\r\n");

	init_sed_eink_commands(OT_INSTANCE);

	while (isRunning)
	{
		otTaskletsProcess(OT_INSTANCE);
		posixPlatformProcessDrivers(OT_INSTANCE);
	}

	return 0;
}
