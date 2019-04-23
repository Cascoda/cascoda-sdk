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
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */
#ifdef USE_DEBUG
#include "cascoda_debug_chili.h"
#endif /* USE_DEBUG */

/* LED RGB Values */
u8_t  LED_R_VAL            = 0;   /* Red */
u8_t  LED_G_VAL            = 0;   /* Green */
u16_t LED_R_BlinkTimeCount = 0;   /* Red   Blink Time Interval Counter */
u16_t LED_G_BlinkTimeCount = 0;   /* Green Blink Time Interval Counter */
u16_t LED_R_OnTime         = 100; /* Red   On  Time [10 ms] */
u16_t LED_G_OnTime         = 100; /* Green On  Time [10 ms] */
u16_t LED_R_OffTime        = 0;   /* Red   Off Time [10 ms] */
u16_t LED_G_OffTime        = 0;   /* Green Off Time [10 ms] */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief LEDSetClrs LED Set/Clear Functions
 *******************************************************************************
 ******************************************************************************/
void CHILI_LED_SetRED(u16_t ton, u16_t toff)
{
	LED_R_VAL     = 1;
	LED_R_OnTime  = ton;
	LED_R_OffTime = toff;
}
void CHILI_LED_SetGREEN(u16_t ton, u16_t toff)
{
	LED_G_VAL     = 1;
	LED_G_OnTime  = ton;
	LED_G_OffTime = toff;
}
void CHILI_LED_ClrRED(void)
{
	LED_R_VAL = 0;
}
void CHILI_LED_ClrGREEN(void)
{
	LED_G_VAL = 0;
}

#if defined(USE_UART)

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise UART for Comms
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief De-Initialise UART
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTDeinit(void)
{
	NVIC_DisableIRQ(UART1_IRQn);
	UART_DisableInt(UART1, UART_IER_RDA_IE_Msk);
	UART_Close(UART1);
}

#endif /* USE_UART */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Perform ADC Conversion
 *******************************************************************************
 ******************************************************************************/
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
		TIME_WaitTicks(50);
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief LED Blinking for TMR2_IRQHandler ISR
 *******************************************************************************
 ******************************************************************************/
void CHILI_LEDBlink(void)
{
	if ((LED_R_BlinkTimeCount == 0) && (LED_R_VAL == 1))
		PA14 = 0;
	else if ((LED_R_BlinkTimeCount > LED_R_OnTime) || (LED_R_VAL == 0))
		PA14 = 1;

	if ((LED_G_BlinkTimeCount == 0) && (LED_G_VAL == 1))
		PA13 = 0;
	else if ((LED_G_BlinkTimeCount > LED_G_OnTime) || (LED_G_VAL == 0))
		PA13 = 1;

	if ((LED_R_BlinkTimeCount >= LED_R_OnTime + LED_R_OffTime) || (LED_R_VAL == 0))
		LED_R_BlinkTimeCount = 0;
	else
		LED_R_BlinkTimeCount += 1;

	if ((LED_G_BlinkTimeCount >= LED_G_OnTime + LED_G_OffTime) || (LED_G_VAL == 0))
		LED_G_BlinkTimeCount = 0;
	else
		LED_G_BlinkTimeCount += 1;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise Essential GPIOs for various Functions
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOInit(void)
{
	/* Note that pull-ups are configured for each used GPIO regardless of default */

	/* Initialise and set Debounce to 8 HCLK cycles */
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_HCLK, GPIO_DBCLKSEL_8);

	/* PB.12: VOLTS_TEST */
	/* output, no pull-up, set to 0 */
	GPIO_SetMode(PB, BIT12, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PB, BIT12);
	PB12 = 0; /* VOLTS_TEST is active high, so switch off to avoid unecessary power consumption */

	/* PA.11: ZIG_RESET */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(PA, BIT11, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT11);
	PA11 = 1; /* RSTB is HIGH */

	/* PA.10: ZIG_IRQB */
	/* input, pull-up */
	GPIO_SetMode(PA, BIT10, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT10);

	/* PA.9: SWITCH */
	/* input, no pull-up, debounced */
	GPIO_SetMode(PA, BIT9, GPIO_PMD_INPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT9);
	GPIO_ENABLE_DEBOUNCE(PA, BIT9);

#if (CASCODA_CHILI_REV == 2)
	/* PA.8: BATT_ON */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(PA, BIT8, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT8);
	PA8 = 1; /* BATT_ON is active high, so switch on for charging */
#endif

	/* SPI1: MFP but SS0 controlled separately as GPIO  */
	/* PB.0: SPI1_MOSI0 MFP output */
	/* PB.1: SPI1_MISO0 MFP input */
	/* PB.2: SPI1_SCLK  MFP output */
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB0_MFP_Msk)) | SYS_PB_L_MFP_PB0_MFP_SPI1_MOSI0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB1_MFP_Msk)) | SYS_PB_L_MFP_PB1_MFP_SPI1_MISO0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB2_MFP_Msk)) | SYS_PB_L_MFP_PB2_MFP_SPI1_SCLK;
	GPIO_DISABLE_PULL_UP(PB, BIT0);
	GPIO_DISABLE_PULL_UP(PB, BIT1);
	GPIO_DISABLE_PULL_UP(PB, BIT2);

	/* PB.3: SPI_CS */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(PB, BIT3, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PB, BIT3);
	PB3 = 1;

	/* PA.14: LED_R */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(PA, BIT14, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT14);
	PA14 = 1;

	/* PA.13: LED_G */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(PA, BIT13, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT13);
	PA13 = 1;

	/* PA.12: CHARGE_STAT */
	/* input, pull-up, debounced */
	GPIO_SetMode(PA, BIT12, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT12); /* CHARGE_STAT is low or tri-state */
	GPIO_ENABLE_DEBOUNCE(PA, BIT12);

	/* PF.0: ICE_DAT MFP */
	GPIO_DISABLE_PULL_UP(PF, BIT0);

	/* PF.1: ICE_CLK MFP */
	GPIO_DISABLE_PULL_UP(PF, BIT1);

	/* TODO: This is causing some current leakage when sleeping: */
	/* PA.0: AD0 MFP (VOLTS) */
	/* Comment out for better power saving: */
	SYS->PA_L_MFP = (SYS->PA_L_MFP & (~SYS_PA_L_MFP_PA0_MFP_Msk)) | SYS_PA_L_MFP_PA0_MFP_ADC_CH0;
	GPIO_DISABLE_DIGITAL_PATH(PA, BIT0);
	/* Uncomment for better power saving: */
	// GPIO_SetMode(PA, BIT0, GPIO_PMD_INPUT)
	GPIO_DISABLE_PULL_UP(PA, BIT0);

	/* PB.15/PC.7: USB_PRESENT */
	/* input, no pull-up, debounced */
	GPIO_SetMode(USBP_PORT, USBP_PINMASK, GPIO_PMD_INPUT);
	GPIO_DISABLE_PULL_UP(USBP_PORT, USBP_PINMASK);
	GPIO_ENABLE_DEBOUNCE(USBP_PORT, USBP_PINMASK);
	/* Immediately get USBPresent State */
	USBPresent = USBP_GPIO;

#if (CASCODA_CHILI_REV == 3)
	/* PB.15: PROGRAM Pin */
	/* enable pull-up just in case */
	GPIO_SetMode(PB, BIT15, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PB, BIT15);
#endif

	/* PF.3: XT1_IN MFP */
	GPIO_DISABLE_PULL_UP(PF, BIT3);

	/* PF.2: XT1_OUT MFP */
	GPIO_DISABLE_PULL_UP(PF, BIT2);

#if defined(USE_UART)
	/* UART1 if used */
	/* PB.4: UART1_RX MFP input */
	/* PB.5: UART1_TX MFP output */
	SYS->PB_L_MFP &= ~(SYS_PB_L_MFP_PB4_MFP_Msk | SYS_PB_L_MFP_PB5_MFP_Msk);
	SYS->PB_L_MFP |= (SYS_PB_L_MFP_PB4_MFP_UART1_RX | SYS_PB_L_MFP_PB5_MFP_UART1_TX);
#endif /* USE_UART */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for PowerDown
 * \params tristateCax - bool: set to 1 to tri-state the CA-821x interface
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerDown(u8_t tristateCax)
{
	/* PA.11: ZIG_RESET */
	/* tri-state - output to input */
	GPIO_SetMode(PA, BIT11, GPIO_PMD_INPUT);

	/* PA.10: ZIG_IRQB */
	/* disable pull-up */
	GPIO_DISABLE_PULL_UP(PA, BIT10);

	if (tristateCax)
	{
		/* SPI1: */
		/* PB.0: tri-state - output to input */
		/* PB.1: tri-state - input  to input */
		/* PB.2: tri-state - output to input */
		SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB0_MFP_Msk)) | SYS_PB_L_MFP_PB0_MFP_GPB0;
		SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB1_MFP_Msk)) | SYS_PB_L_MFP_PB1_MFP_GPB1;
		SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB2_MFP_Msk)) | SYS_PB_L_MFP_PB2_MFP_GPB2;
		GPIO_SetMode(PB, BIT2, GPIO_PMD_INPUT);
		GPIO_SetMode(PB, BIT1, GPIO_PMD_INPUT);
		GPIO_SetMode(PB, BIT0, GPIO_PMD_INPUT);
	}

	/* PB.3: SPI_CS */
	/* tri-state - output to input */
	GPIO_SetMode(PB, BIT3, GPIO_PMD_INPUT);

	/* PA.14: LED_R */
	/* tri-state - output to input - use pull-up otherwise light-dependent photodiode effect! */
	PA14 = 1;
	GPIO_SetMode(PA, BIT14, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT14);

	/* PA.13: LED_G */
	/* tri-state - output to input - use pull-up otherwise light-dependent photodiode effect! */
	PA13 = 1;
	GPIO_SetMode(PA, BIT13, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT13);

	/* PA.12: CHARGE_STAT */
	/* disable pull-up */
	GPIO_DISABLE_PULL_UP(PA, BIT12);

	/* reset LED counters */
	LED_R_BlinkTimeCount = 0;
	LED_G_BlinkTimeCount = 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for active Mode after PowerDown
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerUp(void)
{
	/* PA.11: ZIG_RESET */
	/* input to output */
	GPIO_SetMode(PA, BIT11, GPIO_PMD_OUTPUT);

	/* PA.10: ZIG_IRQB */
	/* enable pull-up */
	GPIO_ENABLE_PULL_UP(PA, BIT10);

	/* SPI1: */
	/* PB.0: tri-state - input to SPI1_MOSI0 */
	/* PB.1: tri-state - input to SPI1_MISO0 */
	/* PB.2: tri-state - input to SPI1_SCLK */
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB0_MFP_Msk)) | SYS_PB_L_MFP_PB0_MFP_SPI1_MOSI0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB1_MFP_Msk)) | SYS_PB_L_MFP_PB1_MFP_SPI1_MISO0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB2_MFP_Msk)) | SYS_PB_L_MFP_PB2_MFP_SPI1_SCLK;

	/* PB.3: SPI_CS */
	/* input to output */
	GPIO_SetMode(PB, BIT3, GPIO_PMD_OUTPUT);

	/* PA.14: LED_R */
	/* input to output */
	GPIO_DISABLE_PULL_UP(PA, BIT14);
	GPIO_SetMode(PA, BIT14, GPIO_PMD_OUTPUT);

	/* PA.13: LED_G */
	/* input to output */
	GPIO_DISABLE_PULL_UP(PA, BIT13);
	GPIO_SetMode(PA, BIT13, GPIO_PMD_OUTPUT);

	/* PA.12: CHARGE_STAT */
	/* enable pull-up */
	GPIO_ENABLE_PULL_UP(PA, BIT12);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable GPIO Interrupts
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOEnableInterrupts(void)
{
	/* SWITCH PA.9 */
	GPIO_EnableInt(PA, 9, GPIO_INT_FALLING);

	/* RFIRQ */
	GPIO_EnableInt(PA, 10, GPIO_INT_FALLING);

	/* PB.15/PC.7: USBPresent */
	GPIO_EnableInt(USBP_PORT, USBP_PIN, GPIO_INT_BOTH_EDGE);

	/* NVIC Enable GP A/B/C IRQ */
	NVIC_EnableIRQ(GPABC_IRQn);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Select System Clocks depending on Power Source
 *******************************************************************************
 ******************************************************************************/
void CHILI_ClockInit(void)
{
	uint32_t delayCnt;

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
			BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
			BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_1);
#endif /* USE_DEBUG */
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
			BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
			BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_2);
#endif /* USE_DEBUG */
		}
	}

	if (USBPresent) /* USB connected and supplies power - switch on PLL */
	{
		if (UseExternalClock) /* 4MHz HXT -> 48MHz PLL */
		{
			if (CLK_EnablePLL(CLK_PLLCTL_PLL_SRC_HXT, FREQ_48MHZ) != FREQ_48MHZ)
			{
				BSP_LEDSigMode(LED_M_SETERROR);
			}
		}
		else
		{
			if (CLK_EnablePLL(CLK_PLLCTL_PLL_SRC_HIRC, FREQ_48MHZ) != FREQ_48MHZ)
			{
				BSP_LEDSigMode(LED_M_SETERROR);
			}
		}
		for (delayCnt = 0; delayCnt < MAX_CLOCK_SWITCH_DELAY; delayCnt++)
		{
			if (CLK->CLKSTATUS & CLK_CLKSTATUS_PLL_STB)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
			if ((!USBPresent) && (!UseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_3);
			else if ((!USBPresent) && (UseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_4);
			else if ((USBPresent) && (!UseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_5);
			else if ((USBPresent) && (UseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_6);
			else
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_7);
#endif /* USE_DEBUG */
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
		BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
		BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_8);
#endif /* USE_DEBUG */
	}

	/* check clock frequency and signal problems */
	SystemCoreClockUpdate();
	if (((USBPresent) && (UseExternalClock) && (SystemCoreClock != 16000000)) ||
	    ((USBPresent) && (!UseExternalClock) && (SystemCoreClock != 16000000)) ||
	    ((!USBPresent) && (UseExternalClock) && (SystemCoreClock != 4000000)) ||
	    ((!USBPresent) && (!UseExternalClock) && (SystemCoreClock != 4000000)))
	{
		BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
		BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_9);
#endif /* USE_DEBUG */
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Clock (Re-)Initialisation.
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_DisableIntOscCal(void)
{
	SYS->IRCTRIMINT = 0xFFFF;
	SYS->IRCTRIMIEN = 0;
	SYS->IRCTRIMCTL = 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief (Re-)Initialise System Timers
 *******************************************************************************
 ******************************************************************************/
void CHILI_TimersInit(void)
{
	/* TIMER0: millisecond periodic tick (AbsoluteTicks) and power-down wake-up */
	/* TIMER1: microsecond timer / counter */
	/* TIMER2: 10 millisecond periodic tick for LED Blink */
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HXT, 0);
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1_S_HXT, 0);
	CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL2_TMR2_S_HXT, 0);
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);
	CLK_EnableModuleClock(TMR2_MODULE);

	/* configure clock selects and dividers for timers 0 and 1 */
	if (UseExternalClock)
	{
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_HXT;
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR1_S_Msk) | CLK_CLKSEL1_TMR1_S_HXT;
		CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_TMR2_S_Msk) | CLK_CLKSEL2_TMR2_S_HXT;
		/*  4 MHZ Clock: prescaler 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 3);
		/*  4 MHZ Clock: prescalar 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 3);
		/*  4 MHZ Clock: prescaler 79 gives 20 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER2, 79);
	}
	else
	{
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_HIRC;
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR1_S_Msk) | CLK_CLKSEL1_TMR1_S_HIRC;
		CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_TMR2_S_Msk) | CLK_CLKSEL2_TMR2_S_HIRC;
		/* 12 MHZ Clock: prescaler 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 11);
		/* 12 MHZ Clock: prescalar 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 11);
		/* 12 MHZ Clock: prescaler 239 gives 20 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER2, 239);
	}
	TIMER0->CTL = TIMER_PERIODIC_MODE;
	TIMER1->CTL = TIMER_CONTINUOUS_MODE;
	TIMER2->CTL = TIMER_PERIODIC_MODE;
	/* 1uSec units, so 1000 is 1ms */
	TIMER_SET_CMP_VALUE(TIMER0, 1000);
	/* 1uSec units, counts microseconds */
	TIMER_SET_CMP_VALUE(TIMER1, 0xFFFFFF);
	/* 20uSec units, so 500 is 10ms */
	TIMER_SET_CMP_VALUE(TIMER2, 500);

	NVIC_EnableIRQ(TMR0_IRQn);
	TIMER_EnableInt(TIMER0);
	TIMER_Start(TIMER0);
	NVIC_EnableIRQ(TMR2_IRQn);
	TIMER_EnableInt(TIMER2);
	TIMER_Start(TIMER2);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Re-Initialisation
 *******************************************************************************
 ******************************************************************************/
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
	if (SPI_SetBusClock(SPI1, FCLK_SPI) != FCLK_SPI)
	{
		BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
		BSP_Debug_Error(DEBUG_ERR_SYSTEMINIT);
#endif /* USE_DEBUG */
	}

	/* set interrupt priorities */
	/* 0: highest  timers */
	/* 1:          downstream dispatch and gpios */
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
