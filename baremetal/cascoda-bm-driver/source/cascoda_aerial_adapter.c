/*
 *  Copyright (c) 2024, Cascoda Ltd.
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
/*
 * Aerial Adapter (printf over Air), interfaces to testref app.
*/

#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "ca821x_api.h"

#include "cascoda-bm/cascoda_aerial_adapter.h"

/* payload with message */
typedef struct aerial_adapter_payload_with_message_t
{
	uint8_t id;
	uint8_t type;
	uint8_t ed;
	uint8_t cs;
	uint8_t status;
	uint8_t message[AERIAL_ADAPTER_MAX_MESSAGE];
} aerial_adapter_payload_with_message_t;

/* payload without message */
typedef struct aerial_adapter_payload_without_message_t
{
	uint8_t id;
	uint8_t type;
	uint8_t ed;
	uint8_t cs;
	uint8_t status;
} aerial_adapter_payload_without_message_t;

/* store application PIB */
static uint8_t                              App_phyCurrentChannel;
static uint16_t                             App_macPANId;
static uint16_t                             App_macShortAddress;
static uint8_t                              App_macMinBE;
static uint8_t                              App_macMaxBE;
static uint8_t                              App_macMaxCSMABackoffs;
static uint8_t                              App_macMaxFrameRetries;
static uint8_t                              App_macRxOnWhenIdle;
static uint8_t                              App_macFrameCounter[4];
static uint8_t                              App_macDSN;
static uint8_t                              App_macSecurityEnabled;
static uint8_t                              App_HWME_EDTHRESHOLD;
static uint8_t                              App_HWME_CSTHRESHOLD;
static MCPS_DATA_confirm_callback           AppDataConfirmCallback;
static MCPS_DATA_indication_callback        AppDataIndicationCallback;
static MLME_COMM_STATUS_indication_callback AppCommStatusIndicationCallback;

/* channel for aerial adapter */
static uint8_t gAerialAdapterChannel = 0;

/* static global to store device ref. pointer */
static struct ca821x_dev *pAerialAdapterDeviceRef;

/* test status */
static uint8_t gAerialAdapterStatus;

/* static functions declarations */
static void     AerialAdapterSetPIB(void);
static void     AerialAdapterRestorePIB(void);
static ca_error AerialAdapter_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error AerialAdapter_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                   struct ca821x_dev                *pDeviceRef);
static ca_error AerialAdapter_MLME_COMM_STATUS_indication(struct MLME_COMM_STATUS_indication_pset *params,
                                                          struct ca821x_dev                       *pDeviceRef);
static ca_error AerialAdapterSendPacket(uint8_t type, aerial_adapter_payload_with_message_t *payload);
static ca_error AerialAdapterSendId(void);

/* store application PIB and set new PIB */
static void AerialAdapterSetPIB(void)
{
	u8_t  attlen;
	u8_t  param8;
	u16_t param16;

	/* save application PIB */
	MLME_GET_request_sync(phyCurrentChannel, 0, &attlen, &App_phyCurrentChannel, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macPANId, 0, &attlen, &App_macPANId, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macShortAddress, 0, &attlen, &App_macShortAddress, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macMinBE, 0, &attlen, &App_macMinBE, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macMaxBE, 0, &attlen, &App_macMaxBE, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macMaxCSMABackoffs, 0, &attlen, &App_macMaxCSMABackoffs, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macMaxFrameRetries, 0, &attlen, &App_macMaxFrameRetries, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macRxOnWhenIdle, 0, &attlen, &App_macRxOnWhenIdle, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macFrameCounter, 0, &attlen, App_macFrameCounter, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macDSN, 0, &attlen, &App_macDSN, pAerialAdapterDeviceRef);
	MLME_GET_request_sync(macSecurityEnabled, 0, &attlen, &App_macSecurityEnabled, pAerialAdapterDeviceRef);
	HWME_GET_request_sync(HWME_EDTHRESHOLD, &attlen, &App_HWME_EDTHRESHOLD, pAerialAdapterDeviceRef);
	HWME_GET_request_sync(HWME_CSTHRESHOLD, &attlen, &App_HWME_CSTHRESHOLD, pAerialAdapterDeviceRef);

	/* save callbacks */
	AppDataConfirmCallback          = pAerialAdapterDeviceRef->callbacks.MCPS_DATA_confirm;
	AppDataIndicationCallback       = pAerialAdapterDeviceRef->callbacks.MCPS_DATA_indication;
	AppCommStatusIndicationCallback = pAerialAdapterDeviceRef->callbacks.MLME_COMM_STATUS_indication;

	/* register new callbacks */
	pAerialAdapterDeviceRef->callbacks.MCPS_DATA_confirm           = &AerialAdapter_MCPS_DATA_confirm;
	pAerialAdapterDeviceRef->callbacks.MCPS_DATA_indication        = &AerialAdapter_MCPS_DATA_indication;
	pAerialAdapterDeviceRef->callbacks.MLME_COMM_STATUS_indication = &AerialAdapter_MLME_COMM_STATUS_indication;

	/* set PIB */
	param8 = gAerialAdapterChannel;
	MLME_SET_request_sync(phyCurrentChannel, 0, 1, &param8, pAerialAdapterDeviceRef); // set 15.4 Channel
	param16 = AERIAL_ADAPTER_PANID;
	MLME_SET_request_sync(macPANId, 0, 2, &param16, pAerialAdapterDeviceRef); // set local PANId
	param16 = AERIAL_ADAPTER_DUT_SHORTADD;
	MLME_SET_request_sync(macShortAddress, 0, 2, &param16, pAerialAdapterDeviceRef); // set local short address
	/* change PIB defaults */
	param8 = 3;
	MLME_SET_request_sync(macMinBE, 0, 1, &param8, pAerialAdapterDeviceRef); // default = 3
	param8 = 5;
	MLME_SET_request_sync(macMaxBE, 0, 1, &param8, pAerialAdapterDeviceRef); // default = 5
	param8 = 5;
	MLME_SET_request_sync(macMaxCSMABackoffs, 0, 1, &param8, pAerialAdapterDeviceRef); // default = 4
	param8 = 7;
	MLME_SET_request_sync(macMaxFrameRetries, 0, 1, &param8, pAerialAdapterDeviceRef); // default = 3
	/* get rid of far away interference .. */
	param8 = AERIAL_ADAPTER_EDTHRESHOLD;
	HWME_SET_request_sync(HWME_EDTHRESHOLD, 1, &param8, pAerialAdapterDeviceRef);
	param8 = AERIAL_ADAPTER_CSTHRESHOLD;
	HWME_SET_request_sync(HWME_CSTHRESHOLD, 1, &param8, pAerialAdapterDeviceRef);
	/* set RxOnWhenIdle */
	param8 = 1;
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &param8, pAerialAdapterDeviceRef);
}

/* restore application PIB */
static void AerialAdapterRestorePIB(void)
{
	/* restore application PIB */
	MLME_SET_request_sync(phyCurrentChannel, 0, 1, &App_phyCurrentChannel, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macPANId, 0, 2, &App_macPANId, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macShortAddress, 0, 2, &App_macShortAddress, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macMinBE, 0, 1, &App_macMinBE, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macMaxBE, 0, 1, &App_macMaxBE, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macMaxCSMABackoffs, 0, 1, &App_macMaxCSMABackoffs, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macMaxFrameRetries, 0, 1, &App_macMaxFrameRetries, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &App_macRxOnWhenIdle, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macSecurityEnabled, 0, 1, &App_macSecurityEnabled, pAerialAdapterDeviceRef);
	HWME_SET_request_sync(HWME_EDTHRESHOLD, 1, &App_HWME_EDTHRESHOLD, pAerialAdapterDeviceRef);
	HWME_SET_request_sync(HWME_CSTHRESHOLD, 1, &App_HWME_CSTHRESHOLD, pAerialAdapterDeviceRef);

	/* for OpenThread etc to re-initialise radio / mac */
	if (cascoda_reinitialise)
		cascoda_reinitialise(pAerialAdapterDeviceRef);

	/* restore callbacks */
	pAerialAdapterDeviceRef->callbacks.MCPS_DATA_confirm           = AppDataConfirmCallback;
	pAerialAdapterDeviceRef->callbacks.MCPS_DATA_indication        = AppDataIndicationCallback;
	pAerialAdapterDeviceRef->callbacks.MLME_COMM_STATUS_indication = AppCommStatusIndicationCallback;

	/* restore running counters */
	MLME_SET_request_sync(macFrameCounter, 0, 4, App_macFrameCounter, pAerialAdapterDeviceRef);
	MLME_SET_request_sync(macDSN, 0, 1, &App_macDSN, pAerialAdapterDeviceRef);
}

/* Callback for MCPS_DATA_confirm */
static ca_error AerialAdapter_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	uint8_t status;

	(void)pDeviceRef;

	status = params->Status;

	if (status)
		ca_log_warn("AerialAdapter_MCPS_DATA_confirm Status %02X", status);

	/* filter non-success mac status that can actually happen from time to time */
	if ((status != MAC_SUCCESS) && (status != MAC_NO_ACK) && (status != MAC_CHANNEL_ACCESS_FAILURE))
	{
		gAerialAdapterStatus = CA_ERROR_FAIL;
	}

	/* supress message other messages */
	return CA_ERROR_SUCCESS;
}

/* Callback for MCPS_DATA_indication */
static ca_error AerialAdapter_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                   struct ca821x_dev                *pDeviceRef)
{
	(void)params;
	(void)pDeviceRef;

	/* only supress message */
	return CA_ERROR_SUCCESS;
}

/* Callback for MLME_COMM_STATUS_indication */
static ca_error AerialAdapter_MLME_COMM_STATUS_indication(struct MLME_COMM_STATUS_indication_pset *params,
                                                          struct ca821x_dev                       *pDeviceRef)
{
	(void)params;
	(void)pDeviceRef;
	/* only supress message */
	return CA_ERROR_SUCCESS;
}

/* send message package */
static ca_error AerialAdapterSendPacket(uint8_t type, aerial_adapter_payload_with_message_t *payload)
{
	ca_error        status;
	struct FullAddr REFAdd;
	uint8_t         msdulen;
	uint8_t        *msdu = (uint8_t *)payload;

	/* construct address */
	PUTLE16(AERIAL_ADAPTER_REF_SHORTADD, REFAdd.Address);
	PUTLE16(AERIAL_ADAPTER_PANID, REFAdd.PANId);
	REFAdd.AddressMode = MAC_MODE_SHORT_ADDR;

	/* fill in data */
	payload->id     = AERIAL_ADAPTER_PKT_ID;
	payload->type   = type;
	payload->ed     = 0; /* not used on reference device side */
	payload->cs     = 0; /* not used on reference device side */
	payload->status = gAerialAdapterStatus;

	msdulen = AERIAL_ADAPTER_MSDU_POS_STATUS + 1;
	if (type == AERIAL_ADAPTER_TYPE_MESSAGE)
		msdulen += strlen(payload->message);

#if CASCODA_CA_VER >= 8212
	uint8_t tx_op[2] = {0x00, 0x00};
	tx_op[0] |= TXOPT0_ACKREQ;
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR,        /* SrcAddrMode */
	                  REFAdd,                     /* DstAddr */
	                  0,                          /* HeaderIELength */
	                  0,                          /* PayloadIELength */
	                  msdulen,                    /* MsduLength */
	                  msdu,                       /* pMsdu */
	                  AERIAL_ADAPTER_MSDU_HANDLE, /* MsduHandle */
	                  tx_op,                      /* pTxOptions */
	                  0,                          /* SchTimestamp */
	                  0,                          /* SchPeriod */
	                  0,                          /* TxChannel */
	                  NULLP,                      /* pHeaderIEList */
	                  NULLP,                      /* pPayloadIEList */
	                  NULLP,                      /* pSecurity */
	                  pAerialAdapterDeviceRef);   /* pDeviceRef */
#else
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR,        /* SrcAddrMode */
	                  REFAdd,                     /* DstAddr */
	                  msdulen,                    /* MsduLength */
	                  msdu,                       /* *pMsdu */
	                  AERIAL_ADAPTER_MSDU_HANDLE, /* MsduHandle */
	                  TXOPT_ACKREQ,               /* TxOptions */
	                  NULLP,                      /* *pSecurity */
	                  pAerialAdapterDeviceRef);   /* pDeviceRef */

#endif // CASCODA_CA_VER >= 8212

	/* make synchronous to avoid switching pib before confirm */
	status = WAIT_Callback(SPI_MCPS_DATA_CONFIRM, 100, NULL, pAerialAdapterDeviceRef);
	if (status)
		ca_log_warn("AerialAdapterSendPacket: WAIT_Callback Status %02X", status);

	return (status);
}

/* send device id to check software version etc. */
static ca_error AerialAdapterSendId(void)
{
	aerial_adapter_payload_with_message_t payload;
	ca_error                              status;

	snprintf(payload.message, AERIAL_ADAPTER_MAX_MESSAGE, "%s, %s\n", EVBME_GetAppName(), ca821x_get_version());
	status = AerialAdapterSendPacket(AERIAL_ADAPTER_TYPE_MESSAGE, &payload);

	if (status)
		ca_log_warn("AerialAdapterSendId Failed");

	return status;
}

/* initialise and start production test */
ca_error aainitialise(uint8_t channel, struct ca821x_dev *pDeviceRef)
{
	aerial_adapter_payload_without_message_t payload;
	ca_error                                 status;

	gAerialAdapterStatus = CA_ERROR_SUCCESS;

	if ((channel < 11) || (channel > 26))
	{
		ca_log_warn("aainitialise: Invalid Channel Number: %u", channel);
		return CA_ERROR_FAIL;
	}
	gAerialAdapterChannel = channel;

	/* store device ref. pointer */
	pAerialAdapterDeviceRef = pDeviceRef;

	/* store application PIB if already set and set new PIB */
	AerialAdapterSetPIB();

	/* initialise air interface with reference device */
	status = AerialAdapterSendPacket(AERIAL_ADAPTER_TYPE_REQUEST, (aerial_adapter_payload_with_message_t *)&payload);
	if (status)
	{
		gAerialAdapterStatus = CA_ERROR_FAIL;
		ca_log_warn("aainitialise: Requesting Connection failed\n");
		AerialAdapterRestorePIB();
		return CA_ERROR_FAIL;
	}

	/* wait for reference device confirmation packet, failure not critical as reference device responded to request */
	status = WAIT_Callback(SPI_MCPS_DATA_INDICATION, 100, NULL, pAerialAdapterDeviceRef);
	if (status)
		ca_log_warn("aainitialise: WAIT_Callback Status %02X", status);

	/* connection established - send id */
	status = AerialAdapterSendId();

	/* restore application PIB */
	AerialAdapterRestorePIB();

	if (status || gAerialAdapterStatus)
	{
		ca_log_warn("aainitialise Fail");
		return CA_ERROR_FAIL;
	}

	return CA_ERROR_SUCCESS;
}

/* aaprintf - use in the same fashion as printf() */
int aaprintf(const char *format, ...)
{
	int      ret = 0;
	ca_error status;

	/* don't do anything if there has been an error */
	if (gAerialAdapterStatus)
		return (EOF);
	;

	/* payload with message */
	aerial_adapter_payload_with_message_t payload;

	va_list va_args;
	va_start(va_args, format);

	ret = vsnprintf(payload.message, AERIAL_ADAPTER_MAX_MESSAGE, format, va_args);

	va_end(va_args);

	if (ret < 0)
		return ret;

	/* store application PIB and set new PIB */
	AerialAdapterSetPIB();

	/* send message packet */
	status = AerialAdapterSendPacket(AERIAL_ADAPTER_TYPE_MESSAGE, &payload);

	if (status)
	{
		gAerialAdapterStatus = status;
		ret                  = EOF;
	}

	/* restore application PIB */
	AerialAdapterRestorePIB();

	return ret;
}
