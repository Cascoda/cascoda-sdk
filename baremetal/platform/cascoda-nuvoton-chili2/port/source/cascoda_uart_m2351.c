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
/*
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * Internal (Non-Interface) Functions for UART Communication
*/
/* System */
#include <stdio.h>
/* Platform */
#include "M2351.h"
/* Cascoda */
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"

#if defined(USE_UART)

#define UART_WAITTIMEOUT 50 /* wait while uart is busy timeout [ms] */

void CHILI_UARTFIFOWrite(u8_t *pBuffer, u32_t BufferSize)
{
	u8_t i;
	/* Wait until Tx FIFO is empty */
	while (!(UART->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
		;
	/* fill FIFO */
	for (i = 0; i < BufferSize; ++i) UART->DAT = pBuffer[i];
}

void CHILI_UARTDMAWrite(u8_t *pBuffer, u32_t BufferSize)
{
	/* Wait until Tx DMA channel is released */
	while (PDMA_IS_CH_BUSY(PDMA0, UART_TX_DMA_CH))
		;

	/* Set transfer width (8 bits) and transfer count */
	PDMA_SetTransferCnt(PDMA0, UART_TX_DMA_CH, PDMA_WIDTH_8, BufferSize);
	/* Set source/destination address and attributes */
	PDMA_SetTransferAddr(PDMA0, UART_TX_DMA_CH, (uint32_t)pBuffer, PDMA_SAR_INC, (uint32_t)&UART->DAT, PDMA_DAR_FIX);
	/* Set request source and basic mode. Needs to be done here. */
	PDMA_SetTransferMode(PDMA0, UART_TX_DMA_CH, PDMA_UART_TX, FALSE, 0);
	/* Enable PDMA Transfer Done Interrupt */
	PDMA_EnableInt(PDMA0, UART_TX_DMA_CH, PDMA_INT_TRANS_DONE);
	/* Enable UART Tx PDMA function */
	UART->INTEN |= UART_INTEN_TXPDMAEN_Msk;
}

u32_t CHILI_UARTFIFORead(u8_t *pBuffer, u32_t BufferSize)
{
	u32_t numBytes = 0;

	/* Note that this is basically redefining the vendor BSP function UART_Read() */
	/* which cannot read a previously unknown number of bytes. */

	while ((!(UART->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk)) && (numBytes < BufferSize)) /* Rx FIFO empty ? */
	{
		pBuffer[numBytes] = UART->DAT;
		++numBytes;
	}

	return (numBytes);
}

void CHILI_UARTDMASetupRead(u8_t *pBuffer, u32_t BufferSize)
{
	/* Note: */
	/* This needs to be fast, so BSP functions have been flattened out to direct register writes */
	/* UART_RX_DMA_CHANNEL fixed to 0 */

	/* Set request source; set basic mode. */
	// PDMA_SetTransferMode(PDMA0, UART_RX_DMA_CH, PDMA_UART_RX, FALSE, 0);
#if (UART_RX_DMA_CH < 4)
	PDMA0->REQSEL0_3 = (PDMA0->REQSEL0_3 & ~(0x7F << 8 * UART_RX_DMA_CH)) | (PDMA_UART_RX << (8 * UART_RX_DMA_CH));
#else
	PDMA0->REQSEL4_7 =
	    (PDMA0->REQSEL4_7 & ~(0x7F << 8 * (UART_RX_DMA_CH - 4))) | (PDMA_UART_RX << (8 * (UART_RX_DMA_CH - 4)));
#endif
	PDMA0->DSCT[UART_RX_DMA_CH].CTL = (PDMA0->DSCT[UART_RX_DMA_CH].CTL & ~PDMA_DSCT_CTL_OPMODE_Msk) | PDMA_OP_BASIC;

	/* Set transfer width (8 bits) and transfer count */
	// PDMA_SetTransferCnt(PDMA0, UART_RX_DMA_CH, PDMA_WIDTH_8, BufferSize);
	PDMA0->DSCT[UART_RX_DMA_CH].CTL &= ~(PDMA_DSCT_CTL_TXCNT_Msk | PDMA_DSCT_CTL_TXWIDTH_Msk);
	PDMA0->DSCT[UART_RX_DMA_CH].CTL |= (PDMA_WIDTH_8 | ((BufferSize - 1) << PDMA_DSCT_CTL_TXCNT_Pos));

	/* Set source/destination address and attributes */
	// PDMA_SetTransferAddr(PDMA0, UART_RX_DMA_CH, (uint32_t)&UART->DAT, PDMA_SAR_FIX, (uint32_t)pBuffer, PDMA_DAR_INC);
	PDMA0->DSCT[UART_RX_DMA_CH].SA = (uint32_t)&UART->DAT;
	PDMA0->DSCT[UART_RX_DMA_CH].DA = (uint32_t)pBuffer;
	PDMA0->DSCT[UART_RX_DMA_CH].CTL &= ~(PDMA_DSCT_CTL_SAINC_Msk | PDMA_DSCT_CTL_DAINC_Msk);
	PDMA0->DSCT[UART_RX_DMA_CH].CTL |= (PDMA_SAR_FIX | PDMA_DAR_INC);

	/* Interrupt switching, note that correct sequence is crucial */
	/* Disable UART Rx interrupt */
	UART->INTEN &= ~UART_INTEN_RDAIEN_Msk;
	/* Enable PDMA Transfer Done Interrupt */
	// PDMA_EnableInt(PDMA0, UART_RX_DMA_CH, PDMA_INT_TRANS_DONE);
	PDMA0->INTEN |= (1 << UART_RX_DMA_CH);
	/* Enable UART Rx PDMA function */
	UART->INTEN |= UART_INTEN_RXPDMAEN_Msk;
}

void CHILI_UARTFIFOIRQHandler(void)
{
	/* tx fifo empty trigerred when tx dma with fast baud rates (why?) */
	if (UART->INTSTS == (UART_INTEN_TXENDIEN_Msk | UART_INTEN_THREIEN_Msk))
		return;
	/* line status - clear */
	if (UART->INTSTS & UART_INTSTS_HWRLSIF_Msk)
		UART->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
	/* receive data available */
	while (UART->INTSTS & UART_INTSTS_RDAINT_Msk) Serial_ReadInterface();
}

void CHILI_UARTDMAIRQHandler(void)
{
	/* UART Tx PDMA transfer done interrupt flag */
	if (PDMA_GET_TD_STS(PDMA0) & (1 << UART_TX_DMA_CH))
	{
		/* Clear PDMA transfer done interrupt flag */
		PDMA_CLR_TD_FLAG(PDMA0, (1 << UART_TX_DMA_CH));
		/* Disable UART Tx PDMA function */
		UART->INTEN &= ~UART_INTEN_TXPDMAEN_Msk;
		PDMA_DisableInt(PDMA0, UART_TX_DMA_CH, PDMA_INT_TRANS_DONE);
	}
	/* UART Rx PDMA transfer done interrupt flag */
	if (PDMA_GET_TD_STS(PDMA0) & (1 << UART_RX_DMA_CH))
	{
		/* Clear PDMA transfer done interrupt flag */
		PDMA_CLR_TD_FLAG(PDMA0, (1 << UART_RX_DMA_CH));
		/* Disable UART Rx PDMA function and enable UART Rx interrupt */
		UART->INTEN &= ~UART_INTEN_RXPDMAEN_Msk;
		UART->INTEN |= UART_INTEN_RDAIEN_Msk;
		PDMA_DisableInt(PDMA0, UART_RX_DMA_CH, PDMA_INT_TRANS_DONE);
		/* finish serial access */
		SerialReadComplete();
	}
}

void CHILI_UARTWaitWhileBusy(void)
{
	u32_t sttime = BSP_ReadAbsoluteTime();
	/* check DMA */
	while (PDMA_IS_CH_BUSY(PDMA0, UART_TX_DMA_CH) || PDMA_IS_CH_BUSY(PDMA0, UART_RX_DMA_CH))
	{
		if ((BSP_ReadAbsoluteTime() - sttime) > UART_WAITTIMEOUT)
			return;
	}

	/* check FIFO */
	if (!(UART->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
	{
		while (!(UART->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
		{
			if ((BSP_ReadAbsoluteTime() - sttime) > UART_WAITTIMEOUT)
				return;
		}
		/* wait for last byte */
		BSP_WaitUs(80);
	}
}

#endif /* USE_UART */
