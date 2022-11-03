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
 * MCU:    Nuvoton Nano120
 * MODULE: Chili 1 (1.2, 1.3)
 * Internal (Non-Interface) Functions
*/
/* System */
#include <stdio.h>
/* Platform */
#include "Nano100Series.h"
#include "adc.h"
#include "gpio.h"
#include "spi.h"
#include "sys.h"
#include "timer.h"
#include "uart.h"
#include "usbd.h"
/* Cascoda */
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */

#if defined(USE_UART)
void CHILI_UARTInit(void)
{
	/* UART1 Clock Enable */
	CLK->APBCLK |= CLK_APBCLK_UART1_EN;
	if (UseExternalClock)
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART_S_Msk) | CLK_CLKSEL1_UART_S_HXT;
	else
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART_S_Msk) | CLK_CLKSEL1_UART_S_HIRC;

	/* Initialise UART */
	UART_SetLine_Config(UART1, 115200, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);
	UART_Open(UART1, 115200);
	UART_EnableInt(UART1, UART_IER_RDA_IE_Msk);
	NVIC_EnableIRQ(UART1_IRQn);
}

void CHILI_UARTDeinit(void)
{
	NVIC_DisableIRQ(UART1_IRQn);
	UART_DisableInt(UART1, UART_IER_RDA_IE_Msk);
	UART_Close(UART1);
}

#endif /* USE_UART */

u32_t CHILI_ADCConversion(u32_t channel, u32_t reference)
{
	u32_t adcval;

	if (reference == ADC_REFSEL_INT_VREF)
	{
		SYS_UnlockReg();
		SYS->Int_VREFCTL = SYS_VREFCTL_BGP_EN | SYS_VREFCTL_REG_EN; /* internal 1.8V reference */
		SYS_LockReg();
	}

	/* scale ADC clock to 1 MHz */
	if (UseExternalClock)
		CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HXT, CLK_ADC_CLK_DIVIDER(4));
	else
		CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HIRC, CLK_ADC_CLK_DIVIDER(12));
	CLK_EnableModuleClock(ADC_MODULE);

	if (reference == ADC_REFSEL_INT_VREF)
	{
		/* wait 50 ms for references to settle */
		BSP_WaitTicks(50);
	}

	/* enable ADC, single-ended mode */
	ADC_Open(ADC, ADC_INPUT_MODE_SINGLE_END, ADC_OPERATION_MODE_SINGLE, (1UL << channel));
	ADC_SET_REF_VOLTAGE(ADC, reference);
	ADC_SetExtraSampleTime(ADC, channel, 0); /* no further sampling time extension */

	ADC_POWER_ON(ADC);
	ADC_START_CONV(ADC);

	while (ADC_IS_BUSY())
		;

	adcval = ADC_GET_CONVERSION_DATA(ADC, channel);

	/* disable adc and references */
	ADC_POWER_DOWN(ADC);
	ADC_Close(ADC);
	SYS_UnlockReg();
	SYS->Int_VREFCTL = 0;
	SYS_LockReg();

	return (adcval);
}

void CHILI_GPIOInit(void)
{
	/* Note that pull-ups are configured for each used GPIO regardless of default */

	/* Initialise and set Debounce to 8 HCLK cycles */
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_HCLK, GPIO_DBCLKSEL_8);

	/* VOLTS_TEST */
	/* output, no pull-up, set to 0 */
	GPIO_SetMode(VOLTS_TEST_PORT, BITMASK(VOLTS_TEST_PIN), GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(VOLTS_TEST_PORT, BITMASK(VOLTS_TEST_PIN));
	VOLTS_TEST_PVAL = 0; /* VOLTS_TEST is active high, so switch off to avoid unecessary power consumption */

	/* ZIG_RESET */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN));
	ZIG_RESET_PVAL = 1; /* RSTB is HIGH */

	/* ZIG_IRQB */
	/* input, pull-up */
	GPIO_SetMode(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN), GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN));

#if (CASCODA_CHILI_REV == 2)
	/* BATT_ON */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(BATT_ON_PORT, BITMASK(BATT_ON_PIN), GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(BATT_ON_PORT, BITMASK(BATT_ON_PIN));
	BATT_ON_PVAL = 1; /* BATT_ON is active high, so switch on for charging */
#endif

	/* SPI1: MFP but SS0 controlled separately as GPIO  */
	CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_SPI);
	GPIO_DISABLE_PULL_UP(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN));
	GPIO_DISABLE_PULL_UP(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN));
	GPIO_DISABLE_PULL_UP(SPI_SCLK_PORT, BITMASK(SPI_SCLK_PIN));

	/* SPI_CS */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(SPI_CS_PORT, BITMASK(SPI_CS_PIN));
	SPI_CS_PVAL = 1;

	/* CHARGE_STAT */
	/* input, pull-up, debounced */
	GPIO_SetMode(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN), GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN)); /* CHARGE_STAT is low or tri-state */
	GPIO_ENABLE_DEBOUNCE(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN));

#if (CASCODA_CHILI_REV == 3)
	/* PROGRAM Pin */
	/* enable pull-up just in case */
	GPIO_SetMode(PROGRAM_PORT, BITMASK(PROGRAM_PIN), GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PROGRAM_PORT, BITMASK(PROGRAM_PIN));
#endif

#if defined(USE_UART)
	/* UART1 if used */
	CHILI_ModuleSetMFP(UART_TXD_PNUM, UART_TXD_PIN, PMFP_UART);
	CHILI_ModuleSetMFP(UART_RXD_PNUM, UART_RXD_PIN, PMFP_UART);
#endif /* USE_UART */
}

void CHILI_GPIOPowerDown(u8_t tristateCax)
{
	/* ZIG_RESET */
	/* tri-state - output to input */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_PMD_INPUT);

	/* ZIG_IRQB */
	/* disable pull-up */
	GPIO_DISABLE_PULL_UP(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN));

	if (tristateCax)
	{
		/* SPI: */
		CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_GPIO);
		CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_GPIO);
		CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_GPIO);
		GPIO_SetMode(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_PMD_INPUT);
		GPIO_SetMode(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_PMD_INPUT);
		GPIO_SetMode(SPI_SCLK_PORT, BITMASK(SPI_SCLK_PIN), GPIO_PMD_INPUT);
	}

	/* SPI_CS */
	/* tri-state - output to input */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_PMD_INPUT);

	/* CHARGE_STAT */
	/* disable pull-up */
	GPIO_DISABLE_PULL_UP(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN));

	/* dynamic GPIO handling */
	CHILI_ModulePowerDownGPIOs();
}

void CHILI_GPIOPowerUp(void)
{
	/* ZIG_RESET */
	/* input to output */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_PMD_OUTPUT);

	/* ZIG_IRQB */
	/* enable pull-up */
	GPIO_ENABLE_PULL_UP(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN));

	/* SPI1: */
	CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_SPI);

	/* SPI_CS */
	/* input to output */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_PMD_OUTPUT);

	/* CHARGE_STAT */
	/* enable pull-up */
	GPIO_ENABLE_PULL_UP(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN));

	/* dynamic GPIO handling */
	CHILI_ModulePowerUpGPIOs();
}

void CHILI_GPIOEnableInterrupts(void)
{
	/* RFIRQ */
	GPIO_EnableInt(ZIG_IRQB_PORT, ZIG_IRQB_PIN, GPIO_INT_FALLING);

	/* NVIC Enable */
	NVIC_EnableIRQ(ZIG_IRQB_IRQn);
}

ca_error CHILI_ClockInit(void)
{
	uint32_t delayCnt;
	ca_error status = CA_ERROR_SUCCESS;

	/* NOTE: __HXT in system_Nano100Series.h has to be changed to correct input clock frequency !! */
#if __HXT != 4000000UL
#error "__HXT Not set correctly in system_Nano100Series.h"
#endif

	if (UseExternalClock) /* 4 MHz clock supplied externally via HXT */
	{
		/* HXT enabled */
		SYS_UnlockReg();
		CLK->PWRCTL &= ~(CLK_PWRCTL_HXT_SELXT); /* switch off HXT feedback for external clock */
		CLK->PWRCTL |= (CLK_PWRCTL_HXT_GAIN | CLK_PWRCTL_HXT_EN | CLK_PWRCTL_LXT_EN | CLK_PWRCTL_LIRC_EN);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->CLKSTATUS & CLK_CLKSTATUS_HXT_STB)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			UseExternalClock = 0;
			status           = CA_ERROR_FAIL;
		}
	}

	if (!UseExternalClock) /* 12 MHz clock from HIRC */
	{
		/* HIRC enabled */
		SYS_UnlockReg();
		CLK->PWRCTL |= (CLK_PWRCTL_HIRC_EN | CLK_PWRCTL_LXT_EN | CLK_PWRCTL_LIRC_EN);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->CLKSTATUS & CLK_CLKSTATUS_HIRC_STB)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			status = CA_ERROR_FAIL;
		}
	}

	if (USBPresent) /* USB connected and supplies power - switch on PLL */
	{
		if (UseExternalClock) /* 4MHz HXT -> 48MHz PLL */
		{
			if (CLK_EnablePLL(CLK_PLLCTL_PLL_SRC_HXT, FREQ_48MHZ) != FREQ_48MHZ)
			{
				status = CA_ERROR_FAIL;
			}
		}
		else
		{
			if (CLK_EnablePLL(CLK_PLLCTL_PLL_SRC_HIRC, FREQ_48MHZ) != FREQ_48MHZ)
			{
				status = CA_ERROR_FAIL;
			}
		}
		for (delayCnt = 0; delayCnt < MAX_CLOCK_SWITCH_DELAY; delayCnt++)
		{
			if (CLK->CLKSTATUS & CLK_CLKSTATUS_PLL_STB)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			status = CA_ERROR_FAIL;
		}
	}

	if (USBPresent) /* USB connected and supplies power - HCLK and USB from PLL */
	{
		SYS_UnlockReg();
		/* divider is 3  -> HCLK = 16 MHz */
		CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_PLL, CLK_HCLK_CLK_DIVIDER(3));
		/* Select IP clock source */
		CLK_SetModuleClock(USBD_MODULE, 0, CLK_USB_CLK_DIVIDER(1));
		/* Enable IP clock */
		CLK_EnableModuleClock(USBD_MODULE);
		SYS_LockReg();
	}
	else /* USB unplugged, power from battery */
	{
		if (UseExternalClock)
		{
			SYS_UnlockReg();
			/* 4 MHz clock supplied externally via HXT */
			CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HXT, CLK_HCLK_CLK_DIVIDER(1));
			SYS_LockReg();
		}
		else
		{
			SYS_UnlockReg();
			/* 12 MHz clock from HIRC */
			CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_HCLK_CLK_DIVIDER(3));
			SYS_LockReg();
		}
		CLK_DisablePLL(); /* PLL Off */
	}

	/* Set HCLK back to HIRC if clock switching error happens */
	if (CLK->CLKSTATUS & CLK_CLKSTATUS_CLK_SW_FAIL)
	{
		SYS_UnlockReg();
		/* HCLK = 12MHz */
		CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_HCLK_CLK_DIVIDER(1));
		SYS_LockReg();
		status = CA_ERROR_FAIL;
	}

	/* check clock frequency and signal problems */
	SystemCoreClockUpdate();
	if (((USBPresent) && (UseExternalClock) && (SystemCoreClock != 16000000)) ||
	    ((USBPresent) && (!UseExternalClock) && (SystemCoreClock != 16000000)) ||
	    ((!USBPresent) && (UseExternalClock) && (SystemCoreClock != 4000000)) ||
	    ((!USBPresent) && (!UseExternalClock) && (SystemCoreClock != 4000000)))
	{
		status = CA_ERROR_FAIL;
	}

	return (status);
}

void CHILI_CompleteClockInit(void)
{
	if (!(CLK->CLKSTATUS & CLK_CLKSTATUS_CLK_SW_FAIL))
	{
		if (UseExternalClock)
		{
			/* disable HIRC */
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HIRC_EN);
			SYS_LockReg();
		}
		else
		{
			/* disable HXT */
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HXT_EN);
			SYS_LockReg();
		}
	}
}

void CHILI_EnableIntOscCal(void)
{
	/* enable HIRC trimming to 12 MHz if HIRC is used and LXT is present */
	SYS->IRCTRIMINT = 0xFFFF;
	if ((!UseExternalClock) && (CLK->CLKSTATUS & CLK_CLKSTATUS_LXT_STB))
	{
		/* trim HIRC to 12MHz */
		SYS->IRCTRIMCTL = (SYS_IRCTRIMCTL_TRIM_12M | SYS_IRCTRIMCTL_LOOP_32CLK | SYS_IRCTRIMCTL_RETRY_512);
		/* enable HIRC-trim interrupt */
		NVIC_EnableIRQ(HIRC_IRQn);
		SYS->IRCTRIMIEN = (SYS_IRCTRIMIEN_FAIL_EN | SYS_IRCTRIMIEN_32KERR_EN);
	}
}

void CHILI_DisableIntOscCal(void)
{
	SYS->IRCTRIMINT = 0xFFFF;
	SYS->IRCTRIMIEN = 0;
	SYS->IRCTRIMCTL = 0;
}

void CHILI_TimersInit(void)
{
	/* TIMER0: millisecond periodic tick (AbsoluteTicks) and power-down wake-up */
	/* TIMER1: microsecond timer / counter */
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HXT, 0);
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1_S_HXT, 0);
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);

	/* configure clock selects and dividers for timers 0 and 1 */
	if (UseExternalClock)
	{
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_HXT;
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR1_S_Msk) | CLK_CLKSEL1_TMR1_S_HXT;
		/*  4 MHZ Clock: prescaler 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 3);
		/*  4 MHZ Clock: prescalar 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 3);
	}
	else
	{
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_HIRC;
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR1_S_Msk) | CLK_CLKSEL1_TMR1_S_HIRC;
		/* 12 MHZ Clock: prescaler 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 11);
		/* 12 MHZ Clock: prescalar 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 11);
	}
	TIMER0->CTL = TIMER_PERIODIC_MODE;
	TIMER1->CTL = TIMER_CONTINUOUS_MODE;
	/* 1uSec units, so 1000 is 1ms */
	TIMER_SET_CMP_VALUE(TIMER0, 1000);
	/* 1uSec units, counts microseconds */
	TIMER_SET_CMP_VALUE(TIMER1, 0xFFFFFF);

	NVIC_EnableIRQ(TMR0_IRQn);
	TIMER_EnableInt(TIMER0);
	TIMER_Start(TIMER0);
}

void CHILI_SystemReInit()
{
#if defined(USE_USB)
	static u8_t USBPresent_d = 0;
#endif /* USE_USB */

/* switch off USB comms if USBPResent has changed and disconnected */
#if defined(USE_USB)
	if (!USBPresent && USBPresent_d)
	{
		USBD_DISABLE_USB();
		CLK_DisableModuleClock(USBD_MODULE);
		NVIC_DisableIRQ(USBD_IRQn);
	}
#endif /* USE_USB */
#if defined(USE_UART)
	CHILI_UARTDeinit();
#endif /* USE_UART */

	/* configure clocks */
	CHILI_ClockInit();
	WDT_Close();

/* switch on USB comms if USBPResent has changed and connected */
#if defined(USE_USB)
	if (USBPresent && !USBPresent_d)
		USB_Initialise();
#endif /* USE_USB */
#if defined(USE_UART)
	CHILI_UARTInit();
#endif /* USE_UART */

	/* initialise timers */
	CHILI_TimersInit();

	/* finish clock initialisation */
	/* this has to be done AFTER all other system config changes ! */
	CHILI_CompleteClockInit();

	/* re-initialise SPI clock rate */
	SPI_SetBusClock(SPI1, FCLK_SPI);

	/* set interrupt priorities */
	/* 0: highest  timers */
	/* 1:          upstream dispatch and gpios */
	/* 2:          upstream   dispatch */
	/* 3: lowest */
	NVIC_SetPriority(TMR0_IRQn, 0);
	NVIC_SetPriority(TMR1_IRQn, 0);
	NVIC_SetPriority(TMR2_IRQn, 0);
	NVIC_SetPriority(TMR3_IRQn, 0);
	NVIC_SetPriority(USBD_IRQn, 2);
	NVIC_SetPriority(GPABC_IRQn, 1);
	NVIC_SetPriority(GPDEF_IRQn, 1);
	NVIC_SetPriority(UART1_IRQn, 2);

#if defined(USE_USB)
	USBPresent_d = USBPresent;
#endif /* USE_USB */
}
