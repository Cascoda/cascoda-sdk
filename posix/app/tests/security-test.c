
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix-evbme.h"
#include "ca821x-posix/ca821x-posix.h"
#include "evbme_messages.h"

/* Colour codes for printf */
#ifndef NO_COLOR
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define BOLDWHITE "\033[1m\033[37m"
#define RESET "\x1b[0m"
#else
#define RED ""
#define GREEN ""
#define YELLOW ""
#define BLUE ""
#define MAGENTA ""
#define CYAN ""
#define BOLDWHITE ""
#define RESET ""
#endif

#define COLOR_SET(C, X) C X RESET

#define M_PANID 0x5ECC
#define M_MSDU_LENGTH 100
#define MAX_INSTANCES 5
#define TX_PERIOD ((struct timespec){0, 0})
#define CHANNEL 22
#define SWAP_COUNTDOWN 100

#define NEW_MODE 0

#if (CASCODA_CA_VER == 8210) && NEW_MODE
#error "CA8210 does not support new security mode."
#endif

struct M_KeyDescriptor_st
{
	struct M_KeyTableEntryFixed Fixed;
	struct M_KeyIdLookupDesc    KeyIdLookupList[1];
	struct M_KeyDeviceDesc      KeyDeviceList[1];
	struct M_KeyUsageDesc       KeyUsageList[1];
};

static uint8_t msdu[M_MSDU_LENGTH] = {1, 2, 3, 4, 5, 6, 7, 0};

uint8_t key1[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
uint8_t key2[] = {0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};

uint8_t addr1[] = {0xBE, 0xEF, 0xEA, 0x7E, 0x50, 0x57, 0xEA, 0x35};
uint8_t addr2[] = {0xFA, 0xCE, 0x0F, 0xFF, 0xFA, 0xCE, 0x17, 0x00};

uint16_t saddr1 = 0xBEEF;
uint16_t saddr2 = 0xFACE;

struct inst_priv
{
	struct ca821x_dev pDeviceRef;
	pthread_mutex_t   confirm_mutex;
	pthread_cond_t    confirm_cond;
	pthread_t         mWorker;
	uint8_t           confirm_done;
	uint16_t          mAddress;
	uint8_t           lastHandle;
	uint16_t          lastAddress;

	uint8_t incKeyIndex;

	struct SecSpec mSecSpec;
	unsigned int   mTx, mRx, mErr;
};

int              numInsts;
struct inst_priv insts[MAX_INSTANCES] = {};

pthread_mutex_t out_mutex = PTHREAD_MUTEX_INITIALIZER;

void initInst(struct inst_priv *cur);

static void quit(int sig)
{
	for (int i = 0; i < numInsts; i++)
	{
		pthread_cancel(insts[i].mWorker);
		pthread_join(insts[i].mWorker, NULL);
	}
	exit(0);
}

static ca_error driverErrorCallback(ca_error error, struct ca821x_dev *pDeviceRef)
{
	struct inst_priv *priv          = pDeviceRef->context;
	pthread_mutex_t * confirm_mutex = &(priv->confirm_mutex);
	pthread_cond_t *  confirm_cond  = &(priv->confirm_cond);

	printf(COLOR_SET(RED, "DRIVER FAILED FOR %x WITH ERROR %d") "\n\r", priv->mAddress, (int)error);
	printf(COLOR_SET(BLUE, "Attempting restart...") "\n\r");

	initInst(priv);

	printf(COLOR_SET(GREEN, "Restart successful!") "\n\r");

	pthread_mutex_lock(confirm_mutex);
	priv->confirm_done = 1;
	pthread_cond_broadcast(confirm_cond);
	pthread_mutex_unlock(confirm_mutex);

	return CA_ERROR_SUCCESS;
}

static ca_error handleEvbmeMessage(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef)
{
	fprintf(stderr, "IN: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

static ca_error handleDataIndication(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef) //Async
{
	struct inst_priv *priv = pDeviceRef->context;
	pthread_mutex_lock(&out_mutex);
	priv->mRx++;
	pthread_mutex_unlock(&out_mutex);

	return CA_ERROR_SUCCESS;
}

static ca_error handleDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef) //Async
{
	struct inst_priv *priv          = pDeviceRef->context;
	pthread_mutex_t * confirm_mutex = &(priv->confirm_mutex);
	pthread_cond_t *  confirm_cond  = &(priv->confirm_cond);

	//TDME_SETSFR_request_sync(0, 0xdb, 0x0A, pDeviceRef);

	if (params->Status == MAC_SUCCESS)
	{
		unsigned int count;
		pthread_mutex_lock(&out_mutex);
		priv->mTx++;
		count = priv->mTx;
		pthread_mutex_unlock(&out_mutex);

		if (count == SWAP_COUNTDOWN)
		{
			uint8_t zeros[4]        = {0};
			priv->mSecSpec.KeyIndex = 1;
			MLME_SET_request_sync(macFrameCounter, 0, 4, zeros, pDeviceRef);
		}
	}
	else
	{
		pthread_mutex_lock(&out_mutex);
		priv->mErr++;
		pthread_mutex_unlock(&out_mutex);
	}

	pthread_mutex_lock(confirm_mutex);
	if (params->MsduHandle == priv->lastHandle)
	{
		priv->confirm_done = 1;
		pthread_cond_broadcast(confirm_cond);
	}
	else
	{
		pthread_mutex_lock(&out_mutex);
		printf(COLOR_SET(RED, "Dev %x: Expected handle %x, got %x") "\r\n",
		       priv->mAddress,
		       priv->lastHandle,
		       params->MsduHandle);
		pthread_mutex_unlock(&out_mutex);
	}
	pthread_mutex_unlock(confirm_mutex);

	return CA_ERROR_SUCCESS;
}

static ca_error handleCommStatusIndication(struct MLME_COMM_STATUS_indication_pset *params,
                                           struct ca821x_dev *                      pDeviceRef)
{
	fprintf(stderr,
	        "COMM-STATUS.indication status %x, kID %d, kInd %d\n",
	        params->Status,
	        params->Security.KeyIdMode,
	        params->Security.KeyIndex);

	for (int i = 0; i < 2; i++)
	{
		struct M_DeviceDescriptor dd;
		uint8_t                   len;
		MLME_GET_request_sync(macDeviceTable, i, &len, &dd, pDeviceRef);

		fprintf(stderr, "DD: fc %x, shaddr %x\n", GETLE32(dd.FrameCounter), GETLE16(dd.ShortAddress));
	}

	struct M_KeyDescriptor_st kd = {0};

	for (int i = 0; i < 2; i++)
	{
		uint8_t len;
		MLME_GET_request_sync(macKeyTable, i, &len, &kd, pDeviceRef);
		printf("KLE%d KDE%d KUE%d KDL%x\n",
		       kd.Fixed.KeyIdLookupListEntries,
		       kd.Fixed.KeyDeviceListEntries,
		       kd.Fixed.KeyUsageListEntries,
		       kd.KeyDeviceList[0].Flags);
	}

	return CA_ERROR_SUCCESS;
}

static ca_error handleGenericDispatchFrame(const struct MAC_Message *buf, struct ca821x_dev *pDeviceRef) //Async
{
	/*
	 * This is a debugging function for unhandled incoming MAC data
	 */

	return CA_ERROR_NOT_HANDLED;
}

static void *inst_worker(void *arg)
{
	struct inst_priv * priv       = arg;
	struct ca821x_dev *pDeviceRef = &(priv->pDeviceRef);

	pthread_mutex_t *confirm_mutex = &(priv->confirm_mutex);
	pthread_cond_t * confirm_cond  = &(priv->confirm_cond);

	uint16_t i = 0;
	while (1)
	{
		struct FullAddr dest;

		do
		{
			i = (i + 1) % numInsts;
		} while (&insts[i] == priv);
		//wait for confirm & reset
		pthread_mutex_lock(confirm_mutex);
		while (!priv->confirm_done) pthread_cond_wait(confirm_cond, confirm_mutex);
		priv->confirm_done = 0;
		priv->lastHandle++;
		pthread_mutex_unlock(confirm_mutex);

		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		nanosleep(&TX_PERIOD, NULL);

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

		//fire
		dest.AddressMode = MAC_MODE_SHORT_ADDR;
		PUTLE16(insts[i].mAddress, dest.Address);
		PUTLE16(M_PANID, dest.PANId);
		pthread_mutex_lock(confirm_mutex);
		priv->lastAddress = insts[i].mAddress;
		pthread_mutex_unlock(confirm_mutex);
		//TDME_SETSFR_request_sync(0, 0xdb, 0x0E, pDeviceRef);
		//if(i == 1)
		MCPS_DATA_request(
		    MAC_MODE_SHORT_ADDR, dest, M_MSDU_LENGTH, msdu, priv->lastHandle, 0x01, &priv->mSecSpec, pDeviceRef);
	}
	return NULL;
}

void drawTableHeader()
{
	printf("|----|");
	for (int i = 0; i < numInsts; i++)
	{
		printf("|----|----|---|");
	}
	printf("\n");
	printf("|----|");
	for (int i = 0; i < numInsts; i++)
	{
		printf("|---" COLOR_SET(BOLDWHITE, "NODE %02d") "---|", i);
	}
	printf("\n");
	printf("|----|");
	for (int i = 0; i < numInsts; i++)
	{
		uint8_t len = 0;
		uint8_t leArr[2];
		if (MLME_GET_request_sync(macShortAddress, 0, &len, leArr, &insts[i].pDeviceRef))
		{
			leArr[0] = 0xAD;
			leArr[1] = 0xDE;
		}
		printf("|-ShAddr %04x-|", GETLE16(leArr));
	}
	printf("\n");
	printf("|TIME|");
	for (int i = 0; i < numInsts; i++)
	{
		printf("|" COLOR_SET(GREEN, "Tx  ") "|Rx  |" COLOR_SET(RED, "Err") "|");
	}
	printf("\n");
}

void drawTableRow(unsigned int time)
{
	printf("|%4d|", time);
	pthread_mutex_lock(&out_mutex);
	for (int i = 0; i < numInsts; i++)
	{
		printf(
		    "|" COLOR_SET(GREEN, "%4d") "|%4d|" COLOR_SET(RED, "%3d") "|", insts[i].mTx, insts[i].mRx, insts[i].mErr);
	}
	pthread_mutex_unlock(&out_mutex);
	printf("\n");
}

void initInst(struct inst_priv *cur)
{
	struct ca821x_dev *pDeviceRef = &(cur->pDeviceRef);

	//Reset the MAC to a default state
	MLME_RESET_request_sync(1, pDeviceRef);

	uint8_t disable = 0; //Disable low LQI rejection @ MAC Layer
	HWME_SET_request_sync(0x11, 1, &disable, pDeviceRef);

	//Set up MAC pib attributes
	uint8_t retries = 3; //Retry transmission 3 times if not acknowledged
	MLME_SET_request_sync(macMaxFrameRetries, 0, sizeof(retries), &retries, pDeviceRef);

	retries = 4; //max 4 CSMA backoffs
	MLME_SET_request_sync(macMaxCSMABackoffs, 0, sizeof(retries), &retries, pDeviceRef);

	uint8_t maxBE = 4; //max BackoffExponent 4
	MLME_SET_request_sync(macMaxBE, 0, sizeof(maxBE), &maxBE, pDeviceRef);

	uint8_t channel = CHANNEL;
	MLME_SET_request_sync(phyCurrentChannel, 0, sizeof(channel), &channel, pDeviceRef);

	uint8_t LEarray[2];
	LEarray[0] = LS0_BYTE(M_PANID);
	LEarray[1] = LS1_BYTE(M_PANID);
	MLME_SET_request_sync(macPANId, 0, 2, LEarray, pDeviceRef);

	LEarray[0] = LS0_BYTE(cur->mAddress);
	LEarray[1] = LS1_BYTE(cur->mAddress);
	MLME_SET_request_sync(macShortAddress, 0, sizeof(cur->mAddress), LEarray, pDeviceRef);

	if (cur == insts) //making for 1
		MLME_SET_request_sync(nsIEEEAddress, 0, 8, addr1, pDeviceRef);
	else
		MLME_SET_request_sync(nsIEEEAddress, 0, 8, addr2, pDeviceRef);

	uint8_t rxOnWhenIdle = 1;
	MLME_SET_request_sync( //enable Rx when Idle
	    macRxOnWhenIdle,
	    0,
	    sizeof(rxOnWhenIdle),
	    &rxOnWhenIdle,
	    pDeviceRef);

	uint8_t se = 1;
	MLME_SET_request_sync(macSecurityEnabled, 0, sizeof(se), &se, pDeviceRef);

	struct M_DeviceDescriptor dd = {0};

	PUTLE16(M_PANID, dd.PANId);
	if (cur != insts) //making for 1
	{
		memcpy(dd.ExtAddress, addr1, 8);
		PUTLE16(saddr1, dd.ShortAddress);
	}
	else
	{
		memcpy(dd.ExtAddress, addr2, 8);
		PUTLE16(saddr2, dd.ShortAddress);
	}

	LEarray[0] = 2;
	MLME_SET_request_sync(macDeviceTableEntries, 0, 1, LEarray, pDeviceRef);

	uint8_t dCount = NEW_MODE ? 1 : 2;

	for (int i = 0; i < dCount; i++)
	{
		MLME_SET_request_sync(macDeviceTable, i, sizeof(dd), &dd, pDeviceRef);
	}

	struct M_KeyDescriptor_st
	{
		struct M_KeyTableEntryFixed Fixed;
		struct M_KeyIdLookupDesc    KeyIdLookupList[1];
		struct M_KeyDeviceDesc      KeyDeviceList[1];
		struct M_KeyUsageDesc       KeyUsageList[1];
	} kd                            = {0};
	kd.Fixed.KeyIdLookupListEntries = 1;
	kd.Fixed.KeyDeviceListEntries   = 1;
	kd.Fixed.KeyUsageListEntries    = 1;
	kd.KeyUsageList[0].Flags        = MAC_FRAME_TYPE_DATA;
	memset(kd.KeyIdLookupList[0].LookupData, 0xFF, 9);
	kd.KeyIdLookupList[0].LookupDataSizeCode = 1;

	MLME_SET_request_sync(macKeyTableEntries, 0, 1, LEarray, pDeviceRef);

	for (int i = 0; i < 2; i++)
	{
		if (i == 0)
			memcpy(kd.Fixed.Key, key1, 16);
		else
			memcpy(kd.Fixed.Key, key2, 16);

		kd.KeyIdLookupList[0].LookupData[0] = i;
		kd.KeyDeviceList[0].Flags           = NEW_MODE ? 0 : i;

#if NEW_MODE
		if (i == 1)
			kd.KeyDeviceList[0].Flags |= KDD_NewMask;
#endif

		MLME_SET_request_sync(macKeyTable, i, sizeof(kd), &kd, pDeviceRef);
	}
}

int main(int argc, char *argv[])
{
	numInsts = 2;

	for (int i = 0; i < numInsts; i++)
	{
		struct inst_priv * cur        = &insts[i];
		struct ca821x_dev *pDeviceRef = &(cur->pDeviceRef);
		if (i == 0)
			cur->mAddress = saddr1;
		else
			cur->mAddress = saddr2;
		cur->confirm_done           = 1;
		cur->mSecSpec.KeyIdMode     = 1;
		cur->mSecSpec.KeyIndex      = 0;
		cur->mSecSpec.SecurityLevel = 0x05;

		pthread_mutex_init(&(cur->confirm_mutex), NULL);
		pthread_cond_init(&(cur->confirm_cond), NULL);

		while (ca821x_util_init(pDeviceRef, &driverErrorCallback, NULL))
		{
			sleep(1); //Wait while there isn't a device available to connect
		}
		pDeviceRef->context = cur;

		//Register callbacks for async messages
		pDeviceRef->callbacks.MCPS_DATA_indication                    = &handleDataIndication;
		pDeviceRef->callbacks.MCPS_DATA_confirm                       = &handleDataConfirm;
		pDeviceRef->callbacks.MLME_COMM_STATUS_indication             = &handleCommStatusIndication;
		pDeviceRef->callbacks.generic_dispatch                        = &handleGenericDispatchFrame;
		EVBME_GetCallbackStruct(pDeviceRef)->EVBME_MESSAGE_indication = &handleEvbmeMessage;
		ca821x_util_start_downstream_dispatch_worker();

		initInst(cur);
		printf("Initialised. %d\r\n", i);
	}

	for (int i = 0; i < numInsts; i++)
	{
		pthread_create(&(insts[i].mWorker), NULL, &inst_worker, &insts[i]);
	}

	signal(SIGINT, quit);

	//Draw the table onscreen every second
	unsigned int time = 0;
	while (1)
	{
		if ((time % 20) == 0)
			drawTableHeader();
		drawTableRow(time);
		sleep(1);
		time++;
	}

	return 0;
}
