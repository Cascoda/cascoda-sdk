/**
 * @file   cascoda_spi.c
 * @brief  SPI Communication Driver Functions
 * @author Wolfgang Bruchner
 * @date   19/07/14
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "mac_messages.h"

#define MAX(x, y) (x > y ? x : y)

/******************************************************************************/
/****** Global Variables for SPI Message Buffers                         ******/
/******************************************************************************/
/** Cyclic SPI receive message FIFO. Messages are read from the SPI and put into
 *  the next available index. */
struct MAC_Message SPI_Receive_Buffer[SPI_RX_FIFO_SIZE];
static int         SPI_FIFO_Start = 0;
static int         SPI_FIFO_End   = 0;
static bool        SPI_FIFO_Full  = false;

/** Pointer to buffer to be used to store synchronous responses */
static struct MAC_Message *SPI_Wait_Buf;

/** Flag to prevent messages being sent while the system is scanning */
u8_t SPI_MLME_SCANNING = 0;

/******************************************************************************/
/****** Static function declarations for cascoda_spi.c                   ******/
/******************************************************************************/
static int                 SPI_SyncWait(uint8_t cmdid, struct ca821x_dev *pDeviceRef);
static struct MAC_Message *SPI_GetFreeBuf();
static struct MAC_Message *getBuf(uint8_t cmdid);
static u8_t                SPI_WaitSlave(void);
static void                SPI_BulkExchange(uint8_t *RxBuf, uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen);
static void                SPI_Error(u8_t errcode, u8_t restart, struct ca821x_dev *pDeviceRef);

bool SPI_IsFifoFull()
{
	return SPI_FIFO_Full;
}

bool SPI_IsFifoEmpty()
{
	return (SPI_FIFO_Start == SPI_FIFO_End) && !SPI_FIFO_Full;
}

bool SPI_IsFifoAlmostFull()
{
	if (SPI_Wait_Buf)
		return SPI_FIFO_Full;
	if (SPI_FIFO_Full)
		return true;
	return (SPI_FIFO_Start == ((SPI_FIFO_End + 1) % SPI_RX_FIFO_SIZE));
}

/**
 *\brief Return an empty async buffer to be filled, or NULL if none available
 */
static struct MAC_Message *SPI_GetFreeBuf()
{
	struct MAC_Message *rval = NULL;

	if (!SPI_IsFifoFull())
	{
		rval         = SPI_Receive_Buffer + SPI_FIFO_End;
		SPI_FIFO_End = (SPI_FIFO_End + 1) % SPI_RX_FIFO_SIZE;

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
		BSP_EnableRFIRQ();
	}
}

/**
 *\brief Get a buffer for Rx based on whether command is sync or not.
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
		rval = SPI_Wait_Buf;
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
 */
#if CASCODA_CA_VER == 8211
static u8_t SPI_WaitSlave(void)
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
		return 0; //Success
	} while (T_Now - T_Start < SPI_T_TIMEOUT);

	printf("SPI_WaitSlave timed out\n");
	return -1; //Wait timed out
}
#elif CASCODA_CA_VER == 8210
static u8_t SPI_WaitSlave(void)
{
	BSP_WaitUs(SPI_T_HOLD_8210);
	return 0;
}
#else
#error "Need SPI_WaitSlave definition for CA_VER"
#endif

/**
 *\brief Bulk transfer an SPI chunk of data, filling with IDLE to equalize channel length
 */
static void SPI_BulkExchange(uint8_t *RxBuf, uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen)
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
 */
#if CASCODA_CA_VER == 8211
static u8_t SPI_GetFirst2Bytes(u8_t *ReadBytes, struct MAC_Message *pTxBuffer)
{
	if (pTxBuffer)
	{
		ReadBytes[0] = pTxBuffer->CommandId;
		ReadBytes[1] = pTxBuffer->Length;
	}

	ReadBytes[0] = BSP_SPIExchangeByte(ReadBytes[0]);
	ReadBytes[1] = BSP_SPIExchangeByte(ReadBytes[1]);

	return 0;
}
#elif CASCODA_CA_VER == 8210
static u8_t SPI_GetFirst2Bytes(u8_t *ReadBytes, struct MAC_Message *pTxBuffer)
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
		return -1;

	return 0;
}
#else
#error "Need SPI_GetFirst2Bytes definition for CA_VER"
#endif

#if CASCODA_CA_VER == 8210
static u8_t SPI_CA8210_Align(u8_t *ReadBytes, struct MAC_Message *pTxBuffer)
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
static u8_t SPI_CA8210_Align(u8_t *ReadBytes, struct MAC_Message *pTxBuffer)
{
	return 0;
}
#endif

int SPI_Exchange(struct MAC_Message *pTxBuffer, struct ca821x_dev *pDeviceRef)
{
	u8_t                TxLen = 0, RxLen = 0, AlignMod = 0;
	u8_t                triageBuf[2] = {0xFF, 0xFF}; //For the first 2 bytes of transfer
	struct MAC_Message *pRxBuffer;

	BSP_SetRFSSBHigh();

	//Wait for slave or time out and get first 2 bytes (commandId and Length)
	if (SPI_WaitSlave())
		return -1;
	if (SPI_GetFirst2Bytes(triageBuf, pTxBuffer))
		return -1;

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

	if (pRxBuffer)
	{
		EVBMECheckSPICommand(pRxBuffer, pDeviceRef); // check if API command requires modification
	}

exit:
	BSP_SetRFSSBHigh(); // end access
	return (pRxBuffer != NULL);
}

/**
 * \brief Wait for synchronous command SPI
 *
 * This function waits until a synchronous command is received over SPI
 *
 * \param cmdid - The id of the command to wait for
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 * \return  0 or \ref evbme_error_code
 *
 */
static int SPI_SyncWait(uint8_t cmdid, struct ca821x_dev *pDeviceRef)
{
	int status     = EVBME_SPI_WAIT_TIMEOUT;
	int startticks = TIME_ReadAbsoluteTime();

	do
	{
		if (SPI_Wait_Buf->CommandId == cmdid)
		{
			status = EVBME_SUCCESS;
			break;
		}
		TIME_WaitTicks(1);
	} while ((TIME_ReadAbsoluteTime() - startticks) < SPI_T_TIMEOUT);

	SPI_Wait_Buf = NULLP;

	return status;
}

int SPI_Send(const uint8_t *buf, size_t len, u8_t *response, struct ca821x_dev *pDeviceRef)
{
	int  Status;
	bool sync = (buf[0] & SPI_SYN) && (buf[0] != SPI_IDLE);

	if (SPI_MLME_SCANNING == 1)
	{
		SPI_Error(EVBME_SPI_SCAN_IN_PROGRESS, 0, pDeviceRef);
		return 1;
	}

	BSP_DisableRFIRQ();
	if (sync)
	{
		SPI_Wait_Buf            = (struct MAC_Message *)response;
		SPI_Wait_Buf->CommandId = SPI_IDLE;
	}

	Status = SPI_Exchange((struct MAC_Message *)buf, pDeviceRef); // tx packet

	BSP_EnableRFIRQ();

	if (Status < 0)
	{
		if (sync)
			SPI_Wait_Buf = NULL;
		SPI_Error(EVBME_SPI_SEND_EXCHANGE_FAIL, 0, pDeviceRef);
		return 1;
	}

	if (sync)
	{
		int ret;

		ret = SPI_SyncWait(sync_pairings[buf[0] & SPI_MID_MASK], pDeviceRef);
		if (ret)
		{
			SPI_Error(ret, 0, pDeviceRef);
			return 1;
		}
	}

	return 0;
}

/**
 * \brief SPI Error Handling
 *
 * \param message - Error Message
 * \param restart - Flag for restarting interface when non-zero
 * \param pDeviceRef - Pointer to initialised \ref ca821x_dev struct
 *
 */
static void SPI_Error(u8_t errcode, u8_t restart, struct ca821x_dev *pDeviceRef)
{
	BSP_SetRFSSBHigh();
	printf("SPI %dms: Error code %02x\n", TIME_ReadAbsoluteTime(), errcode);
	if (restart)
	{
		EVBME_CAX_Restart(pDeviceRef);
	}
	EVBME_ERROR_Indication(errcode, restart, pDeviceRef);
}

void SPI_Initialise(void)
{
	BSP_SPIInit();
	memset(SPI_Receive_Buffer, SPI_IDLE, sizeof(SPI_Receive_Buffer));
	SPI_FIFO_Start = 0;
	SPI_FIFO_End   = 0;
	BSP_EnableRFIRQ();
}
