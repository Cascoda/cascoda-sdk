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
static otCoapResource sDiscoverResource;
static otCoapResource sImageResource;
static const char *   sDiscoverUri = "ca/di";
static const char *   sImageUri    = "ca/img";

static char imageBuffer[1024] = {0};

struct connected_device
{
	uint8_t        ip[16];
	struct timeval last_wakeup;
};

#define MAX_CONNECTED_DEVICES_LIST 255
static time_t                  TIMEOUT_S                                     = 600;
static int                     free_device_idx                               = 0;
static struct connected_device connected_devices[MAX_CONNECTED_DEVICES_LIST] = {};

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

	bool           is_connected      = false;
	struct timeval time_now          = {0};
	int            timed_out_devices = 0;
	gettimeofday(&time_now, NULL);

	for (int i = 0; i < free_device_idx; ++i)
	{
		// Check if the device that sent the request has previously connected
		if (memcmp(connected_devices[i].ip, aMessageInfo->mPeerAddr.mFields.m8, 16) == 0)
		{
			is_connected = true;
			// Update the last wakeup time
			connected_devices[i].last_wakeup = time_now;
		}

		// Have any devices timed out?
		time_t last_wakeup_s = connected_devices[i].last_wakeup.tv_sec;

		if ((time_now.tv_sec > last_wakeup_s + TIMEOUT_S) && last_wakeup_s != 0)
		{
			printf_time("[%x:%x:%x:%x:%x:%x:%x:%x] timed out!\r\n",
			            GETBE16(connected_devices[i].ip + 0),
			            GETBE16(connected_devices[i].ip + 2),
			            GETBE16(connected_devices[i].ip + 4),
			            GETBE16(connected_devices[i].ip + 6),
			            GETBE16(connected_devices[i].ip + 8),
			            GETBE16(connected_devices[i].ip + 10),
			            GETBE16(connected_devices[i].ip + 12),
			            GETBE16(connected_devices[i].ip + 14));
			printf_time("Last seen %ds ago.\r\n", time_now.tv_sec - last_wakeup_s);

			timed_out_devices++;
		}
	}

	if (!is_connected)
	{
		memcpy(connected_devices[free_device_idx].ip, aMessageInfo->mPeerAddr.mFields.m8, 16);
		printf_time("New device connected!\r\n");
		free_device_idx++;
	}

	printf_time("Connected devices: %d\r\n", free_device_idx - timed_out_devices);

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

static void handleImageRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	otError     error           = OT_ERROR_NONE;
	otMessage * responseMessage = NULL;
	otInstance *OT_INSTANCE     = aContext;

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

	responseMessage = otCoapNewMessage(OT_INSTANCE, NULL);
	if (responseMessage == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	// Find the URI Query option
	const otCoapOption *option;
	char                filename[255] = {0};

	for (option = otCoapMessageGetFirstOption(aMessage); option != NULL; option = otCoapMessageGetNextOption(aMessage))
	{
		if (option->mNumber == OT_COAP_OPTION_URI_QUERY)
		{
			char uri_query[255] = {0};
			SuccessOrExit(otCoapMessageGetOptionValue(aMessage, uri_query));

			// URI query is of the form "id=001.gz". Check first three characters
			// to ensure the query key is what we expect
			if (memcmp(uri_query, "id=", 3) == 0)
			{
				// Copy from the third character of the query
				// id=001.gz
				//    ^ this one onwards
				strcpy(filename, (uri_query + 3));
			}
		}
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
		goto exit;
	}

	fclose(fin);

	// CoAP header
	otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_CONTENT);
	otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

	otCoapMessageSetPayloadMarker(responseMessage);

	// CoAP Payload: contents of the image buffer
	SuccessOrExit(error = otMessageAppend(responseMessage, imageBuffer, filesize));
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

	while (isRunning)
	{
		otTaskletsProcess(OT_INSTANCE);
		posixPlatformProcessDrivers(OT_INSTANCE);
	}

	return 0;
}
