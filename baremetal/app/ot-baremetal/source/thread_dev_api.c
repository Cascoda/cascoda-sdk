/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2015, CA-821X Thread Code                           ******/
/******************************************************************************/
/******************************************************************************/
/****** Thread API Procedures                                            ******/
/******************************************************************************/
/******************************************************************************/
/****** Revision           Notes                                         ******/
/******************************************************************************/
/****** 1.0  01/09/15  PB  Release Baseline                              ******/
/******************************************************************************/
/******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "ca821x_api.h"

#include "ot_api_headers.h"
#include "ot_api_messages.h"
#include "platform.h"

// Insert Application-Specific Includes here
#include "openthread/coap.h"
#include "openthread/dataset.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/message.h"
#include "openthread/platform/radio.h"
#include "openthread/thread.h"

extern otInstance *OT_INSTANCE;
u8_t               HandleMac; // 0 means Mac indications and confirms passed through
                              // usb; 1 means given to openthread

u8_t xmalloc_buf[256];
u8_t buf1 = 0;
u8_t buf2 = 0;
int  putchar(int OutChar);

#if (0)
int otCliOutput(const char *aBuf, uint16_t aBufLength, void *aContext)
{
	//EVBME_Message( (u8_t)aBufLength, (u8_t *)aBuf );

	u8_t i;

	for (i = 0; i < aBufLength; i++)
	{
		putchar(aBuf[i]);
	}
	return 0;
}
#endif

/******************************************************************************/
/******************************************************************************/
/****** xcalloc()                                                        ******/
/******************************************************************************/
/******************************************************************************/
/** xcalloc()
 **
 ** \brief Trivial calloc() implementation for mbed use. It only ever needs to
 **        allocate two buffers size 128 and 108 concurrently
 **
 ** \return Pointer to allocated buffer or NULLP
 **/
static void *xcalloc(size_t a, size_t b)
{
	//printf("xcalloc %d ", a*b );
	if ((a * b) <= 128)
	{
		if (buf1 == 0)
		{
			buf1 = 1;
			//printf("0\n");
			return xmalloc_buf;
		}
		else if (buf2 == 0)
		{
			buf2 = 1;
			//printf("1\n");
			return xmalloc_buf + 128;
		}
	}
	// cannot allocate
	// printf("none\n");
	return 0;
}

/******************************************************************************/
/******************************************************************************/
/****** xfree()                                                          ******/
/******************************************************************************/
/******************************************************************************/
/** xfree()
 **
 ** \brief Trivial free() implementation for mbed use. It only needs to allocate
 **        two buffers size 128 and 108 at a time
 **
 ** \return void
 **/
static void xfree(void *a)
{
	if (a == xmalloc_buf)
	{
		buf1 = 0;
		//printf("xfree 0\n");
	}
	else if (a == (xmalloc_buf + 128))
	{
		buf2 = 0;
		//printf("xfree 1\n");
	}
}

size_t missing_strlcpy(uint8_t *dst, uint8_t *src, size_t max)
{
	size_t i;

	for (i = 0; i < max; i++)
	{
		if ((*dst++ = *src++) == 0)
		{
			break;
		}
	}
	return i;
}

/******************************************************************************/
/******************************************************************************/
/****** Openthread_Confirm()                                             ******/
/******************************************************************************/
/******************************************************************************/
/** Openthread_Confirm()
 **
 ** \brief Confirm SET, IFCONFIG or THREAD request
 **
 ** \return None
 **/
static void Openthread_Confirm(otError error, u8_t DispatchId)
{
	OT_GEN_CNF_t response;

	if (BSP_IsUSBPresent())
	{
		response.DispatchId = DispatchId;
		response.Status     = (u8_t)error;
		MAC_Message(THREAD_UPLINK_ID, 2, &response.DispatchId);
	}
}

int otApi_Dispatch(struct SerialBuffer *SerialRxBuffer)
{
	otError            error = OT_ERROR_NONE;
	otLinkModeConfig   linkMode;
	OTApiMsg_t         TAM;
	struct ca821x_dev *pDeviceRef = PlatformGetDeviceRef();
	int                ret        = 0;

	if (SerialRxBuffer->CmdId == THREAD_DOWNLINK_ID)
	{
		ret     = 1;
		TAM.Ptr = &SerialRxBuffer->CmdId;
		switch (SerialRxBuffer->Data[0])
		{
		case OT_CMD_IFCONFIG:
			switch (TAM.IfConfig->Control)
			{
			case 0:
				printf("IFCONFIG DOWN\n");
				error = otIp6SetEnabled(OT_INSTANCE, false);
				memset(&(pDeviceRef->callbacks), 0, sizeof(pDeviceRef->callbacks));
				HandleMac = 0;
				//PlatformRadioReinit();
				//OpenThreadStarted = 0;
				break;
			case 1:
				printf("IFCONFIG UP\n");
				PlatformRadioInitWithDev(pDeviceRef);
				HandleMac = 1;
				error     = otIp6SetEnabled(OT_INSTANCE, true);
				break;
			case 2:
				if (otIp6IsEnabled(OT_INSTANCE))
				{
					printf("Up\n");
				}
				else
				{
					printf("Down\n");
				}
				break;
			default:
				error = OT_ERROR_INVALID_ARGS;
				break;
			}
			Openthread_Confirm(error, OT_CNF_IFCONFIG);
			break;
		case OT_CMD_THREAD:
			switch (TAM.Thread->Control)
			{
			case 0:
				printf("THREAD STOP\n");
				error = otThreadSetEnabled(OT_INSTANCE, false);
				otThreadSetAutoStart(OT_INSTANCE, false);
				break;
			case 1:
				printf("THREAD START\n");
				error = otThreadSetEnabled(OT_INSTANCE, true);
				otThreadSetAutoStart(OT_INSTANCE, true);
				break;
			default:
				error = OT_ERROR_INVALID_ARGS;
				break;
			}
			Openthread_Confirm(error, OT_CNF_THREAD);
			break;
		case OT_CMD_SET:
			switch (TAM.Set->AttributeId)
			{
			case OT_ATTR_CHANNEL:
				error = otLinkSetChannel(OT_INSTANCE, TAM.Set->AttributeValue[0]);
				break;
			case OT_ATTR_PANID:
				printf("Panid %04X\n", GETLE16(TAM.Set->AttributeValue));
				error = otLinkSetPanId(OT_INSTANCE, GETLE16(TAM.Set->AttributeValue));
				break;
			case OT_ATTR_MODE_RX_ON_WHEN_IDLE:
				linkMode               = otThreadGetLinkMode(OT_INSTANCE);
				linkMode.mRxOnWhenIdle = (TAM.Set->AttributeValue[0] ? 1 : 0);
				error                  = otThreadSetLinkMode(OT_INSTANCE, linkMode);
				break;
			case OT_ATTR_MODE_SECURE:
				linkMode                     = otThreadGetLinkMode(OT_INSTANCE);
				linkMode.mSecureDataRequests = (TAM.Set->AttributeValue[0] ? 1 : 0);
				error                        = otThreadSetLinkMode(OT_INSTANCE, linkMode);
				break;
			case OT_ATTR_MODE_FFD:
				linkMode             = otThreadGetLinkMode(OT_INSTANCE);
				linkMode.mDeviceType = (TAM.Set->AttributeValue[0] ? 1 : 0);
				error                = otThreadSetLinkMode(OT_INSTANCE, linkMode);
				break;
			case OT_ATTR_MODE_NETDATA:
				linkMode              = otThreadGetLinkMode(OT_INSTANCE);
				linkMode.mNetworkData = (TAM.Set->AttributeValue[0] ? 1 : 0);
				error                 = otThreadSetLinkMode(OT_INSTANCE, linkMode);
				break;
			case OT_ATTR_POLL_PERIOD:
				otLinkSetPollPeriod(OT_INSTANCE, (u32_t)(TAM.Set->AttributeValue[0]) * 1000);
				break;
			case OT_ATTR_MASTER_KEY:
			{
				otMasterKey key;

				memcpy(key.m8, TAM.Set->AttributeValue, 16);
				error = otThreadSetMasterKey(OT_INSTANCE, &key);
			}
			break;
			case OT_ATTR_NETWORK_NAME:
				TAM.Set->AttributeValue[TAM.Set->Length] = 0;
				error = otThreadSetNetworkName(OT_INSTANCE, (char const *)TAM.Set->AttributeValue);
				break;
			case OT_ATTR_EXT_PANID:
			{
				otExtendedPanId extPanId;

				memcpy(extPanId.m8, TAM.Set->AttributeValue, 8);
				error = otThreadSetExtendedPanId(OT_INSTANCE, &extPanId);
			}

			break;
			case OT_ATTR_KEY_SEQUENCE:
				error = OT_ERROR_NOT_IMPLEMENTED;
				break;
			case OT_ATTR_EXT_ADDR:
			{
				otExtAddress extAddress;

				memcpy(extAddress.m8, TAM.Set->AttributeValue, 8);
				error = otLinkSetExtendedAddress(OT_INSTANCE, &extAddress);
			}
			break;

			case OT_ATTR_CHILD_TIMEOUT:
				//otSetChildTimeout( OT_INSTANCE, A_GetLE32( TAM.Set->AttributeValue ) );
				error = OT_ERROR_NOT_IMPLEMENTED;
				break;
			case OT_ATTR_CONTEXT_REUSE_DELAY:
				//otSetContextIdReuseDelay( OT_INSTANCE, A_GetLE32( TAM.Set->AttributeValue ) );
				error = OT_ERROR_NOT_IMPLEMENTED;
				break;
			case OT_ATTR_LEADER_DATA:
			case OT_ATTR_LEADER_WEIGHT:
			case OT_ATTR_NETWORK_ID_TIMEOUT:
			case OT_ATTR_RLOC16:
			case OT_ATTR_ROUTER_UPGRADE_THRESHOLD:
			default:
				error = OT_ERROR_NOT_IMPLEMENTED;
				break;
			case OT_ATTR_AUTO_START:
				error = otThreadSetAutoStart(OT_INSTANCE, (TAM.Set->AttributeValue[0] ? 1 : 0));
				break;
			}
			Openthread_Confirm(error, OT_CNF_SET);
			break;

		case OT_CMD_GET:
		{
			OT_GET_CNF_t response;
			u8_t         Length = 3;

			response.DispatchId  = OT_CNF_GET;
			response.AttributeId = TAM.Get->AttributeId;
			switch (TAM.Get->AttributeId)
			{
			case OT_ATTR_CHANNEL:
				response.AttributeValue[0] = otLinkGetChannel(OT_INSTANCE);
				Length++;
				break;
			case OT_ATTR_PANID:
				error = OT_ERROR_NOT_IMPLEMENTED;
				break;
			case OT_ATTR_MODE_RX_ON_WHEN_IDLE:
				linkMode                   = otThreadGetLinkMode(OT_INSTANCE);
				response.AttributeValue[0] = linkMode.mRxOnWhenIdle;
				Length++;
				break;
			case OT_ATTR_MODE_SECURE:
				linkMode                   = otThreadGetLinkMode(OT_INSTANCE);
				response.AttributeValue[0] = linkMode.mSecureDataRequests;
				Length++;
				break;
			case OT_ATTR_MODE_FFD:
				linkMode                   = otThreadGetLinkMode(OT_INSTANCE);
				response.AttributeValue[0] = linkMode.mDeviceType;
				Length++;
				break;
			case OT_ATTR_MODE_NETDATA:
				linkMode                   = otThreadGetLinkMode(OT_INSTANCE);
				response.AttributeValue[0] = linkMode.mNetworkData;
				Length++;
				break;
			case OT_ATTR_POLL_PERIOD:
			case OT_ATTR_MASTER_KEY:
			case OT_ATTR_NETWORK_NAME:
			case OT_ATTR_EXT_PANID:
			case OT_ATTR_KEY_SEQUENCE:
			case OT_ATTR_EXT_ADDR:
			case OT_ATTR_CHILD_TIMEOUT:
			case OT_ATTR_CONTEXT_REUSE_DELAY:
			case OT_ATTR_LEADER_DATA:
			case OT_ATTR_LEADER_WEIGHT:
			case OT_ATTR_NETWORK_ID_TIMEOUT:
			case OT_ATTR_RLOC16:
			case OT_ATTR_ROUTER_UPGRADE_THRESHOLD:
			default:
				error = OT_ERROR_NOT_IMPLEMENTED;
				break;
			case OT_ATTR_AUTO_START:
				response.AttributeValue[0] = otThreadGetAutoStart(OT_INSTANCE);
				Length++;
				break;
			}
			response.Status = (u8_t)error;
			MAC_Message(THREAD_UPLINK_ID, Length, &response.DispatchId);
		}
		break;
		case OT_CMD_STATE:
		{
			OT_STATE_CNF_t response;
			switch (TAM.State->Control)
			{
			case 0:
				break;
			case 1:
				error = otThreadBecomeDetached(OT_INSTANCE);
				break;
			case 2:
				error = otThreadBecomeChild(OT_INSTANCE);
				break;
			default:
				error = OT_ERROR_NOT_IMPLEMENTED;
				break;
			}
			response.State      = otThreadGetDeviceRole(OT_INSTANCE);
			response.DispatchId = OT_CNF_STATE;
			response.Status     = (u8_t)error;
			MAC_Message(THREAD_UPLINK_ID, 3, &response.DispatchId);
		}
		break;
		case OT_CMD_FACTORY_RESET:
		{
			otInstanceFactoryReset(OT_INSTANCE);
			Openthread_Confirm(error, OT_CNF_FACTORY_RESET);
		}
		case OT_CMD_APPLICATION_CMD:
		{
			error = OT_ERROR_NOT_IMPLEMENTED;
		}
		break;
		default:
			break;
		}
	}
	else if (HandleMac)
	{
		// if openthread is handling mac drop mac requests from usb
		// TODO send MCPS_DATA_CONFIRM back up
		if ((SerialRxBuffer->CmdId == SPI_MCPS_DATA_REQUEST) || (SerialRxBuffer->CmdId == SPI_MCPS_PURGE_REQUEST))
		{
			ret = 1;
		}
	}
	return ret;
}
