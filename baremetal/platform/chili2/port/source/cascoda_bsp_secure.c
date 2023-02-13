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
#include <arm_cmse.h>
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
#include "cascoda_secure.h"

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
volatile u8_t     WDTimeout            = 0;
volatile u8_t     UseExternalClock     = 0;
volatile fsys_mhz SystemFrequency      = DEFAULT_FSYS;
volatile u8_t     EnableCommsInterface = 1;

__NONSECURE_ENTRY void CHILI_SetUseExternalClock(u8_t use_ext_clock)
{
	UseExternalClock = use_ext_clock;
}

__NONSECURE_ENTRY u8_t CHILI_GetUseExternalClock(void)
{
	return UseExternalClock;
}

__NONSECURE_ENTRY void CHILI_SetEnableCommsInterface(u8_t enable_coms_interface)
{
	EnableCommsInterface = enable_coms_interface;
}

__NONSECURE_ENTRY u8_t CHILI_GetEnableCommsInterface(void)
{
	return EnableCommsInterface;
}

__NONSECURE_ENTRY void CHILI_SetSystemFrequency(fsys_mhz system_frequency)
{
	SystemFrequency = system_frequency;
}

__NONSECURE_ENTRY fsys_mhz CHILI_GetSystemFrequency(void)
{
	return SystemFrequency;
}

__NONSECURE_ENTRY void CHILI_GetUID(uint32_t *uid_out)
{
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3L)
	// Check that the output memory is actually nonsecure.
	uid_out = cmse_check_address_range(uid_out, 3 * sizeof(uint32_t), CMSE_NONSECURE);
#endif
	if (uid_out == NULL)
		return;

	SYS_UnlockReg();
	FMC_Open();
	uid_out[0] = FMC_ReadUID(0);
	uid_out[1] = FMC_ReadUID(1);
	uid_out[2] = FMC_ReadUID(2);
	FMC_Close();
	SYS_LockReg();
}

__NONSECURE_ENTRY void CHILI_EnableSpiModuleClock(void)
{
	SYS_UnlockReg();
	CLK_EnableModuleClock(SPI_MODULE);
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
__NONSECURE_ENTRY void BSP_Waiting(void)
{
}
