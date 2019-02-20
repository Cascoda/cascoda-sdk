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

#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"

#include "openthread/instance.h"
#include "openthread/platform/logging.h"

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
	va_list ap;
	u8_t *  pLevel = "??? ", *pRegion = "??? ";

	switch (aLogLevel)
	{
	case OT_LOG_LEVEL_NONE:
		pLevel = "NONE ";
		break;

	case OT_LOG_LEVEL_CRIT:
		pLevel = "CRIT ";
		break;

	case OT_LOG_LEVEL_WARN:
		pLevel = "WARN ";
		break;

	case OT_LOG_LEVEL_INFO:
		pLevel = "INFO ";
		break;

	case OT_LOG_LEVEL_DEBG:
		pLevel = "DEBG ";
		break;
	}

	switch (aLogRegion)
	{
	case OT_LOG_REGION_API:
		pRegion = "API  ";
		break;

	case OT_LOG_REGION_MLE:
		pRegion = "MLE  ";
		break;

	case OT_LOG_REGION_ARP:
		pRegion = "ARP  ";
		break;

	case OT_LOG_REGION_NET_DATA:
		pRegion = "NETD ";
		break;

	case OT_LOG_REGION_IP6:
		pRegion = "IPV6 ";
		break;

	case OT_LOG_REGION_ICMP:
		pRegion = "ICMP ";
		break;

	case OT_LOG_REGION_MAC:
		pRegion = "MAC  ";
		break;

	case OT_LOG_REGION_MEM:
		pRegion = "MEM  ";
		break;

	case OT_LOG_REGION_NCP:
		pRegion = "NCP  ";
		break;

	case OT_LOG_REGION_MESH_COP:
		pRegion = "MCOP ";
		break;

	case OT_LOG_REGION_NET_DIAG:
		pRegion = "DIAG ";
		break;

	default:
		pRegion = "XXXX ";
		break;
	}
	printf("%02d:%02d:%02d.%03d %s %s ", Hours, Minutes, Seconds, Ticks, pLevel, pRegion);

	va_start(ap, aFormat);
	vprintf(aFormat, ap);
	va_end(ap);
	printf("\n");
}
