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
 * Internal (Non-Interface) RTC functions for allways-usable watchdog
*/
/* System */
#include <stdio.h>
/* Platform */
#include "M2351.h"
/* Cascoda */
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili.h"

/* pointer to ISR callback function */
static int (*ModuleRTCCallback)(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks if RTC is running
 *******************************************************************************
 * \return 0: RTC disabled, 1: RTC running
 *******************************************************************************
 ******************************************************************************/
static u8_t CHILI_RTCIsRunning(void)
{
	if (RTC->INIT & RTC_INIT_ACTIVE_Msk)
		return 1;
	return 0;
}

void CHILI_RTCIRQHandler(void)
{
	/* call callback if registered */
	if (ModuleRTCCallback)
		ModuleRTCCallback();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disables RTC Alarm
 *******************************************************************************
 ******************************************************************************/
void BSP_RTCDisableAlarm(void)
{
	RTC_DisableInt(RTC_INTEN_ALMIEN_Msk);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise RTC
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY void BSP_RTCInitialise(void)
{
	if (!CHILI_RTCIsRunning())
	{
		/* clock selection, either LXT or LIRC32 */
		/* note that HIRC doesn't work in DPD, therefore is not selectable */
		if (USE_RTC_LXT)
		{
			/* LXT */
			SYS->GPF_MFPL = (SYS->GPF_MFPL & (~SYS_GPF_MFPL_PF4MFP_Msk)) | X32_OUT_PF4;
			SYS->GPF_MFPL = (SYS->GPF_MFPL & (~SYS_GPF_MFPL_PF5MFP_Msk)) | X32_IN_PF5;
			SYS_UnlockReg();
			CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
			SYS_LockReg();

			u32_t clock_stable = CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

			// Reset chip if clock is still unstable
			if (clock_stable == 0)
			{
				BSP_ChipReset();
			}

			CLK_SetModuleClock(RTC_MODULE, CLK_CLKSEL3_RTCSEL_LXT, 0);
			CLK_EnableModuleClock(RTC_MODULE);
			RTC_WaitAccessEnable();
			RTC->LXTCTL &= ~(RTC_LXTCTL_C32KS_Msk + RTC_LXTCTL_LIRC32KEN_Msk);
		}
		else
		{
			/* LIRC32 */
			CLK_SetModuleClock(RTC_MODULE, CLK_CLKSEL3_RTCSEL_LXT, 0);
			CLK_EnableModuleClock(RTC_MODULE);
			RTC_WaitAccessEnable();
			RTC->LXTCTL |= (RTC_LXTCTL_C32KS_Msk + RTC_LXTCTL_LIRC32KEN_Msk);
		}
		/* 24 hour mode is default */
		/* note default date+time is 2015/08/08 00:00:00 */
		RTC_Open(NULL);
	}
}

__NONSECURE_ENTRY void BSP_RTCRegisterCallback(int (*callback)(void))
{
	ModuleRTCCallback = callback;
}
