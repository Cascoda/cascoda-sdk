/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

static int            isRunning;
static otCoapResource sSensorResource;
static otCoapResource sDiscoverResource;
static const char *   sSensorUri   = "ca/se";
static const char *   sDiscoverUri = "ca/di";

void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
	(void)aBuf;
	(void)aBufLength;
}
void otPlatUartSendDone(void)
{
}

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

static void handleDiscover(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage * responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_GET)
		return;

	printf_time("Server received discover from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
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
		printf("Discover response failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

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

	//Allocate a buffer and store the received serialised CBOR message into it
	unsigned char *buffer = malloc(length);
	otMessageRead(aMessage, offset, buffer, length);

	//Initialise the CBOR parser
	SuccessOrExit(cbor_parser_init(buffer, length, 0, &parser, &value));

	//Extract the values corresponding to the keys from the CBOR map
	SuccessOrExit(cbor_value_map_find_value(&value, temp_key, &t_result));
	SuccessOrExit(cbor_value_map_find_value(&value, humidity_key, &h_result));
	SuccessOrExit(cbor_value_map_find_value(&value, counter_key, &c_result));
	SuccessOrExit(cbor_value_map_find_value(&value, light_key, &l_result));

	//Retrieve and print the sensor values
	printf_time("Server received");

	bool first_print = true;

	if (cbor_value_get_type(&t_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&t_result, &temperature));
		printf(" temperature %d.%d*C", (int)(temperature / 10), (int)abs(temperature % 10));
		first_print = false;
	}
	if (cbor_value_get_type(&h_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&h_result, &humidity));
		if (!first_print)
			printf(",");
		printf(" humidity %d%%", (int)humidity);
		first_print = false;
	}
	if (cbor_value_get_type(&c_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int64(&c_result, &pir_counter));
		if (!first_print)
			printf(",");
		printf(" PIR count %d", (int)pir_counter);
		first_print = false;
	}
	if (cbor_value_get_type(&l_result) != CborInvalidType)
	{
		if (!first_print)
			printf(",");
		SuccessOrExit(cbor_value_get_int64(&l_result, &light_level));
		printf(" light level %d", (int)light_level);
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
	//Free buffer
	free(buffer);
	if (error != OT_ERROR_NONE && responseMessage != NULL)
	{
		printf_time("Temperature ack failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

static void registerCoapResources(otInstance *aInstance)
{
	otError error = OT_ERROR_NONE;

	SuccessOrExit(error = otCoapStart(aInstance, OT_DEFAULT_COAP_PORT));

	memset(&sSensorResource, 0, sizeof(sSensorResource));
	sSensorResource.mUriPath = sSensorUri;
	sSensorResource.mContext = aInstance;
	sSensorResource.mHandler = &handleSensorData;

	memset(&sDiscoverResource, 0, sizeof(sDiscoverResource));
	sDiscoverResource.mUriPath = sDiscoverUri;
	sDiscoverResource.mContext = aInstance;
	sDiscoverResource.mHandler = &handleDiscover;

	SuccessOrExit(error = otCoapAddResource(aInstance, &sSensorResource));
	SuccessOrExit(error = otCoapAddResource(aInstance, &sDiscoverResource));

exit:
	assert(error == OT_ERROR_NONE);
	return;
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

	isRunning = 1;
	signal(SIGINT, quit);

	/* Hardcoded demo-specific config */
	otMasterKey key = {0xca, 0x5c, 0x0d, 0xa5, 0x01, 0x07, 0xca, 0x5c, 0x0d, 0xaa, 0xca, 0xfe, 0xbe, 0xef, 0xde, 0xad};
	otIp6SetEnabled(OT_INSTANCE, true);
	otLinkSetPanId(OT_INSTANCE, 0xc0da);
	otThreadSetMasterKey(OT_INSTANCE, &key);
	otLinkSetChannel(OT_INSTANCE, 22);
	otThreadSetEnabled(OT_INSTANCE, true);

	registerCoapResources(OT_INSTANCE);

	printf_time("Initialisation complete.\r\n");

	while (isRunning)
	{
		otTaskletsProcess(OT_INSTANCE);
		posixPlatformProcessDrivers(OT_INSTANCE);
	}

	return 0;
}
