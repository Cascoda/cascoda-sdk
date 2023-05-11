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

#include <stdio.h>
#include <string.h>

#include "NuMicro.h"

#include "cascoda_chili_config.h"
#include "ldrom_uart.h"

struct SerialUARTBuf       gRxBuffer;
struct SerialUARTBuf       gTxBuffer;
volatile enum serial_state gSerialRxState;
volatile bool              gSerialTxStalled;

static uint8_t          SerialCmdId;
static uint8_t          SerialCmdLen;
static volatile uint8_t SerialRemainder;
static uint8_t          SerialCount;

static uint32_t SerialRead(uint8_t *pBuffer, uint32_t BufferSize)
{
	uint32_t numBytes = 0;

	while ((!(UART->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk)) && (numBytes < BufferSize)) /* Rx FIFO empty ? */
	{
		pBuffer[numBytes] = UART->DAT;
		++numBytes;
	}

	return numBytes;
}

static void SerialWrite(uint8_t *pBuffer, uint32_t BufferSize)
{
	uint8_t i;
	/* Wait until Tx FIFO is empty */
	while (!(UART->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
		;
	/* fill FIFO */
	for (i = 0; i < BufferSize; ++i) UART->DAT = pBuffer[i];
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Serial Read
 *******************************************************************************
 ******************************************************************************/
static void SerialReadComplete(void)
{
	gSerialRxState  = SERIAL_INBETWEEN;
	SerialRemainder = 0;
	SerialCount     = 0;
}

static void SerialSendRxRdy()
{
	uint8_t buf[3];

	buf[0] = SERIAL_SOM;  /* start-of-packet delimiter */
	buf[1] = EVBME_RXRDY; /* CmdId  = EVBME_RXRDY */
	buf[2] = 0x00;        /* CmdLen = 0 */
	SerialWrite(buf, 3);
}

static void SerialSendRxFail()
{
	uint8_t buf[3];

	buf[0] = SERIAL_SOM;   /* start-of-packet delimiter */
	buf[1] = EVBME_RXFAIL; /* CmdId  = EVBME_RXFAIL */
	buf[2] = 0x00;         /* CmdLen = 0 */
	SerialWrite(buf, 3);
}

void RxHandled(void)
{
	gRxBuffer.isReady = false;
	SerialSendRxRdy();
	SerialReadComplete();
}

void TxReady(void)
{
	uint8_t meta_len = sizeof(gTxBuffer.SofPkt) + sizeof(gTxBuffer.cmdid) + sizeof(gTxBuffer.len);
	if (!gSerialTxStalled)
		SerialWrite(&gTxBuffer.SofPkt, gTxBuffer.len + meta_len);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Find start of block from Serial input
 *******************************************************************************
 * \return 1 if start found , 0 if not
 *******************************************************************************
 ******************************************************************************/
static uint8_t SerialFindStart(void)
{
	uint8_t InputChar;

	if (SerialRead(&InputChar, 1) != 0)
		if (InputChar == SERIAL_SOM)
			return 1;

	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Receive RX_RDY Handshake Packet
 *******************************************************************************
 * \return 1 if RX_RDY received, 0 if not
 *******************************************************************************
 ******************************************************************************/
static uint8_t SerialReceivedRxRdy(void)
{
	if ((SerialCmdId == EVBME_RXRDY) && (SerialCmdLen == 0))
	{
		gSerialTxStalled = false;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Receive RX_FAIL Handshake Packet
 *******************************************************************************
 * \return 1 if RX_FAIL received, 0 if not
 *******************************************************************************
 ******************************************************************************/
static uint8_t SerialReceivedRxFail(void)
{
	if ((SerialCmdId == EVBME_RXFAIL) && (SerialCmdLen == 0))
		return 1;

	return 0;
}

static uint8_t SerialReadInterface(void)
{
	uint8_t Count;

	while (1)
	{
		switch (gSerialRxState)
		{
		case SERIAL_INBETWEEN:
			if (SerialFindStart())
			{
				gSerialRxState = SERIAL_CMDID;
				continue;
			}
			return 0;
		case SERIAL_CMDID:
			if ((Count = SerialRead(&SerialCmdId, 1)) != 0)
			{
				gSerialRxState = SERIAL_CMDLEN;
				continue;
			}
			return 0;
		case SERIAL_CMDLEN:
			if ((Count = SerialRead(&SerialCmdLen, 1)) != 0)
			{
				if (SerialReceivedRxRdy() || SerialReceivedRxFail())
				{
					gSerialRxState = SERIAL_INBETWEEN;
					return 1;
				}
				else
				{
					gRxBuffer.cmdid = SerialCmdId;
					gRxBuffer.len   = SerialCmdLen;
					SerialRemainder = SerialCmdLen;
					SerialCount     = 0;
					if (SerialRemainder == 0)
					{
						gSerialRxState    = SERIAL_INBETWEEN; // exit if 0 API packet length !!
						gRxBuffer.isReady = true;
						return 1;
					}
					else
					{
						gSerialRxState = SERIAL_DATA;
						continue;
					}
				}
			}
			return 0;
		case SERIAL_DATA:
			if ((Count = SerialRead(&gRxBuffer.data.generic + SerialCount, SerialRemainder)) != 0)
			{
				SerialCount += Count;
				SerialRemainder -= Count;
				if (SerialRemainder == 0)
				{
					gSerialRxState    = SERIAL_INBETWEEN;
					SerialRemainder   = 0;
					SerialCount       = 0;
					gRxBuffer.isReady = true;
					return 1;
				}
			}
			return 0;
		}
	}
} // End of Serial_ReadInterface()

/*---------------------------------------------------------------------------------------------------------*/
/* INTSTS to handle UART interrupt event                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#if (UART_CHANNEL == 0)
void UART0_IRQHandler(void)
#elif (UART_CHANNEL == 1)
void UART1_IRQHandler(void)
#elif (UART_CHANNEL == 2)
void UART2_IRQHandler(void)
#elif (UART_CHANNEL == 4)
void UART4_IRQHandler(void)
#elif (UART_CHANNEL == 5)
void UART5_IRQHandler(void)
#endif
{
	/*----- Determine interrupt source -----*/
	uint32_t u32IntSrc = UART->INTSTS;

	/* tx fifo empty trigerred when tx dma with fast baud rates (why?) */
	if (UART->INTSTS == (UART_INTEN_TXENDIEN_Msk | UART_INTEN_THREIEN_Msk))
		return;
	/* line status - clear */
	if (UART->INTSTS & UART_INTSTS_HWRLSIF_Msk)
		UART->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
	/* receive data available  & not switched to DMA */
	while ((UART->INTSTS & UART_INTSTS_RDAINT_Msk) && (UART->INTEN & UART_INTEN_RDAIEN_Msk)) SerialReadInterface();
}

void UART_Init(void)
{
	/*---------------------------------------------------------------------------------------------------------*/
	/* Init UART                                                                                               */
	/*---------------------------------------------------------------------------------------------------------*/
	/* Select UART function */
	UART->FUNCSEL = UART_FUNCSEL_UART;
	/* Set UART line configuration */
	UART->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
	/* Set UART Rx and RTS trigger level */
	UART->FIFO &= ~(UART_FIFO_RFITL_Msk | UART_FIFO_RTSTRGLV_Msk);
	/* Set UART baud rate */
	UART->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, 115200));
	/* Set time-out interrupt comparator */
	UART->TOUT = (UART->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);
	NVIC_SetPriority(UART_IRQn, 2);
	NVIC_EnableIRQ(UART_IRQn);
	/* Enable tim-out counter, Rx tim-out interrupt and Rx ready interrupt */
	UART->INTEN = (UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk | UART_INTEN_RDAIEN_Msk);

	gSerialRxState = SERIAL_INBETWEEN;
	SerialCmdId    = 0xFF;
	SerialCmdLen   = 0;
}