/*
 * Copyright (C) 2019  Cascoda, Ltd.
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

/* Local Functions */
static void CHILI_UARTDMAInitialise(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise UART for Comms
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTInit(void)
{
	if (UART_BAUDRATE <= 115200)
	{
		// 4 MHz
		if (UseExternalClock)
			CLK_SetModuleClock(UART_MODULE, UART_CLK_HXT, UART_CLKDIV(1));
		else
			CLK_SetModuleClock(UART_MODULE, UART_CLK_HIRC, UART_CLKDIV(3));
	}
	else
	{
		// 48 MHz
		if ((SystemFrequency == FSYS_32MHZ) || (SystemFrequency == FSYS_64MHZ))
			CLK_SetModuleClock(UART_MODULE, UART_CLK_PLL, UART_CLKDIV(2));
		else
			CLK_SetModuleClock(UART_MODULE, UART_CLK_PLL, UART_CLKDIV(1));
	}

	CLK_EnableModuleClock(UART_MODULE);

	/* Initialise UART */
	SYS_ResetModule(UART_RST);
	UART_SetLineConfig(UART, UART_BAUDRATE, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);
	UART_Open(UART, UART_BAUDRATE);
	UART_EnableInt(UART, UART_INTEN_RDAIEN_Msk);
	NVIC_EnableIRQ(UART_IRQn);

	CHILI_ModuleSetMFP(UART_TXD_PNUM, UART_TXD_PIN, PMFP_UART);
	CHILI_ModuleSetMFP(UART_RXD_PNUM, UART_RXD_PIN, PMFP_UART);

	/* enable DMA */
	CHILI_UARTDMAInitialise();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief De-Initialise UART
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTDeinit(void)
{
	NVIC_DisableIRQ(UART_IRQn);
	UART_DisableInt(UART, UART_INTEN_RDAIEN_Msk);
	CLK_DisableModuleClock(UART_MODULE);
	UART_Close(UART);
	/* disable DMA */
	PDMA_Close(PDMA0);
	CLK_DisableModuleClock(PDMA0_MODULE);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialises DMA access for UART
 *******************************************************************************
 ******************************************************************************/
static void CHILI_UARTDMAInitialise(void)
{
	CLK_EnableModuleClock(PDMA0_MODULE);

	/* Reset PDMA module */
	SYS_ResetModule(PDMA0_RST);

	/* Open DMA channels */
	PDMA_Open(PDMA0, (1 << UART_RX_DMA_CH) | (1 << UART_TX_DMA_CH));

	/* Single request type */
	PDMA_SetBurstType(PDMA0, UART_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
	PDMA_SetBurstType(PDMA0, UART_RX_DMA_CH, PDMA_REQ_SINGLE, 0);

	/* Disable table interrupt */
	PDMA0->DSCT[UART_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;
	PDMA0->DSCT[UART_RX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

	NVIC_EnableIRQ(PDMA0_IRQn);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Complete Buffer Write using FIFO
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTFIFOWrite(u8_t *pBuffer, u32_t BufferSize)
{
	u8_t i;
	/* Wait until Tx FIFO is empty */
	while (!(UART->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
		;
	/* fill FIFO */
	for (i = 0; i < BufferSize; ++i) UART->DAT = pBuffer[i];
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Complete Buffer Write using DMA
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Buffer FIFO Read
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 * \return Number of Characters placed in Buffer
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set up DMA for UART Read
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Interrupt Handler for UART FIFO
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Interrupt Handler for UART DMA
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Waits While UART transfer is going on (DMA or FIFO)
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 * \return Number of Characters placed in Buffer
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTWaitWhileBusy(void)
{
	u32_t sttime = TIME_ReadAbsoluteTime();
	/* check DMA */
	while (PDMA_IS_CH_BUSY(PDMA0, UART_TX_DMA_CH) || PDMA_IS_CH_BUSY(PDMA0, UART_RX_DMA_CH))
	{
		if ((TIME_ReadAbsoluteTime() - sttime) > UART_WAITTIMEOUT)
			return;
	}

	/* check FIFO */
	if (!(UART->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
	{
		while (!(UART->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk))
		{
			if ((TIME_ReadAbsoluteTime() - sttime) > UART_WAITTIMEOUT)
				return;
		}
		/* wait for last byte */
		BSP_WaitUs(80);
	}
}

#endif /* USE_UART */
