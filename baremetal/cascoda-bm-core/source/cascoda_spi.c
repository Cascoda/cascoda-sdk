/*
 *  Copyright (c) 2019, Cascoda Ltd.
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
/**
 * @file
 * @brief  SPI Communication Driver Functions
 */
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#ifndef SPI_RX_FIFO_SIZE
#define SPI_RX_FIFO_SIZE 7
#endif

#ifndef SPI_RX_FIFO_RESV
#define SPI_RX_FIFO_RESV 4
#endif

#if !(SPI_RX_FIFO_RESV < SPI_RX_FIFO_SIZE)
#error "SPI_RX_FIFO_RESV must be less than SPI_RX_FIFO_SIZE"
#endif

/******************************************************************************/
/****** Global Variables for SPI Message Buffers                         ******/
/******************************************************************************/
/** Cyclic SPI receive message FIFO. Messages are read from the SPI and put into
 *  the next available index. */
struct MAC_Message SPI_Receive_Buffer[SPI_RX_FIFO_SIZE];
static volatile int         SPI_FIFO_Start = 0;
static volatile int         SPI_FIFO_End   = 0;
static volatile bool        SPI_FIFO_Full  = false;
static volatile uint32_t    waterMark      = 0; //!< Actual current fifo level
static volatile uint32_t    highWaterMark  = 0; //!< Actual worst fifo level

// Sync chaining system
static volatile bool syncChainActive   = false; //!< Sync chaining is currently active
static volatile bool syncChainInFlight = false; //!< A sync chain block message is currently in flight
//The sync chain message is an MLME_GET for MAC_ACK_WAIT_DURATION
static const uint8_t syncChainMessage[] = {0x45, 0x02, 0x40, 0x00};

/** Pointer to buffer to be used to store synchronous responses */
static struct MAC_Message *SPI_Wait_Buf;

/******************************************************************************/
/****** Static function declarations for cascoda_spi.c                   ******/
/******************************************************************************/
static ca_error            SPI_SyncWait(uint8_t cmdid);
static struct MAC_Message *SPI_GetFreeBuf();
static struct MAC_Message *getBuf(uint8_t cmdid);
static ca_error            SPI_WaitSlave(void);
static void                SPI_Error(ca_error errcode);

// declarations for secure functions
void SetSpiExternallyInUse(bool in_use);
bool GetSpiExternallyInUse();
void SetSpiExternalCompleteCallback(void (*new_cb)(void));
void CallSpiExternalCompleteCallback();

void SetExchangeInProgress(bool val);
bool IsExchangeInProgress();
void SetFinalExchange(bool val);
bool IsFinalExchange();

bool SPI_IsFifoFull()
{
	bool rval;

	BSP_DisableRFIRQ();
	rval = SPI_FIFO_Full;
	BSP_EnableRFIRQ();

	return rval;
}

bool SPI_IsFifoEmpty()
{
	bool rval;

	BSP_DisableRFIRQ();
	rval = (SPI_FIFO_Start == SPI_FIFO_End) && !SPI_FIFO_Full;
	BSP_EnableRFIRQ();

	return rval;
}

/**
 * Does the FIFO have a single buffer in use, and is that buffer currently being filled?
 *
 * RFIRQ must be enabled when calling this.
 * @return True if there is only a single FIFO buffer allocated AND it is not ready for reading.
 */
static bool SPI_IsSingleFifoBufInUse()
{
	bool rval;

	BSP_DisableRFIRQ();
	rval = (SPI_FIFO_End == ((SPI_FIFO_Start + 1) % SPI_RX_FIFO_SIZE)); //One item in the fifo
	rval = rval && IsExchangeInProgress();                              //Currently being filled.
	BSP_EnableRFIRQ();

	return rval;
}

bool SPI_IsFifoAlmostFull()
{
	//This deals with the case that we are receiving a sync response
	if (SPI_Wait_Buf)
		return false;
	//This deals with the case that the fifo is full (otherwise can't differentiate from empty)
	if (SPI_FIFO_Full)
		return true;
	/* Skipping the 'full' case that is handled above, check the reserved
	 * FIFO slots are all empty. */
	for (int i = 1; i <= SPI_RX_FIFO_RESV; i++)
	{
		if (SPI_FIFO_Start == ((SPI_FIFO_End + i) % SPI_RX_FIFO_SIZE))
			return true;
	}
	return false;
}

bool SPI_IsExchangeWithCA821xInProgress()
{
	return IsExchangeInProgress();
}

bool SPI_IsSyncChainInFlight()
{
	return syncChainInFlight;
}

void SPI_StartSyncChain(struct ca821x_dev *pDeviceRef)
{
	BSP_DisableRFIRQ();
	syncChainActive   = true;
	syncChainInFlight = true;

	SPI_Exchange((struct MAC_Message *)syncChainMessage, pDeviceRef);
	BSP_EnableRFIRQ();
}

void SPI_StopSyncChain(struct ca821x_dev *pDeviceRef)
{
	struct MAC_Message response;

	BSP_DisableRFIRQ();
	SPI_Wait_Buf            = &response;
	SPI_Wait_Buf->CommandId = SPI_IDLE;

	syncChainActive   = false;
	syncChainInFlight = false;

	//Kick the CA-821x to spit out the sync command
	SPI_Exchange(NULL, pDeviceRef);
	BSP_EnableRFIRQ();

	SPI_SyncWait(0x45);
}

static void SPI_IncrementWaterMark()
{
    waterMark++;

#ifdef CASCODA_SPI_BUF_TRACKING // NOTE: THIS IS A PRINT INSIDE AN INTERRUPT!
    printf("@ SBT alloc, used: %d/%d\n", waterMark, SPI_RX_FIFO_SIZE);
#endif
    
	if (waterMark > highWaterMark)
	{
		highWaterMark = waterMark;
		ca_log_debg("New SPI hwm: %d", highWaterMark);
	}
}

static void SPI_ExchangeBlocking(uint8_t *RxBuf, const uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen)
{
	u32_t               starttime = 0;
	SetExchangeInProgress(true);
	BSP_SPIExchange(RxBuf, TxBuf, RxLen, TxLen);
	starttime = BSP_ReadAbsoluteTime();
	while (IsExchangeInProgress())
	{
		BSP_ResetOnLoopStuck(starttime);
	}
}

/**
 *\brief Return an empty async buffer to be filled, or NULL if none available
 *
 * RFIRQ must be disabled when calling.
 */
static struct MAC_Message *SPI_GetFreeBuf()
{
	struct MAC_Message *rval = NULL;

	if (!SPI_FIFO_Full)
	{
		rval         = SPI_Receive_Buffer + SPI_FIFO_End;
		SPI_FIFO_End = (SPI_FIFO_End + 1) % SPI_RX_FIFO_SIZE;
		SPI_IncrementWaterMark();

		if (SPI_FIFO_Start == SPI_FIFO_End)
			SPI_FIFO_Full = true;
	}

	return rval;
}

struct MAC_Message *SPI_PeekFullBuf()
{
	struct MAC_Message *rval = NULL;

	if (!SPI_IsFifoEmpty() && !SPI_IsSingleFifoBufInUse())
	{
		rval = SPI_Receive_Buffer + SPI_FIFO_Start;
	}

	return rval;
}

void SPI_DequeueFullBuf()
{
	if (!SPI_IsFifoEmpty())
	{
		BSP_DisableRFIRQ();
		SPI_FIFO_Start = (SPI_FIFO_Start + 1) % SPI_RX_FIFO_SIZE;
		SPI_FIFO_Full  = false;
		waterMark--;
#ifdef CASCODA_SPI_BUF_TRACKING
		printf("@ SBT dequeue, used: %d/%d\n", waterMark, SPI_RX_FIFO_SIZE);
#endif
		BSP_EnableRFIRQ();
	}
}

/**
 *\brief Get a buffer for Rx based on whether command is sync or not.
 *
 * RFIRQ must be disabled when calling.
 *
 *\retval pointer to buffer or NULL upon failure
 */
static struct MAC_Message *getBuf(uint8_t cmdid)
{
	struct MAC_Message *rval = NULL;

	//Not valid receive (IDLE or error)
	if ((cmdid & 0x80) || !(cmdid & SPI_S2M))
		goto exit;

	//Return appropriate buffer based on sync or not
	if (cmdid & SPI_SYN)
	{
		/* If the message being received is the sync chain message response,
		 * just discard it. Also move on the sync chain state. Otherwise, this
		 * is a valid sync chain response, and should be stored as such.*/
		if (syncChainInFlight)
		{
			rval              = NULL;
			syncChainInFlight = false;
		}
		else
		{
			rval = SPI_Wait_Buf;
		}
	}
	else
	{
		rval = SPI_GetFreeBuf();
	}

exit:
	if (rval)
		rval->CommandId = cmdid;
	return rval;
}

/**
 *\brief Wait until the slave is available to exchange SPI Data.
 *
 * RFIRQ must be disabled when calling.
 */
#if CASCODA_CA_VER >= 8211
static ca_error SPI_WaitSlave(void)
{
	u32_t T_Start, T_Now;

	// loop on spi access start until RFIRQ is low
	T_Start = TIME_ReadAbsoluteTime();
	do
	{
		T_Now = TIME_ReadAbsoluteTime();
		if (BSP_SenseRFIRQ())
		{
			// back-off wait
			BSP_WaitUs(SPI_T_BACKOFF);
			continue;
		}
		// start access
		BSP_SetRFSSBLow();
		BSP_WaitUs(SPI_T_CSHOLD);
		if (BSP_SenseRFIRQ())
		{
			// back-off wait
			BSP_SetRFSSBHigh();
			continue;
		}
		return CA_ERROR_SUCCESS;
	} while (T_Now - T_Start < SPI_T_TIMEOUT);

	ca_log_crit("SPI_WaitSlave timed out");
	return CA_ERROR_SPI_WAIT_TIMEOUT;
}
#elif CASCODA_CA_VER == 8210
static ca_error SPI_WaitSlave(void)
{
	BSP_WaitUs(SPI_T_HOLD_8210);
	return CA_ERROR_SUCCESS;
}
#else
#error "Need SPI_WaitSlave definition for CA_VER"
#endif // CASCODA_CA_VER >= 8211

/**
 *\brief Exchange the first 2 bytes of a transaction (CommandId and Length)
 *
 * On the CA-8210, this can time out with NACKs, but NACKs were removed from the
 * CA-8211.
 *
 * RFIRQ must be disabled when calling.
 */
#if CASCODA_CA_VER >= 8211
static ca_error SPI_GetFirst2Bytes(u8_t *ReadBytes, const struct MAC_Message *pTxBuffer)
{
	uint8_t txBytes[] = {0xFF, 0xFF};
	if (pTxBuffer)
	{
		txBytes[0] = pTxBuffer->CommandId;
		txBytes[1] = pTxBuffer->Length;
	}

	SPI_ExchangeBlocking(ReadBytes, txBytes, 2, 2);

	return CA_ERROR_SUCCESS;
}
#elif CASCODA_CA_VER == 8210
static ca_error SPI_GetFirst2Bytes(u8_t *ReadBytes, const struct MAC_Message *pTxBuffer)
{
	u32_t   T_Start, T_Now = TIME_ReadAbsoluteTime();
	uint8_t txBytes[] = {0xFF, 0xFF};

	if (pTxBuffer)
	{
		txBytes[0] = pTxBuffer->CommandId;
		txBytes[1] = pTxBuffer->Length;
	}

	T_Start = T_Now;
	do
	{
		BSP_SetRFSSBLow();

		SPI_ExchangeBlocking(ReadBytes, txBytes, 2, 2);

		if (ReadBytes[1] != SPI_NACK)
			break;

		BSP_SetRFSSBHigh();
		BSP_WaitUs(SPI_T_BACKOFF);
		T_Now = TIME_ReadAbsoluteTime();

	} while (T_Now - T_Start < SPI_T_TIMEOUT);
	if (T_Now - T_Start >= SPI_T_TIMEOUT)
		return CA_ERROR_SPI_NACK_TIMEOUT;

	return CA_ERROR_SUCCESS;
}
#else
#error "Need SPI_GetFirst2Bytes definition for CA_VER"
#endif // CASCODA_CA_VER >= 8211

#if CASCODA_CA_VER == 8210
static u8_t SPI_CA8210_Align(u8_t *ReadBytes, const struct MAC_Message *pTxBuffer)
{
	u8_t rval = 0;

	if ((ReadBytes[0] & 0x80) && !(ReadBytes[1] & 0x80))
	{
		//Misaligned (First byte nack, second is not)
		u8_t OutByte = 0xFF;

		if (pTxBuffer)
			OutByte = pTxBuffer->PData.Payload[0];
		ReadBytes[0] = ReadBytes[1];
		SPI_ExchangeBlocking(ReadBytes + 1, &OutByte, 1, 1);
		rval = 1;
	}

	return rval;
}
#else
static u8_t SPI_CA8210_Align(u8_t *ReadBytes, const struct MAC_Message *pTxBuffer)
{
	(void)ReadBytes;
	(void)pTxBuffer;
	return 0;
}
#endif // CASCODA_CA_VER == 8210

ca_error SPI_Exchange(const struct MAC_Message *pTxBuffer, struct ca821x_dev *pDeviceRef)
{
	u8_t                TxLen = 0, RxLen = 0, AlignMod = 0;
	bool                setSyncChain = false;
	u8_t                triageBuf[2] = {0xFF, 0xFF}; //For the first 2 bytes of transfer
	struct MAC_Message *pRxBuffer;
	ca_error            error = CA_ERROR_SUCCESS;
	u32_t               starttime = 0;
	(void)pDeviceRef;

    // To understand why "SPI_Wait_Buf && !pTxBuffer" is equivalent to
    // checking if we are handling a sync response, please read this comment
    // in full, because some context and knowledge of the code as a whole is
    // required. NOTE: In the following explanation, the terms "request" and 
    // "response" can be understood as "MOSI" and "MISO". This is not directly
    // related to the CA821X API terminology of "request, confirm, indication,
    // response".

    // 1. About SPI_Wait_Buf
    // This pointer is NULL, except when a sync request/response is in progress.
    // See the function "SPI_Send()": Before the call to "SPI_Exchange()", the
    // tx buffer's command ID is being checked. If it matches up with a SPI_SYNC
    // command type, then the SPI_Wait_Buf is set to point to where the response
    // should go.
    // The SPI_Wait_Buf pointer is only set back to NULL in the function "SPI_SyncWait()",
    // after the code execution exits from the while loop in which it is waiting for
    // an interrupt from the MAC, which is sending the RESPONSE back.
    // CONCLUSION ABOUT SPI_Wait_Buf:
    // - During SYNC REQUEST AND RESPONSE, it is NOT NULL
    // - Otherwise, it is NULL

    // 2. About pTxBuffer
    // pTxBuffer points to the buffer containing the data to be transmitted during
    // the exchange that occurs in this "SPI_Exchange()" function.
    // This is allowed to be NULL, in the case where an exchange doesn't care
    // about sending data, but only about reading data (from MISO, aka the MAC).
    // One such example where this occurs, is when we are reading data from the MAC,
    // such as when receiving a SYNCHRONOUS (or ASYNCHRONOUS) command.
    // The m2351 has an interrupt registered on a certain pin (ZIG), which the
    // MAC can trigger in order to show that it wants to send something to the m2351.
    // See "GPC_IRQHandler()", which is the interrupt in question. This function
    // calls "DISPATCH_ReadCA821X()", which calls "SPI_Exchange(NULLP, pDeviceRef)".
    // So, during a sync or async MISO message from the MAC, the "pTxBuffer" is always
    // set to NULL.
    // CONCLUSION ABOUT pTxBuffer
    // - Always NULL if exchange is initiated by the MAC (whether ASYNC or SYNC)
    // - Not NULL if exchange not initiated by the MAC

    // 3. Combining things together
    // - if (SPI_Wait_Buf != NULL) we know that we are currently either
    // doing a SYNCHRONOUS REQUEST, or a SYNCHRONOUS RESPONSE 
    // - if (pTxBuffer == NULL) we know that we are NOT doing a request (because
    // a request involves sending data); we know that we ARE doing a RESPONSE 
    // So SPI_WAIT_BUF && !pTxBuffer means that we are doing a SYNCHRONOUS RESPONSE

	bool is_sync_response = SPI_Wait_Buf && !pTxBuffer;

	/* If the receive fifo is full, and this isn't a sync response,
	 * then we can't safely do SPI exchange.*/
	if (SPI_FIFO_Full && !is_sync_response)
	{
		ca_log_warn("SPI_Exchange failed - No buffers");
		return CA_ERROR_NO_BUFFER;
	}

	starttime = BSP_ReadAbsoluteTime();
	//Wait for previous SPI exchange to complete...
	while (IsExchangeInProgress())
	{
		BSP_ResetOnLoopStuck(starttime);
	}

	BSP_SetRFSSBHigh();

	//Wait for slave or time out
	if ((error = SPI_WaitSlave()))
		return error;

	//Sync Chain if necessary
	if (!pTxBuffer && !syncChainInFlight && syncChainActive)
	{
		pTxBuffer    = (struct MAC_Message *)syncChainMessage;
		setSyncChain = true;
	}

	//get first 2 bytes (commandId and Length)
	if ((error = SPI_GetFirst2Bytes(triageBuf, pTxBuffer)))
		return error;

	AlignMod = SPI_CA8210_Align(triageBuf, pTxBuffer);

	if (pTxBuffer)
		TxLen = pTxBuffer->Length - AlignMod;
	pRxBuffer = getBuf(triageBuf[0]);

	if (!TxLen && !pRxBuffer)
	{
		BSP_SetRFSSBHigh();
		goto exit;
	}

	if (pRxBuffer)
	{
		pRxBuffer->Length = RxLen = triageBuf[1];
		RxLen -= AlignMod;
	}

	SetFinalExchange(true);
	if (TxLen != 0)
	{
		SPI_ExchangeBlocking(pRxBuffer ? pRxBuffer->PData.Payload : NULLP, pTxBuffer->PData.Payload, RxLen, TxLen);
	}
	else
	{
		SetExchangeInProgress(true);
		BSP_SPIExchange(pRxBuffer ? pRxBuffer->PData.Payload : NULLP, pTxBuffer->PData.Payload, RxLen, TxLen);
	}

	if (setSyncChain)
		syncChainInFlight = true;

exit:
	return error;
}

/**
 * \brief Wait for synchronous command response over SPI
 *
 * This function waits until a synchronous command response is received over SPI
 *
 * RFIRQ must be enabled when calling.
 *
 * \param cmdid - The command id of the synchronous request
 *
 * \return  0 or evbme_error_code
 *
 */
static ca_error SPI_SyncWait(uint8_t cmdid)
{
	ca_error status     = CA_ERROR_SPI_WAIT_TIMEOUT;
	int      startticks = TIME_ReadAbsoluteTime();
	uint8_t  rspid      = ca821x_get_sync_response_id(cmdid);

	if (!rspid)
		return CA_ERROR_INVALID_ARGS;

	do
	{
		//TODO: This is perhaps a bit aggressive, as we don't need to wait for every exchange to finish, just the sync one.
		if (SPI_Wait_Buf->CommandId == rspid && !SPI_IsExchangeWithCA821xInProgress())
		{
			status = CA_ERROR_SUCCESS;
			break;
		}
		WAIT_ms(1);
	} while ((TIME_ReadAbsoluteTime() - startticks) < SPI_T_TIMEOUT);

	BSP_DisableRFIRQ();
	SPI_Wait_Buf = NULLP;
	BSP_EnableRFIRQ();

	if (status != CA_ERROR_SUCCESS) {
		ca_log_warn("SPI_SyncWait failed with error %u, rspid = %u", status, rspid);
	}

	return status;
}

ca_error SPI_Send(const uint8_t *buf, u8_t *response, struct ca821x_dev *pDeviceRef)
{
	ca_error Status = CA_ERROR_SUCCESS;
	bool     sync   = (buf[0] & SPI_SYN) && (buf[0] != SPI_IDLE);

	BSP_DisableRFIRQ();

	BSP_SetSPIMOSIOutput();

	if (sync)
	{
		SPI_Wait_Buf            = (struct MAC_Message *)response;
		SPI_Wait_Buf->CommandId = SPI_IDLE;
	}

	Status = SPI_Exchange((const struct MAC_Message *)buf, pDeviceRef); // tx packet

	if (Status)
	{
		if (sync)
			SPI_Wait_Buf = NULL;
		BSP_EnableRFIRQ();
		goto exit;
	}

	BSP_SetSPIMOSITristate();

	BSP_EnableRFIRQ();

	if (sync)
	{
		Status = SPI_SyncWait(buf[0]);
	}

exit:
	if (Status)
		SPI_Error(Status);
	return Status;
}

/**
 * \brief SPI Error Handling
 *
 * \param errcode - Error code
 *
 */
static void SPI_Error(ca_error errcode)
{
	BSP_SetRFSSBHigh();
	ca_log_crit("SPI Error %s", ca_error_str(errcode));
	// If we are missing a synchronous response from the MAC, that is an unrecoverable
	// error, so all we can do is reboot. First we send a reboot command upstream, so that
	// the application controlling this Chili knows it needs to reinitialize. Then, we
	// perform the actual reboot of the Chili once we are sure the command is received.
	if (errcode == CA_ERROR_SPI_WAIT_TIMEOUT)
	{
		ca_log_crit("Rebooting Chili & sending upstream reboot command!");
		uint8_t message[] = {DFU_REBOOT, 0x0};
		MAC_Message(EVBME_DFU_CMD, sizeof(message), message);
		WAIT_ms(50);
		BSP_SystemReset(SYSRESET_APROM);
	}
}

void SPI_ExchangeComplete()
{
#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	if (GetSpiExternallyInUse())
	{
		CallSpiExternalCompleteCallback();
		return;
	}
#endif
	if (IsFinalExchange())
	{
		BSP_SetRFSSBHigh(); // end access
		SetFinalExchange(false);
	}
	SetExchangeInProgress(false);
}

void SPI_Initialise(void)
{
	BSP_SPIInit();
	memset(SPI_Receive_Buffer, SPI_IDLE, sizeof(SPI_Receive_Buffer));
	SPI_FIFO_Start = 0;
	SPI_FIFO_End   = 0;
	BSP_EnableRFIRQ();
}

bool SPI_GetExternallyInUseStatus(void)
{
#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
	return GetSpiExternallyInUse();
#else
	return false;
#endif
}

#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
ca_error SPI_SetExternallyInUseStatus(bool status, void (*callback)(void))
{
	if (!status)
	{
		SetSpiExternallyInUse(status);
	}
	else
	{
		BSP_DisableRFIRQ();

		if (SPI_IsExchangeWithCA821xInProgress())
		{
			BSP_EnableRFIRQ();
			return CA_ERROR_BUSY;
		}

		SetSpiExternallyInUse(status);

		SetSpiExternalCompleteCallback(callback);

		BSP_EnableRFIRQ();
	}

	return CA_ERROR_SUCCESS;
}
#endif
