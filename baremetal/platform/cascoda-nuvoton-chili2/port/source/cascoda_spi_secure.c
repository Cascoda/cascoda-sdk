/*
 *  Copyright (c) 2020, Cascoda Ltd.
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

/* System */
#include <arm_cmse.h>
#include <assert.h>
#include <stdio.h>
/* Platform */
#include "M2351.h"

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili.h"
#include "cascoda_secure.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

enum SPIDMA_NextState
{
	SPIDMA_END,
	SPIDMA_RX,
	SPIDMA_TX
};
static enum SPIDMA_NextState spi_nextstate = SPIDMA_END;
static uint8_t               spi_nextLen   = 0;
static uint8_t *             spi_nextptr;

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
static void (*SPIComplete_callback)(void);

static void Chili_SpiComplete()
{
	if (SPIComplete_callback)
		SPIComplete_callback();
}
#else
static void Chili_SpiComplete()
{
	SPI_ExchangeComplete();
}
#endif

static inline void Chili_SPISetDMASrc(uint8_t channel, uint32_t srcSelect)
{
	assert(channel < 8);
	if (channel < 4)
	{
		PDMA0->REQSEL0_3 |= srcSelect << (8 * channel);
	}
	else
	{
		channel -= 4;
		PDMA0->REQSEL4_7 |= srcSelect << (8 * channel);
	}
}

static void Chili_SPISetDMA(uint8_t *RxBuf, const uint8_t *TxBuf, uint8_t Len)
{
	static uint8_t IdleByte = 0xFF, JunkByte = 0;
	uint32_t       dsct_rx, dsct_tx;

	assert(RxBuf || TxBuf);
	assert(Len >= 1);

	//Reset channels
	PDMA0->CHRST |= (1 << SPI_RX_DMA_CH) | (1 << SPI_TX_DMA_CH);
	Chili_SPISetDMASrc(SPI_RX_DMA_CH, PDMA_SPI0_RX + (2 * SPI_NUM));
	Chili_SPISetDMASrc(SPI_TX_DMA_CH, PDMA_SPI0_TX + (2 * SPI_NUM));

	// Shared config for dma channels
	dsct_rx = ((uint32_t)((Len - 1) << PDMA_DSCT_CTL_TXCNT_Pos) & PDMA_DSCT_CTL_TXCNT_Msk);
	dsct_rx |= PDMA_REQ_SINGLE | PDMA_OP_BASIC;
	dsct_tx = dsct_rx;

	//No address increment on the SPI registers
	dsct_tx |= PDMA_DAR_FIX;
	dsct_rx |= PDMA_SAR_FIX;

	//Addresses
	PDMA0->DSCT[SPI_RX_DMA_CH].DA = (uint32_t)RxBuf;
	PDMA0->DSCT[SPI_TX_DMA_CH].SA = (uint32_t)TxBuf;
	PDMA0->DSCT[SPI_RX_DMA_CH].SA = (uint32_t)&SPI->RX;
	PDMA0->DSCT[SPI_TX_DMA_CH].DA = (uint32_t)&SPI->TX;

	if (!TxBuf)
	{
		// DMA Rx Only
		//No address increment on the SPI registers or idle byte
		dsct_tx |= PDMA_SAR_FIX;
		//Addresses
		PDMA0->DSCT[SPI_TX_DMA_CH].SA = (uint32_t)&IdleByte;
	}
	else if (!RxBuf)
	{
		// DMA Tx Only
		//No address increment on the SPI registers or junk buffer
		dsct_rx |= PDMA_DAR_FIX;
		//Addresses
		PDMA0->DSCT[SPI_RX_DMA_CH].DA = (uint32_t)&JunkByte;
	}

	//Enable Channels (No tx interrupt on purpose, because RX one comes in after.
	PDMA0->CHCTL |= (1 << SPI_RX_DMA_CH) | (1 << SPI_TX_DMA_CH);
	PDMA0->INTEN |= (1 << SPI_RX_DMA_CH) /*| (1 << SPI_TX_DMA_CH)*/;

	//CTL Fields
	PDMA0->DSCT[SPI_RX_DMA_CH].CTL = dsct_rx;
	PDMA0->DSCT[SPI_TX_DMA_CH].CTL = dsct_tx;

	// Enable PDMA Rx and Tx, and reset PDMA system
	SPI->PDMACTL = SPI_PDMACTL_PDMARST_Msk;
	SPI->PDMACTL = SPI_PDMACTL_RXPDMAEN_Msk | SPI_PDMACTL_TXPDMAEN_Msk;
}

static void Chili_SPIDMAContinue()
{
	switch (spi_nextstate)
	{
	case SPIDMA_RX:
		spi_nextstate = SPIDMA_END;
		Chili_SPISetDMA(spi_nextptr, NULL, spi_nextLen);
		break;
	case SPIDMA_TX:
		spi_nextstate = SPIDMA_END;
		Chili_SPISetDMA(NULL, spi_nextptr, spi_nextLen);
		break;
	case SPIDMA_END:
		Chili_SpiComplete();
		break;
	}
}

void CHILI_SPIDMAIRQHandler()
{
	PDMA_CLR_TD_FLAG(PDMA0, (1 << SPI_RX_DMA_CH) | (1 << SPI_TX_DMA_CH));
	SPI->PDMACTL = SPI_PDMACTL_PDMARST_Msk;
	Chili_SPIDMAContinue();
}

static void Chili_SPIDMAExchange(uint8_t *RxBuf, const uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen)
{
	uint8_t minLen = MIN(RxLen, TxLen);
	uint8_t maxLen = MAX(RxLen, TxLen);

	if (minLen == 0 || minLen == maxLen)
	{
		//Single transfer
		Chili_SPISetDMA(RxLen ? RxBuf : NULL, TxLen ? TxBuf : NULL, maxLen);
		spi_nextstate = SPIDMA_END;
	}
	else
	{
		//Bidirectional transfer followed by one-way
		Chili_SPISetDMA(RxBuf, TxBuf, minLen);
		if (TxLen == minLen)
		{
			spi_nextstate = SPIDMA_RX;
			spi_nextptr   = RxBuf + minLen;
		}
		else
		{
			spi_nextstate = SPIDMA_TX;
			spi_nextptr   = (uint8_t *)TxBuf + minLen;
		}
		spi_nextLen = maxLen - minLen;
	}
}

__NONSECURE_ENTRY void BSP_SPIExchange(uint8_t *RxBuf, const uint8_t *TxBuf, uint8_t RxLen, uint8_t TxLen)
{
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
	// Check that the pointers are actually nonsecure.
	if (RxLen && !cmse_check_address_range(RxBuf, RxLen, CMSE_NONSECURE))
		return;
	if (TxLen && !cmse_check_address_range((void *)TxBuf, TxLen, CMSE_NONSECURE))
		return;
#endif

	Chili_SPIDMAExchange(RxBuf, TxBuf, RxLen, TxLen);
}

__NONSECURE_ENTRY void CHILI_RegisterSPIComplete(void (*callback)(void))
{
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
	SPIComplete_callback = callback;
#else
	(void)callback;
#endif
}
