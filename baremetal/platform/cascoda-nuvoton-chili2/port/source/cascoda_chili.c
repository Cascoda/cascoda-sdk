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
#include "usbd.h"
/* Cascoda */
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */

/* define system configuaration mask */
#define CLKCFG_ENPLL 0x01
#define CLKCFG_ENHXT 0x02
#define CLKCFG_ENHIRC 0x04
#define CLKCFG_ENHIRC48 0x08

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

	/* EADC clock source is PCLK->HCLK, divide down to 1 MHz */
	if (SystemFrequency == FSYS_64MHZ)
		CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(64));
	else if (SystemFrequency == FSYS_48MHZ)
		CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(48));
	else if (SystemFrequency == FSYS_32MHZ)
		CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(32));
	else if (SystemFrequency == FSYS_24MHZ)
		CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(24));
	else if (SystemFrequency == FSYS_16MHZ)
		CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(16));
	else if (SystemFrequency == FSYS_12MHZ)
		CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(12));
	else /* FSYS_4MHZ */
		CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(4));

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
 * \brief Initialise Essential GPIOs for various Functions
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOInit(void)
{
	/* Note that pull-ups are configured for each used GPIO regardless of default */

	/* Initialise and set Debounce to 8 HCLK cycles */
	GPIO_SET_DEBOUNCE_TIME(PA, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PC, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PD, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PE, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PF, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PG, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PH, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);

#if (CASCODA_CHILI2_CONFIG == 1)
	/* VOLTS_TEST */
	/* output, no pull-up, set to 0 */
	GPIO_SetMode(VOLTS_TEST_PORT, BITMASK(VOLTS_TEST_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(VOLTS_TEST_PORT, BITMASK(VOLTS_TEST_PIN), GPIO_PUSEL_DISABLE);
	VOLTS_TEST_PVAL = 0; /* VOLTS_TEST is active high, so switch off to avoid unecessary power consumption */
#endif

	/* ZIG_RESET */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_PUSEL_DISABLE);
	ZIG_RESET_PVAL = 1; /* RSTB is HIGH */

	/* ZIG_IRQB */
	/* input, pull-up */
	GPIO_SetMode(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN), GPIO_MODE_INPUT);
	GPIO_SetPullCtl(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN), GPIO_PUSEL_PULL_UP);

	/* SPI: MFP but SS0 controlled separately as GPIO */
	CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_SPI);
	GPIO_SetPullCtl(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_SCLK_PORT, BITMASK(SPI_SCLK_PIN), GPIO_PUSEL_DISABLE);

	/* SPI_CS */
	/* output, no pull-up, set to 1 */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_PUSEL_DISABLE);
	SPI_CS_PVAL = 1;

#if (CASCODA_CHILI2_CONFIG == 1)
	/* CHARGE_STAT */
	/* input, pull-up, debounced */
	GPIO_SetMode(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN), GPIO_MODE_INPUT);
	GPIO_SetPullCtl(
	    CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN), GPIO_PUSEL_PULL_UP); /* CHARGE_STAT is low or tri-state */
	GPIO_ENABLE_DEBOUNCE(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN));
#endif

	/* PROGRAM Pin */
	/* currently nor implemented */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for PowerDown
 * \params tristateCax - bool: set to 1 to tri-state the CA-821x interface
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerDown(u8_t tristateCax)
{
	/* ZIG_RESET */
	/* tri-state - output to input */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_MODE_INPUT);

	/* ZIG_IRQB */
	/* disable pull-up */
	GPIO_SetPullCtl(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN), GPIO_PUSEL_DISABLE);

	if (tristateCax)
	{
		/* SPI: */
		CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_GPIO);
		CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_GPIO);
		CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_GPIO);
		GPIO_SetMode(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_SCLK_PORT, BITMASK(SPI_SCLK_PIN), GPIO_MODE_INPUT);
	}

	/* SPI_CS */
	/* tri-state - output to input */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_MODE_INPUT);

#if (CASCODA_CHILI2_CONFIG == 1)
	/* CHARGE_STAT */
	/* disable pull-up */
	GPIO_SetPullCtl(CHARGE_STAT_PORT, CHARGE_STAT_PIN, GPIO_PUSEL_DISABLE);
#endif

	/* dynamic GPIO handling */
	CHILI_ModulePowerDownGPIOs();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for active Mode after PowerDown
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerUp(void)
{
	/* ZIG_RESET */
	/* input to output */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_MODE_OUTPUT);

	/* ZIG_IRQB */
	/* enable pull-up */
	GPIO_SetPullCtl(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN), GPIO_PUSEL_PULL_UP);

	/* SPI: */
	CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_SPI);

	/* SPI_CS */
	/* input to output */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_MODE_OUTPUT);

	/* CHARGE_STAT */
	/* enable pull-up */
#if (CASCODA_CHILI2_CONFIG == 1)
	GPIO_SetPullCtl(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN), GPIO_PUSEL_PULL_UP);
#endif

	/* dynamic GPIO handling */
	CHILI_ModulePowerUpGPIOs();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable GPIO Interrupts
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOEnableInterrupts(void)
{
	/* RFIRQ */
	GPIO_EnableInt(ZIG_IRQB_PORT, ZIG_IRQB_PIN, GPIO_INT_FALLING);

	NVIC_EnableIRQ(ZIG_IRQB_IRQn);
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
ca_error CHILI_ClockInit(fsys_mhz fsys, u8_t enable_comms)
{
	uint32_t delayCnt;
	ca_error status = CA_ERROR_SUCCESS;
	u8_t     clkcfg;

	/* NOTE: __HXT in system_M2351.h has to be changed to correct input clock frequency !! */
#if __HXT != 4000000UL
#error "__HXT Not set correctly in system_M2351.h"
#endif

	/* Oscillator sources:
	 *	- HXT 		 4 MHz
	 *	- HIRC		12 MHz
	 *	- HIRC48	48 MHz */

	clkcfg = CHILI_GetClockConfigMask(fsys, enable_comms);

	/* enable HXT */
	if (clkcfg & CLKCFG_ENHXT)
	{
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
			status = CA_ERROR_FAIL;
			printf("HXT Enable Fail\n");
			return (status);
		}
	}

	/* enable HIRC */
	if (clkcfg & CLKCFG_ENHIRC)
	{
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
			status = CA_ERROR_FAIL;
			printf("HIRC Enable Fail\n");
			return (status);
		}
	}

	/* enable HIRC48 */
	if (clkcfg & CLKCFG_ENHIRC48)
	{
		SYS_UnlockReg();
		CLK->PWRCTL |= (CLK_PWRCTL_HIRC48EN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->STATUS & CLK_STATUS_HIRC48STB_Msk)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			status = CA_ERROR_FAIL;
			printf("HIRC48 Enable Fail\n");
			return (status);
		}
	}

	/* enable PLL */
	if (clkcfg & CLKCFG_ENPLL)
	{
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
		FOUT = 48 MHz:
		HIRC      12    4   96   48   3 12  2     2    10      1   0x0008440A
		HXT        4    4   96   48   1 12  2     0    10      1   0x0000400A
		FOUT = 96 MHz (for 32MHz HCLK)
		HIRC      12    4   96   48   3 12  1     2    10      0   0x0008040A
		HXT        4    4   96   48   1 12  1     0    10      0   0x0000000A
		FOUT = 64 MHz:(for 64MHz HCLK)
		HIRC      12    4  128   64   3 16  2     2    14      1   0x0008440E
		HXT        4    4  128   64   1 16  2     0    14      1   0x0000400E
		*****************************************************************************************
		Note: Using CLK_EnablePLL() does not always give correct results, so avoid!
		*/
		SYS_UnlockReg();
		if (clkcfg & CLKCFG_ENHXT)
		{
			if (fsys == FSYS_64MHZ)
				CLK->PLLCTL = 0x0000400E;
			else if (fsys == FSYS_32MHZ)
				CLK->PLLCTL = 0x0000000A;
			else
				CLK->PLLCTL = 0x0000400A;
		}
		else
		{
			if (fsys == FSYS_64MHZ)
				CLK->PLLCTL = 0x0008440E;
			else if (fsys == FSYS_32MHZ)
				CLK->PLLCTL = 0x0008040A;
			else
				CLK->PLLCTL = 0x0008440A;
		}
		SYS_LockReg();
		for (delayCnt = 0; delayCnt < MAX_CLOCK_SWITCH_DELAY; delayCnt++)
		{
			if (CLK->STATUS & CLK_STATUS_PLLSTB_Msk)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			status = CA_ERROR_FAIL;
			printf("PLL Enable Fail\n");
			return (status);
		}
	}

	/* set system clock */
	SYS_UnlockReg();
	if (clkcfg & CLKCFG_ENHXT)
	{
		if (fsys == FSYS_4MHZ)
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));
		else if (fsys == FSYS_12MHZ)
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(4));
		else if (fsys == FSYS_16MHZ)
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(3));
		else if (fsys == FSYS_24MHZ)
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(2));
		else if (fsys == FSYS_32MHZ)
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(3));
		else if (fsys == FSYS_48MHZ)
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));
		else /* FSYS_64MHZ */
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));
	}
	else
	{
		if ((clkcfg & CLKCFG_ENHIRC) && (fsys == FSYS_4MHZ))
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(3));
		else if ((clkcfg & CLKCFG_ENHIRC) && (fsys == FSYS_12MHZ))
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
		else if (clkcfg & CLKCFG_ENPLL)
		{
			if (fsys == FSYS_4MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(12));
			else if (fsys == FSYS_12MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(4));
			else if (fsys == FSYS_16MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(3));
			else if (fsys == FSYS_24MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(2));
			else if (fsys == FSYS_32MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(3));
			else if (fsys == FSYS_48MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));
			else if (fsys == FSYS_64MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));
		}
		else if (clkcfg & CLKCFG_ENHIRC48)
		{
			if (fsys == FSYS_4MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC48, CLK_CLKDIV0_HCLK(12));
			else if (fsys == FSYS_12MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC48, CLK_CLKDIV0_HCLK(4));
			else if (fsys == FSYS_16MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC48, CLK_CLKDIV0_HCLK(3));
			else if (fsys == FSYS_24MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC48, CLK_CLKDIV0_HCLK(2));
			else if (fsys == FSYS_48MHZ)
				CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC48, CLK_CLKDIV0_HCLK(1));
		}
	}

#ifdef USE_USB
	/* set USB clock */
	SYS_UnlockReg();
	if (enable_comms)
	{
		if (clkcfg & CLKCFG_ENHIRC48)
		{
			CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_HIRC48, CLK_CLKDIV0_USB(1));
		}
		else
		{
			if (fsys == FSYS_32MHZ)
				CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(2));
			else
				CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(1));
		}
		CLK_EnableModuleClock(USBD_MODULE);
	}
	else
	{
		CLK_DisableModuleClock(USBD_MODULE);
	}
	SYS_LockReg();
#endif

	/* Set HCLK back to HIRC if clock switching error happens */
	if (CLK->STATUS & CLK_STATUS_CLKSFAIL_Msk)
	{
		SYS_UnlockReg();
		CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;
		CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
		SYS_LockReg();
		printf("Clock Switch Fail, Restarting with 12MHz\n");
		status = CA_ERROR_FAIL;
	}

	return (status);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Clock (Re-)Initialisation.
 *******************************************************************************
 ******************************************************************************/
void CHILI_CompleteClockInit(fsys_mhz fsys, u8_t enable_comms)
{
	u8_t clkcfg;

	clkcfg = CHILI_GetClockConfigMask(fsys, enable_comms);

	if (!(CLK->STATUS & CLK_STATUS_CLKSFAIL_Msk))
	{
		SYS_UnlockReg();
		if (!(clkcfg & CLKCFG_ENPLL)) /* disable PLL */
			CLK_DisablePLL();
		if (!(clkcfg & CLKCFG_ENHIRC48)) /* disable HIRC48 */
			CLK->PWRCTL &= ~(CLK_PWRCTL_HIRC48EN_Msk);
		if (!(clkcfg & CLKCFG_ENHIRC)) /* disable HIRC */
			CLK->PWRCTL &= ~(CLK_PWRCTL_HIRCEN_Msk);
		if (!(clkcfg & CLKCFG_ENHXT)) /* disable HXT */
			CLK->PWRCTL &= ~(CLK_PWRCTL_HXTEN_Msk);
		SYS_LockReg();
	}
	else
	{
		printf("Clock Switch Fail\n");
	}
	CHILI_EnableIntOscCal();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_EnableIntOscCal(void)
{
	/* enable HIRC trimming */
	SYS->TISTS12M = 0xFFFF;
	if ((CLK->PWRCTL & CLK_PWRCTL_HIRCEN_Msk) && (CLK->STATUS & CLK_STATUS_LXTSTB_Msk))
	{
		/* trim HIRC using LXT */
		SYS->TCTL12M = 0x00F1;
		/* enable HIRC-trim interrupt */
		NVIC_EnableIRQ(CKFAIL_IRQn);
		SYS->TIEN12M = (SYS_TIEN12M_TFAILIEN_Msk | SYS_TIEN12M_CLKEIEN_Msk);
	}

	/* enable HIRC48 trimming */
	SYS->TISTS48M = 0xFFFF;
	if ((CLK->PWRCTL & CLK_PWRCTL_HIRC48EN_Msk) && (CLK->STATUS & CLK_STATUS_LXTSTB_Msk))
	{
		/* trim HIRC48 using LXT */
		SYS->TCTL48M = 0x00F1;
		/* enable HIRC48-trim interrupt */
		NVIC_EnableIRQ(CKFAIL_IRQn);
		SYS->TIEN48M = (SYS_TIEN48M_TFAILIEN_Msk | SYS_TIEN48M_CLKEIEN_Msk);
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

	SYS->TISTS48M = 0xFFFF;
	SYS->TIEN48M  = 0;
	SYS->TCTL48M  = 0;
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
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);

	/* configure clock selects and dividers for timers 0 and 1 */
	/* clocks are fixed to HXT (4 MHz) or HIRC (12 MHz) */
	if (UseExternalClock)
	{
		CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);
		CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
	}
	else
	{
		CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);
		CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	}

	TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1000000);
	TIMER_Open(TIMER1, TIMER_CONTINUOUS_MODE, 1000000);

	/* prescale value has to be set after TIMER_Open() is called! */
	if (UseExternalClock)
	{
		/*  4 MHZ Clock: prescaler 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 3);
		/*  4 MHZ Clock: prescalar 3 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 3);
	}
	else
	{
		/* 12 MHZ Clock: prescaler 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER0, 11);
		/* 12 MHZ Clock: prescalar 11 gives 1 uSec units */
		TIMER_SET_PRESCALE_VALUE(TIMER1, 11);
	}

	/* note timers are 24-bit (+ 8-bit prescale) */
	/* 1uSec units, so 1000 is 1ms */
	TIMER_SET_CMP_VALUE(TIMER0, 1000);
	/* 1uSec units, counts microseconds */
	TIMER_SET_CMP_VALUE(TIMER1, 0xFFFFFF);

	NVIC_EnableIRQ(TMR0_IRQn);
	TIMER_EnableInt(TIMER0);
	TIMER_Start(TIMER0);

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
	static u8_t comms_have_been_enabled = 0;
#endif /* USE_USB */

	/* switch off comms */
#if defined(USE_USB)
	if (!EnableCommsInterface)
		USB_Deinitialise();
#endif /* USE_USB */
#if defined(USE_UART)
	if (!EnableCommsInterface)
		CHILI_UARTDeinit();
#endif /* USE_UART */

	/* configure clocks */
	CHILI_ClockInit(SystemFrequency, EnableCommsInterface);

	/* switch on comms */
#if defined(USE_USB)
	/* do not initialise if already active as connection will be closed */
	if ((EnableCommsInterface) && (!comms_have_been_enabled))
		USB_Initialise();
#endif /* USE_USB */
#if defined(USE_UART)
	/* requires re-initialisation to set correct baudrate */
	if (EnableCommsInterface)
		CHILI_UARTInit();
#endif /* USE_UART */

	/* initialise timers */
	CHILI_TimersInit();

	/* finish clock initialisation */
	/* this has to be done AFTER all other system config changes ! */
	CHILI_CompleteClockInit(SystemFrequency, EnableCommsInterface);

	/* re-initialise SPI clock rate */
	SPI_SetBusClock(SPI, FCLK_SPI);

	/* set interrupt priorities */
	/* 0: highest  timers */
	/* 1:          downstream dispatch and gpios */
	/* 2:          upstream   dispatch */
	/* 3: lowest */
	NVIC_SetPriority(TMR0_IRQn, 0);
	NVIC_SetPriority(TMR1_IRQn, 0);
	NVIC_SetPriority(TMR2_IRQn, 0);
	NVIC_SetPriority(TMR3_IRQn, 0);
	NVIC_SetPriority(GPA_IRQn, 1);
	NVIC_SetPriority(GPB_IRQn, 1);
	NVIC_SetPriority(GPC_IRQn, 1);
	NVIC_SetPriority(GPD_IRQn, 1);
	NVIC_SetPriority(GPE_IRQn, 1);
	NVIC_SetPriority(GPF_IRQn, 1);
	NVIC_SetPriority(GPG_IRQn, 1);
	NVIC_SetPriority(GPH_IRQn, 1);
#if defined(USE_USB)
	NVIC_SetPriority(USBD_IRQn, 2);
#endif
#if defined(USE_UART)
	NVIC_SetPriority(UART_IRQn, 2);
#endif

#if defined(USE_USB)
	comms_have_been_enabled = EnableCommsInterface;
#endif
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Clock
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_GetClockConfigMask(fsys_mhz fsys, u8_t enable_comms)
{
	u8_t mask = 0;

	/* check if PLL required */
	if (UseExternalClock)
	{
		if (fsys > FSYS_4MHZ) /* PLL needed to generate fsys from HXT */
			mask |= CLKCFG_ENPLL;
#ifdef USE_USB
		if (enable_comms) /* PLL needed to USB Clock */
			mask |= CLKCFG_ENPLL;
#endif
	}
	else
	{
		if ((fsys == FSYS_32MHZ) || (fsys == FSYS_64MHZ)) /* PLL needed to generate fsys from HIRC */
			mask |= CLKCFG_ENPLL;
	}
	if (enable_comms)
	{
#ifdef USE_UART
		if (UART_BAUDRATE > 115200) /* PLL needed to generate UART clock */
			mask |= CLKCFG_ENPLL;
#endif
	}

	/* check if HXT required */
	if (UseExternalClock)
		mask |= CLKCFG_ENHXT; /* external clock always requires HXT */

	/* check if HIRC required */
	if (!UseExternalClock)
		mask |= CLKCFG_ENHIRC; /* internal clock always requires HIRC for timers etc. */

	/* check if HIRC48 required */
	if (fsys == FSYS_64MHZ) /* if 64MHz HIRC48 is needed for USB clock */
	{
#ifdef USE_USB
		if (enable_comms)
			mask |= CLKCFG_ENHIRC48;
#endif
	}
	if (!UseExternalClock) /* internal clock */
	{
#ifdef USE_USB
		if (enable_comms) /* HIRC48 always used for USB clock */
			mask |= CLKCFG_ENHIRC48;
#endif
		if (!(mask & CLKCFG_ENPLL))
		{
			if (fsys > FSYS_12MHZ) /* HIRC48 needed to generate fsys from HIRC48 */
				mask |= CLKCFG_ENHIRC48;
		}
	}

	return (mask);
}
