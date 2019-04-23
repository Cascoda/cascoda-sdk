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
 * Internal (Non-Interface) Functions
*/
/* System */
#include <stdio.h>
/* Platform */
#include "M2351.h"
#include "eadc.h"
#include "fmc.h"
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
	/* Set multi-function pins for UART0 RXD and TXD */
	SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | UART0_RXD_PB12 | UART0_TXD_PB13;

	if (UseExternalClock)
		CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
	else
		CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(3));
	CLK_EnableModuleClock(UART0_MODULE);

	/* Initialise UART */
	SYS_ResetModule(UART0_RST);
	UART_SetLineConfig(UART0, 115200, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);
	UART_Open(UART0, 115200);
	UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk);
	NVIC_EnableIRQ(UART0_IRQn);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief De-Initialise UART
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTDeinit(void)
{
	NVIC_DisableIRQ(UART0_IRQn);
	UART_DisableInt(UART0, UART_INTEN_RDAIEN_Msk);
	UART_Close(UART0);
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

	/* enable references */
	SYS_UnlockReg();
	SYS->VREFCTL = (SYS->VREFCTL & ~SYS_VREFCTL_VREFCTL_Msk) | reference;
	SYS_LockReg();

	/* enable EADC module clock */
	CLK_EnableModuleClock(EADC_MODULE);

	/* EADC clock source is 16MHz, set divider to 16 -> 1 MHz */
	CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(16));

	/* reset EADC module */
	SYS_ResetModule(EADC_RST);

	if (reference != SYS_VREFCTL_VREF_AVDD)
	{
		/* wait 50 ms for references to settle */
		TIME_WaitTicks(50);
	}

	/* set input mode as single-end and enable the A/D converter */
	EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);

	/* configure the sample module 0 for analog input channel n and software trigger source.*/
	if (channel < 16)
	{
		EADC_ConfigSampleModule(EADC, channel, EADC_SOFTWARE_TRIGGER, channel);
	}

	EADC_SetExtendSampleTime(EADC, channel, 0); /* no further sampling time extension */

	/* clear flags */
	EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);

	/* reset the ADC interrupt indicator and trigger sample module to start A/D conversion */
	EADC_START_CONV(EADC, (1UL << channel));

	/* wait until EADC conversion is done */
	while (!EADC_GET_INT_FLAG(EADC, EADC_STATUS2_AVALID_Msk))
		;

	/* get the conversion result */
	adcval = EADC_GET_CONV_DATA(EADC, channel);

	/* disable adc and references */
	EADC_Close(EADC);
	CLK_DisableModuleClock(EADC_MODULE);

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
		PB2 = 0;
	else if ((LED_R_BlinkTimeCount > LED_R_OnTime) || (LED_R_VAL == 0))
		PB2 = 1;

	if ((LED_G_BlinkTimeCount == 0) && (LED_G_VAL == 1))
		PB3 = 0;
	else if ((LED_G_BlinkTimeCount > LED_G_OnTime) || (LED_G_VAL == 0))
		PB3 = 1;

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
	GPIO_SET_DEBOUNCE_TIME(PA, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(RFIRQ_PORT, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(RFSS_PORT, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(SPI_MISO_PORT, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);

#if (CASCODA_CHILI2_CONFIG == 1)
	/* PB.13: VOLTS_TEST */
	/* output, no pull-up, set to 0 */
	GPIO_SetMode(PB, BIT13, GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(PB, BIT13, GPIO_PUSEL_DISABLE);
	PB13 = 0; /* VOLTS_TEST is active high, so switch off to avoid unecessary power consumption */
#endif

	/* PC.1: ZIG_RESET */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_PUSEL_DISABLE);
	RSTB = 1; /* RSTB is HIGH */

	/* PC.0: ZIG_IRQB */
	/* input, pull-up */
	GPIO_SetMode(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_MODE_INPUT);
	GPIO_SetPullCtl(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_PUSEL_PULL_UP);

	/* PB.4: SWITCH */
	/* input, no pull-up, debounced */
	/* currently no switch so pull up */
	GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_PULL_UP);

	/* SPI: MFP but SS0 controlled separately as GPIO */
	/* SPI_MOSI MFP output */
	/* SPI_MISO MFP input */
	/* SPI_SCLK MFP output */
	SET_MFP_SPI();
	GPIO_SetPullCtl(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_CLK_PORT, BITMASK(SPI_CLK_PIN), GPIO_PUSEL_DISABLE);

	/* PA.3: SPI_CS */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_PUSEL_DISABLE);
	RFSS = 1;

	/* PB.2: LED_R */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(PB, BIT2, GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(PB, BIT2, GPIO_PUSEL_DISABLE);
	PB2 = 1;

	/* PB.3: LED_G */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(PB, BIT3, GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(PB, BIT3, GPIO_PUSEL_DISABLE);
	PB3 = 1;

#if (CASCODA_CHILI2_CONFIG == 1)
	/* PB.0: CHARGE_STAT */
	/* input, pull-up, debounced */
	GPIO_SetMode(PB, BIT0, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_PULL_UP); /* CHARGE_STAT is low or tri-state */
	GPIO_ENABLE_DEBOUNCE(PB, BIT0);
#endif

	/* PF.0: ICE_DAT MFP */

	/* PF.1: ICE_CLK MFP */

#if (CASCODA_CHILI2_CONFIG == 1)
	/* PB.1: EADC0 MFP (VOLTS) */
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB1MFP_Msk)) | SYS_GPB_MFPL_PB1MFP_EADC0_CH1;
	GPIO_DISABLE_DIGITAL_PATH(PB, BIT1);
	GPIO_SetPullCtl(PB, BIT1, GPIO_PUSEL_DISABLE);
#endif

#if (CASCODA_CHILI2_CONFIG == 1)
	/* PB.12: USB_PRESENT */
	/* input, no pull-up, debounced */
	GPIO_SetMode(PB, BIT12, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_DISABLE);
	GPIO_ENABLE_DEBOUNCE(PB, BIT12);
	/* Immediately get USBPresent State */
	USBPresent = PB12;
#else
	USBPresent = 1;
#endif

	/* PB.12: PROGRAM Pin */
	/* currently nor implemented */

	/* PF.3: XT1_IN MFP */
	GPIO_SetPullCtl(PF, BIT3, GPIO_PUSEL_DISABLE);

	/* PF.2: XT1_OUT MFP */
	GPIO_SetPullCtl(PF, BIT2, GPIO_PUSEL_DISABLE);

	/* PF.5: XT32_IN MFP */
	GPIO_SetPullCtl(PF, BIT5, GPIO_PUSEL_DISABLE);

	/* PF.4: XT32_OUT MFP */
	GPIO_SetPullCtl(PF, BIT4, GPIO_PUSEL_DISABLE);

#if defined(USE_UART)
	/* UART0 if used */
	/* PB.12: UART0_RX MFP input */
	/* PB.13: UART0_TX MFP output */
	SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | UART0_RXD_PB12 | UART0_TXD_PB13;
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
	/* PC.1: ZIG_RESET */
	/* tri-state - output to input */
	GPIO_SetMode(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_MODE_INPUT);

	/* PC.0: ZIG_IRQB */
	/* disable pull-up */
	GPIO_SetPullCtl(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_PUSEL_DISABLE);

	if (tristateCax)
	{
		/* SPI: */
		/* SPI_MOSI: tri-state - output to input */
		/* SPI_MISO: tri-state - input  to input */
		/* SPI_CLK:  tri-state - output to input */
		SET_MFP_GPIO();
		GPIO_SetMode(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_CLK_PORT, BITMASK(SPI_CLK_PIN), GPIO_MODE_INPUT);
	}

	/* PA.3: SPI_CS */
	/* tri-state - output to input */
	GPIO_SetMode(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_MODE_INPUT);

	/* PB.2: LED_R */
	/* tri-state - output to input - use pull-up otherwise light-dependent photodiode effect! */
	PB2 = 1;
	GPIO_SetMode(PB, BIT2, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT2, GPIO_PUSEL_PULL_UP);

	/* PB.3: LED_G */
	/* tri-state - output to input - use pull-up otherwise light-dependent photodiode effect! */
	PB3 = 1;
	GPIO_SetMode(PB, BIT3, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT3, GPIO_PUSEL_PULL_UP);

#if (CASCODA_CHILI2_CONFIG == 1)
	/* PB.0: CHARGE_STAT */
	/* disable pull-up */
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_DISABLE);
#endif

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
	/* PC.1: ZIG_RESET */
	/* input to output */
	GPIO_SetMode(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_MODE_OUTPUT);

	/* PC.0: ZIG_IRQB */
	/* enable pull-up */
	GPIO_SetPullCtl(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_PUSEL_PULL_UP);

	/* SPI1: */
	/* PA.0: tri-state - input to SPI1_MOSI0 */
	/* PA.1: tri-state - input to SPI1_MISO0 */
	/* PA.2: tri-state - input to SPI1_SCLK */
	SET_MFP_SPI();

	/* PA.3: SPI_CS */
	/* input to output */
	GPIO_SetMode(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_MODE_OUTPUT);

	/* PB.2: LED_R */
	/* input to output */
	GPIO_SetPullCtl(PB, BIT2, GPIO_PUSEL_DISABLE);
	GPIO_SetMode(PB, BIT2, GPIO_MODE_OUTPUT);

	/* PB.3: LED_G */
	/* input to output */
	GPIO_SetPullCtl(PB, BIT3, GPIO_PUSEL_DISABLE);
	GPIO_SetMode(PB, BIT3, GPIO_MODE_OUTPUT);

	/* PB.0: CHARGE_STAT */
	/* enable pull-up */
#if (CASCODA_CHILI2_CONFIG == 1)
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_PULL_UP);
#endif
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable GPIO Interrupts
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOEnableInterrupts(void)
{
	/* SWITCH PB.4 */

	/* RFIRQ */
	GPIO_EnableInt(RFIRQ_PORT, RFIRQ_PIN, GPIO_INT_FALLING);

#if (CASCODA_CHILI2_CONFIG == 1)
	/* PB.12: USBPresent */
	GPIO_EnableInt(PB, 12, GPIO_INT_BOTH_EDGE);
	/* NVIC Enable GPB IRQ */
	NVIC_EnableIRQ(GPB_IRQn);
#endif
}

/******************************************************************************/
/**************************************************************************/ /**
 * \brief Configure CFGXT1 bit for HXT mode in CONFIG0
 *
 * This isn't specifically related to dataflash, but in order to set the clock
 * on the M2351, the config is loaded into the flash.
 *******************************************************************************
 * \param clk_external - 1: external clock input on HXT, 0: HXT is crystal osc.
 *******************************************************************************
 ******************************************************************************/
void CHILI_SetClockExternalCFGXT1(u8_t clk_external)
{
	const uint32_t DATA_FLASH_CONFIG0_CFGXT1_MASK = 0x08000000; /* mask for CONFIG0 CFGXT1 bit */
	uint32_t       cfg0;

	SYS_UnlockReg();
	FMC_Open();
	cfg0 = FMC_Read(FMC_USER_CONFIG_0);
	if (clk_external)
	{
		if (cfg0 & DATA_FLASH_CONFIG0_CFGXT1_MASK)
		{
			FMC_ENABLE_CFG_UPDATE();
			FMC_Write(FMC_USER_CONFIG_0, cfg0 & ~DATA_FLASH_CONFIG0_CFGXT1_MASK);
			FMC_DISABLE_CFG_UPDATE();
		}
	}
	else
	{
		if (!(cfg0 & DATA_FLASH_CONFIG0_CFGXT1_MASK))
		{
			FMC_ENABLE_CFG_UPDATE();
			FMC_Erase(FMC_CONFIG_BASE);
			FMC_Write(FMC_USER_CONFIG_0, cfg0 | DATA_FLASH_CONFIG0_CFGXT1_MASK);
			FMC_DISABLE_CFG_UPDATE();
		}
	}
	FMC_Close();
	SYS_LockReg();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Select System Clocks depending on Power Source
 *******************************************************************************
 ******************************************************************************/
void CHILI_ClockInit(void)
{
	uint32_t delayCnt;

	/* NOTE: __HXT in system_M2351.h has to be changed to correct input clock frequency !! */
#if __HXT != 4000000UL
#error "__HXT Not set correctly in system_M2351.h"
#endif

	if (UseExternalClock) /* 4 MHz clock supplied externally via HXT */
	{
		/* HXT enabled */
		SYS_UnlockReg();
		CLK->PWRCTL |= (CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->STATUS & CLK_STATUS_HXTSTB_Msk)
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
		CLK->PWRCTL |= (CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->STATUS & CLK_STATUS_HIRCSTB_Msk)
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
		SYS_UnlockReg();
		/*
		*****************************************************************************************
		PLL Setup
		*****************************************************************************************
		FREF =  2 -   8 MHz
		FVCO = 96 - 200 MHz
		FOUT = 24 - 144 MHz
		*****************************************************************************************
		                    FIN FREF FVCO FOUT  NR NF NO INDIV FBDIV OUTDIV   PLLCTL
		*****************************************************************************************
		CLKExternal = 0      12    4   96   48   3 12  2     2    10      1   0x0008440A
		CLKExternal = 1       4    4   96   48   1 12  2     0    10      1   0x0000400A
		*****************************************************************************************
		Note: Using CLK_EnablePLL() does not always give correct results, so avoid!
		*/
		if (UseExternalClock)
			CLK->PLLCTL = 0x0000400A;
		else
			CLK->PLLCTL = 0x0008440A;
		SYS_LockReg();
		for (delayCnt = 0; delayCnt < MAX_CLOCK_SWITCH_DELAY; delayCnt++)
		{
			if (CLK->STATUS & CLK_STATUS_PLLSTB_Msk)
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
		CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(3));
		/* Select IP clock source */
		CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(1));
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
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));
			SYS_LockReg();
		}
		else
		{
			SYS_UnlockReg();
			/* 12 MHz clock from HIRC */
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(3));
			SYS_LockReg();
		}
		CLK_DisablePLL(); /* PLL Off */
	}

	/* Set HCLK back to HIRC if clock switching error happens */
	if (CLK->STATUS & CLK_STATUS_CLKSFAIL_Msk)
	{
		SYS_UnlockReg();
		/* HCLK = 12MHz */
		CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
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
	if (!(CLK->STATUS & CLK_STATUS_CLKSFAIL_Msk))
	{
		if (UseExternalClock)
		{
			/* disable HIRC */
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HIRCEN_Msk);
			SYS_LockReg();
		}
		else
		{
			/* disable HXT */
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HXTEN_Msk);
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
	SYS->TISTS12M = 0xFFFF;
	if ((!UseExternalClock) && (CLK->STATUS & CLK_STATUS_LXTSTB_Msk))
	{
		/* trim HIRC to 12MHz */
		SYS->TCTL12M = 0xF1;
		/* enable HIRC-trim interrupt */
		NVIC_EnableIRQ(CKFAIL_IRQn);
		SYS->TIEN12M = (SYS_TIEN12M_TFAILIEN_Msk | SYS_TIEN12M_CLKEIEN_Msk);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_DisableIntOscCal(void)
{
	SYS->TISTS12M = 0xFFFF;
	SYS->TIEN12M  = 0;
	SYS->TCTL12M  = 0;
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
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);
	CLK_EnableModuleClock(TMR2_MODULE);

	/* configure clock selects and dividers for timers 0 and 1 */
	if (UseExternalClock)
	{
		CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);
		CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
		CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_HXT, 0);
	}
	else
	{
		CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);
		CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
		CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_HIRC, 0);
	}

	TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1000000);
	TIMER_Open(TIMER1, TIMER_CONTINUOUS_MODE, 1000000);
	TIMER_Open(TIMER2, TIMER_PERIODIC_MODE, 50000);

	/* prescale value has to be set after TIMER_Open() is called! */
	if (UseExternalClock)
	{
		/*  4 MHZ Clock: prescaler 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 3);
		/*  4 MHZ Clock: prescalar 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 3);
		/*  4 MHZ Clock: prescaler 79 gives 20 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER2, 79);
	}
	else
	{
		/* 12 MHZ Clock: prescaler 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 11);
		/* 12 MHZ Clock: prescalar 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 11);
		/* 12 MHZ Clock: prescaler 239 gives 20 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER2, 239);
	}

	/* note timers are 24-bit (+ 8-bit prescale) */
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

	/* For some reason, this is what is needed to kick the linker into actually linking the ISR file */
	TMR0_IRQHandler();
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
		USB_Deinitialise();
#endif /* USE_USB */
#if defined(USE_UART)
	CHILI_UARTDeinit();
#endif /* USE_UART */

	/* configure clocks */
	CHILI_ClockInit();

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
	NVIC_SetPriority(GPA_IRQn, 1);
	NVIC_SetPriority(GPB_IRQn, 1);
	NVIC_SetPriority(GPC_IRQn, 1);
	NVIC_SetPriority(GPD_IRQn, 1);
	NVIC_SetPriority(GPE_IRQn, 1);
	NVIC_SetPriority(GPF_IRQn, 1);
	NVIC_SetPriority(UART0_IRQn, 2);

#if defined(USE_USB)
	USBPresent_d = USBPresent;
#endif /* USE_USB */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Startup called from Assembler
 *******************************************************************************
 ******************************************************************************/
int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0)
{
	(void)n32In_R0;
	(void)n32In_R1;
	(void)pn32Out_R0;

	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Startup called from Assembler
 *******************************************************************************
 ******************************************************************************/
uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
	(void)lr;
	(void)msp;
	(void)psp;

	while (1)
		;
	return 0;
}
