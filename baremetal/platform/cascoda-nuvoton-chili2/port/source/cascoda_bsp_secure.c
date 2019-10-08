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

/* System */
#include <stdio.h>
/* Platform */
#include "M2351.h"
#include "fmc.h"
#include "gpio.h"
#include "sys.h"
#include "timer.h"

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
volatile u8_t     WDTimeout            = 0;
volatile u8_t     UseExternalClock     = 0;
volatile fsys_mhz SystemFrequency      = DEFAULT_FSYS;
volatile u8_t     EnableCommsInterface = 1;

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY u64_t BSP_GetUniqueId(void)
{
	u64_t rval = 0;
	u32_t extra_byte;

	SYS_UnlockReg();
	FMC_Open();
	extra_byte = FMC_ReadUID(2);
	rval       = FMC_ReadUID(0) ^ extra_byte;
	rval       = rval << 32ULL;
	rval |= FMC_ReadUID(1) ^ extra_byte;
	FMC_Close();
	SYS_LockReg();

	return rval;
}

__NONSECURE_ENTRY void CHILI_EnableSpiModuleClock(void)
{
	/* Unlock protected registers */
	SYS_UnlockReg();
	CLK_EnableModuleClock(SPI_MODULE);
	/* Lock protected registers */
	SYS_LockReg();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY wakeup_reason BSP_GetWakeupReason(void)
{
	if (CLK_GetPMUWKSrc() & CLK_PMUSTS_PINWK_Msk) /* DPD */
		return WAKEUP_DEEP_POWERDOWN;
	if (CLK_GetPMUWKSrc() & CLK_PMUSTS_GPCWK_Msk) /* SPD */
		return WAKEUP_DEEP_POWERDOWN;
	if (CLK_GetPMUWKSrc() & CLK_PMUSTS_RTCWK_Msk)
		return WAKEUP_RTCALARM;
	if (SYS_GetResetSrc() & SYS_RSTSTS_WDTRF_Msk)
	{
		SYS_ClearResetSrc(SYS_RSTSTS_WDTRF_Msk);
		return WAKEUP_WATCHDOG;
	}
	if (SYS_GetResetSrc() & SYS_RSTSTS_SYSRF_Msk)
	{
		SYS_ClearResetSrc(SYS_RSTSTS_SYSRF_Msk);
		return WAKEUP_SYSRESET;
	}
	return WAKEUP_POWERON;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_WatchdogEnable(u32_t timeout_ms)
{
	u32_t tout;

	SYS_UnlockReg();
	/* enable WDT clock, LIRC (10kHz) */
	CLK_EnableModuleClock(WDT_MODULE);
	CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_LIRC, 0);

	/* select timeout */
	if (timeout_ms <= 2)
		tout = WDT_TIMEOUT_2POW4; /*  0.0016 s */
	else if (timeout_ms <= 7)
		tout = WDT_TIMEOUT_2POW6; /*  0.0064 s */
	else if (timeout_ms <= 26)
		tout = WDT_TIMEOUT_2POW8; /*  0.0256 s */
	else if (timeout_ms <= 103)
		tout = WDT_TIMEOUT_2POW10; /*  0.1024 s */
	else if (timeout_ms <= 410)
		tout = WDT_TIMEOUT_2POW12; /*  0.4096 s */
	else if (timeout_ms <= 1639)
		tout = WDT_TIMEOUT_2POW14; /*  1.6384 s */
	else if (timeout_ms <= 6554)
		tout = WDT_TIMEOUT_2POW16; /*  6.5536 s */
	else
		tout = WDT_TIMEOUT_2POW18; /* 26.2144 s */

	WDT->ALTCTL = WDT_RESET_DELAY_3CLK;
	WDT->CTL    = tout | WDT_CTL_WDTEN_Msk | WDT_CTL_RSTEN_Msk;
	SYS_LockReg();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_WatchdogDisable(void)
{
	SYS_UnlockReg();
	WDT_Close();
	CLK_DisableModuleClock(WDT_MODULE);
	SYS_LockReg();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_WatchdogReset(void)
{
	WDT_RESET_COUNTER();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY u8_t BSP_IsWatchdogTriggered(void)
{
	u8_t retval = WDTimeout;
	if (retval)
		WDTimeout = 0;
	return retval;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_UseExternalClock(u8_t useExternalClock)
{
	if (UseExternalClock == useExternalClock)
		return;
	UseExternalClock = useExternalClock;

#ifdef USE_UART
	CHILI_UARTWaitWhileBusy();
#endif

	if (useExternalClock)
	{
		/* swap then disable */
		CHILI_SystemReInit();
		CHILI_EnableIntOscCal();
	}
	else
	{
		/* disable then swap */
		CHILI_DisableIntOscCal();
		CHILI_SystemReInit();
	}
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_SystemConfig(fsys_mhz fsys, u8_t enable_comms)
{
	if (enable_comms)
	{
		/* check frequency settings */
#if defined(USE_USB)
		if (fsys == FSYS_4MHZ)
		{
			ca_log_crit("SystemConfig Error: Minimum clock frequency for USB is 12MHz");
			return;
		}
#endif
#if defined(USE_UART)
		if ((fsys == FSYS_4MHZ) && (UART_BAUDRATE > 115200))
		{
			ca_log_crit("SystemConfig Error: Minimum clock frequency for UART @%u is 12MHz", UART_BAUDRATE);
			return;
		}
		if ((fsys == FSYS_64MHZ) && (UART_BAUDRATE == 6000000))
		{
			ca_log_crit("SystemConfig Error: UART @%u cannot be used at 64 MHz", UART_BAUDRATE);
			return;
		}
#endif
	}

#ifdef USE_UART
	CHILI_UARTWaitWhileBusy();
#endif

	SystemFrequency      = fsys;
	EnableCommsInterface = enable_comms;

	/* system  initialisation (clocks, timers ..) */
	CHILI_SystemReInit();
	cascoda_isr_secure_init();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_Waiting(void)
{
}
