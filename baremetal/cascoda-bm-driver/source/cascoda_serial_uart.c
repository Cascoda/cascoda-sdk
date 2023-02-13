/**
 * @file
 * @brief  Serial Communication Driver Functions (UART)
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

#include <stdbool.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"

#if defined(USE_UART)

/******************************************************************************/
/****** Definitions for Serial State                                     ******/
/******************************************************************************/
enum serial_state
{
	SERIAL_INBETWEEN = 0,
	SERIAL_CMDID     = 1,
	SERIAL_CMDLEN    = 2,
	SERIAL_DATA      = 3,
};

#define SERIAL_SOM (0xDE)

#define SERIAL_TIMEOUT 1000 /* serial rx_rdy timeout in [ms] */
#define MAX_TIMEOUTS 5      /* number of rx_rdy timeouts before the Chili stops waiting */
#define RX_TIMEOUT 200      /* serial rx timeout in [ms] */

/******************************************************************************/
/****** Global Variables for Serial State                                ******/
/******************************************************************************/
static u8_t                SerialCount;                      //!< Number of bytes read so far
static volatile u8_t       SerialRemainder;                  //!< Number of bytes left to receive
volatile enum serial_state SerialRxState = SERIAL_INBETWEEN; //!< State of serial receive state machine

/* combine Serial Buffer and Framing for UART transfers */
struct SerialUARTBuffer
{
	u8_t                SofPkt;    /* Start of Packet Delimiter */
	struct SerialBuffer SerialBuf; /* Serial Buffer */
};

/******************************************************************************/
/****** Global Variables for Serial Message Buffers                      ******/
/******************************************************************************/
struct SerialBuffer            SerialRxBuffer;             //!< Global buffer for received serial messages
static struct SerialUARTBuffer SerialTxBuffer;             //!< Global buffer for transmitted serial messages
volatile bool                  SerialRxPending    = false; //!< Receive Packet is ready
static volatile bool           SerialTxStalled    = false; //!< Transmit waiting for RxRdy
static volatile bool           SerialTxResendReq  = false; //!< Transmit resend required
static volatile u32_t          SerialRxTimeout    = 0;     //!< Rx timeout
static u32_t                   SerialTxTimeout    = 0;     //!< RxRdy timeout
static u8_t                    SerialTimeoutCount = 0;     //!< Number of timeouts

static u8_t SerialCmdId  = 0xFF;
static u8_t SerialCmdLen = 0;

/* Local Functions */
static u8_t SerialFindStart(void);
static u8_t SerialReceivedRxRdy(void);
static u8_t SerialReceivedRxFail(void);
static void SerialResend(void);
static void SerialSetTxTimeout(void);
static void SerialCheckTxTimeout(void);
static void SerialCheckRxTimeout(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Find start of block from Serial input
 *******************************************************************************
 * \return 1 if start found , 0 if not
 *******************************************************************************
 ******************************************************************************/
static u8_t SerialFindStart(void)
{
	u8_t InputChar;

	if (BSP_SerialRead(&InputChar, 1) != 0)
	{
		if (InputChar == SERIAL_SOM)
		{
			return 1;
		}
	}
	return 0;
} // End of SerialFindStart()

u8_t Serial_ReadInterface(void)
{
	u8_t Count;

	while (1)
	{
		switch (SerialRxState)
		{
		case SERIAL_INBETWEEN:
			if (SerialFindStart())
			{
				SerialRxState   = SERIAL_CMDID;
				SerialRxTimeout = TIME_ReadAbsoluteTime();
				continue;
			}
			return 0;
		case SERIAL_CMDID:
			if ((Count = BSP_SerialRead(&SerialCmdId, 1)) != 0)
			{
				SerialRxState = SERIAL_CMDLEN;
				continue;
			}
			return 0;
		case SERIAL_CMDLEN:
			if ((Count = BSP_SerialRead(&SerialCmdLen, 1)) != 0)
			{
				if (SerialReceivedRxRdy() || SerialReceivedRxFail())
				{
					SerialRxState = SERIAL_INBETWEEN;
					return 1;
				}
				else
				{
					SerialRxBuffer.CmdId  = SerialCmdId;
					SerialRxBuffer.CmdLen = SerialCmdLen;
					SerialRemainder       = SerialCmdLen;
					SerialCount           = 0;
					if (SerialRemainder == 0)
					{
						SerialRxState   = SERIAL_INBETWEEN; // exit if 0 API packet length !!
						SerialRxPending = true;
						return 1;
					}
					else
					{
						SerialRxState = SERIAL_DATA;
						continue;
					}
				}
			}
			return 0;
		case SERIAL_DATA:
			if ((Count = BSP_SerialRead(SerialRxBuffer.Data + SerialCount, SerialRemainder)) != 0)
			{
				SerialCount += Count;
				SerialRemainder -= Count;
				if (SerialRemainder == 0)
				{
					SerialRxState   = SERIAL_INBETWEEN;
					SerialRemainder = 0;
					SerialCount     = 0;
					SerialRxPending = true;
					return 1;
				}
			}
			return 0;
		}
	}
} // End of Serial_ReadInterface()

u8_t SerialGetCommand(void)
{
	SerialCheckRxTimeout();
	if (SerialTxResendReq)
	{
		SerialResend();
	}
	if (SerialRxPending)
	{
		SerialTimeoutCount = 0;
	}
	return SerialRxPending;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Serial Read
 *******************************************************************************
 ******************************************************************************/
void SerialReadComplete(void)
{
	SerialRxState   = SERIAL_INBETWEEN;
	SerialRemainder = 0;
	SerialCount     = 0;
	SerialRxPending = true;
}

void SerialSendRxRdy()
{
	uint8_t buf[3];

	buf[0] = SERIAL_SOM;  /* start-of-packet delimiter */
	buf[1] = EVBME_RXRDY; /* CmdId  = EVBME_RXRDY */
	buf[2] = 0x00;        /* CmdLen = 0 */
	BSP_SerialWriteAll(buf, 3);
}

void SerialSendRxFail()
{
	uint8_t buf[3];

	buf[0] = SERIAL_SOM;   /* start-of-packet delimiter */
	buf[1] = EVBME_RXFAIL; /* CmdId  = EVBME_RXFAIL */
	buf[2] = 0x00;         /* CmdLen = 0 */
	BSP_SerialWriteAll(buf, 3);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Receive RX_RDY Handshake Packet
 *******************************************************************************
 * \return 1 if RX_RDY received, 0 if not
 *******************************************************************************
 ******************************************************************************/
static u8_t SerialReceivedRxRdy(void)
{
	if ((SerialCmdId == EVBME_RXRDY) && (SerialCmdLen == 0))
	{
		SerialTxStalled    = false;
		SerialTimeoutCount = 0;
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
static u8_t SerialReceivedRxFail(void)
{
	if ((SerialCmdId == EVBME_RXFAIL) && (SerialCmdLen == 0))
	{
		SerialTxResendReq  = true;
		SerialTimeoutCount = 0;
		return 1;
	}
	return 0;
}

/**
 * Resend a nack-ed transmission
 */
static void SerialResend(void)
{
	SerialTxResendReq = false;
	BSP_SerialWriteAll(&SerialTxBuffer.SofPkt, SerialTxBuffer.SerialBuf.CmdLen + 3);
	SerialSetTxTimeout();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets RX_RDY Handshake Tx Timeout
 *******************************************************************************
 ******************************************************************************/
static void SerialSetTxTimeout(void)
{
	SerialTxStalled = true;
	SerialTxTimeout = TIME_ReadAbsoluteTime();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks RX_RDY Handshake Tx Timeout
 *******************************************************************************
 ******************************************************************************/
static void SerialCheckTxTimeout(void)
{
	while (SerialTxStalled == true)
	{
		if (SerialTimeoutCount >= MAX_TIMEOUTS)
		{
			SerialTxStalled = false;
			break;
		}
		else if (SerialTxResendReq)
		{
			SerialResend();
		}
		else if ((TIME_ReadAbsoluteTime() - SerialTxTimeout) > SERIAL_TIMEOUT)
		{
			SerialTxStalled = false;
			SerialTimeoutCount++;
			break;
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks Rx Timeout
 *******************************************************************************
 ******************************************************************************/
static void SerialCheckRxTimeout(void)
{
	if (SerialRxState != SERIAL_INBETWEEN && (TIME_ReadAbsoluteTime() - SerialRxTimeout) > RX_TIMEOUT)
	{
		BSP_SerialRead(NULL, 0); //Cancel DMA
		SerialReadComplete();    //Reset rx state machine
		SerialRxPending = false; //Remove pending transaction
		SerialSendRxFail();      //Signal for repeat send
	}
}

void EVBME_Message_UART(char *pBuffer, size_t Count)
{
	SerialGetCommand();
	SerialCheckTxTimeout();
	SerialTxBuffer.SofPkt           = SERIAL_SOM;
	SerialTxBuffer.SerialBuf.CmdId  = EVBME_MESSAGE_INDICATION;
	SerialTxBuffer.SerialBuf.CmdLen = Count;
	memcpy(SerialTxBuffer.SerialBuf.Data, pBuffer, Count);
	BSP_SerialWriteAll(&SerialTxBuffer.SofPkt, Count + 3);
	SerialSetTxTimeout();
	BSP_SerialWaitWhileBusy(); /* for long printf buffer sometimes seems to be overwritten by next printf */
} // End of EVBME_Message()

void MAC_Message_UART(u8_t CommandId, u8_t Count, const u8_t *pBuffer)
{
	SerialGetCommand();
	SerialCheckTxTimeout();
	SerialTxBuffer.SofPkt           = SERIAL_SOM;
	SerialTxBuffer.SerialBuf.CmdId  = CommandId;
	SerialTxBuffer.SerialBuf.CmdLen = Count;
	memcpy(SerialTxBuffer.SerialBuf.Data, pBuffer, Count);
	BSP_SerialWriteAll(&SerialTxBuffer.SofPkt, Count + 3);
	SerialSetTxTimeout();
} // End of MAC_Message()

#endif
