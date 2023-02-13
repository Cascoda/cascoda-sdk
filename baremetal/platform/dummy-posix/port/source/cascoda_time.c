/*
 * cascoda_time.c
 *
 *  Created on: 22 Nov 2018
 *      Author: ciaran
 */
#include "cascoda-bm/cascoda_interface.h"

volatile u32_t AbsoluteTicks = 0; // Global Timer Absolute Ticks in ms

//TODO: The 1ms tick function isn't remotely ISR safe, so should be fixed.
//The 1msTick function needs some kind of locking.

/******************************************************************************/
/***************************************************************************/
u32_t BSP_ReadAbsoluteTime(void)
{
	return AbsoluteTicks;
} // End of BSP_ReadAbsoluteTime()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief 1 ms Tick for TMR0_IRQHandler ISR
 *******************************************************************************
 ******************************************************************************/
void CHILI_1msTick(void)
{
	AbsoluteTicks++;
} // End of CHILI_1msTick()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief FastForward time by the given amount
 *******************************************************************************
 * \param ticks - Time in Ticks (1ms/100ms)
 *******************************************************************************
 ******************************************************************************/
void CHILI_FastForward(u32_t ticks)
{
	AbsoluteTicks += ticks;
}

/******************************************************************************/
/***************************************************************************/
void BSP_WaitTicks(u32_t ticks)
{
	u32_t Start;

	Start = AbsoluteTicks;
	while ((AbsoluteTicks - Start) < ticks) BSP_Waiting();
} // End of BSP_WaitTicks()
