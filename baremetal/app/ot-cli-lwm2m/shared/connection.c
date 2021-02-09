/*******************************************************************************
 *
 * Copyright (c) 2021, Cascoda Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Distribution License v1.0
 * which accompanies this distribution.
 *
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ciaran Woodward, Cascoda Ltd - Initial implementation for Cascoda SDK and OpenThread
 *
 *******************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ca-ot-util/cascoda_dns.h"
#include "openthread/dns.h"
#include "openthread/thread.h"
#include "ca821x_log.h"

#include "connection.h"
#include "object_security.h"

static connection_t conpool[CONPOOL_SIZE] = {};
static uint8_t      pkt_buf[LINK_MTU];

static connection_t *GetFreeCon()
{
	for (int i = 0; i < CONPOOL_SIZE; i++)
	{
		if (!conpool[i].inUse)
		{
			connection_t *rval = conpool + i;
			memset(rval, 0, sizeof(*rval));
			return conpool + i;
		}
	}
	return NULL;
}

static connection_t *connection_new_incoming(otInstance *aInstance, lwm2m_context_t *lwm2mH)
{
	connection_t *newConn = GetFreeCon();

	if (!newConn)
		goto exit;

	newConn->lwm2mH     = lwm2mH;
	newConn->inUse      = true;
	newConn->otInstance = aInstance;

exit:
	return newConn;
}

static void connection_udp_receive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	connection_t *connection = aContext;
	uint16_t      len;

	len = otMessageRead(aMessage, 0, pkt_buf, sizeof(pkt_buf));

	if (len)
	{
		lwm2m_handle_packet(connection->lwm2mH, pkt_buf, len, connection);
		connection_rx_info_callback(len);
	}
}

static void connection_dns_callback(ca_error aError, const otIp6Address *aAddress, dns_index aIndex, void *aContext)
{
	connection_t *connection = aContext;

	if (aError == CA_ERROR_SUCCESS)
	{
		otSockAddr sa = {};
		memcpy(&(sa.mAddress), aAddress, sizeof(*aAddress));
		sa.mPort = connection->socket.mPeerName.mPort;

		aError = otUdpOpen(connection->otInstance, &(connection->socket), &connection_udp_receive, connection);
		if (!aError)
			aError = otUdpBind(&(connection->socket), &(connection->socket.mSockName));
		if (!aError)
			aError = otUdpConnect(&(connection->socket), &sa);
		//TODO: Signal connection complete to lwm2m layer
	}

	if (aError)
	{
		ca_log_crit("Failed to connect to server, error %s", ca_error_str(aError));
		// We don't close the connection now, but allow the core stack to fail and clean up
	}
}

ca_error split_hostname(char *uri, char **hostp, char **portp)
{
	ca_error error = CA_ERROR_INVALID_ARGS;
	char *   host;
	char *   port;

	// parse uri in the form "coaps://[host]:[port]"
	if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
	{
		host = uri + strlen("coaps://");
	}
	else if (0 == strncmp(uri, "coap://", strlen("coap://")))
	{
		host = uri + strlen("coap://");
	}
	else
	{
		goto exit;
	}
	port = strrchr(host, ':');
	if (port == NULL)
		goto exit;
	// remove brackets
	if (host[0] == '[')
	{
		host++;
		if (*(port - 1) == ']')
		{
			*(port - 1) = 0;
		}
		else
			goto exit;
	}
	// split strings
	*port = 0;
	port++;

	error  = CA_ERROR_SUCCESS;
	*portp = port;
	*hostp = host;
exit:
	return error;
}

connection_t *connection_create(otInstance *     aInstance,
                                lwm2m_context_t *lwm2mH,
                                lwm2m_object_t * secObjectP,
                                uint16_t         secObjInstID)
{
	connection_t *connection = connection_new_incoming(aInstance, lwm2mH);
	const char *  const_uri;
	char *        uri = NULL;
	char *        host;
	char *        port;
	int           porti = 0;
	ca_error      error;

	if (!connection)
		return NULL;

	const_uri = security_get_server_uri(secObjectP, secObjInstID);
	if (const_uri == NULL)
		return NULL;
	uri = strdup(const_uri);
	if (uri == NULL)
		return NULL;

	error = split_hostname(uri, &host, &port);

	if (error)
		goto exit;

	porti = atoi(port);

	if (porti <= 0 || porti > UINT16_MAX)
		goto exit;

	ca_log_info("Opening connection to server at %s:%s\r\n", host, port);
	//TODO: First message sent to server will fail while DNS is resolving. Find a way to ignore those failures.
	//This is due to the wakaama stack expecting this function to be blocking, but we resolve in nonblocking way
	//and ignore the lwm2m_buffer_send calls while the resolve is in progress.
	connection->socket.mPeerName.mPort = (uint16_t)porti;
	error                              = DNS_HostToIpv6(aInstance, host, &connection_dns_callback, connection);

exit:
	if (error)
	{
		connection_free(connection);
		connection = NULL;
	}
	free(uri);

	return connection;
}

void connection_free(connection_t *con)
{
	otUdpClose(&con->socket);
	con->inUse = false;
}

static ca_error connection_send(connection_t *connP, uint8_t *buffer, size_t length)
{
	otMessageInfo messageInfo;
	otMessage *   msg = otUdpNewMessage(connP->otInstance, NULL);
	otError       err;

	if (length > LINK_MTU)
	{
		ca_log_warn("Failed to transmit, message too big");
		return CA_ERROR_INVALID_ARGS;
	}

	memset(&messageInfo, 0, sizeof(messageInfo));

	err = otMessageAppend(msg, buffer, (uint16_t)length);
	if (err)
		goto exit;

	err = otUdpSend(&(connP->socket), msg, &messageInfo);
	if (err)
		goto exit;

	connection_tx_info_callback((uint16_t)length);

exit:
	if (err)
	{
		otMessageFree(msg);
		return CA_ERROR_FAIL;
	}
	return CA_ERROR_SUCCESS;
}

uint8_t lwm2m_buffer_send(void *sessionH, uint8_t *buffer, size_t length, void *userdata)
{
	connection_t *connP     = (connection_t *)sessionH;
	uint32_t      zeroCheck = 0;

	(void)userdata; /* unused */

	if (connP == NULL)
	{
		ca_log_warn("#> failed sending %lu bytes, missing connection\r\n", length);
		return COAP_500_INTERNAL_SERVER_ERROR;
	}

	for (int i = 0; i < (OT_IP6_ADDRESS_SIZE / sizeof(uint32_t)); i++)
	{
		zeroCheck |= connP->socket.mPeerName.mAddress.mFields.m32[i];
	}

	if (!zeroCheck)
	{
		ca_log_info("#> Server address not configured, dropping packet\r\n", length);
		return COAP_500_INTERNAL_SERVER_ERROR;
	}

	if (connection_send(connP, buffer, length))
	{
		ca_log_warn("#> failed sending %lu bytes\r\n", length);
		return COAP_500_INTERNAL_SERVER_ERROR;
	}

	return COAP_NO_ERROR;
}

bool lwm2m_session_is_equal(void *session1, void *session2, void *userData)
{
	(void)userData; /* unused */

	return (session1 == session2);
}
