/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  Modifications Copyright (c) 2020, Cascoda Ltd.
 *
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

/**
 * @file
 *   This file implements lwip net interface with OpenThread.
 */

#include <assert.h>
#include <string.h>

#include <openthread/config.h>
#include <openthread/icmp6.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/message.h>
#include <openthread/platform/radio.h>
#include <openthread/thread.h>

#include "cascoda-util/cascoda_time.h"
#include "ca821x_log.h"

#include "lwip/dns.h"
#include "lwip/mld6.h"
#include "lwip/netif.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"

#include "lwip-port.h"

#define VerifyOrExit(aCondition, ...) \
	do                                \
	{                                 \
		if (!(aCondition))            \
		{                             \
			__VA_ARGS__;              \
			goto exit;                \
		}                             \
	} while (false)

/**
 * This structure stores information needed to send a IPv6 packet.
 *
 */
struct OutputEvent
{
	struct OutputEvent *mNext;
	uint16_t            mLength;
	uint8_t             mData[1];
};

static struct netif       sNetif;
static struct otInstance *sInstance = NULL;

static bool IsLinkLocal(const struct otIp6Address *aAddress)
{
	return aAddress->mFields.m16[0] == htons(0xfe80);
}

static err_t netifOutputIp6(struct netif *aNetif, struct pbuf *aBuffer, const ip6_addr_t *aPeerAddr)
{
	(void)aPeerAddr;

	err_t        err     = ERR_OK;
	otError      error   = OT_ERROR_NONE;
	otMessage *  message = NULL;
	struct pbuf *curBuf  = NULL;

	ca_log_info("netif output");
	assert(aNetif == &sNetif);

	message = otIp6NewMessage(sInstance, NULL);
	VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

	while (aBuffer)
	{
		error   = otMessageAppend(message, aBuffer->payload, aBuffer->len);
		aBuffer = aBuffer->next;
		if (error)
			goto exit;
	}

	error   = otIp6Send(sInstance, message);
	message = NULL;

exit:
	if (error != OT_ERROR_NONE)
	{
		if (message != NULL)
		{
			otMessageFree(message);
		}

		ca_log_warn("Failed to transmit IPv6 packet: %s", otThreadErrorToString(error));
	}

	return err;
}

static err_t netifInit(struct netif *aNetif)
{
	aNetif->name[0]    = 'o';
	aNetif->name[1]    = 't';
	aNetif->hwaddr_len = sizeof(otExtAddress);
	memset(aNetif->hwaddr, 0, sizeof(aNetif->hwaddr));
	aNetif->mtu   = 1280;
	aNetif->flags = NETIF_FLAG_BROADCAST;
#if LWIP_IPV4
	aNetif->output = NULL;
#endif
#if LWIP_IPV6
	aNetif->output_ip6 = netifOutputIp6;
#endif
	aNetif->num = 0;

	return ERR_OK;
}

struct netif *otrGetNetif(void)
{
	return &sNetif;
}

static void addMulticastAddress(const otIp6Address *aAddress)
{
#if LWIP_IPV6_MLD
	ip6_addr_t multicast_addr;

	memcpy(&multicast_addr.addr, aAddress, sizeof(*aAddress));
#if LWIP_IPV6_SCOPES
	multicast_addr.zone = IP6_NO_ZONE;
#endif
	mld6_joingroup_netif(&sNetif, &multicast_addr);
#else
	(void)aAddress;
#endif
}

static void addAddress(const struct otIp6Address *aAddress)
{
	otError error = OT_ERROR_NONE;
	err_t   err   = ERR_OK;

	if (IsLinkLocal(aAddress))
	{
		netif_ip6_addr_set(&sNetif, 0, (const ip6_addr_t *)(aAddress));
		netif_ip6_addr_set_state(&sNetif, 0, IP6_ADDR_PREFERRED);
	}
	else
	{
		int8_t                   index  = -1;
		const otMeshLocalPrefix *prefix = otThreadGetMeshLocalPrefix(sInstance);

		err = netif_add_ip6_address(&sNetif, (const ip6_addr_t *)aAddress, &index);
		VerifyOrExit(err == ERR_OK && index != -1, error = OT_ERROR_FAILED);
		if (memcmp(aAddress, prefix, sizeof(prefix->m8)) != 0)
		{
			netif_ip6_addr_set_state(&sNetif, index, IP6_ADDR_PREFERRED);
		}
		else
		{
			netif_ip6_addr_set_state(&sNetif, index, IP6_ADDR_VALID);
		}
	}

exit:
	if (error != OT_ERROR_NONE)
	{
		ca_log_info("Failed to add address: %d", error);
	}
}

static void delMulticastAddress(const otIp6Address *aAddress)
{
#if LWIP_IPV6_MLD
	ip6_addr_t multicast_addr;

	memcpy(&multicast_addr.addr, aAddress, sizeof(*aAddress));
#if LWIP_IPV6_SCOPES
	multicast_addr.zone = IP6_NO_ZONE;
#endif
	mld6_leavegroup_netif(&sNetif, &multicast_addr);
#else
	(void)aAddress;
#endif
}

static void delAddress(const otIp6Address *aAddress)
{
	int8_t index;

	index = netif_get_ip6_addr_match(&sNetif, (const ip6_addr_t *)aAddress);
	if (index != -1)
		netif_ip6_addr_set_state(&sNetif, index, IP6_ADDR_INVALID);
}

static void setupDns(void)
{
	ip_addr_t dnsServer = {};

	otIp6AddressFromString("64:ff9b::808:808", (otIp6Address *)(&dnsServer));
#if LWIP_IPV4
	dnsServer.type = IPADDR_TYPE_V6;
#endif

	dns_init();
	dns_setserver(0, &dnsServer);
}

static void processStateChange(otChangedFlags aFlags, void *aContext)
{
	otInstance *instance = aContext;

	if (OT_CHANGED_THREAD_NETIF_STATE | aFlags)
	{
		if (otLinkIsEnabled(instance))
		{
			ca_log_info("netif up");
			netif_set_up(&sNetif);
		}
		else
		{
			ca_log_info("netif down");
			netif_set_down(&sNetif);
		}
	}
}

static void processAddress(const otIp6Address *aAddress, uint8_t aPrefixLength, bool aIsAdded, void *aContext)
{
	(void)aContext;

	// All multicast addresses have prefix ff00::/8
	bool isMulticast = (aAddress->mFields.m8[0] == 0xff);

	ca_log_info("address changed");

	if (aIsAdded)
	{
		if (isMulticast)
		{
			addMulticastAddress(aAddress);
		}
		else
		{
			addAddress(aAddress);
		}
	}
	else
	{
		if (isMulticast)
		{
			delMulticastAddress(aAddress);
		}
		else
		{
			delAddress(aAddress);
		}
	}
}

static void processReceive(otMessage *aMessage, void *aContext)
{
	otError      error     = OT_ERROR_NONE;
	err_t        err       = ERR_OK;
	uint16_t     length    = otMessageGetLength(aMessage);
	uint16_t     lengthRem = length;
	struct pbuf *buffer    = NULL;
	otInstance * aInstance = aContext;

	assert(sNetif.state == aInstance);

	buffer = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);

	VerifyOrExit(buffer != NULL, error = OT_ERROR_NO_BUFS);

	for (struct pbuf *curBuf = buffer; lengthRem && curBuf; curBuf = curBuf->next)
	{
		uint16_t copyLen = LWIP_MIN(lengthRem, curBuf->len);
		VerifyOrExit(copyLen == otMessageRead(aMessage, length - lengthRem, curBuf->payload, copyLen),
		             error = OT_ERROR_ABORT);
		lengthRem -= copyLen;
	}

	VerifyOrExit(ERR_OK == sNetif.input(buffer, &sNetif), error = OT_ERROR_FAILED);

exit:
	if (error != OT_ERROR_NONE)
	{
		if (buffer != NULL)
		{
			pbuf_free(buffer);
		}

		if (error == OT_ERROR_FAILED)
		{
			ca_log_warn("%s failed for lwip error %d", __func__, err);
		}

		ca_log_warn("%s failed: %s", __func__, otThreadErrorToString(error));
	}

	otMessageFree(aMessage);
}

u32_t sys_now()
{
	return TIME_ReadAbsoluteTime();
}

void LWIP_NetifInit(struct otInstance *aInstance)
{
	sInstance = aInstance;

	memset(&sNetif, 0, sizeof(sNetif));

#if LWIP_IPV4
	netif_add(&sNetif, NULL, NULL, NULL, sInstance, netifInit, netif_input);
#else
	netif_add(&sNetif, sInstance, netifInit, netif_input);
#endif
	netif_set_link_up(&sNetif);

	ca_log_info("Initialize netif");

	otIp6SetAddressCallback(sInstance, processAddress, sInstance);
	otIp6SetReceiveCallback(sInstance, processReceive, sInstance);
	otSetStateChangedCallback(sInstance, processStateChange, sInstance);
	otIp6SetReceiveFilterEnabled(sInstance, true);
	otIcmp6SetEchoMode(sInstance, OT_ICMP6_ECHO_HANDLER_DISABLED);

	netif_set_default(&sNetif);

	setupDns();
}
