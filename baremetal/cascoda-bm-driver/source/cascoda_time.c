/*
 * cascoda_time.c
 *
 *  Created on: 22 Nov 2018
 *      Author: ciaran
 */
#include "cascoda-bm/cascoda_time.h"

u16_t          Ticks         = 0; // Global Timer ms
u8_t           Seconds       = 0; // Global Timer Seconds
u8_t           Minutes       = 0; // Global Timer Minutes
u16_t          Hours         = 0; // Global Timer Hours
volatile u32_t AbsoluteTicks = 0; // Global Timer Absolute Ticks in ms

//TODO: The Fastforward function doesn't touch the Seconds, Minutes etc.
//TODO: The 1ms tick function isn't remotely ISR safe, so should be fixed.
//Recommendation is to change to a 64-bit AbsoluteTicks, and calculate the others
//when requested based on that value. Also the 1msTick function needs some kind of
//locking.

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Record the start time for an event
 *******************************************************************************
 * \return Absolute time in milliseconds
 *******************************************************************************
 ******************************************************************************/
u32_t TIME_ReadAbsoluteTime(void)
{
	return AbsoluteTicks;
} // End of TIME_ReadAbsoluteTime()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief 1 ms Tick for TMR0_IRQHandler ISR
 *******************************************************************************
 ******************************************************************************/
void TIME_1msTick(void)
{
	AbsoluteTicks++;
	if (++Ticks > 999)
	{
		Ticks = 0;
		if (++Seconds > 59)
		{
			Seconds = 0;
			if (++Minutes > 59)
			{
				Minutes = 0;
				Hours++;
			}
		}
	}

} // End of TIME_1msTick()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief FastForward time by the given amount
 *******************************************************************************
 * \param ticks - Time in Ticks (1ms/100ms)
 *******************************************************************************
 ******************************************************************************/
void TIME_FastForward(u32_t ticks)
{
	AbsoluteTicks += ticks;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wait for specified Ticks Time
 *******************************************************************************
 * \param Ticks - Time in Ticks (1ms/100ms)
 *******************************************************************************
 ******************************************************************************/
void TIME_WaitTicks(u32_t ticks)
{
	u32_t Start;

	Start = AbsoluteTicks;
	while ((AbsoluteTicks - Start) < ticks)
		;
} // End of TIME_WaitTicks()
