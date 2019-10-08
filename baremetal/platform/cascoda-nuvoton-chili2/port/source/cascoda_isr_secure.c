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
/* Platform */
#include "M2351.h"
#include "gpio.h"
#include "timer.h"
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"

extern volatile u8_t asleep;

void cascoda_isr_secure_init(void)
{
	//This function is here to kick the linker, do not remove!
	return;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 1: IRC TRIM interrupt
 *******************************************************************************
 ******************************************************************************/
void IRC_IRQHandler(void)
{
	/* 12 MHz HIRC */
	if (SYS->TISTS12M & SYS_TISTS12M_TFAILIF_Msk) /* Get Trim Failure Interrupt */
	{
		ca_log_warn("HIRC Trim Failure Interrupt");
		SYS->TISTS12M = SYS_TISTS12M_TFAILIF_Msk;
	}
	if (SYS->TISTS12M & SYS_TISTS12M_CLKERRIF_Msk) /* Get Clock Error Interrupt */
	{
		ca_log_warn("HIRC Clock Error Interrupt");
		SYS->TISTS12M = SYS_TISTS12M_CLKERRIF_Msk;
	}

	/* 48 MHz HIRC48 */
	if (SYS->TISTS48M & SYS_TISTS48M_TFAILIF_Msk) /* Get Trim Failure Interrupt */
	{
		ca_log_warn("HIRC48 Trim Failure Interrupt");
		SYS->TISTS48M = SYS_TISTS48M_TFAILIF_Msk;
	}
	if (SYS->TISTS48M & SYS_TISTS48M_CLKERRIF_Msk) /* Get Clock Error Interrupt */
	{
		ca_log_warn("HIRC48 Clock Error Interrupt");
		SYS->TISTS48M = SYS_TISTS48M_CLKERRIF_Msk;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 2: Clock controller interrupt for wake-up from power-down state
 *******************************************************************************
 ******************************************************************************/
void PWRWU_IRQHandler(void)
{
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 4: Clock fail detected interrupt
 *******************************************************************************
 ******************************************************************************/
void CLKFAIL_IRQHandler(void)
{
	uint32_t u32Reg;

	/* Unlock protected registers */
	SYS_UnlockReg();

	u32Reg = CLK->CLKDSTS;

	if (u32Reg & CLK_CLKDSTS_HXTFIF_Msk)
	{
		/* HCLK is switched to HIRC automatically if HXT clock fail interrupt is happened */
		ca_log_warn("HXT Clock is stopped! HCLK is switched to HIRC.");
		/* Disable HXT clock fail interrupt */
		CLK->CLKDCTL &= ~(CLK_CLKDCTL_HXTFDEN_Msk | CLK_CLKDCTL_HXTFIEN_Msk);
		/* Write 1 to clear HXT Clock fail interrupt flag */
		CLK->CLKDSTS = CLK_CLKDSTS_HXTFIF_Msk;
	}

	if (u32Reg & CLK_CLKDSTS_LXTFIF_Msk)
	{
		/* LXT clock fail interrupt is happened */
		ca_log_warn("LXT Clock is stopped!");
		/* Disable LXT clock fail interrupt */
		CLK->CLKDCTL &= ~(CLK_CLKDCTL_LXTFIEN_Msk | CLK_CLKDCTL_LXTFDEN_Msk);
		/* Write 1 to clear LXT Clock fail interrupt flag */
		CLK->CLKDSTS = CLK_CLKDSTS_LXTFIF_Msk;
	}

	if (u32Reg & CLK_CLKDSTS_HXTFQIF_Msk)
	{
		/* HCLK should be switched to HIRC if HXT clock frequency monitor interrupt is happened */
		CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
		ca_log_warn("HXT Frequency is abnormal! HCLK is switched to HIRC.");
		/* Disable HXT clock frequency monitor interrupt */
		CLK->CLKDCTL &= ~(CLK_CLKDCTL_HXTFQDEN_Msk | CLK_CLKDCTL_HXTFQIEN_Msk);
		/* Write 1 to clear HXT Clock frequency monitor interrupt */
		CLK->CLKDSTS = CLK_CLKDSTS_HXTFQIF_Msk;
	}

	/* Lock protected registers */
	SYS_LockReg();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 6: RTC interrupt
 *******************************************************************************
 ******************************************************************************/
void RTC_IRQHandler(void)
{
	/* RTC alarm interrupt */
	if (RTC_GET_ALARM_INT_FLAG(RTC))
	{
		/* Clear RTC alarm interrupt flag */
		RTC_CLEAR_ALARM_INT_FLAG(RTC);
		/* rtc can act as watchdog during powerdown */
		if (asleep)
			WDTimeout = 1;
		else
			CHILI_RTCIRQHandler();
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 32: Timer 0 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR0_IRQHandler(void)
{
	/* clear all timer interrupt status bits */
	TIMER0->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;

	if (!asleep)
		CHILI_1msTick(); /* timer0 for system ticks */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 33: Timer 1 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR1_IRQHandler(void)
{
	/* clear all timer interrupt status bits */
	TIMER1->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 40: PDMA0 interrupt
 *******************************************************************************
 ******************************************************************************/
void PDMA0_IRQHandler(void)
{
	/* Get PDMA interrupt status */
	uint32_t u32Status = PDMA_GET_INT_STATUS(PDMA0);
	if (u32Status & PDMA_INTSTS_ABTIF_Msk) /* Target Abort */
	{
		ca_log_crit("DMA ABORT");
		PDMA0->ABTSTS = PDMA0->ABTSTS;
		return;
	}
#if defined(USE_UART)
	if (u32Status & PDMA_INTSTS_TDIF_Msk) /* Transfer Done */
		CHILI_UARTDMAIRQHandler();
#endif /* USE_UART */
}
