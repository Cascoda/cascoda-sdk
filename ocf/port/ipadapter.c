/*
// Copyright 2018 Oleksandr Grytsov
// Modifications copyright (c) 2020, Cascoda Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "port/oc_log.h"
#include "oc_buffer.h"
#include "oc_endpoint.h"

#include <assert.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/random_noncrypto.h>
#include <openthread/thread.h>
#include <openthread/udp.h>

extern otInstance *OT_INSTANCE;

static otUdpSocket unicast_socket;
static otSockAddr  unicast_addr = {};
static otUdpSocket multicast_socket;
static otSockAddr  multicast_addr = {};
static otUdpSocket secured_socket;
static otSockAddr  secured_addr = {};

#define OCF_MCAST_PORT_UNSECURED (5683)
uint32_t OCF_SERVER_PORT_UNSECURED;
uint32_t OCF_SERVER_PORT_SECURED;

static oc_endpoint_t *eps;

static void udp_receive_cbk(void *context, otMessage *ot_message, const otMessageInfo *ot_message_info)
{
	(void)context;

	OC_DBG("Receive udp cbk");

	oc_message_t *oc_message = oc_allocate_message();

	if (oc_message)
	{
		uint16_t payloadLength = otMessageGetLength(ot_message) - otMessageGetOffset(ot_message);
		if (otMessageRead(ot_message, otMessageGetOffset(ot_message), oc_message->data, payloadLength) != payloadLength)
		{
			OC_ERR("Can't read message");
			return;
		}
		oc_message->length         = payloadLength;
		oc_message->endpoint.flags = IPV6;
		memcpy(oc_message->endpoint.addr.ipv6.address, ot_message_info->mPeerAddr.mFields.m8, OT_IP6_ADDRESS_SIZE);
		oc_message->endpoint.addr.ipv6.port = ot_message_info->mPeerPort;

#ifdef OC_SECURITY
		if (ot_message_info->mSockPort == OCF_SERVER_PORT_SECURED)
		{
			oc_message->endpoint.flags |= SECURED;
			oc_message->encrypted = 1;
		}
#endif
		if (ot_message_info->mSockPort == OCF_MCAST_PORT_UNSECURED)
		{
			oc_message->endpoint.flags |= MULTICAST;
		}

#ifdef OC_DEBUG
		PRINT("Incoming message from ");
		PRINTipaddr(oc_message->endpoint);
		PRINT("\n\n");
		PRINT("Peer Port: %d\n", ot_message_info->mPeerPort);
		PRINT("Sock Port: %d\n", ot_message_info->mSockPort);
#endif /* OC_DEBUG */

		oc_network_event(oc_message);
	}
}

static void free_endpoints(void)
{
	oc_endpoint_t *ep = eps, *next;
	while (ep != NULL)
	{
		next = ep->next;
		oc_free_endpoint(ep);
		ep = next;
	}
}

uint32_t get_scope(const otNetifAddress *address)
{
	// Scope of the current address
	uint32_t scope;
	// If the address is a routing locator, we already have a mesh-local address
	// that stays constant
	if (address->mRloc)
	{
		scope = 0;
	}
	else if (address->mScopeOverrideValid)
	{
		scope = address->mScopeOverride;
	}
	else
	{
		// Do not have a scope override - determine the scope 'manually'

		// multicast
		if (address->mAddress.mFields.m8[0] == 0xff)
		{
			scope = address->mAddress.mFields.m8[1] & 0xf;
		}
		// link-local
		else if ((address->mAddress.mFields.m8[0] == 0xfe) && ((address->mAddress.mFields.m8[1] & 0xc0) == 0x80))
		{
			scope = 2;
		}
		// loopback
		else if (address->mAddress.mFields.m32[0] == 0 && address->mAddress.mFields.m32[1] == 0 &&
		         address->mAddress.mFields.m32[2] == 0 && address->mAddress.mFields.m8[12] == 0 &&
		         address->mAddress.mFields.m8[13] == 0 && address->mAddress.mFields.m8[14] == 0 &&
		         address->mAddress.mFields.m8[15] == 1)
		{
			scope = 0;
		}
		// global
		else
		{
			scope = 14;
		}
	}
	return scope;
}

oc_endpoint_t *oc_connectivity_get_endpoints(size_t device)
{
	(void)device;
	const otNetifAddress *address;
	const otNetifAddress *best_address;
	uint32_t              best_scope = 0;
	oc_endpoint_t *       ep;

	// We want our endpoints to use the latest IP addresses from OpenThread.
	// Therefore, we must first free the list of endpoints.
	free_endpoints();

	address = otIp6GetUnicastAddresses(OT_INSTANCE);

	OC_DBG("Finding the best endpoint address...\n");
	// Search for the address with the highest scope
	while (address)
	{
		uint32_t scope = get_scope(address);

		if (scope > best_scope)
		{
			best_address = address;
			best_scope   = scope;
		}
		address = address->mNext;
	}
	ep = oc_new_endpoint();
	// No more memory left for endpoints, so return the list.
	if (!ep)
	{
		return eps;
	}
	// Save the head of the list, to be cleared when getting the endpoints once more.
	eps = ep;

	// Populate the contents of the endpoint.
	ep->flags = IPV6;
	memcpy(ep->addr.ipv6.address, best_address->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);
	ep->addr.ipv6.port = OCF_SERVER_PORT_UNSECURED;
	ep->device         = 0;

	OC_DBG("Endpoints: \n", NULL);
	OC_LOGipaddr(*ep);
#ifdef OC_SECURITY
	// Add a secured endpoint at the same address.
	oc_endpoint_t *ep_secure = oc_new_endpoint();
	if (!ep_secure)
	{
		return eps;
	}
	ep_secure->flags = IPV6 | SECURED;
	memcpy(ep_secure->addr.ipv6.address, best_address->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);
	ep_secure->addr.ipv6.port = OCF_SERVER_PORT_SECURED;
	ep_secure->device         = 0;
	OC_LOGipaddr(*ep_secure);

	// Update the endpoint linked lists
	ep->next = ep_secure;
#else
	ep->next = NULL;
#endif

	return eps;
}

int oc_send_buffer(oc_message_t *message)
{
	otMessageSettings settings   = {true, OT_MESSAGE_PRIORITY_NORMAL};
	otMessage *       ot_message = otUdpNewMessage(OT_INSTANCE, &settings);
	otUdpSocket *     socket;

	if (!ot_message)
	{
		OC_ERR("No more buffer to send");
		return -1;
	}

	if (otMessageAppend(ot_message, message->data, message->length) != OT_ERROR_NONE)
	{
		OC_ERR("Can't append message");
		return -1;
	}

	otMessageInfo message_info;

	memset(&message_info, 0, sizeof(otMessageInfo));

	memcpy(&message_info.mPeerAddr.mFields, message->endpoint.addr.ipv6.address, OT_IP6_ADDRESS_SIZE);
	message_info.mPeerPort = message->endpoint.addr.ipv6.port;

#ifdef OC_DEBUG
	PRINT("Outgoing message to ");
	PRINTipaddr(message->endpoint);
	PRINT("\n\n");
#endif /* OC_DEBUG */

	if (message->endpoint.flags & SECURED)
		socket = &secured_socket;
	else
		socket = &unicast_socket;

	if (otUdpSend(socket, ot_message, &message_info) != OT_ERROR_NONE)
	{
		OC_ERR("Can't send message");
		return -1;
	}
	return message->length;
}

int subscribe_mcast(void)
{
	otIp6Address maddr;
	otError      error;

	// Site-local address
	error = otIp6AddressFromString("ff05::158", &maddr);
	// Can't convert mcast address
	assert(error == OT_ERROR_NONE);

	error = otIp6SubscribeMulticastAddress(OT_INSTANCE, &maddr);
	// Can't subscribe mcast address
	assert(error == OT_ERROR_NONE);

	// Link-local address
	error = otIp6AddressFromString("ff02::158", &maddr);
	// Can't convert mcast address
	assert(error == OT_ERROR_NONE);

	error = otIp6SubscribeMulticastAddress(OT_INSTANCE, &maddr);
	// Can't subscribe mcast address
	assert(error == OT_ERROR_NONE);

	// Realm-local address
	error = otIp6AddressFromString("ff03::158", &maddr);
	// Can't convert mcast address
	assert(error == OT_ERROR_NONE);

	error = otIp6SubscribeMulticastAddress(OT_INSTANCE, &maddr);
	// Can't subscribe mcast address
	assert(error == OT_ERROR_NONE);

	return 0;
}

int oc_connectivity_init(size_t device)
{
	(void)device;
	OC_DBG("Connectivity init");

	if (subscribe_mcast())
		return -1;

	if (otUdpOpen(OT_INSTANCE, &unicast_socket, udp_receive_cbk, NULL) != OT_ERROR_NONE)
	{
		OC_ERR("Can't open unicast socket");
		return -1;
	}

	unicast_addr.mPort = 0;

	if (otUdpBind(&unicast_socket, &unicast_addr) != OT_ERROR_NONE)
	{
		OC_ERR("Can't bind unicast port");
		return -1;
	}
	OCF_SERVER_PORT_UNSECURED = unicast_socket.mSockName.mPort;

	if (otUdpOpen(OT_INSTANCE, &multicast_socket, udp_receive_cbk, NULL) != OT_ERROR_NONE)
	{
		OC_ERR("Can't open multicast socket");
		return -1;
	}

	multicast_addr.mPort = OCF_MCAST_PORT_UNSECURED;

	if (otUdpBind(&multicast_socket, &multicast_addr) != OT_ERROR_NONE)
	{
		OC_ERR("Can't bind multicast port");
		return -1;
	}

	if (otUdpOpen(OT_INSTANCE, &secured_socket, udp_receive_cbk, NULL) != OT_ERROR_NONE)
	{
		OC_ERR("Can't open secured socket");
		return -1;
	}

	secured_addr.mPort = 0;

	if (otUdpBind(&secured_socket, &secured_addr) != OT_ERROR_NONE)
	{
		OC_ERR("Can't bind secured port");
		return -1;
	}

	OCF_SERVER_PORT_SECURED = secured_socket.mSockName.mPort;

	return 0;
}

void oc_connectivity_shutdown(size_t device)
{
	(void)device;

	OC_DBG("Connectivity shutdown: %d", device);

	otIp6SetEnabled(OT_INSTANCE, false);
	free_endpoints();
}

#ifdef OC_CLIENT
void oc_send_discovery_request(oc_message_t *message)
{
	OC_DBG("Send discovery request");

	oc_send_buffer(message);
}
#endif /* OC_CLIENT */

/*
 * oc_network_event_handler_mutex_* are defined only to comply with the
 * connectivity interface, but are not used since the adapter process does
 * not preempt the process running the event loop.
*/
void oc_network_event_handler_mutex_init(void)
{
}

void oc_network_event_handler_mutex_lock(void)
{
}

void oc_network_event_handler_mutex_unlock(void)
{
}

void oc_network_event_handler_mutex_destroy(void)
{
}
