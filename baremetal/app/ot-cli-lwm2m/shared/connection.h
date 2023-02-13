/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * Copyright (c) 2021, Cascoda Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Distribution License v1.0
 * which accompanies this distribution.
 *
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Ciaran Woodward, Cascoda Ltd - modification for Cascoda SDK and OpenThread
 *
 *******************************************************************************/

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <liblwm2m.h>
#include <stdio.h>
#include <unistd.h>

#include "openthread/ip6.h"
#include "openthread/udp.h"

#include "ca821x_error.h"

#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT 5683
#define LWM2M_DTLS_PORT_STR "5684"
#define LWM2M_DTLS_PORT 5684
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT 5685

enum
{
	CONPOOL_SIZE = 2,    //!< Maximum number of concurrent connections
	LINK_MTU     = 1280, //!< Maximum Transmission Unit for link
};

/**
 * The struct for holding information about each connection the lwm2m stack uses.
 */
typedef struct connection_t
{
	otUdpSocket      socket;     //!< Openthread socket structure, used for UDP communications
	otInstance      *otInstance; //!< Pointer to openthread instance
	lwm2m_context_t *lwm2mH;     //!< Pointer to instance of lwm2m
	bool             inUse;      //!< Used internally to determine whether this connection structure is in use
} connection_t;

/**
 * Connect to the server defined in the security object.
 *
 * Note that the actual connection may not be complete when this function returns, due to host resolution.
 *
 * Called from client code directly to create a connection when connecting to a server.
 *
 * @param aInstance Openthread Instance pointer
 * @param lwm2mH  Wakaama instance pointer
 * @param secObjectP  Pointer to the security object
 * @param secObjInstID Security object instance ID
 * @return A pointer to the new connection, or NULL if error
 */
connection_t *connection_create(otInstance      *aInstance,
                                lwm2m_context_t *lwm2mH,
                                lwm2m_object_t  *secObjectP,
                                uint16_t         secObjInstID);

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
