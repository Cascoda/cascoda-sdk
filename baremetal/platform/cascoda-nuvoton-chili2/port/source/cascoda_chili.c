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
#include "cascoda_secure.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */

/* define system configuaration mask */
#define CLKCFG_ENPLL 0x01
#define CLKCFG_ENHXT 0x02
#define CLKCFG_ENHIRC 0x04
#define CLKCFG_ENHIRC48 0x08

u32_t CHILI_ADCConversion(u32_t channel, u32_t reference)
{
	u32_t adcval;

	CHILI_InitADC(reference);

	if (reference != SYS_VREFCTL_VREF_AVDD)
	{
		/* wait 50 ms for references to settle */
		BSP_WaitTicks(50);
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
	CHILI_DeinitADC();

	return (adcval);
}

void CHILI_GPIOInit(void)
{
	/* required after SPD wakeup, otherwise GPIO's frozen */
	CHILI_GPIOInitClock();

	/* basic initialisation: re-configure all GPIOs to input with pull-up */
	PA->MODE  = 0x00000000;
	PB->MODE  = 0x00000000;
	PC->MODE  = 0x00000000;
	PD->MODE  = 0x00000000;
	PE->MODE  = 0x00000000;
	PF->MODE  = 0x00000000;
	PG->MODE  = 0x00000000;
	PH->MODE  = 0x00000000;
	PA->PUSEL = 0x55555555;
	PB->PUSEL = 0x55555555;
	PC->PUSEL = 0x55555555;
	PD->PUSEL = 0x55555555;
	PE->PUSEL = 0x55555555;
	PF->PUSEL = 0x55555555;
	PG->PUSEL = 0x55555555;
	PH->PUSEL = 0x55555555;

	/* pull-ups are re-configured for each used GPIO regardless of default */

	/* Initialise and set Debounce to 8 HCLK cycles */
	GPIO_SET_DEBOUNCE_TIME(PA, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PC, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PD, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PE, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PF, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PG, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PH, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);

#if defined(USE_USB) || (CASCODA_CHILI2_CONFIG == 1)
	/* disable pull-up on VBUS */
	GPIO_SetPullCtl(VBUS_PORT, BITMASK(VBUS_PIN), GPIO_PUSEL_DISABLE);
#endif

#if (CASCODA_CHILI2_CONFIG == 1)
	/* disable pull-up on VBUS_CONNECTED */
	GPIO_SetPullCtl(VBUS_CONNECTED_PORT, BITMASK(VBUS_CONNECTED_PIN), GPIO_PUSEL_DISABLE);
#endif

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

	/* X32I / X32O MFP */
	CHILI_ModuleSetMFP(X32I_PNUM, X32I_PIN, PMFP_X32);
	CHILI_ModuleSetMFP(X32O_PNUM, X32O_PIN, PMFP_X32);
	GPIO_SetPullCtl(X32I_PORT, BITMASK(X32I_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(X32O_PORT, BITMASK(X32O_PIN), GPIO_PUSEL_DISABLE);
}

void CHILI_GPIOPowerDown(u8_t tristateCax)
{
	/* ZIG_RESET */
	/* tri-state - output to input */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_MODE_INPUT);
	GPIO_SetPullCtl(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_PUSEL_PULL_UP);

	if (tristateCax)
	{
		/* SPI: */
		CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_GPIO);
		CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_GPIO);
		CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_GPIO);
		GPIO_SetMode(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_SCLK_PORT, BITMASK(SPI_SCLK_PIN), GPIO_MODE_INPUT);
		GPIO_SetPullCtl(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_PUSEL_PULL_UP);
		GPIO_SetPullCtl(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_PUSEL_PULL_UP);
		GPIO_SetPullCtl(SPI_SCLK_PORT, BITMASK(SPI_SCLK_PIN), GPIO_PUSEL_PULL_UP);
	}

	/* SPI_CS */
	/* tri-state - output to input */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_MODE_INPUT);
	GPIO_SetPullCtl(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_PUSEL_PULL_UP);

#if (CASCODA_CHILI2_CONFIG == 1)
	/* CHARGE_STAT */
	/* disable pull-up */
	GPIO_SetPullCtl(CHARGE_STAT_PORT, CHARGE_STAT_PIN, GPIO_PUSEL_DISABLE);
#endif

	/* dynamic GPIO handling */
	CHILI_ModulePowerDownGPIOs();
}

void CHILI_GPIOPowerUp(void)
{
	/* ZIG_RESET */
	/* input to output */
	GPIO_SetMode(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(ZIG_RESET_PORT, BITMASK(ZIG_RESET_PIN), GPIO_PUSEL_DISABLE);

	/* SPI: */
	CHILI_ModuleSetMFP(SPI_MOSI_PNUM, SPI_MOSI_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_MISO_PNUM, SPI_MISO_PIN, PMFP_SPI);
	CHILI_ModuleSetMFP(SPI_SCLK_PNUM, SPI_SCLK_PIN, PMFP_SPI);
	GPIO_SetPullCtl(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_SCLK_PORT, BITMASK(SPI_SCLK_PIN), GPIO_PUSEL_DISABLE);

	/* SPI_CS */
	/* input to output */
	GPIO_SetMode(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(SPI_CS_PORT, BITMASK(SPI_CS_PIN), GPIO_PUSEL_DISABLE);

	/* CHARGE_STAT */
	/* enable pull-up */
#if (CASCODA_CHILI2_CONFIG == 1)
	GPIO_SetPullCtl(CHARGE_STAT_PORT, BITMASK(CHARGE_STAT_PIN), GPIO_PUSEL_PULL_UP);
#endif

	/* dynamic GPIO handling */
	CHILI_ModulePowerUpGPIOs();
}

void CHILI_GPIOEnableInterrupts(void)
{
	/* RFIRQ */
	GPIO_EnableInt(ZIG_IRQB_PORT, ZIG_IRQB_PIN, GPIO_INT_FALLING);
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
	TZ_NVIC_EnableIRQ_NS(ZIG_IRQB_IRQn);
#endif
	NVIC_EnableIRQ(ZIG_IRQB_IRQn);
}
