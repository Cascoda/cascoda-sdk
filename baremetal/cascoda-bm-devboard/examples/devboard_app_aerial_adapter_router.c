/*
 *  Copyright (c) 2023, Cascoda Ltd.
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
 * Example application aerial-adapter with network (sed and router)
*/

/*
 * Button Allocation:
 *
 * Switch  Pos   Press     Function
 *
 * SW1     1     Short     LED on
 * SW2     1     Short     enable/disable indirect transmissions
 * SW3     1     Short     -
 * SW4     1     Short     switch between aerial-adapter and serial (usb/uart)
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_aerial_adapter.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_tasklet.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#if CASCODA_CA_VER != 8212 // exclude CA8212

/* Insert Application-Specific Includes here */
#include "devboard_btn.h"

/* indirect transmit packets period in [seconds] */
#define TXINDIRECT_PERIOD 10

/* msdu length for indirect packet */
#define MSDULENGTH 97

/* enable security when (1) */
#define ENABLE_SECURITY 1

/* air-adapter channel */
#define AIR_CHANNEL 15

/* PIB Values */
#define MAC_CHANNEL 13

#define MAC_PANID 0xCA5C

#define MAC_LONGADD_ROUTER                             \
	{                                                  \
		0x01, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA \
	}

#define MAC_SHORTADD_ROUTER 0xCA01

#define MAC_LONGADD_SED                                \
	{                                                  \
		0x02, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA \
	}

#define MAC_SHORTADD_SED 0xCA02

/* security pib */
#if (ENABLE_SECURITY)
uint8_t sMacSecurityEnabled = 1;
uint8_t sMacKeyTableEntries = 2;
uint8_t sMacKeyTable0[52]   = {
    0x02, 0x01, 0x0C,                                                                               // N1, N2, N3
    0xAD, 0x44, 0x3B, 0xEF, 0x80, 0xB7, 0x33, 0x6F, 0x2F, 0xF8, 0xB9, 0xA7, 0xA4, 0x7F, 0x01, 0x6A, // Key
    0x00, 0x01, 0xCA, 0x5C, 0xCA, 0x00, 0x00, 0x00, 0x00, 0x00,                                     // KeyIdLookupList
    0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA, 0x01,
    0x00,                                                                  // KeyDeviceList
    0x00, 0x01, 0x02, 0x13, 0x23, 0x33, 0x43, 0x53, 0x63, 0x73, 0x83, 0x93 // KeyUsageList
};
uint8_t sMacKeyTable1[52] = {
    0x02, 0x01, 0x0C,                                                                               // N1, N2, N3
    0xAD, 0x44, 0x3B, 0xEF, 0x80, 0xB7, 0x33, 0x6F, 0x2F, 0xF8, 0xB9, 0xA7, 0xA4, 0x7F, 0x01, 0x6A, // Key
    0x00, 0x02, 0xCA, 0x5C, 0xCA, 0x00, 0x00, 0x00, 0x00, 0x00,                                     // KeyIdLookupList
    0x00, 0x02, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA, 0x01,
    0x01,                                                                  // KeyDeviceList
    0x00, 0x01, 0x02, 0x13, 0x23, 0x33, 0x43, 0x53, 0x63, 0x73, 0x83, 0x93 // KeyUsageList
};
uint8_t sMacDeviceTableEntries = 2;
uint8_t sMacDeviceTable0[17] =
    {0x5C, 0xCA, 0x01, 0xCA, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t sMacDeviceTable1[17] =
    {0x5C, 0xCA, 0x02, 0xCA, 0x02, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t sMacDefaultKeySource[8] = {0xCA, 0x5C, 0x0D, 0xA0, 0x00, 0x00, 0x00, 0x01};
#endif

/* for switching between printf() and aaprintf() */
typedef int (*printf_function)(const char *format, ...);
printf_function xprintf;

/* switch between aerial adapter and serial */
static bool g_use_aerial_adapter = true;

/* enable indirect data to sed when */
static bool g_enable_indirect = false;

/* Over-Air exchange storage */
static struct MCPS_DATA_confirm_pset    gDataCnf;
static struct MCPS_DATA_indication_pset gDataInd;
static bool                             gDataIndReceived = false;
static bool                             gDataCnfReceived = false;

/* device button callbacks */
static void sw2_callback(void *context);
static void sw4_callback(void *context);

/* other static device functions */
static void hardware_init(void);
static void hardware_poll(void);

/* Over-Air functions */
static void     AirInitPIB(struct ca821x_dev *pDeviceRef);
static void     AirInitPIBRollingValues(struct ca821x_dev *pDeviceRef);
static ca_error AirTransmitIndirect(void *aContext);
static ca_error AirProcessDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error AirProcessDataIndication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error AirProcessCommStatusIndication(struct MLME_COMM_STATUS_indication_pset *params,
                                               struct ca821x_dev                       *pDeviceRef);
static void     AirRegisterCallBacks(struct ca821x_dev *pDeviceRef);
static void     AirReportStatus(char *str, uint8_t status);
static void     AirReportDataConfirm(void);
static void     AirReportDataIndication(void);

/* 15.4 device reference made global */
struct ca821x_dev g_deviceref;

/* scheduling tasklet for polling */
static ca_tasklet g_air_tx_indirect_tasklet;

/* switch 2 callback: enable/disable tx indirect */
static void sw2_callback(void *context)
{
	(void)context;

	/* toggle sleep flag */
	if (g_enable_indirect == true)
		g_enable_indirect = false;
	else
		g_enable_indirect = true;

	/* schedule next indirect tx */
	if (g_enable_indirect == true)
		TASKLET_ScheduleDelta(&g_air_tx_indirect_tasklet, (TXINDIRECT_PERIOD * 1000), NULL);

	xprintf("ROUTER: TX_INDIRECT %s\n", (g_enable_indirect ? "enabled" : "disabled"));
}

/* switch 4 callback: switching between aerial-adapter and serial */
static void sw4_callback(void *context)
{
	/* toggle between aerial-adapter and serial */
	if (g_use_aerial_adapter)
		g_use_aerial_adapter = false;
	else
		g_use_aerial_adapter = true;

	/* select printf function */
	if (g_use_aerial_adapter)
		xprintf = &aaprintf;
	else
		xprintf = &printf;

	/* reinitialise if required */
	if (g_use_aerial_adapter)
	{
		aainitialise(AIR_CHANNEL, &g_deviceref);
		xprintf("ROUTER: Using aaprintf()\n");
	}
	else
	{
		xprintf("ROUTER: Using printf()\n");
	}
}

/* device hardware initialisation for application */
static void hardware_init(void)
{
	/* Register the SW1 as {permanent LED to flag sleep mode */
	DVBD_RegisterLEDOutput(DEV_SWITCH_1, JUMPER_POS_1);
	DVBD_SetLED(DEV_SWITCH_1, LED_ON);

	/* Register SW2 enable/disable tx indirect */
	DVBD_RegisterButtonIRQInput(DEV_SWITCH_2, JUMPER_POS_1);
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_2, &sw2_callback, NULL, BTN_SHORTPRESS_RELEASED);

	/* Register SW4 switching between aerial-adapter and serial */
	DVBD_RegisterButtonIRQInput(DEV_SWITCH_4, JUMPER_POS_1);
	DVBD_SetButtonShortPressCallback(DEV_SWITCH_4, &sw4_callback, NULL, BTN_SHORTPRESS_RELEASED);
}

/* Device hardware polling function */
static void hardware_poll(void)
{
	/* check buttons */
	if (!DVBD_CanSleep())
		DVBD_PollButtons();

	if (gDataIndReceived)
		AirReportDataIndication();

	if (gDataCnfReceived)
		AirReportDataConfirm();
}

/* EVBME callback to re-initialise radio for OpenThread */
static int hardware_reinitialise(struct ca821x_dev *pDeviceRef)
{
	AirInitPIB(pDeviceRef);

	/* for OpenThread: */
	//otLinkSyncExternalMac(OT_INSTANCE);

	return 0;
}

/* initialise PIB */
static void AirInitPIB(struct ca821x_dev *pDeviceRef)
{
	/* standard pib */
	uint8_t  longaddress[8];
	uint8_t  channel      = MAC_CHANNEL;
	uint16_t panid        = MAC_PANID;
	uint16_t shortaddress = MAC_SHORTADD_ROUTER;
	uint8_t  edcsthr      = 0x80;
	uint8_t  rxon         = 1;
	uint16_t persisttime;

	memcpy(longaddress, (uint8_t[])MAC_LONGADD_ROUTER, 8);

	MLME_SET_request_sync(phyCurrentChannel, 0, 1, &channel, pDeviceRef);    // set 15.4 Channel
	MLME_SET_request_sync(nsIEEEAddress, 0, 8, longaddress, pDeviceRef);     // set extended (IEEE Address)
	MLME_SET_request_sync(macPANId, 0, 2, &panid, pDeviceRef);               // set local PANId
	MLME_SET_request_sync(macShortAddress, 0, 2, &shortaddress, pDeviceRef); // set local short address

	/* get rid of far away interference .. */
	HWME_SET_request_sync(HWME_EDTHRESHOLD, 1, &edcsthr, pDeviceRef);
	HWME_SET_request_sync(HWME_CSTHRESHOLD, 1, &edcsthr, pDeviceRef);

	/* initialise security */
#if (ENABLE_SECURITY)
	MLME_SET_request_sync(macSecurityEnabled, 0, 1, &sMacSecurityEnabled, pDeviceRef);       // MacSecurityEnabled
	MLME_SET_request_sync(macKeyTableEntries, 0, 1, &sMacKeyTableEntries, pDeviceRef);       // MacKeyTableEntries
	MLME_SET_request_sync(macKeyTable, 0, 52, sMacKeyTable0, pDeviceRef);                    // MacKeyTable
	MLME_SET_request_sync(macKeyTable, 1, 52, sMacKeyTable1, pDeviceRef);                    // MacKeyTable
	MLME_SET_request_sync(macDeviceTableEntries, 0, 1, &sMacDeviceTableEntries, pDeviceRef); // MacDeviceTableEntries
	MLME_SET_request_sync(macDeviceTable, 0, 17, sMacDeviceTable0, pDeviceRef);              // MacDeviceTable
	MLME_SET_request_sync(macDeviceTable, 1, 17, sMacDeviceTable1, pDeviceRef);              // MacDeviceTable
	MLME_SET_request_sync(macDefaultKeySource, 0, 8, sMacDefaultKeySource, pDeviceRef);      // MacDefaultKeySource
#endif

	/* increase macTransactionPersistenceTime to around 2x TXINDIRECT_PERIOD to be above wakeup period of sed */
	/* unit is 15.36 ms */
	persisttime = ((2000 * TXINDIRECT_PERIOD) / 16) & 0xFFFF;
	MLME_SET_request_sync(macTransactionPersistenceTime, 0, 2, &persisttime, pDeviceRef);

	/* switch receiver on */
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &rxon, pDeviceRef);
}

/* initialise rolling values in PIN (on startup only) */
static void AirInitPIBRollingValues(struct ca821x_dev *pDeviceRef)
{
	uint8_t framecounter[4] = {0, 0, 0, 1};
	uint8_t dsn[1]          = {0};

	MLME_SET_request_sync(macFrameCounter, 0, 4, framecounter, pDeviceRef);
	MLME_SET_request_sync(macDSN, 0, 1, dsn, pDeviceRef);
}

/* add indirect data packet */
static ca_error AirTransmitIndirect(void *aContext)
{
	uint8_t         status;
	struct FullAddr sed_f_addr;
	uint16_t        panid        = MAC_PANID;
	uint16_t        shortaddress = MAC_SHORTADD_SED;
	uint8_t         msdu[MSDULENGTH];
	static uint8_t  data0 = 0;
	uint8_t         txopt;
	struct SecSpec  sec_spec;

	/* schedule next indirect packet */
	if (g_enable_indirect == false)
		return CA_ERROR_SUCCESS;

	TASKLET_ScheduleDelta(&g_air_tx_indirect_tasklet, (TXINDIRECT_PERIOD * 1000), NULL);

	/* fill msdu */
	for (uint8_t i = 0; i < MSDULENGTH; ++i)
	{
		msdu[i] = (i + data0) & 0xFF;
	}

	/* router address */
	sed_f_addr.AddressMode = MAC_MODE_SHORT_ADDR;
	PUTLE16(panid, sed_f_addr.PANId);
	PUTLE16(shortaddress, sed_f_addr.Address);

	/* tx options */
	txopt = TXOPT_ACKREQ + TXOPT_INDIRECT;

	/* security spec */
#if (ENABLE_SECURITY)
	/* 4 = ENC */
	/* 5 = ENC-MIC-32 */
	sec_spec.SecurityLevel = 5;
	/* 0 = IMPLICIT */
	sec_spec.KeyIdMode = 0;
	memcpy(sec_spec.KeySource, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 8);
	sec_spec.KeyIndex = 0;
#else
	sec_spec.SecurityLevel = 0; // ENC 0x05 // ENC-MIC-32
#endif

	status = MCPS_DATA_request(MAC_MODE_SHORT_ADDR, // SrcAddrMode,
	                           sed_f_addr,          // DstAddr,
	                           MSDULENGTH,          // MsduLength,
	                           msdu,                // pMsdu,
	                           data0,               // MsduHandle,
	                           txopt,               // TxOptions,
	                           &sec_spec,           // pSecurity,
	                           &g_deviceref);

	++data0;

	if (status)
	{
		AirReportStatus("TX_REQ", status);
	}

	return CA_ERROR_SUCCESS;
}

/* callback for data confirm */
static ca_error AirProcessDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	gDataCnf.MsduHandle = params->MsduHandle;
	gDataCnf.Status     = params->Status;

	gDataCnfReceived = true;

	return CA_ERROR_SUCCESS;
}

/* callback for data indication */
static ca_error AirProcessDataIndication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	uint16_t shortadd, panid;

	/* check received packet */
	panid    = (uint16_t)(params->Src.PANId[1] << 8) + params->Src.PANId[0];
	shortadd = (uint16_t)(params->Src.Address[1] << 8) + params->Src.Address[0];

	/* filter out other devices */
	if ((panid != MAC_PANID) || (shortadd != MAC_SHORTADD_SED))
		return CA_ERROR_SUCCESS;

	gDataInd.Msdu[0] = params->Msdu[0];

	gDataIndReceived = true;

	return CA_ERROR_SUCCESS;
}

/* callback for comm status indication */
static ca_error AirProcessCommStatusIndication(struct MLME_COMM_STATUS_indication_pset *params,
                                               struct ca821x_dev                       *pDeviceRef)
{
	uint16_t panid;

	/* check received packet */
	panid = GETLE16(params->PANId);

	/* filter out other devices */
	if (panid != MAC_PANID)
		return CA_ERROR_SUCCESS;

	/* otherwise flag indication */
	return CA_ERROR_NOT_HANDLED;
}

/* register callbacks */
static void AirRegisterCallBacks(struct ca821x_dev *pDeviceRef)
{
	/* register callbacks */
	pDeviceRef->callbacks.MCPS_DATA_confirm           = &AirProcessDataConfirm;
	pDeviceRef->callbacks.MCPS_DATA_indication        = &AirProcessDataIndication;
	pDeviceRef->callbacks.MLME_COMM_STATUS_indication = &AirProcessCommStatusIndication;
}

/* report mac status */
static void AirReportStatus(char *str, uint8_t status)
{
	uint8_t *ststr[] = {"???", "CH_ACC_FAIL", "NO_ACK", "NO_DATA"};
	uint8_t  sel     = 0;

	if (status == MAC_CHANNEL_ACCESS_FAILURE)
		sel = 1;
	else if (status == MAC_NO_ACK)
		sel = 2;
	else if (status == MAC_NO_DATA)
		sel = 3;

	if (sel)
		xprintf("ROUTER: %s: %s\n", str, ststr[sel]);
	else
		xprintf("ROUTER: %s: ?? (0x%02X)\n", str, status);
}

/* report data confirm */
static void AirReportDataConfirm(void)
{
	if (gDataCnf.Status != MAC_SUCCESS)
		xprintf("ROUTER: TX_DATA: %02X - ST: %02X\n", gDataCnf.MsduHandle, gDataCnf.Status);
	else
		xprintf("ROUTER: TX_DATA: %02X\n", gDataCnf.MsduHandle);

	gDataCnfReceived = false;
}

/* report data indication */
static void AirReportDataIndication(void)
{
	xprintf("ROUTER: RX_DATA: %02X\n", gDataInd.Msdu[0]);

	gDataIndReceived = false;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Main Program Endless Loop
 *******************************************************************************
 * \return Does not return
 *******************************************************************************
 ******************************************************************************/
int main(void)
{
	ca821x_api_init(&g_deviceref);

	/* Initialisation of Chip and EVBME */
	/* Returns a Status of CA_ERROR_SUCCESS/CA_ERROR_FAIL for further Action */
	/* in case there is no UpStream Communications Channel available */
	EVBMEInitialise(CA_TARGET_NAME, &g_deviceref);
	/* EVBME callback to re-initialise radio for OpenThread */
	cascoda_reinitialise = hardware_reinitialise;

	/* Device hardware initialisation for application */
	hardware_init();

	if (g_use_aerial_adapter)
	{
		/* initialise aerial adapter */
		aainitialise(AIR_CHANNEL, &g_deviceref);
		xprintf = &aaprintf;
	}
	else
	{
		xprintf = &printf;
	}

	/* register callbacks */
	AirRegisterCallBacks(&g_deviceref);

	/* initialise MAC */
	AirInitPIB(&g_deviceref);
	AirInitPIBRollingValues(&g_deviceref);

	/* Initialise scheduling for indirect tx */
	TASKLET_Init(&g_air_tx_indirect_tasklet, &AirTransmitIndirect);

	/* Endless Polling Loop */
	while (1)
	{
		cascoda_io_handler(&g_deviceref);
		/* poll device (process buttons / sensors etc.) */
		hardware_poll();

	} /* while(1) */
}

#else // exclude CA8212

int main(void)
{
	while (1)
		;
}

#endif // exclude CA8212
