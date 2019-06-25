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

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "openthread/platform/logging.h"
#include "openthread/platform/toolchain.h"

#include "code_utils.h"
#include "openthread-core-config.h"

// Macro to append content to end of the log string.

#define LOG_PRINTF(...)                                                                   \
	charsWritten = snprintf(&logString[offset], sizeof(logString) - offset, __VA_ARGS__); \
	otEXPECT_ACTION(charsWritten >= 0, logString[offset] = 0);                            \
	offset += (unsigned int)charsWritten;                                                 \
	otEXPECT_ACTION(offset < sizeof(logString), logString[sizeof(logString) - 1] = 0)

OT_TOOL_WEAK void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
	struct timeval tv;
	char           timeString[40];
	char           logString[512];
	unsigned int   offset;
	int            charsWritten;
	va_list        args;

	offset = 0;

	gettimeofday(&tv, NULL);
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));

	LOG_PRINTF("%s.%06d ", timeString, (uint32_t)tv.tv_usec);

	switch (aLogLevel)
	{
	case OT_LOG_LEVEL_NONE:
		LOG_PRINTF("NONE ");
		break;

	case OT_LOG_LEVEL_CRIT:
		LOG_PRINTF("CRIT ");
		break;

	case OT_LOG_LEVEL_WARN:
		LOG_PRINTF("WARN ");
		break;

	case OT_LOG_LEVEL_INFO:
		LOG_PRINTF("INFO ");
		break;

	case OT_LOG_LEVEL_DEBG:
		LOG_PRINTF("DEBG ");
		break;
	}

	switch (aLogRegion)
	{
	case OT_LOG_REGION_API:
		LOG_PRINTF("API  ");
		break;

	case OT_LOG_REGION_MLE:
		LOG_PRINTF("MLE  ");
		break;

	case OT_LOG_REGION_ARP:
		LOG_PRINTF("ARP  ");
		break;

	case OT_LOG_REGION_NET_DATA:
		LOG_PRINTF("NETD ");
		break;

	case OT_LOG_REGION_IP6:
		LOG_PRINTF("IPV6 ");
		break;

	case OT_LOG_REGION_ICMP:
		LOG_PRINTF("ICMP ");
		break;

	case OT_LOG_REGION_MAC:
		LOG_PRINTF("MAC  ");
		break;

	case OT_LOG_REGION_MEM:
		LOG_PRINTF("MEM  ");
		break;

	case OT_LOG_REGION_NCP:
		LOG_PRINTF("NCP  ");
		break;

	case OT_LOG_REGION_COAP:
		LOG_PRINTF("COAP ");
		break;

	case OT_LOG_REGION_MESH_COP:
		LOG_PRINTF("MCOP ");
		break;

	case OT_LOG_REGION_NET_DIAG:
		LOG_PRINTF("NDG  ");
		break;

	case OT_LOG_REGION_CLI:
		LOG_PRINTF("CLI  ");
		break;

	case OT_LOG_REGION_PLATFORM:
		LOG_PRINTF("PLAT ");
		break;

	case OT_LOG_REGION_CORE:
		LOG_PRINTF("CORE ");
		break;

	case OT_LOG_REGION_UTIL:
		LOG_PRINTF("UTIL ");
		break;
	}

	va_start(args, aFormat);
	charsWritten = vsnprintf(&logString[offset], sizeof(logString) - offset, aFormat, args);
	va_end(args);

	otEXPECT_ACTION(charsWritten >= 0, logString[offset] = 0);

exit:
	fprintf(stderr, "%s\r\n", logString);
}
