/**
 * @file
 * @brief  SPI Communication Driver Functions
 */
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
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_interface_core.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#define MAX(x, y) (x > y ? x : y)

/******************************************************************************/
/****** Global Variables for SPI Message Buffers                         ******/
/******************************************************************************/
/** Cyclic SPI receive message FIFO. Messages are read from the SPI and put into
 *  the next available index. */
struct MAC_Message       SPI_Receive_Buffer[SPI_RX_FIFO_SIZE];
static int               SPI_FIFO_Start = 0;
static int               SPI_FIFO_End   = 0;
static bool              SPI_FIFO_Full  = false;
static uint32_t          waterMark      = 0; //!< Actual current fifo level
static volatile uint32_t highWaterMark  = 0; //!< Actual worst fifo level

// Sync chaining system
static volatile bool syncChainActive   = 0; //!< Sync chaining is currently active
static volatile bool syncChainInFlight = 0; //!< A sync chain block message is currently in flight
//The sync chain message is an MLME_GET for MAC_ACK_WAIT_DURATION
static const uint8_t syncChainMessage[] = {0x45, 0x02, 0x40, 0x00};

/** Pointer to buffer to be used to store synchronous responses */
static struct MAC_Message *SPI_Wait_Buf;

/** Flag to prevent messages being sent while the system is scanning */
u8_t SPI_MLME_SCANNING = 0;

/******************************************************************************/
/****** Static function declarations for cascoda_spi.c                   ******/
/******************************************************************************/
static ca_error            SPI_SyncWait(uint8_t cmdid);
static struct MAC_Message *SPI_GetFreeBuf();
static struct MAC_Message *getBuf(uint8_t cmdid);
static ca_error            SPI_WaitSlave(void);
static void                SPI_BulkExchange(uint8_t *RxBuf, const uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen);
static void                SPI_Error(ca_error errcode);

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

bool SPI_IsFifoAlmostFull()
{
	//This deals with the case that we are receiving a sync response
	if (SPI_Wait_Buf)
		return SPI_FIFO_Full;
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
	if (++waterMark > highWaterMark)
	{
		highWaterMark = waterMark;
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

	if (!SPI_IsFifoEmpty())
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
#if CASCODA_CA_VER == 8211
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
#endif

/**
 *\brief Bulk transfer an SPI chunk of data, filling with IDLE to equalize channel length
 *
 * RFIRQ must be disabled when calling.
 */
static void SPI_BulkExchange(uint8_t *RxBuf, const uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen)
{
	int     TxDataLeft, RxDataLeft;
	uint8_t junk;

	//Transmit & Receive command payloads asynchronously using transmit and receive buffers
	TxDataLeft = TxLen;                //Total amount of valid data to send
	RxDataLeft = RxLen;                //Total amount of valid data to receive
	TxLen = RxLen = MAX(RxLen, TxLen); //Total number of byte transactions
	for (u8_t i = 0, j = 0; (TxLen != 0) || (RxLen != 0);)
	{
		if (TxDataLeft)
		{
			if (BSP_SPIPushByte(TxBuf[i]))
			{
				i++;
				TxLen--;
				TxDataLeft--;
			}
		}
		else if (TxLen && BSP_SPIPushByte(SPI_IDLE))
		{
			TxLen--;
		}

		if (RxDataLeft)
		{
			if (BSP_SPIPopByte(RxBuf + j))
			{
				j++;
				RxLen--;
				RxDataLeft--;
			}
		}
		else if (RxLen && BSP_SPIPopByte(&junk))
		{
			RxLen--;
		}
	}
}

/**
 *\brief Exchange the first 2 bytes of a transaction (CommandId and Length)
 *
 * On the CA-8210, this can time out with NACKs, but NACKs were removed from the
 * CA-8211.
 *
 * RFIRQ must be disabled when calling.
 */
#if CASCODA_CA_VER == 8211
static ca_error SPI_GetFirst2Bytes(u8_t *ReadBytes, const struct MAC_Message *pTxBuffer)
{
	if (pTxBuffer)
	{
		ReadBytes[0] = pTxBuffer->CommandId;
		ReadBytes[1] = pTxBuffer->Length;
	}

	ReadBytes[0] = BSP_SPIExchangeByte(ReadBytes[0]);
	ReadBytes[1] = BSP_SPIExchangeByte(ReadBytes[1]);

	return CA_ERROR_SUCCESS;
}
#elif CASCODA_CA_VER == 8210
static ca_error SPI_GetFirst2Bytes(u8_t *ReadBytes, const struct MAC_Message *pTxBuffer)
{
	u32_t T_Start, T_Now = TIME_ReadAbsoluteTime();
	T_Start = T_Now;
	do
	{
		BSP_SetRFSSBLow();
		if (pTxBuffer)
		{
			ReadBytes[0] = pTxBuffer->CommandId;
			ReadBytes[1] = pTxBuffer->Length;
		}

		ReadBytes[0] = BSP_SPIExchangeByte(ReadBytes[0]);
		ReadBytes[1] = BSP_SPIExchangeByte(ReadBytes[1]);

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
#endif

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
		ReadBytes[1] = BSP_SPIExchangeByte(OutByte);
		rval         = 1;
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
#endif

ca_error SPI_Exchange(const struct MAC_Message *pTxBuffer, struct ca821x_dev *pDeviceRef)
{
	u8_t                TxLen = 0, RxLen = 0, AlignMod = 0;
	bool                setSyncChain = false;
	u8_t                triageBuf[2] = {0xFF, 0xFF}; //For the first 2 bytes of transfer
	struct MAC_Message *pRxBuffer;
	ca_error            error = CA_ERROR_SUCCESS;
	(void)pDeviceRef;

	/* If the receive fifo is full, and this isn't a sync response,
	 * then we can't safely do SPI exchange.*/
	if (SPI_FIFO_Full && (SPI_Wait_Buf && !pTxBuffer))
	{
		ca_log_warn("SPI_Exchange failed - No buffers");
		return CA_ERROR_NO_BUFFER;
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
		goto exit;
	}

	if (pRxBuffer)
	{
		pRxBuffer->Length = RxLen = triageBuf[1];
		RxLen -= AlignMod;
	}

	SPI_BulkExchange(pRxBuffer->PData.Payload, pTxBuffer->PData.Payload, RxLen, TxLen);

	if (pTxBuffer && pTxBuffer->CommandId == SPI_MLME_SCAN_REQUEST) // disable SPI sends during scan
		SPI_MLME_SCANNING = 1;
	else if ((triageBuf[0] == SPI_MLME_SCAN_CONFIRM) || (triageBuf[0] == SPI_HWME_WAKEUP_INDICATION))
		SPI_MLME_SCANNING = 0; // re-enable SPI sends

	if (setSyncChain)
		syncChainInFlight = true;

exit:
	BSP_SetRFSSBHigh(); // end access
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
		if (SPI_Wait_Buf->CommandId == rspid)
		{
			status = CA_ERROR_SUCCESS;
			break;
		}
		WAIT_ms(1);
	} while ((TIME_ReadAbsoluteTime() - startticks) < SPI_T_TIMEOUT);

	BSP_DisableRFIRQ();
	SPI_Wait_Buf = NULLP;
	BSP_EnableRFIRQ();

	return status;
}

ca_error SPI_Send(const uint8_t *buf, size_t len, u8_t *response, struct ca821x_dev *pDeviceRef)
{
	ca_error Status = CA_ERROR_SUCCESS;
	bool     sync   = (buf[0] & SPI_SYN) && (buf[0] != SPI_IDLE);
	(void)len;

	if (SPI_MLME_SCANNING == 1)
	{
		Status = CA_ERROR_SPI_SCAN_IN_PROGRESS;
		goto exit;
	}

	BSP_DisableRFIRQ();
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
	ca_log_crit("SPI: Error code %02x", errcode);
}

void SPI_Initialise(void)
{
	BSP_SPIInit();
	memset(SPI_Receive_Buffer, SPI_IDLE, sizeof(SPI_Receive_Buffer));
	SPI_FIFO_Start = 0;
	SPI_FIFO_End   = 0;
	BSP_EnableRFIRQ();
}
