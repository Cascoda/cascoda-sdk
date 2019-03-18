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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
static otCoapResource sTempResource;
static otCoapResource sDiscoverResource;
static const char *   sTempUri     = "ca/te";
static const char *   sDiscoverUri = "ca/di";

void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
	(void)aBuf;
	(void)aBufLength;
}
void otPlatUartSendDone(void)
{
}

static void handleDiscover(void *               aContext,
                           otCoapHeader *       aHeader,
                           otMessage *          aMessage,
                           const otMessageInfo *aMessageInfo)
{
	otError      error = OT_ERROR_NONE;
	otCoapHeader responseHeader;
	otMessage *  responseMessage = NULL;
	otInstance * OT_INSTANCE     = aContext;

	if (otCoapHeaderGetCode(aHeader) != OT_COAP_CODE_GET)
		return;

	printf("Server received discover from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 0),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 2),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 4),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 6),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 8),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 10),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 12),
	       GETBE16(aMessageInfo->mPeerAddr.mFields.m8 + 14));

	otCoapHeaderInit(&responseHeader, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CONTENT);
	otCoapHeaderSetMessageId(&responseHeader, otCoapHeaderGetMessageId(aHeader));
	otCoapHeaderSetToken(&responseHeader, otCoapHeaderGetToken(aHeader), otCoapHeaderGetTokenLength(aHeader));

	otCoapHeaderSetPayloadMarker(&responseHeader);

	responseMessage = otCoapNewMessage(OT_INSTANCE, &responseHeader);
	if (responseMessage == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	SuccessOrExit(error = otMessageAppend(responseMessage, otThreadGetMeshLocalEid(OT_INSTANCE), sizeof(otIp6Address)));
	SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));

exit:
	if (error != OT_ERROR_NONE && responseMessage != NULL)
	{
		printf("Discover response failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

static void handleTemperature(void *               aContext,
                              otCoapHeader *       aHeader,
                              otMessage *          aMessage,
                              const otMessageInfo *aMessageInfo)
{
	otError      error = OT_ERROR_NONE;
	otCoapHeader responseHeader;
	otMessage *  responseMessage = NULL;
	otInstance * OT_INSTANCE     = aContext;
	uint16_t     length          = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	int32_t      temperature;

	if (otCoapHeaderGetCode(aHeader) != OT_COAP_CODE_POST)
		return;

	if (length != sizeof(temperature))
		return;

	otMessageRead(aMessage, otMessageGetOffset(aMessage), &temperature, sizeof(temperature));

	printf("Server received temperature %d.%d*C from [%x:%x:%x:%x:%x:%x:%x:%x]\r\n",
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

	otCoapHeaderInit(&responseHeader, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_VALID);
	otCoapHeaderSetMessageId(&responseHeader, otCoapHeaderGetMessageId(aHeader));
	otCoapHeaderSetToken(&responseHeader, otCoapHeaderGetToken(aHeader), otCoapHeaderGetTokenLength(aHeader));

	responseMessage = otCoapNewMessage(OT_INSTANCE, &responseHeader);
	if (responseMessage == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	SuccessOrExit(error = otCoapSendResponse(OT_INSTANCE, responseMessage, aMessageInfo));

exit:
	if (error != OT_ERROR_NONE && responseMessage != NULL)
	{
		printf("Temperature ack failed: Error %d: %s\r\n", error, otThreadErrorToString(error));
		otMessageFree(responseMessage);
	}
}

static void registerCoapResources(otInstance *aInstance)
{
	otError error = OT_ERROR_NONE;

	SuccessOrExit(error = otCoapStart(aInstance, OT_DEFAULT_COAP_PORT));

	memset(&sTempResource, 0, sizeof(sTempResource));
	sTempResource.mUriPath = sTempUri;
	sTempResource.mContext = aInstance;
	sTempResource.mHandler = &handleTemperature;

	memset(&sDiscoverResource, 0, sizeof(sDiscoverResource));
	sDiscoverResource.mUriPath = sDiscoverUri;
	sDiscoverResource.mContext = aInstance;
	sDiscoverResource.mHandler = &handleDiscover;

	SuccessOrExit(error = otCoapAddResource(aInstance, &sTempResource));
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
	otThreadSetAutoStart(OT_INSTANCE, true);

	registerCoapResources(OT_INSTANCE);

	while (isRunning)
	{
		otTaskletsProcess(OT_INSTANCE);
		posixPlatformProcessDrivers(OT_INSTANCE);
	}

	return 0;
}
