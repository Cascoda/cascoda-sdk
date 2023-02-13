/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * Copyright (c) 2020, Cascoda Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Distribution License v1.0
 * which accompanies this distribution.
 *
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Ciaran Woodward, Cascoda Ltd - modification for Cascoda SDK, OpenThread and mbedtls
 *
 *******************************************************************************/

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <liblwm2m.h>
#include <stdio.h>
#include <unistd.h>

#include "mbedtls/certs.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/timing.h"

#include "openthread/ip6.h"
#include "openthread/random_crypto.h"
#include "openthread/udp.h"

#include "cascoda-util/cascoda_tasklet.h"
#include "ca821x_error.h"

#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT 5683
#define LWM2M_DTLS_PORT_STR "5684"
#define LWM2M_DTLS_PORT 5684
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT 5685

enum
{
	CIPHERSUITE_COUNT = 1,    //!< We only configure one ciphersuite on the client, based on authentication method
	CONPOOL_SIZE      = 2,    //!< Maximum number of concurrent connections
	LINK_MTU          = 1280, //!< Maximum Transmission Unit for link
};

/**
 * The struct for holding information about each connection the lwm2m stack uses.
 */
typedef struct connection_t
{
	otUdpSocket         socket;          //!< Openthread socket structure, used for UDP communications
	mbedtls_ssl_context ssl;             //!< mbedtls ssl context and state
	mbedtls_ssl_config  conf;            //!< mbedtls ssl config info
	ca_tasklet          mbedtls_tasklet; //!< Tasklet for managing mbedtls timing
	otInstance         *otInstance;      //!< Pointer to openthread instance
	lwm2m_context_t    *lwm2mH;          //!< Pointer to instance of lwm2m
	lwm2m_object_t     *securityObj;     //!< Pointer to lwm2m security instance
	int                 cipherSuites[CIPHERSUITE_COUNT + 1]; //!< List of the cipher suites in use
	int                 securityInstId;                      //!< lwm2m security instance ID
	uint32_t            intermediateTime; //!< Used with mbedtls_tasklet to implement the intermediate time
	bool                inUse;            //!< Used internally to determine whether this connection structure is in use
	bool                isTimerPassed;    //!< Used in with mbedtls_tasklet to determine expired or cancelled
	bool                isSecure;         //!< Used to determine whether this connection is secured with dtls
} connection_t;

/**
 * Called from example code directly to create a connection when connecting to a server.
 *
 * Note that the connection will not be immediately active, and will require some time to resolve hostname and form
 * DTLS connection with the server.
 *
 * @param aInstance  Openthread Instance pointer
 * @param lwm2mH  Wakaama context pointer
 * @param securityObj  Pointer to LWM2M security object (to extract creds)
 * @param instanceId  Security object instance ID
 * @return  Pointer to the connection created, or NULL if error.
 */
connection_t *connection_create(otInstance      *aInstance,
                                lwm2m_context_t *lwm2mH,
                                lwm2m_object_t  *securityObj,
                                int              instanceId);

/**
 * Free a connection created with connection_create.
 * @param con The connection to free
 */
void connection_free(connection_t *con);

/**
 * Callback that can be implemented by the application to track receive statistics
 * @param aPktLen      Length of the received packet
 */
void connection_rx_info_callback(uint16_t aPktLen);

/**
 * Callback that can be implemented by the application to track transmit statistics
 * @param aPktLen      Length of the received packet
 */
void connection_tx_info_callback(uint16_t aPktLen);

#endif
