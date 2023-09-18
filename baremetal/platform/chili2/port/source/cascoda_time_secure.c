/*
 * cascoda_time.c
 *
 *  Created on: 22 Nov 2018
 *      Author: ciaran
 */
#include <stdint.h>
#include "cascoda-bm/cascoda_interface.h"
#include "M2351.h"
#include "timer.h"

/* Global Timer Absolute Ticks in ms. */
/* Set up to overflow within 5 minutes */
volatile u32_t AbsoluteTicks = UINT32_MAX - 5 * 60 * 1000;

/* microsecond timer most significant bit */
volatile u8_t MicroSecondMSB = 0;

/* Note: CHILI_1msTick and CHILI_MicroTimerTick are directly called from the */
/* TMR0_IRQHandler and TMR1_IRQHandler isr.                                  */
/* Although there is no specific locking, only the TIMER0 and TIMER1 IRQs    */
/* have the highest priority (0), so they are intrinsically ISR safe         */

/*---------------------------------------------------------------------------*
 * See cascoda_chili.h for docs                                              *
 *---------------------------------------------------------------------------*/
void CHILI_1msTick(void)
{
	AbsoluteTicks++;
}

/*---------------------------------------------------------------------------*
 * See cascoda_chili.h for docs                                              *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void CHILI_FastForward(u32_t ticks)
{
	AbsoluteTicks += ticks;
}

/*---------------------------------------------------------------------------*
 * See cascoda_chili.h for docs                                              *
 *---------------------------------------------------------------------------*/
void CHILI_MicroTimerTick(void)
{
	/* isr called every 16.777 seconds (2^24 us) */
	MicroSecondMSB++;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface_core.h for docs                          *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY u32_t BSP_ReadAbsoluteTime(void)
{
	return AbsoluteTicks;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface_core.h for docs                          *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_WaitTicks(u32_t ticks)
{
	u32_t Start;

	Start = AbsoluteTicks;
	while ((AbsoluteTicks - Start) < ticks) BSP_Waiting();
}

/*---------------------------------------------------------------------------*/
/* MicroSeconds Timer functions:                                             */
/* Timer has to be specifically enabled (by calling BSP_MicroTimerStart()) . */
/* 32-bit extension gives a run time of 71.582 minutes until wrap-around.    */
/* Timer does not operate in power-down modes as no time base available.     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface_core.h for docs                          *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_MicroTimerStart(void)
{
	TIMER_Stop(TIMER1);
	TIMER_ResetCounter(TIMER1);
	MicroSecondMSB = 0;
	NVIC_EnableIRQ(TMR1_IRQn);
	TIMER_EnableInt(TIMER1);
	TIMER_Start(TIMER1);
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface_core.h for docs                          *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY u32_t BSP_MicroTimerStop(void)
{
	TIMER_Stop(TIMER1);
	TIMER_DisableInt(TIMER1);
	NVIC_DisableIRQ(TMR1_IRQn);
	return ((MicroSecondMSB << 24) | (TIMER_GetCounter(TIMER1) & 0x00FFFFFF));
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface_core.h for docs                          *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY u32_t BSP_MicroTimerGet(void)
{
	return ((MicroSecondMSB << 24) | (TIMER_GetCounter(TIMER1) & 0x00FFFFFF));
}
