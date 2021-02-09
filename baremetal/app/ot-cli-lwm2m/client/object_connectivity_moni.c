/*******************************************************************************
 *
 * Copyright (c) 2014 Bosch Software Innovations GmbH Germany. 
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    Ciaran Woodward, Cascoda ltd - Ported to Cascoda SDK & Openthread - see git log
 *    
 *******************************************************************************/

/*
 *  This Connectivity Monitoring object is optional and has a single instance
 * 
 *  Resources:
 *
 *          Name             | ID | Oper. | Inst. | Mand.|  Type   | Range | Units |
 *  Network Bearer           |  0 |  R    | Single|  Yes | Integer |       |       |
 *  Available Network Bearer |  1 |  R    | Multi |  Yes | Integer |       |       |
 *  Radio Signal Strength    |  2 |  R    | Single|  Yes | Integer |       | dBm   |
 *  Link Quality             |  3 |  R    | Single|  No  | Integer | 0-100 |   %   | Not included
 *  IP Addresses             |  4 |  R    | Multi |  Yes | String  |       |       |
 *  Router IP Addresses      |  5 |  R    | Multi |  No  | String  |       |       | Not included
 *  Link Utilization         |  6 |  R    | Single|  No  | Integer | 0-100 |   %   | Not included
 *  APN                      |  7 |  R    | Multi |  No  | String  |       |       | Not included
 *  Cell ID                  |  8 |  R    | Single|  No  | Integer |       |       | Not included
 *  SMNC                     |  9 |  R    | Single|  No  | Integer | 0-999 |   %   | Not included
 *  SMCC                     | 10 |  R    | Single|  No  | Integer | 0-999 |       | Not included
 *
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liblwm2m.h"
#include "lwm2mclient.h"

#include "openthread/config.h"
#include "openthread/thread.h"
#include "ca821x_endian.h"

// Resource Id's:
#define RES_M_NETWORK_BEARER 0
#define RES_M_AVL_NETWORK_BEARER 1
#define RES_M_RADIO_SIGNAL_STRENGTH 2
#define RES_O_LINK_QUALITY 3
#define RES_M_IP_ADDRESSES 4
#define RES_O_ROUTER_IP_ADDRESS 5
#define RES_O_LINK_UTILIZATION 6
#define RES_O_APN 7
#define RES_O_CELL_ID 8
#define RES_O_SMNC 9
#define RES_O_SMCC 10

#define VALUE_NETWORK_BEARER_IEEE_802_15_4 23 //IEEE 802.15.4
#define VALUE_AVL_NETWORK_BEARER_1 23         //IEEE 802.15.4
#define VALUE_AVL_NETWORK_BEARER_2 21         //WLAN
#define VALUE_AVL_NETWORK_BEARER_3 41         //Ethernet
#define VALUE_AVL_NETWORK_BEARER_4 42         //DSL
#define VALUE_AVL_NETWORK_BEARER_5 43         //PLC
#define VALUE_IP_ADDRESS_1 "192.168.178.101"
#define VALUE_IP_ADDRESS_2 "192.168.178.102"
#define VALUE_ROUTER_IP_ADDRESS_1 "192.168.178.001"
#define VALUE_ROUTER_IP_ADDRESS_2 "192.168.178.002"

#define MAX_IP_ADDRS (10)

/**
 * Get the average RSSI of the best router neighbor
 * @return average RSSI of best router neighbor, in dBm
 */
static int8_t get_rssi()
{
	int8_t                 rval         = INT8_MIN;
	otNeighborInfoIterator neighborIter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
	otNeighborInfo         neighborInfo;

	switch (otThreadGetDeviceRole(get_ot_instance()))
	{
	case OT_DEVICE_ROLE_DISABLED:
	case OT_DEVICE_ROLE_DETACHED:
		break;
	case OT_DEVICE_ROLE_CHILD:
		otThreadGetParentAverageRssi(get_ot_instance(), &rval);
		break;
	case OT_DEVICE_ROLE_LEADER:
	case OT_DEVICE_ROLE_ROUTER:
		while (otThreadGetNextNeighborInfo(get_ot_instance(), &neighborIter, &neighborInfo) == OT_ERROR_NONE)
		{
			if (neighborInfo.mIsChild)
				continue;
			if (neighborInfo.mAverageRssi > rval)
				rval = neighborInfo.mAverageRssi;
		}
		break;
	}

	return rval;
}

static uint8_t prv_set_value(lwm2m_data_t *dataP)
{
	lwm2m_data_t *subTlvP;
	size_t        count;
	size_t        i;

	switch (dataP->id)
	{
	case RES_M_NETWORK_BEARER:
		if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
			return COAP_404_NOT_FOUND;
		lwm2m_data_encode_int(VALUE_NETWORK_BEARER_IEEE_802_15_4, dataP);
		return COAP_205_CONTENT;

	case RES_M_AVL_NETWORK_BEARER:
	{
		if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
		{
			count   = dataP->value.asChildren.count;
			subTlvP = dataP->value.asChildren.array;
		}
		else
		{
			count   = 1; // reduced to 1 instance to fit in one block size
			subTlvP = lwm2m_data_new(count);
			for (i = 0; i < count; i++) subTlvP[i].id = i;
			lwm2m_data_encode_instances(subTlvP, count, dataP);
		}

		for (i = 0; i < count; i++)
		{
			switch (subTlvP[i].id)
			{
			case 0:
				lwm2m_data_encode_int(VALUE_AVL_NETWORK_BEARER_1, subTlvP + i);
				break;
			default:
				return COAP_404_NOT_FOUND;
			}
		}
		return COAP_205_CONTENT;
	}

	case RES_M_RADIO_SIGNAL_STRENGTH: //s-int
		if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
			return COAP_404_NOT_FOUND;
		lwm2m_data_encode_int(get_rssi(), dataP);
		return COAP_205_CONTENT;

	case RES_M_IP_ADDRESSES:
	{
		const otNetifAddress *addresses[MAX_IP_ADDRS]; //Max 10 addresses
		uint32_t              numAddrs = 0;
		const otNetifAddress *curAddr  = otIp6GetUnicastAddresses(get_ot_instance());

		// Convert the linked list into an array of pointers for easy processing
		while (curAddr && numAddrs < MAX_IP_ADDRS)
		{
			addresses[numAddrs] = curAddr;
			curAddr             = curAddr->mNext;
			numAddrs++;
		}

		if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
		{
			count   = dataP->value.asChildren.count;
			subTlvP = dataP->value.asChildren.array;
		}
		else
		{
			count   = numAddrs;
			subTlvP = lwm2m_data_new(count);
			for (i = 0; i < count; i++) subTlvP[i].id = i;
			lwm2m_data_encode_instances(subTlvP, count, dataP);
		}

		for (i = 0; i < count; i++)
		{
			if (subTlvP[i].id < numAddrs)
			{
				char buffer[strlen("0000:0000:0000:0000:0000:0000:0000:0000")];
				curAddr = addresses[subTlvP[i].id];

				//Convert the binary IPv6 address into a string, and add it to the data
				snprintf(buffer,
				         sizeof(buffer),
				         "%x:%x:%x:%x:%x:%x:%x:%x",
				         GETBE16(curAddr->mAddress.mFields.m8 + 0),
				         GETBE16(curAddr->mAddress.mFields.m8 + 2),
				         GETBE16(curAddr->mAddress.mFields.m8 + 4),
				         GETBE16(curAddr->mAddress.mFields.m8 + 6),
				         GETBE16(curAddr->mAddress.mFields.m8 + 8),
				         GETBE16(curAddr->mAddress.mFields.m8 + 10),
				         GETBE16(curAddr->mAddress.mFields.m8 + 12),
				         GETBE16(curAddr->mAddress.mFields.m8 + 14));
				lwm2m_data_encode_string(buffer, subTlvP + i);
			}
			else
			{
				return COAP_404_NOT_FOUND;
			}
		}
		return COAP_205_CONTENT;
	}
	break;

	default:
		return COAP_404_NOT_FOUND;
	}
}

static uint8_t prv_read(uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP)
{
	uint8_t result;
	int     i;

	// this is a single instance object
	if (instanceId != 0)
	{
		return COAP_404_NOT_FOUND;
	}

	// is the server asking for the full object ?
	if (*numDataP == 0)
	{
		uint16_t resList[] = {
		    RES_M_NETWORK_BEARER, RES_M_AVL_NETWORK_BEARER, RES_M_RADIO_SIGNAL_STRENGTH, RES_M_IP_ADDRESSES};
		int nbRes = sizeof(resList) / sizeof(uint16_t);

		*dataArrayP = lwm2m_data_new(nbRes);
		if (*dataArrayP == NULL)
			return COAP_500_INTERNAL_SERVER_ERROR;
		*numDataP = nbRes;
		for (i = 0; i < nbRes; i++)
		{
			(*dataArrayP)[i].id = resList[i];
		}
	}

	i = 0;
	do
	{
		result = prv_set_value((*dataArrayP) + i);
		i++;
	} while (i < *numDataP && result == COAP_205_CONTENT);

	return result;
}

lwm2m_object_t *get_object_conn_m(void)
{
	/*
     * The get_object_conn_m() function create the object itself and return a pointer to the structure that represent it.
     */
	lwm2m_object_t *connObj;

	connObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

	if (NULL != connObj)
	{
		memset(connObj, 0, sizeof(lwm2m_object_t));

		/*
         * It assigns his unique ID
         */
		connObj->objID = LWM2M_CONN_MONITOR_OBJECT_ID;

		/*
         * and its unique instance
         *
         */
		connObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
		if (NULL != connObj->instanceList)
		{
			memset(connObj->instanceList, 0, sizeof(lwm2m_list_t));
		}
		else
		{
			lwm2m_free(connObj);
			return NULL;
		}

		/*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
		connObj->readFunc    = prv_read;
		connObj->executeFunc = NULL;
		connObj->userData    = NULL;
	}
	return connObj;
}

void free_object_conn_m(lwm2m_object_t *objectP)
{
	lwm2m_free(objectP->userData);
	lwm2m_list_free(objectP->instanceList);
	lwm2m_free(objectP);
}
