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
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"

/* local functions */
static void  CHILI_RTCConvertSecondsToDateAndTime(i64_t seconds, S_RTC_TIME_DATA_T *dt);
static i64_t CHILI_RTCConvertDateAndTimeToSeconds(S_RTC_TIME_DATA_T *dt);
static u8_t  CHILI_RTCIsDateAndTimeValid(S_RTC_TIME_DATA_T *dt);
static void  CHILI_RTCAddSecondsToDateAndTime(u32_t seconds, S_RTC_TIME_DATA_T *dt);
static u8_t  CHILI_RTCIsRunning(void);
static void  CHILI_RTCPrintDateAndTime(S_RTC_TIME_DATA_T dt);

/* pointer to ISR callback function */
static int (*ModuleRTCCallback)(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Converts Seconds to RTC Date+Time
 *******************************************************************************
 * \param seconds - Seconds (UTC) to be converted
 * \param dt      - Pointer to RTC Date+Time data structure
 *******************************************************************************
 ******************************************************************************/
static void CHILI_RTCConvertSecondsToDateAndTime(i64_t seconds, S_RTC_TIME_DATA_T *dt)
{
	u64_t a, b, c, d, e, f;
	i64_t sval;

	sval = seconds;

	/* seconds to hours, minutes and seconds */
	dt->u32Second = (u32_t)(sval % 60);
	sval /= 60;
	dt->u32Minute = (u32_t)(sval % 60);
	sval /= 60;
	dt->u32Hour = (u32_t)(sval % 24);
	sval /= 24;

	/* seconds to date */
	a = (u64_t)((4 * sval + 102032) / 146097 + 15);
	b = (u64_t)(sval + 2442113 + a - (a / 4));
	c = (20 * b - 2442) / 7305;
	d = b - 365 * c - (c / 4);
	e = d * 1000 / 30601;
	f = d - e * 30 - e * 601 / 1000;

	/* january and february are counted as months 13 and 14 of the previous year */
	if (e <= 13)
	{
		c -= 4716;
		e -= 1;
	}
	else
	{
		c -= 4715;
		e -= 13;
	}

	/* retrieve year, month and day */
	dt->u32Day   = (u32_t)f;
	dt->u32Month = (u32_t)e;
	dt->u32Year  = (u32_t)c;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Converts RTC Date+Time to Seconds (UTC)
 *******************************************************************************
 * \param dt     - Pointer to RTC Date+Time data structure
 *******************************************************************************
 * \return Seconds (UTC)
 *******************************************************************************
 ******************************************************************************/
static i64_t CHILI_RTCConvertDateAndTimeToSeconds(S_RTC_TIME_DATA_T *dt)
{
	u32_t y, m, d;
	i64_t sec;

	/* year. month, day */
	y = dt->u32Year;
	m = dt->u32Month;
	d = dt->u32Day;

	/* january and february are counted as months 13 and 14 of the previous year */
	if (m <= 2)
	{
		m += 12;
		y -= 1;
	}

	/* years to days */
	sec = (365 * y) + (y / 4) - (y / 100) + (y / 400);
	/* months to days */
	sec += (30 * m) + (3 * (m + 1) / 5) + d;
	/* UTC time starts on January 1st, 1970 */
	sec -= 719561;
	/* days to seconds */
	sec *= 86400;
	/* add hours, minutes and seconds */
	sec += (3600 * dt->u32Hour) + (60 * dt->u32Minute) + dt->u32Second;

	return sec;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks if RTC Time+Date is valid
 *******************************************************************************
 * \param dt     - Pointer to RTC Date+Time data structure
 *******************************************************************************
 * \return 0: Not Valid (Error in Format), 1: Valid Format
 *******************************************************************************
 ******************************************************************************/
static u8_t CHILI_RTCIsDateAndTimeValid(S_RTC_TIME_DATA_T *dt)
{
	/* seconds */
	if (dt->u32Second > 59)
		return 0;
	/* minutes */
	if (dt->u32Minute > 59)
		return 0;
	/* hours */
	if (dt->u32Hour > 23)
		return 0;
	/* days	*/
	if (dt->u32Day < 1)
		return 0;
	if (((dt->u32Month == 1) || (dt->u32Month == 3) || (dt->u32Month == 5) || (dt->u32Month == 7) ||
	     (dt->u32Month == 8) || (dt->u32Month == 10) || (dt->u32Month == 12)) &&
	    (dt->u32Day > 31))
		return 0;
	if (((dt->u32Month == 4) || (dt->u32Month == 6) || (dt->u32Month == 9) || (dt->u32Month == 11)) &&
	    (dt->u32Day > 30))
		return 0;
	if ((dt->u32Month == 2) && ((dt->u32Year % 4) == 0) && (dt->u32Day > 29))
		return 0;
	if ((dt->u32Month == 2) && ((dt->u32Year % 4) != 0) && (dt->u32Day > 28))
		return 0;
	/* month */
	if ((dt->u32Month < 1) || (dt->u32Month > 12))
		return 0;
	/* year (UTC), 64 bit */
	if (dt->u32Year < 1970)
		return 0;
	return 1;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Adds Seconds to RTC Date+Time
 *******************************************************************************
 * \param seconds - Seconds (UTC) to be added
 * \param dt      - Pointer to RTC Date+Time data structure
 *******************************************************************************
 * \return 0: Not Valid (Error in Format), 1: Valid Format
 *******************************************************************************
 ******************************************************************************/
static void CHILI_RTCAddSecondsToDateAndTime(u32_t seconds, S_RTC_TIME_DATA_T *dt)
{
	i64_t tcalc;

	tcalc = CHILI_RTCConvertDateAndTimeToSeconds(dt);
	tcalc += seconds;
	CHILI_RTCConvertSecondsToDateAndTime(tcalc, dt);
}

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
 * \brief Print Date+Time
 *******************************************************************************
 * \param dt     - RTC Date+Time data structure
 *******************************************************************************
 ******************************************************************************/
static void CHILI_RTCPrintDateAndTime(S_RTC_TIME_DATA_T dt)
{
	printf("%d/%02d/%02d %02d:%02d:%02d", dt.u32Year, dt.u32Month, dt.u32Day, dt.u32Hour, dt.u32Minute, dt.u32Second);
	if (!CHILI_RTCIsDateAndTimeValid(&dt))
		printf(" (ERR)");
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
			CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set RTC Alarm in seconds from current time
 *******************************************************************************
 * \param seconds - seconds to be added to current time
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY ca_error BSP_RTCSetAlarmSeconds(u32_t seconds)
{
	S_RTC_TIME_DATA_T dt;

	if (!CHILI_RTCIsRunning())
	{
		printf("RTC not running\n");
		return CA_ERROR_FAIL;
	}

	RTC_GetDateAndTime(&dt);
	CHILI_RTCAddSecondsToDateAndTime(seconds, &dt);
	if (!CHILI_RTCIsDateAndTimeValid(&dt))
	{
		printf("Invalid Date&Time: ");
		return CA_ERROR_INVALID;
	}

	/* set alarm */
	RTC_SetAlarmDateAndTime(&dt);
	/* enable interrupt */
	NVIC_EnableIRQ(RTC_IRQn);
	RTC_EnableInt(RTC_INTEN_ALMIEN_Msk);

	return CA_ERROR_SUCCESS;
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
 * \brief Set RTC Date+Time
 *******************************************************************************
 * \param dateandtime - RTCDateAndTime structure
  *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
ca_error BSP_RTCSetDateAndTime(struct RTCDateAndTime dateandtime)
{
	S_RTC_TIME_DATA_T dt;

	dt.u32TimeScale = RTC_CLOCK_24;
	dt.u32Year      = dateandtime.year;
	dt.u32Month     = dateandtime.month;
	dt.u32Day       = dateandtime.day;
	dt.u32Hour      = dateandtime.hour;
	dt.u32Minute    = dateandtime.min;
	dt.u32Second    = dateandtime.sec;
	if (!CHILI_RTCIsDateAndTimeValid(&dt))
	{
		printf("Invalid Date&Time: ");
		CHILI_RTCPrintDateAndTime(dt);
		printf("\n");
		return CA_ERROR_INVALID;
	}
	RTC_SetDateAndTime(&dt);
	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Get RTC Date+Time
 *******************************************************************************
 * \param dateandtime - pointer to RTCDateAndTime structure
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY void BSP_RTCGetDateAndTime(struct RTCDateAndTime *dateandtime)
{
	S_RTC_TIME_DATA_T dt;

	RTC_GetDateAndTime(&dt);
	dateandtime->year  = dt.u32Year;
	dateandtime->month = dt.u32Month;
	dateandtime->day   = dt.u32Day;
	dateandtime->hour  = dt.u32Hour;
	dateandtime->min   = dt.u32Minute;
	dateandtime->sec   = dt.u32Second;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Converts Unix Time seconds to RTC Date+Time
 *******************************************************************************
 * \param seconds - Unix time
 * \param dateandtime - pointer to RTCDateAndTime structure
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY void BSP_RTCConvertSecondsToDateAndTime(i64_t seconds, struct RTCDateAndTime *dateandtime)
{
	S_RTC_TIME_DATA_T dt;

	CHILI_RTCConvertSecondsToDateAndTime(seconds, &dt);
	dateandtime->year  = dt.u32Year;
	dateandtime->month = dt.u32Month;
	dateandtime->day   = dt.u32Day;
	dateandtime->hour  = dt.u32Hour;
	dateandtime->min   = dt.u32Minute;
	dateandtime->sec   = dt.u32Second;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Converts RTC Date+Time to Unix Time seconds
 *******************************************************************************
 * \param dateandtime - RTCDateAndTime structure
 *******************************************************************************
 * \return Unix time seconds
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY i64_t BSP_RTCConvertDateAndTimeToSeconds(const struct RTCDateAndTime *dateandtime)
{
	S_RTC_TIME_DATA_T dt;

	dt.u32TimeScale = RTC_CLOCK_24;
	dt.u32Year      = dateandtime->year;
	dt.u32Month     = dateandtime->month;
	dt.u32Day       = dateandtime->day;
	dt.u32Hour      = dateandtime->hour;
	dt.u32Minute    = dateandtime->min;
	dt.u32Second    = dateandtime->sec;

	return (CHILI_RTCConvertDateAndTimeToSeconds(&dt));
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Register RTC IRQ function callback
 *******************************************************************************
 * \param callback - pointer to ISR for input
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY void BSP_RTCRegisterCallback(int (*callback)(void))
{
	ModuleRTCCallback = callback;
}
