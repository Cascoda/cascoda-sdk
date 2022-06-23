
#define _DEFAULT_SOURCE 1
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
#define BOLDWHITE "\x1b[1m\x1b[37m"
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

#define CHANNEL 22
#define M_PANID 0x1AAA
#define MAX_MSDU_LEN 100
#define MIN_MSDU_LEN 100
#define MAX_INSTANCES 5
#define TX_PERIOD ((struct timespec){0, getRand(6000000, 7000000)})
#define TO_BACKOFF ((struct timespec){0, 10000000})
#define WAIT_CONFIRM 0
#define ONE_DIRECTION 0
#define INSERT_SYNC (getRand(0, 0))
#define INDIRECT 1
#define INDIRECTJUNK 0
#define USELONGADDR (getRand(0, 0))
#define ACKREQ (getRand(1, 1))
#define NUMRETRIES 4

#define HISTORY_LENGTH 200
#define MSDU_HISTORY 100

#define M_MSDU_LENGTH (getRand(MIN_MSDU_LEN, MAX_MSDU_LEN))

static struct SecSpec sSecSpec = {0};

#define STATUS_RECEIVED (1 << 0)
#define STATUS_ACKNOWLEDGED (1 << 1)
#define STATUS_REPEATED (1 << 2)
#define STATUS_CONFIRMED (1 << 3)

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

	uint8_t mJunkInQueue[INDIRECTJUNK];

	uint32_t          mExpectedData[HISTORY_LENGTH];
	uint8_t           mExpectedStatus[HISTORY_LENGTH];
	struct inst_priv *mExpectedSource[HISTORY_LENGTH];
	size_t            mExpectedIndex;

	size_t  idIndex;
	uint8_t mMsduHandles[MSDU_HISTORY];
	size_t  prevExpectedId[MSDU_HISTORY];

	uint8_t msdu[MAX_MSDU_LEN];

	unsigned int mTx, mSourced, mRx, mAckRemote, mErr, mRestarts, mBadRx, mBadTx, mCAF, mNack, mRepeats, mMissed,
	    mUnexpected, mMissedAcked, mAckLost, mTO, mBackoff, mConfirmLost, mConfirmDup;
};

int              numInsts;
struct inst_priv insts[MAX_INSTANCES] = {};

pthread_mutex_t out_mutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rand_mutex = PTHREAD_MUTEX_INITIALIZER;

void initInst(struct inst_priv *cur);

static int getRand(int min, int max)
{
	int rval;

	pthread_mutex_lock(&rand_mutex);
	rval = (rand() % (max - min + 1)) + min;
	pthread_mutex_unlock(&rand_mutex);

	return rval;
}

static struct inst_priv *getInstFromAddr(uint16_t shaddr)
{
	for (int i = 0; i < numInsts; i++)
	{
		if (insts[i].mAddress == shaddr)
		{
			return &(insts[i]);
		}
	}
	return NULL;
}

static size_t addExpected(struct inst_priv *target, struct inst_priv *source, uint32_t payload)
{
	size_t *index = &target->mExpectedIndex;
	*index        = (*index + 1) % HISTORY_LENGTH;

	if (!(target->mExpectedStatus[*index] & STATUS_RECEIVED))
	{
		target->mMissed++;
		if (target->mExpectedStatus[*index] & STATUS_ACKNOWLEDGED)
			target->mMissedAcked++;
	}
	if (!(target->mExpectedStatus[*index] & STATUS_ACKNOWLEDGED))
	{
		if (target->mExpectedSource[*index] != NULL)
			target->mExpectedSource[*index]->mAckLost++;
	}
	if (!(target->mExpectedStatus[*index] & STATUS_CONFIRMED))
	{
		if (target->mExpectedSource[*index] != NULL)
			target->mExpectedSource[*index]->mConfirmLost++;
	}
	target->mExpectedStatus[*index] = 0;
	target->mExpectedData[*index]   = payload;
	target->mExpectedSource[*index] = source;

	return *index;
}

static void processAcked(struct inst_priv *target, size_t id)
{
	target->mExpectedStatus[id] |= STATUS_ACKNOWLEDGED;
}

static uint8_t processConfirmed(struct inst_priv *target, size_t id)
{
	if ((target->mExpectedStatus[id] & STATUS_CONFIRMED)) //This would be bad, double confirm or something
	{
		return 0;
	}
	target->mExpectedStatus[id] |= STATUS_CONFIRMED;
	return 1;
}

static void processReceived(struct inst_priv *target, uint32_t payload)
{
	for (size_t i = 0; i < HISTORY_LENGTH; i++)
	{
		if (target->mExpectedData[i] == payload)
		{
			if (target->mExpectedStatus[i] & STATUS_RECEIVED)
			{
				target->mRepeats++;
			}
			target->mExpectedStatus[i] |= STATUS_RECEIVED;
			return;
		}
	}
	fprintf(
	    stderr, "Unexpected payload: %x, last sent: %x\r\n", payload, target->mExpectedData[target->mExpectedIndex]);
	target->mUnexpected++;
}

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

	pthread_mutex_lock(&out_mutex);
	priv->mRestarts++;
	pthread_mutex_unlock(&out_mutex);

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
	struct inst_priv *other, *priv = pDeviceRef->context;
	pthread_mutex_lock(&out_mutex);
	priv->mRx++;
	processReceived(priv, GETLE32(params->Msdu));

	if (params->Msdu[params->MsduLength] != 0)
		fprintf(stderr, "Unexpected security level!");

	pthread_mutex_unlock(&out_mutex);

	if ((other = getInstFromAddr(GETLE16(params->Src.Address))) != NULL)
	{
		pthread_mutex_lock(&out_mutex);
		other->mSourced++;
		pthread_mutex_unlock(&out_mutex);
	}

	return CA_ERROR_SUCCESS;
}

static void fillIndirectJunk(struct inst_priv *priv)
{
	for (int i = 0; i < INDIRECTJUNK; i++)
	{
		if (priv->mJunkInQueue[i] == 0)
		{
			uint8_t curAddrMode;
			if (USELONGADDR)
				curAddrMode = MAC_MODE_LONG_ADDR;
			else
				curAddrMode = MAC_MODE_SHORT_ADDR;

			struct FullAddr dest;
			PUTLE16(M_PANID, dest.PANId);
			PUTLE16(0xDEAD, dest.Address);
			dest.AddressMode = curAddrMode;

			MCPS_DATA_request(curAddrMode, dest, M_MSDU_LENGTH, priv->msdu, i, 0x05, &sSecSpec, &(priv->pDeviceRef));
			priv->mJunkInQueue[i] = 1;
		}
	}
}

static ca_error handleDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef) //Async
{
	struct inst_priv *other, *priv = pDeviceRef->context;
	pthread_mutex_t * confirm_mutex = &(priv->confirm_mutex);
	pthread_cond_t *  confirm_cond  = &(priv->confirm_cond);
	uint16_t          dstAddr;

	//Catch expiring junk
	if (params->MsduHandle < INDIRECTJUNK)
	{
		priv->mJunkInQueue[params->MsduHandle] = 0;
		return CA_ERROR_SUCCESS;
	}

	pthread_mutex_lock(confirm_mutex);
	dstAddr = priv->lastAddress;
	pthread_mutex_unlock(confirm_mutex);

	if ((other = getInstFromAddr(dstAddr)) != NULL)
	{
		pthread_mutex_lock(&out_mutex);
		for (int i = 0; i < MSDU_HISTORY; i++)
		{
			if (priv->mMsduHandles[i] == params->MsduHandle)
			{
				if (!processConfirmed(other, priv->prevExpectedId[i]))
					priv->mConfirmDup++;
				if (params->Status == MAC_SUCCESS)
					processAcked(other, priv->prevExpectedId[i]);
			}
		}
		if (params->Status == MAC_SUCCESS)
			other->mAckRemote++;
		pthread_mutex_unlock(&out_mutex);
	}

	switch (params->Status)
	{
	case MAC_SUCCESS:
		pthread_mutex_lock(&out_mutex);
		priv->mTx++;
		pthread_mutex_unlock(&out_mutex);
		break;

	case MAC_CHANNEL_ACCESS_FAILURE:
		pthread_mutex_lock(&out_mutex);
		priv->mCAF++;
		priv->mErr++;
		pthread_mutex_unlock(&out_mutex);
		break;

	case MAC_TRANSACTION_OVERFLOW:
		pthread_mutex_lock(&out_mutex);
		priv->mTO++;
		priv->mBackoff = 1;
		priv->mErr++;
		pthread_mutex_unlock(&out_mutex);
		break;

	case MAC_NO_ACK:
		pthread_mutex_lock(&out_mutex);
		priv->mNack++;
		priv->mErr++;
		pthread_mutex_unlock(&out_mutex);
		break;
	case MAC_SYSTEM_ERROR:
		printf("SystemError");
		//Fall through
	default:
		pthread_mutex_lock(&out_mutex);
		priv->mErr++;
		pthread_mutex_unlock(&out_mutex);
		break;
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
		//printf(COLOR_SET(RED, "Dev %x: Expected handle %x, got %x") "\r\n", priv->mAddress, priv->lastHandle, params->MsduHandle);
		pthread_mutex_unlock(&out_mutex);
	}
	pthread_mutex_unlock(confirm_mutex);

	return CA_ERROR_SUCCESS;
}

static ca_error handleGenericDispatchFrame(const struct MAC_Message *msg, struct ca821x_dev *pDeviceRef) //Async
{
	struct inst_priv *priv = pDeviceRef->context;
	/*
	 * This is a debugging function for unhandled incoming MAC data
	 */
	fprintf(stderr, "%x: Unexpected command 0x%02x\r\n", priv->mAddress, msg->CommandId);

	for (int i = 0; i < msg->Length; i++)
	{
		fprintf(stderr, " %02x", msg->PData.Payload);
	}
	fprintf(stderr, "\r\n");

	return CA_ERROR_SUCCESS;
}

static void *inst_worker(void *arg)
{
	struct inst_priv * priv       = arg;
	struct ca821x_dev *pDeviceRef = &(priv->pDeviceRef);
	uint32_t           payload;

	pthread_mutex_t *confirm_mutex = &(priv->confirm_mutex);
	pthread_cond_t * confirm_cond  = &(priv->confirm_cond);

	payload = getRand(0, 0x7FFFFFFF);

	uint16_t i = 0;
	while (1)
	{
		struct FullAddr dest   = {0};
		uint8_t         txOpts = 0;
		uint8_t         curAddrMode;

		if (ACKREQ)
			txOpts |= 0x01;

		if (INDIRECT && !i && getRand(0, 5))
			txOpts |= 0x04;

		if (USELONGADDR)
			curAddrMode = MAC_MODE_LONG_ADDR;
		else
			curAddrMode = MAC_MODE_SHORT_ADDR;

		payload++;

		do
		{
			i = (i + 1) % numInsts;
		} while (&insts[i] == priv);
		//wait for confirm & reset
		pthread_mutex_lock(confirm_mutex);
		while (!priv->confirm_done) pthread_cond_wait(confirm_cond, confirm_mutex);
		if (WAIT_CONFIRM)
			priv->confirm_done = 0;

		priv->lastHandle++;
		if (priv->lastHandle < INDIRECTJUNK)
			priv->lastHandle = INDIRECTJUNK; //Reserve lowest handles for indirect junk
		pthread_mutex_unlock(confirm_mutex);

		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		nanosleep(&TX_PERIOD, NULL);

		if (INSERT_SYNC)
		{
			uint8_t len;
			uint8_t leArr[2];
			MLME_GET_request_sync(macShortAddress, 0, &len, leArr, pDeviceRef);
		}

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		if (ONE_DIRECTION)
		{
			if (i)
				continue;
		}

		if (INDIRECT && i)
		{
			struct FullAddr fa = {0};
			PUTLE16(M_PANID, fa.PANId);
			PUTLE16(insts[i].mAddress, fa.Address);
			fa.AddressMode = curAddrMode;
#if CASCODA_CA_VER == 8210
			uint8_t interval[2] = {0, 0};
			MLME_POLL_request_sync(fa, interval, &sSecSpec, pDeviceRef);
#else
			MLME_POLL_request_sync(fa, &sSecSpec, pDeviceRef);
#endif
			continue;
		}

		pthread_mutex_lock(&out_mutex);
		priv->mMsduHandles[priv->idIndex]   = priv->lastHandle;
		priv->prevExpectedId[priv->idIndex] = addExpected(&(insts[i]), priv, payload);
		priv->idIndex                       = (priv->idIndex + 1) % MSDU_HISTORY;
		if (priv->mBackoff)
		{
			priv->mBackoff = 0;
			pthread_mutex_unlock(&out_mutex);
			nanosleep(&TO_BACKOFF, NULL);
		}
		else
		{
			pthread_mutex_unlock(&out_mutex);
		}

		//fire
		PUTLE16(M_PANID, dest.PANId);
		PUTLE16(insts[i].mAddress, dest.Address);
		dest.AddressMode = curAddrMode;

		pthread_mutex_lock(confirm_mutex);
		priv->lastAddress = insts[i].mAddress;
		pthread_mutex_unlock(confirm_mutex);
		PUTLE32(payload, priv->msdu);
		MCPS_DATA_request(
		    curAddrMode, dest, M_MSDU_LENGTH, priv->msdu, priv->lastHandle, txOpts, &sSecSpec, pDeviceRef);

		fillIndirectJunk(priv);
	}
	return NULL;
}

void drawTableHeader()
{
	printf("|----|");
	for (int i = 0; i < numInsts; i++)
	{
		printf("|----|----|----|----|---|---|---|---|");
	}
	printf("\n");
	printf("Digest of statistics:\n");
	for (int i = 0; i < numInsts; i++)
	{
		pthread_mutex_lock(&out_mutex);
		printf("Node %d:\n\treceived %d repeated frames"
		       "\n\tmissed %d packets (of which %d were acked!)"
		       "\n\treceived %d unknown payloads"
		       "\n\tencountered %d Channel Access Failures"
		       "\n\tsent %d packets that weren't acknowledged (but %d made it through anyway)"
		       "\n\tTriggered %d Transaction Overflows"
		       "\n\tLost %d Confirms, got %d duplicates\n",
		       i,
		       insts[i].mRepeats,
		       insts[i].mMissed,
		       insts[i].mMissedAcked,
		       insts[i].mUnexpected,
		       insts[i].mCAF,
		       insts[i].mNack,
		       insts[i].mAckLost,
		       insts[i].mTO,
		       insts[i].mConfirmLost,
		       insts[i].mConfirmDup);
		pthread_mutex_unlock(&out_mutex);
	}
	printf("|----|");
	for (int i = 0; i < numInsts; i++)
	{
		printf("|----|----|----|----|---|---|---|---|");
	}
	printf("\n");
	printf("|----|");
	for (int i = 0; i < numInsts; i++)
	{
		printf("|--------------" COLOR_SET(BOLDWHITE, "NODE %02d") "--------------|", i);
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
		printf("|------------ShAddr %04x------------|", GETLE16(leArr));
	}
	printf("\n");
	printf("|TIME|");
	for (int i = 0; i < numInsts; i++)
	{
		printf("|" COLOR_SET(GREEN, "Tx  ") "|Srcd|Rx  |AckR|" COLOR_SET(RED, "Err|eRx|eTx|Rst") "|");
	}
	printf("\n");
}

void drawTableRow(unsigned int time)
{
	printf("|%4d|", time);
	pthread_mutex_lock(&out_mutex);
	for (int i = 0; i < numInsts; i++)
	{
		printf("|" COLOR_SET(GREEN, "%4d") "|%4d|%4d|%4d|" COLOR_SET(RED, "%3d|%3d|%3d|%3d") "|",
		       insts[i].mTx,
		       insts[i].mSourced,
		       insts[i].mRx,
		       insts[i].mAckRemote,
		       insts[i].mErr,
		       insts[i].mBadRx,
		       insts[i].mBadTx,
		       insts[i].mRestarts);
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
	uint8_t retries = 4; //Retry transmission 3 times if not acknowledged
	MLME_SET_request_sync(macMaxFrameRetries, 0, sizeof(retries), &retries, pDeviceRef);

	retries = NUMRETRIES; //max 4 CSMA backoffs
	MLME_SET_request_sync(macMaxCSMABackoffs, 0, sizeof(retries), &retries, pDeviceRef);

	uint8_t maxBE = 3; //max BackoffExponent 4
	MLME_SET_request_sync(macMaxBE, 0, sizeof(maxBE), &maxBE, pDeviceRef);

	uint8_t minBE = 1;
	MLME_SET_request_sync(macMinBE, 0, sizeof(minBE), &minBE, pDeviceRef);

	uint8_t channel = CHANNEL;
	MLME_SET_request_sync(phyCurrentChannel, 0, sizeof(channel), &channel, pDeviceRef);

	uint8_t LEarray[8] = {0};
	LEarray[0]         = LS0_BYTE(M_PANID);
	LEarray[1]         = LS1_BYTE(M_PANID);
	MLME_SET_request_sync(macPANId, 0, 2, LEarray, pDeviceRef);

	LEarray[0] = LS0_BYTE(cur->mAddress);
	LEarray[1] = LS1_BYTE(cur->mAddress);

	MLME_SET_request_sync(nsIEEEAddress, 0, 8, LEarray, pDeviceRef);

	MLME_SET_request_sync(macShortAddress, 0, sizeof(cur->mAddress), LEarray, pDeviceRef);

	uint8_t rxOnWhenIdle = 1;
	//if (INDIRECT && (cur == insts))
	//	rxOnWhenIdle = 0;
	MLME_SET_request_sync( //enable Rx when Idle
	    macRxOnWhenIdle,
	    0,
	    sizeof(rxOnWhenIdle),
	    &rxOnWhenIdle,
	    pDeviceRef);
}

int main(int argc, char *argv[])
{
	if (argc <= 2)
		return -1;
	numInsts = argc - 1;

	time_t t;
	srand((unsigned)time(&t));

	if (argc - 1 > MAX_INSTANCES)
	{
		printf("Please increase MAX_INSTANCES in main.c");
		return -1;
	}
	for (int i = 0; i < numInsts; i++)
	{
		struct inst_priv * cur        = &insts[i];
		struct ca821x_dev *pDeviceRef = &(cur->pDeviceRef);
		cur->mAddress                 = atoi(argv[i + 1]);
		cur->confirm_done             = 1;
		memset(cur->mExpectedStatus,
		       STATUS_RECEIVED | STATUS_ACKNOWLEDGED | STATUS_CONFIRMED,
		       sizeof(cur->mExpectedStatus));

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
