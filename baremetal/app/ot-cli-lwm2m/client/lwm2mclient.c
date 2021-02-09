/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Benjamin Cabé - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Christian Renz - Please refer to git log
 *    Ricky Liu - Please refer to git log
 *    Ciaran Woodward - Adaption to the Cascoda SDK
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>
 Bosch Software Innovations GmbH - Please refer to git log
 Ciaran Woodward <c.woodward@cascoda.com> - Please refer to git log

*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ca-ot-util/cascoda_dns.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-util/cascoda_rand.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "openthread/cli.h"
#include "openthread/tasklet.h"

#include "liblwm2m.h"
#include "lwm2mclient.h"
#include "object_security.h"
#include "platform.h"
#include "sntp_helper.h"
#ifdef WITH_MBEDTLS
#include "mbedtlsconnection.h"
#else
#include "connection.h"
#endif

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))
#define DEFAULT_SERVER_IPV6 "[::1]" //CW: Set to actual server

#define OBJ_COUNT 9
lwm2m_object_t *objArray[OBJ_COUNT];

// only backup security and server objects
#define BACKUP_OBJECT_COUNT 2
lwm2m_object_t *backupObjectArray[BACKUP_OBJECT_COUNT];

typedef struct
{
	lwm2m_object_t *securityObjP;
	lwm2m_object_t *serverObject;
	connection_t *  connList[CONPOOL_SIZE];
} client_data_t;

otInstance *            OT_INSTANCE;
static lwm2m_context_t *s_lwm2mH;
static client_data_t    s_client_data;
static char *           pskBuffer = NULL;

static ca_tasklet battery_level_tasklet;
static ca_tasklet lwm2m_tasklet;

otInstance *get_ot_instance(void)
{
	return OT_INSTANCE;
}

static void lwm2m_step_now()
{
	TASKLET_Cancel(&lwm2m_tasklet);
	TASKLET_ScheduleDelta(&lwm2m_tasklet, 0, NULL);
}

void handle_value_changed(lwm2m_context_t *lwm2mH, lwm2m_uri_t *uri, const char *value, size_t valueLength)
{
	lwm2m_object_t *object = (lwm2m_object_t *)LWM2M_LIST_FIND(lwm2mH->objectList, uri->objectId);

	if (NULL != object)
	{
		if (object->writeFunc != NULL)
		{
			lwm2m_data_t *dataP;
			int           result;

			dataP = lwm2m_data_new(1);
			if (dataP == NULL)
			{
				fprintf(stderr, "Internal allocation failure !\n");
				return;
			}
			dataP->id = uri->resourceId;

#ifndef LWM2M_VERSION_1_0
			if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uri))
			{
				lwm2m_data_t *subDataP = lwm2m_data_new(1);
				if (subDataP == NULL)
				{
					fprintf(stderr, "Internal allocation failure !\n");
					lwm2m_data_free(1, dataP);
					return;
				}
				subDataP->id = uri->resourceInstanceId;
				lwm2m_data_encode_nstring(value, valueLength, subDataP);
				lwm2m_data_encode_instances(subDataP, 1, dataP);
			}
			else
#endif
			{
				lwm2m_data_encode_nstring(value, valueLength, dataP);
			}

			result = object->writeFunc(uri->instanceId, 1, dataP, object, LWM2M_WRITE_PARTIAL_UPDATE);
			if (COAP_405_METHOD_NOT_ALLOWED == result)
			{
				switch (uri->objectId)
				{
				case LWM2M_DEVICE_OBJECT_ID:
					result = device_change(dataP, object);
					break;
				default:
					break;
				}
			}

			if (COAP_204_CHANGED != result)
			{
				fprintf(stderr, "Failed to change value!\n");
			}
			else
			{
				fprintf(stderr, "value changed!\n");
				lwm2m_resource_value_changed(lwm2mH, uri);
			}
			lwm2m_data_free(1, dataP);
			return;
		}
		else
		{
			fprintf(stderr, "write not supported for specified resource!\n");
		}
		return;
	}
	else
	{
		fprintf(stderr, "Object not found !\n");
	}
}

void *lwm2m_connect_server(uint16_t secObjInstID, void *userData)
{
	client_data_t *dataP;
	const char *   const_uri;
	char *         uri;
	char *         host;
	char *         port;
	int            porti    = 0;
	connection_t * newConnP = NULL;

	dataP = (client_data_t *)userData;

	newConnP = connection_create(OT_INSTANCE, s_lwm2mH, dataP->securityObjP, secObjInstID);

	if (newConnP == NULL)
	{
		fprintf(stderr, "Connection creation failed.\r\n");
	}
	else
	{
		for (int i = 0; i < CONPOOL_SIZE; i++)
		{
			if (s_client_data.connList[i] == NULL)
				dataP->connList[i] = newConnP;
		}
	}

exit:
	lwm2m_free(uri);
	return (void *)newConnP;
}

void lwm2m_close_connection(void *sessionH, void *userData)
{
	client_data_t *app_data;
	connection_t * targetP;

	app_data = (client_data_t *)userData;
	targetP  = (connection_t *)sessionH;

	connection_free(targetP);

	for (int i = 0; i < CONPOOL_SIZE; i++)
	{
		if (s_client_data.connList[i] == targetP)
			s_client_data.connList[i] = NULL;
	}
}

static void prv_output_servers(int argc, char *argv[])
{
	lwm2m_server_t *targetP;

	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

	targetP = s_lwm2mH->bootstrapServerList;

	if (s_lwm2mH->bootstrapServerList == NULL)
	{
		otCliOutputFormat("No Bootstrap Server.\r\n");
	}
	else
	{
		otCliOutputFormat("Bootstrap Servers:\r\n");
		for (targetP = s_lwm2mH->bootstrapServerList; targetP != NULL; targetP = targetP->next)
		{
			otCliOutputFormat(" - Security Object ID %d", targetP->secObjInstID);
			otCliOutputFormat("\tHold Off Time: %lu s", (unsigned long)targetP->lifetime);
			otCliOutputFormat("\tstatus: ");
			switch (targetP->status)
			{
			case STATE_DEREGISTERED:
				otCliOutputFormat("DEREGISTERED\r\n");
				break;
			case STATE_BS_HOLD_OFF:
				otCliOutputFormat("CLIENT HOLD OFF\r\n");
				break;
			case STATE_BS_INITIATED:
				otCliOutputFormat("BOOTSTRAP INITIATED\r\n");
				break;
			case STATE_BS_PENDING:
				otCliOutputFormat("BOOTSTRAP PENDING\r\n");
				break;
			case STATE_BS_FINISHED:
				otCliOutputFormat("BOOTSTRAP FINISHED\r\n");
				break;
			case STATE_BS_FAILED:
				otCliOutputFormat("BOOTSTRAP FAILED\r\n");
				break;
			default:
				otCliOutputFormat("INVALID (%d)\r\n", (int)targetP->status);
			}
		}
	}

	if (s_lwm2mH->serverList == NULL)
	{
		otCliOutputFormat("No LWM2M Server.\r\n");
	}
	else
	{
		otCliOutputFormat("LWM2M Servers:\r\n");
		for (targetP = s_lwm2mH->serverList; targetP != NULL; targetP = targetP->next)
		{
			otCliOutputFormat(" - Server ID %d", targetP->shortID);
			otCliOutputFormat("\tstatus: ");
			switch (targetP->status)
			{
			case STATE_DEREGISTERED:
				otCliOutputFormat("DEREGISTERED\r\n");
				break;
			case STATE_REG_PENDING:
				otCliOutputFormat("REGISTRATION PENDING\r\n");
				break;
			case STATE_REGISTERED:
				fprintf(stdout,
				        "REGISTERED\tlocation: \"%s\"\tLifetime: %lus\r\n",
				        targetP->location,
				        (unsigned long)targetP->lifetime);
				break;
			case STATE_REG_UPDATE_PENDING:
				otCliOutputFormat("REGISTRATION UPDATE PENDING\r\n");
				break;
			case STATE_DEREG_PENDING:
				otCliOutputFormat("DEREGISTRATION PENDING\r\n");
				break;
			case STATE_REG_FAILED:
				otCliOutputFormat("REGISTRATION FAILED\r\n");
				break;
			default:
				otCliOutputFormat("INVALID (%d)\r\n", (int)targetP->status);
			}
		}
	}
}

static void prv_change(int argc, char *argv[])
{
	lwm2m_uri_t uri;
	int         result;

	if (argc < 1)
		goto syntax_error;

	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

	result = lwm2m_stringToUri(argv[0], strlen(argv[0]), &uri);
	if (result == 0)
		goto syntax_error;

	if (argc >= 2)
	{
		handle_value_changed(s_lwm2mH, &uri, argv[1], strlen(argv[1]));
	}
	else
	{
		fprintf(stderr, "report change!\n");
		lwm2m_resource_value_changed(s_lwm2mH, &uri);
	}
	lwm2m_step_now();

	return;

syntax_error:
	otCliOutputFormat("Syntax error! lwchange <URI> [VALUE]\n");
}

static void prv_object_list(int argc, char *argv[])
{
	lwm2m_object_t *objectP;

	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

	for (objectP = s_lwm2mH->objectList; objectP != NULL; objectP = objectP->next)
	{
		if (objectP->instanceList == NULL)
		{
			otCliOutputFormat("/%d ", objectP->objID);
		}
		else
		{
			lwm2m_list_t *instanceP;

			for (instanceP = objectP->instanceList; instanceP != NULL; instanceP = instanceP->next)
			{
				otCliOutputFormat("/%d/%d  ", objectP->objID, instanceP->id);
			}
		}
		otCliOutputFormat("\r\n");
	}
}

static void prv_update(int argc, char *argv[])
{
	if (argc == 0)
		goto syntax_error;

	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

	uint16_t serverId = (uint16_t)atoi(argv[0]);
	int      res      = lwm2m_update_registration(s_lwm2mH, serverId, false);
	lwm2m_step_now();
	if (res != 0)
	{
		otCliOutputFormat("Registration update error: %d\r\n", res);
	}
	return;

syntax_error:
	otCliOutputFormat("Syntax error ! lwupdate <SERVER ID>\n");
}

static ca_error update_battery_level(void *context)
{
	char        value[15];
	int         valueLength;
	lwm2m_uri_t uri;
	uint32_t    level;

	RAND_GetBytes(sizeof(level), &level);
	level = level % 100;

	if (lwm2m_stringToUri("/3/0/9", 6, &uri))
	{
		valueLength = sprintf(value, "%d", level);
		fprintf(stderr, "New Battery Level: %d\n", level);
		handle_value_changed((lwm2m_context_t *)context, &uri, value, valueLength);
	}

	TASKLET_ScheduleDelta(&battery_level_tasklet, 10000, NULL);
	return CA_ERROR_SUCCESS;
}

static void prv_add(int argc, char *argv[])
{
	lwm2m_object_t *objectP;
	int             res;

	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

	objectP = objArray[5];
	if (objectP == NULL || objectP->next)
	{
		otCliOutputFormat("Creating object 31024 failed.\r\n");
		return;
	}
	res = lwm2m_add_object(s_lwm2mH, objectP);
	if (res != 0)
	{
		otCliOutputFormat("Adding object 31024 failed: %d\r\n", res);
	}
	else
	{
		lwm2m_step_now();
		otCliOutputFormat("Object 31024 added.\r\n");
	}
	return;
}

static void prv_remove(int argc, char *argv[])
{
	int res;

	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

	res = lwm2m_remove_object(s_lwm2mH, 31024);
	if (res != 0)
	{
		otCliOutputFormat("Removing object 31024 failed: %d\r\n", res);
	}
	else
	{
		lwm2m_step_now();
		otCliOutputFormat("Object 31024 removed.\r\n");
	}
	return;
}

#ifdef LWM2M_BOOTSTRAP

static void prv_display_objects(int argc, char *argv[])
{
	lwm2m_object_t *object;

	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

	for (object = s_lwm2mH->objectList; object != NULL; object = object->next)
	{
		if (NULL != object)
		{
			switch (object->objID)
			{
			case LWM2M_SECURITY_OBJECT_ID:
				security_display_object(object);
				break;
			case LWM2M_SERVER_OBJECT_ID:
				display_server_object(object);
				break;
			case LWM2M_ACL_OBJECT_ID:
				break;
			case LWM2M_DEVICE_OBJECT_ID:
				display_device_object(object);
				break;
			case LWM2M_CONN_MONITOR_OBJECT_ID:
				break;
			case LWM2M_FIRMWARE_UPDATE_OBJECT_ID:
				display_firmware_object(object);
				break;
			case LWM2M_LOCATION_OBJECT_ID:
				display_location_object(object);
				break;
			case LWM2M_CONN_STATS_OBJECT_ID:
				break;
			case TEST_OBJECT_ID:
				display_test_object(object);
				break;
			}
		}
	}
}

static void prv_display_backup(int argc, char *argv[])
{
	int i;
	for (i = 0; i < BACKUP_OBJECT_COUNT; i++)
	{
		lwm2m_object_t *object = backupObjectArray[i];
		if (NULL != object)
		{
			switch (object->objID)
			{
			case LWM2M_SECURITY_OBJECT_ID:
				security_display_object(object);
				break;
			case LWM2M_SERVER_OBJECT_ID:
				display_server_object(object);
				break;
			default:
				break;
			}
		}
	}
}

static void prv_backup_objects(lwm2m_context_t *context)
{
	uint16_t i;

	for (i = 0; i < BACKUP_OBJECT_COUNT; i++)
	{
		if (NULL != backupObjectArray[i])
		{
			switch (backupObjectArray[i]->objID)
			{
			case LWM2M_SECURITY_OBJECT_ID:
				clean_security_object(backupObjectArray[i]);
				lwm2m_free(backupObjectArray[i]);
				break;
			case LWM2M_SERVER_OBJECT_ID:
				clean_server_object(backupObjectArray[i]);
				lwm2m_free(backupObjectArray[i]);
				break;
			default:
				break;
			}
		}
		backupObjectArray[i] = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
		memset(backupObjectArray[i], 0, sizeof(lwm2m_object_t));
	}

	/*
     * Backup content of objects 0 (security) and 1 (server)
     */
	security_copy_object(backupObjectArray[0],
	                     (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID));
	copy_server_object(backupObjectArray[1],
	                   (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID));
}

static void prv_restore_objects(lwm2m_context_t *context)
{
	lwm2m_object_t *targetP;

	/*
     * Restore content  of objects 0 (security) and 1 (server)
     */
	targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID);
	// first delete internal content
	clean_security_object(targetP);
	// then restore previous object
	security_copy_object(targetP, backupObjectArray[0]);

	targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID);
	// first delete internal content
	clean_server_object(targetP);
	// then restore previous object
	copy_server_object(targetP, backupObjectArray[1]);

	// restart the old servers
	otCliOutputFormat("[BOOTSTRAP] ObjectList restored\r\n");
}

static void update_bootstrap_info(lwm2m_client_state_t *previousBootstrapState, lwm2m_context_t *context)
{
	if (*previousBootstrapState != context->state)
	{
		*previousBootstrapState = context->state;
		switch (context->state)
		{
		case STATE_BOOTSTRAPPING:
			otCliOutputFormat("[BOOTSTRAP] backup security and server objects\r\n");
			prv_backup_objects(context);
			break;
		default:
			break;
		}
	}
}

static void close_backup_object()
{
	int i;
	for (i = 0; i < BACKUP_OBJECT_COUNT; i++)
	{
		if (NULL != backupObjectArray[i])
		{
			switch (backupObjectArray[i]->objID)
			{
			case LWM2M_SECURITY_OBJECT_ID:
				clean_security_object(backupObjectArray[i]);
				lwm2m_free(backupObjectArray[i]);
				break;
			case LWM2M_SERVER_OBJECT_ID:
				clean_server_object(backupObjectArray[i]);
				lwm2m_free(backupObjectArray[i]);
				break;
			default:
				break;
			}
		}
	}
}
#endif

void connection_rx_info_callback(uint16_t aPktLen)
{
	conn_s_updateRxStatistic(objArray[7], aPktLen);
}

void connection_tx_info_callback(uint16_t aPktLen)
{
	conn_s_updateTxStatistic(objArray[7], aPktLen);
}

void print_usage(void)
{
	otCliOutputFormat("Usage: lwstart [OPTION]\r\n");
	otCliOutputFormat("Launch a LWM2M client.\r\n");
	otCliOutputFormat("Options:\r\n");
	otCliOutputFormat("  -n NAME\tSet the endpoint name of the Client. Default: testlwm2mclient\r\n");
	otCliOutputFormat("  -l PORT\tSet the local UDP port of the Client. Default: 0 (ephemeral)\r\n");
	fprintf(stdout,
	        "  -h HOST\tSet the hostname of the LWM2M Server to connect to. Default: " DEFAULT_SERVER_IPV6 "\r\n");
	fprintf(stdout,
	        "  -p PORT\tSet the port of the LWM2M Server to connect to. Default: " LWM2M_STANDARD_PORT_STR "\r\n");
	otCliOutputFormat("  -t TIME\tSet the lifetime of the Client. Default: 300\r\n");
	otCliOutputFormat("  -b\t\tBootstrap requested.\r\n");
	otCliOutputFormat("  -c\t\tChange battery level over time.\r\n");
#ifdef WITH_MBEDTLS
	fprintf(
	    stdout,
	    "  -i STRING\tSet the device management or bootstrap server PSK identity. If not set use none secure mode\r\n");
	fprintf(stdout,
	        "  -s HEXSTRING\tSet the device management or bootstrap server Pre-Shared-Key. If not set use none secure "
	        "mode\r\n");
#endif
	otCliOutputFormat("\r\n");
}

static void prv_lwm2m_start(int argc, char *argv[])
{
	int         result;
	int         opt;
	int         lifetime           = 300;
	bool        bootstrapRequested = false;
	bool        serverPortChanged  = false;
	const char *localPort          = "0";
	const char *server             = NULL;
	const char *serverPort         = LWM2M_STANDARD_PORT_STR;
	char *      name               = "testlwm2mclient";

	char *pskId = NULL;
#ifdef WITH_MBEDTLS
	char *psk = NULL;
#endif
	uint16_t pskLen = -1;

	char serverUri[50];
	int  serverId = 123;

	if (s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M already started");
		return;
	}

	memset(&s_client_data, 0, sizeof(client_data_t));

	opt = 0;
	while (opt < argc)
	{
		if (argv[opt] == NULL || argv[opt][0] != '-' || argv[opt][2] != 0)
		{
			print_usage();
			return;
		}
		switch (argv[opt][1])
		{
		case 'b':
			bootstrapRequested = true;
			if (!serverPortChanged)
				serverPort = LWM2M_BSSERVER_PORT_STR;
			break;
		case 'c':
			TASKLET_ScheduleDelta(&battery_level_tasklet, 10000, NULL);
			break;
		case 't':
			opt++;
			if (opt >= argc)
			{
				print_usage();
				return;
			}
			if (!(lifetime = atoi(argv[opt])))
			{
				print_usage();
				return;
			}
			break;
#ifdef WITH_MBEDTLS
		case 'i':
			opt++;
			if (opt >= argc)
			{
				print_usage();
				return;
			}
			pskId = argv[opt];
			break;
		case 's':
			opt++;
			if (opt >= argc)
			{
				print_usage();
				return;
			}
			psk = argv[opt];
			break;
#endif
		case 'n':
			opt++;
			if (opt >= argc)
			{
				print_usage();
				return;
			}
			name = argv[opt];
			break;
		case 'l':
			opt++;
			if (opt >= argc)
			{
				print_usage();
				return;
			}
			localPort = argv[opt];
			break;
		case 'h':
			opt++;
			if (opt >= argc)
			{
				print_usage();
				return;
			}
			server = argv[opt];
			break;
		case 'p':
			opt++;
			if (opt >= argc)
			{
				print_usage();
				return;
			}
			serverPort        = argv[opt];
			serverPortChanged = true;
			break;
		default:
			print_usage();
			return;
		}
		opt += 1;
	}

	if (!server)
	{
		server = DEFAULT_SERVER_IPV6;
	}

	/*
     * Now the main function fill an array with each object, this list will be later passed to liblwm2m.
     * Those functions are located in their respective object file.
     */
#ifdef WITH_MBEDTLS
	if (psk != NULL)
	{
		pskLen    = strlen(psk) / 2;
		pskBuffer = malloc(pskLen);

		if (!serverPortChanged)
			serverPort = LWM2M_DTLS_PORT_STR;

		if (NULL == pskBuffer)
		{
			fprintf(stderr, "Failed to create PSK binary buffer\r\n");
			return;
		}
		// Hex string to binary
		char *h       = psk;
		char *b       = pskBuffer;
		char  xlate[] = "0123456789ABCDEF";

		for (; *h; h += 2, ++b)
		{
			char *l = strchr(xlate, toupper(*h));
			char *r = strchr(xlate, toupper(*(h + 1)));

			if (!r || !l)
			{
				fprintf(stderr, "Failed to parse Pre-Shared-Key HEXSTRING\r\n");
				return;
			}

			*b = ((l - xlate) << 4) + (r - xlate);
		}
	}
#endif

#ifdef WITH_MBEDTLS
	snprintf(serverUri, sizeof(serverUri), "coaps://%s:%s", server, serverPort);
#else
	snprintf(serverUri, sizeof(serverUri), "coap://%s:%s", server, serverPort);
#endif
#ifdef LWM2M_BOOTSTRAP
	objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, bootstrapRequested);
#else
	objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, false);
#endif
	if (NULL == objArray[0])
	{
		fprintf(stderr, "Failed to create security object\r\n");
		return;
	}
	s_client_data.securityObjP = objArray[0];

	objArray[1] = get_server_object(serverId, "U", lifetime, false);
	if (NULL == objArray[1])
	{
		fprintf(stderr, "Failed to create server object\r\n");
		return;
	}

	objArray[2] = get_object_device();
	if (NULL == objArray[2])
	{
		fprintf(stderr, "Failed to create Device object\r\n");
		return;
	}

	objArray[3] = get_object_firmware();
	if (NULL == objArray[3])
	{
		fprintf(stderr, "Failed to create Firmware object\r\n");
		return;
	}

	objArray[4] = get_object_location();
	if (NULL == objArray[4])
	{
		fprintf(stderr, "Failed to create location object\r\n");
		return;
	}

	objArray[5] = get_test_object();
	if (NULL == objArray[5])
	{
		fprintf(stderr, "Failed to create test object\r\n");
		return;
	}

	objArray[6] = get_object_conn_m();
	if (NULL == objArray[6])
	{
		fprintf(stderr, "Failed to create connectivity monitoring object\r\n");
		return;
	}

	objArray[7] = get_object_conn_s();
	if (NULL == objArray[7])
	{
		fprintf(stderr, "Failed to create connectivity statistics object\r\n");
		return;
	}

	int instId  = 0;
	objArray[8] = acc_ctrl_create_object();
	if (NULL == objArray[8])
	{
		fprintf(stderr, "Failed to create Access Control object\r\n");
		return;
	}
	else if (acc_ctrl_obj_add_inst(objArray[8], instId, 3, 0, serverId) == false)
	{
		fprintf(stderr, "Failed to create Access Control object instance\r\n");
		return;
	}
	else if (acc_ctrl_oi_add_ac_val(objArray[8], instId, 0, 0x0F) == false)
	{
		fprintf(stderr, "Failed to create Access Control ACL default resource\r\n");
		return;
	}
	else if (acc_ctrl_oi_add_ac_val(objArray[8], instId, 999, 0x01) == false)
	{
		fprintf(stderr, "Failed to create Access Control ACL resource for serverId: 999\r\n");
		return;
	}
	/*
     * The liblwm2m library is now initialized with the functions that will be in
     * charge of communication
     */
	s_lwm2mH = lwm2m_init(&s_client_data);
	if (NULL == s_lwm2mH)
	{
		fprintf(stderr, "lwm2m_init() failed\r\n");
		return;
	}

	/*
     * We configure the liblwm2m library with the name of the client - which shall be unique for each client -
     * the number of objects we will be passing through and the objects array
     */
	result = lwm2m_configure(s_lwm2mH, name, NULL, NULL, OBJ_COUNT, objArray);
	if (result != 0)
	{
		fprintf(stderr, "lwm2m_configure() failed: 0x%X\r\n", result);
		return;
	}

	otCliOutputFormat("LWM2M Client \"%s\" started on port %s\r\n", name, localPort);

	// Start lwm2m in 100 ms
	TASKLET_ScheduleDelta(&lwm2m_tasklet, 100, NULL);
}

static void prv_lwm2m_stop(int argc, char *argv[])
{
	if (!s_lwm2mH)
	{
		otCliOutputFormat("Error: LWM2M not started");
		return;
	}

#ifdef WITH_MBEDTLS
	free(pskBuffer);
#endif

#ifdef LWM2M_BOOTSTRAP
	close_backup_object();
#endif
	lwm2m_close(s_lwm2mH);
	s_lwm2mH = NULL;

	for (int i = 0; i < CONPOOL_SIZE; i++)
	{
		if (s_client_data.connList[i])
		{
			connection_free(s_client_data.connList[i]);
			s_client_data.connList[i] = NULL;
		}
	}

	clean_security_object(objArray[0]);
	lwm2m_free(objArray[0]);
	clean_server_object(objArray[1]);
	lwm2m_free(objArray[1]);
	free_object_device(objArray[2]);
	free_object_firmware(objArray[3]);
	free_object_location(objArray[4]);
	free_test_object(objArray[5]);
	free_object_conn_m(objArray[6]);
	free_object_conn_s(objArray[7]);
	acl_ctrl_free_object(objArray[8]);

#ifdef MEMORY_TRACE
	if (g_quit == 1)
	{
		trace_print(0, 1);
	}
#endif
}

static ca_error lwm2m_tasklet_fn(void *context)
{
	lwm2m_client_state_t previousState = s_lwm2mH->state;
	time_t               nextTime_s    = 60; //Next time in seconds defaults to 60 seconds
	int                  result;

	result = lwm2m_step(s_lwm2mH, &nextTime_s);
	fprintf(stdout, " -> State: ");
	switch (s_lwm2mH->state)
	{
	case STATE_INITIAL:
		fprintf(stdout, "STATE_INITIAL\r\n");
		break;
	case STATE_BOOTSTRAP_REQUIRED:
		fprintf(stdout, "STATE_BOOTSTRAP_REQUIRED\r\n");
		break;
	case STATE_BOOTSTRAPPING:
		fprintf(stdout, "STATE_BOOTSTRAPPING\r\n");
		break;
	case STATE_REGISTER_REQUIRED:
		fprintf(stdout, "STATE_REGISTER_REQUIRED\r\n");
		break;
	case STATE_REGISTERING:
		fprintf(stdout, "STATE_REGISTERING\r\n");
		break;
	case STATE_READY:
		fprintf(stdout, "STATE_READY\r\n");
		break;
	default:
		fprintf(stdout, "Unknown...\r\n");
		break;
	}
	if (result != 0)
	{
		fprintf(stderr, "lwm2m_step() failed: 0x%X\r\n", result);
		if (previousState == STATE_BOOTSTRAPPING)
		{
			fprintf(stdout, "[BOOTSTRAP] restore security and server objects\r\n");
			prv_restore_objects(s_lwm2mH);
			s_lwm2mH->state = STATE_INITIAL;
		}
		else
			return CA_ERROR_SUCCESS;
	}
#ifdef LWM2M_BOOTSTRAP
	update_bootstrap_info(&previousState, s_lwm2mH);
#endif

	TASKLET_ScheduleDelta(&lwm2m_tasklet, nextTime_s * 1000, NULL);

	return CA_ERROR_SUCCESS;
}

static int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret = 0;

	if (buf[0] == OT_SERIAL_DOWNLINK)
	{
		PlatformUartReceive(buf + 2, buf[1]);
		ret = 1;
	}
	return ret;
}

int main(int argc, char *argv[])
{
	struct ca821x_dev dev;
	ca821x_api_init(&dev);
	cascoda_serial_dispatch = ot_serial_dispatch;

	// Initialisation
	EVBMEInitialise(CA_TARGET_NAME, &dev);

	PlatformRadioInitWithDev(&dev);
	OT_INSTANCE = otInstanceInitSingle();

	otCliUartInit(OT_INSTANCE);

	TASKLET_Init(&lwm2m_tasklet, &lwm2m_tasklet_fn);
	TASKLET_Init(&battery_level_tasklet, &update_battery_level);
	DNS_Init(OT_INSTANCE);
	SNTP_Init();
	SNTP_Update();

	/*
     * This is an array of CLI commands described as { command, callback }.
     */
	otCliCommand commands[] = {{"lwstart", prv_lwm2m_start},   //Start LWM2M
	                           {"lwstop", prv_lwm2m_stop},     //Stop LWM2M
	                           {"lwlist", prv_output_servers}, //List servers and server states
	                           {"lwchange", prv_change},       //Try to change a local value using the URI
	                           {"lwupdate", prv_update},       //Trigger a server registration update
#ifdef LWM2M_BOOTSTRAP
	                           {"lwdispb", prv_display_backup}, //Display backup objects
#endif
	                           {"lwls", prv_object_list},       //List URIs of available objects
	                           {"lwdisp", prv_display_objects}, //Display objects
	                           {"lwadd", prv_add},              //Add the test object (added by default)
	                           {"lwrm", prv_remove}};           //Remove the test object

	otCliSetUserCommands(commands, ARRAY_LENGTH(commands));

	// Endless Polling Loop
	while (1)
	{
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
	}

	return 0;
}
