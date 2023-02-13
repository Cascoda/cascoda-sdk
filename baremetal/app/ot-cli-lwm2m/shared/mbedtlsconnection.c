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
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "openthread/dns.h"
#include "openthread/thread.h"
#include "ca821x_log.h"

#include "mbedtlsconnection.h"
#include "object_security.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"

static connection_t conpool[CONPOOL_SIZE] = {}; //!< Pool of connection data structs
static otMessage   *rxMessage; //!< Receive message pointer, only used to pass received message to callback.

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

static ca_error mbedtls_tasklet_callback(void *context)
{
	connection_t *connection = context;

	connection->isTimerPassed = true;

	mbedtls_ssl_handshake(&connection->ssl);
}

static connection_t *connection_new_incoming(otInstance *aInstance, lwm2m_context_t *lwm2mH)
{
	connection_t *newConn = GetFreeCon();

	if (!newConn)
		goto exit;

	newConn->lwm2mH     = lwm2mH;
	newConn->inUse      = true;
	newConn->otInstance = aInstance;
	TASKLET_Init(&newConn->mbedtls_tasklet, &mbedtls_tasklet_callback);

exit:
	return newConn;
}

static void connection_udp_receive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	static uint8_t pkt_buf[LINK_MTU];

	connection_t *connection = aContext;

	if (connection->isSecure)
	{
		int rval = 0;

		rxMessage = aMessage;
		do
		{
			//TODO: There is more logic that can be done with the return value here, to optimise
			rval = mbedtls_ssl_read(&connection->ssl, pkt_buf, sizeof(pkt_buf));
			if (rval > 0)
			{
				lwm2m_handle_packet(connection->lwm2mH, pkt_buf, rval, connection);
				connection_rx_info_callback(rval);
			}
		} while (rval > 0);
		rxMessage = NULL;
	}
	else
	{
		uint16_t len;

		len = otMessageRead(aMessage, 0, pkt_buf, sizeof(pkt_buf));
		if (len)
		{
			lwm2m_handle_packet(connection->lwm2mH, pkt_buf, len, connection);
			connection_rx_info_callback(len);
		}
	}
}

/**
 * \brief          Callback type: set a pair of timers/delays to watch
 *
 * \param ctx      Context pointer
 * \param int_ms   Intermediate delay in milliseconds
 * \param fin_ms   Final delay in milliseconds
 *                 0 cancels the current timer.
 *
 * \note           This callback must at least store the necessary information
 *                 for the associated \c mbedtls_ssl_get_timer_t callback to
 *                 return correct information.
 *
 * \note           If using a event-driven style of programming, an event must
 *                 be generated when the final delay is passed. The event must
 *                 cause a call to \c mbedtls_ssl_handshake() with the proper
 *                 SSL context to be scheduled. Care must be taken to ensure
 *                 that at most one such call happens at a time.
 *
 * \note           Only one timer at a time must be running. Calling this
 *                 function while a timer is running must cancel it. Cancelled
 *                 timers must not generate any event.
 */
static void mbedtls_set_timer(void *ctx, uint32_t int_ms, uint32_t fin_ms)
{
	connection_t *connection = ctx;
	uint32_t      curTime;

	TASKLET_Cancel(&connection->mbedtls_tasklet);

	if (fin_ms == 0)
		return;

	curTime = TIME_ReadAbsoluteTime();

	connection->intermediateTime = curTime + int_ms;
	TASKLET_ScheduleAbs(&connection->mbedtls_tasklet, curTime, curTime + fin_ms, connection);
}

/**
 * MbedTLS callback: get status of timer
 *
 * @param ctx      Context pointer
 *
 * @return         This callback must return:
 *                 -1 if cancelled (fin_ms == 0),
 *                  0 if none of the delays have passed,
 *                  1 if only the intermediate delay has passed,
 *                  2 if the final delay has passed.
 */
static int mbedtls_get_timer(void *ctx)
{
	connection_t *connection = ctx;

	if (!TASKLET_IsQueued(&connection->mbedtls_tasklet))
	{
		if (connection->isTimerPassed)
			return 2;
		else
			return -1;
	}
	if (TIME_Cmp(TIME_ReadAbsoluteTime(), connection->intermediateTime) < 0)
		return 0;
	return 1;
}

/**
 * \brief          Callback type: send data on the network.
 *
 * \param ctx      Context for the send callback
 * \param buf      Buffer holding the data to send
 * \param len      Length of the data to send
 *
 * \return         The callback must return the number of bytes sent if any,
 *                 or a non-zero error code. \c MBEDTLS_ERR_SSL_WANT_WRITE
 *                 must be returned when the operation would block.
 *
 * \note           The callback is allowed to send fewer bytes than requested.
 *                 It must always return the number of bytes actually sent.
 */
static int mbedtls_ssl_send(void *ctx, const unsigned char *buf, size_t len)
{
	connection_t *connection = ctx;
	otMessageInfo messageInfo;
	otMessage    *msg = otUdpNewMessage(connection->otInstance, NULL);
	otError       err;

	if (len > LINK_MTU)
	{
		ca_log_warn("Failed to transmit, message too big");
		return MBEDTLS_ERR_NET_SEND_FAILED;
	}

	memset(&messageInfo, 0, sizeof(messageInfo));

	err = otMessageAppend(msg, buf, (uint16_t)len);
	if (err)
		goto exit;

	err = otUdpSend(connection->otInstance, &(connection->socket), msg, &messageInfo);
	if (err)
		goto exit;

exit:
	if (err)
	{
		otMessageFree(msg);

		if (err == OT_ERROR_NO_BUFS)
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		return MBEDTLS_ERR_NET_SEND_FAILED;
	}
	return (int)len;
}

/**
 * \brief          Callback type: receive data from the network.
 *
 * \param ctx      Context for the receive callback
 * \param buf      Buffer to write the received data to
 * \param len      Length of the receive buffer
 *
 * \return         The callback must return the number of bytes received,
 *                 or a non-zero error code. \c MBEDTLS_ERR_SSL_WANT_READ
 *                 must be returned when the operation would block.
 *
 * \note           The callback may receive fewer bytes than the length of the
 *                 buffer. It must always return the number of bytes actually
 *                 received and written to the buffer.
 */
static int mbedtls_ssl_recv(void *ctx, unsigned char *buf, size_t len)
{
	connection_t *connection = ctx;
	uint16_t      offset     = 0;
	uint16_t      length;

	if (!rxMessage)
		return MBEDTLS_ERR_SSL_WANT_READ;

	offset = otMessageGetOffset(rxMessage);
	length = otMessageRead(rxMessage, offset, buf, len);
	if (length == 0)
	{
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	else
	{
		offset += length;
		otMessageSetOffset(rxMessage, offset);
	}

	return length;
}

static mbedtls_entropy_context  entropy_ctx;
static mbedtls_ctr_drbg_context ctr_drbg_ctx;

static void connection_dns_callback(ca_error aError, const otIp6Address *aAddress, dns_index aIndex, void *aContext)
{
	connection_t *connection = aContext;
	int           rval       = 0;

	if (aError == CA_ERROR_SUCCESS)
	{
		otSockAddr sa      = {};
		int        secMode = 0;
		otError    otErr;

		memcpy(&(sa.mAddress), aAddress, sizeof(*aAddress));
		sa.mPort = connection->socket.mPeerName.mPort;

		otErr = otUdpOpen(connection->otInstance, &(connection->socket), &connection_udp_receive, connection);
		if (!otErr)
			otErr = otUdpBind(
			    connection->otInstance, &(connection->socket), &(connection->socket.mSockName), OT_NETIF_UNSPECIFIED);
		if (!otErr)
			otErr = otUdpConnect(connection->otInstance, &(connection->socket), &sa);

		if (otErr)
		{
			aError = CA_ERROR_FAIL;
			goto exit;
		}

		connection->isSecure = true;
		//mbedtls initialisation
		mbedtls_ssl_init(&connection->ssl);
		mbedtls_ssl_config_init(&connection->conf);
		mbedtls_ssl_config_defaults(
		    &connection->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
		//mbedtls_ssl_conf_authmode(&connection->conf, MBEDTLS_SSL_VERIFY_REQUIRED); //Pick one! (probably this one for prod)
		mbedtls_ssl_conf_authmode(&connection->conf, MBEDTLS_SSL_VERIFY_NONE); //Pick one!

		mbedtls_ctr_drbg_init(&ctr_drbg_ctx);
		mbedtls_entropy_init(&entropy_ctx);
		mbedtls_ctr_drbg_seed(&ctr_drbg_ctx, mbedtls_entropy_func, &entropy_ctx, NULL, 0);

		mbedtls_ssl_conf_rng(&connection->conf, mbedtls_ctr_drbg_random, &ctr_drbg_ctx);
		mbedtls_ssl_conf_min_version(&connection->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
		mbedtls_ssl_conf_max_version(&connection->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

		secMode = security_get_mode(connection->securityObj, connection->securityInstId);
		switch (secMode)
		{
		case LWM2M_SECURITY_MODE_PRE_SHARED_KEY:;
			const char *psk;
			const char *pskid;
			size_t      psklen;
			size_t      pskidlen;

			connection->cipherSuites[0] = MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8;
			connection->cipherSuites[1] = 0;

			psk   = security_get_secret_key(connection->securityObj, connection->securityInstId, &psklen);
			pskid = security_get_public_id(connection->securityObj, connection->securityInstId, &pskidlen);
			rval  = mbedtls_ssl_conf_psk(&connection->conf, psk, psklen, pskid, pskidlen);
			mbedtls_ssl_conf_ciphersuites(&connection->conf, connection->cipherSuites);
			break;
			//TODO: configure other security modes
		case LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY:
		case LWM2M_SECURITY_MODE_CERTIFICATE:
		case LWM2M_SECURITY_MODE_NONE:
		default:
			ca_log_crit("Unsupported security mode %d!", secMode);
			aError = CA_ERROR_INVALID_ARGS;
			goto exit;
		}

		if (rval)
		{
			aError = CA_ERROR_FAIL;
			goto exit;
		}

		if (mbedtls_ssl_setup(&connection->ssl, &connection->conf))
		{
			aError = CA_ERROR_FAIL;
			goto exit;
		}

		mbedtls_ssl_set_bio(&connection->ssl, connection, mbedtls_ssl_send, mbedtls_ssl_recv, NULL);
		mbedtls_ssl_set_timer_cb(&connection->ssl, connection, &mbedtls_set_timer, &mbedtls_get_timer);

		//TODO: Can call mbedtls_ssl_set_hostname to verify certificate hostname here if desired.

		rval = mbedtls_ssl_handshake(&connection->ssl);
		if (rval != 0 && rval != MBEDTLS_ERR_SSL_WANT_READ && rval != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			aError = CA_ERROR_FAIL;
			goto exit;
		}
		// Now we need to wait a bit while the handshake completes...
		// Mbedtls should be called with the timer and received packets by calling the mbedtls_ssl_read function

		//TODO: Signal connection complete to lwm2m layer
	}

exit:
	if (aError)
	{
		ca_log_crit("Failed to connect to server, error %s", ca_error_str(aError));
		// We don't close the connection now, but allow the core stack to fail and clean up
	}
}

ca_error split_hostname(char *uri, char **hostp, char **portp)
{
	ca_error error = CA_ERROR_INVALID_ARGS;
	char    *host;
	char    *port;

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

connection_t *connection_create(otInstance      *aInstance,
                                lwm2m_context_t *lwm2mH,
                                lwm2m_object_t  *securityObj,
                                int              instanceId)
{
	connection_t *connection = connection_new_incoming(aInstance, lwm2mH);
	const char   *const_uri;
	char	     *uri = NULL;
	char	     *host;
	char	     *port;
	int           porti = 0;
	ca_error      error;

	if (!connection)
		goto exit;

	connection->securityObj    = securityObj;
	connection->securityInstId = instanceId;
	const_uri                  = security_get_server_uri(securityObj, instanceId);
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

	if (error)
	{
		connection_free(connection);
		connection = NULL;
	}

exit:
	return connection;
}

void connection_free(connection_t *con)
{
	mbedtls_ssl_free(&con->ssl);
	mbedtls_ssl_config_free(&con->conf);
	otUdpClose(con->otInstance, &con->socket);
	con->inUse    = false;
	con->isSecure = false;
	TASKLET_Cancel(&con->mbedtls_tasklet);
}

static ca_error connection_send(connection_t *connP, uint8_t *buffer, size_t length)
{
	otMessageInfo messageInfo;
	otMessage    *msg = otUdpNewMessage(connP->otInstance, NULL);
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

	err = otUdpSend(connP->otInstance, &(connP->socket), msg, &messageInfo);
	if (err)
		goto exit;

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

	if (connP->isSecure)
	{
		int rval = mbedtls_ssl_write(&connP->ssl, buffer, length);
		if (rval < 0)
		{
			ca_log_warn("#> failed sending %lu bytes, error -0x%x\r\n", length, -rval);
			return COAP_500_INTERNAL_SERVER_ERROR;
		}
	}
	else
	{
		if (connection_send(connP, buffer, length))
		{
			ca_log_warn("#> failed sending %lu bytes\r\n", length);
			return COAP_500_INTERNAL_SERVER_ERROR;
		}
	}

	connection_tx_info_callback((uint16_t)length);
	return COAP_NO_ERROR;
}

bool lwm2m_session_is_equal(void *session1, void *session2, void *userData)
{
	(void)userData; /* unused */

	return (session1 == session2);
}
