/*
 * cascoda_time.c
 *
 *  Created on: 22 Nov 2018
 *      Author: ciaran
 */
#include <stdint.h>
#include "cascoda-bm/cascoda_interface.h"
#include "system_M2351.h"

// Global Timer Absolute Ticks in ms.
// Set up to overflow within 5 minutes
volatile u32_t AbsoluteTicks = UINT32_MAX - 5 * 60 * 1000;

//TODO: The 1ms tick function isn't remotely ISR safe, so should be fixed.
//The 1msTick function needs some kind of locking.

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY u32_t BSP_ReadAbsoluteTime(void)
{
	return AbsoluteTicks;
} // End of BSP_ReadAbsoluteTime()

void CHILI_1msTick(void)
{
	AbsoluteTicks++;
} // End of CHILI_1msTick()

__NONSECURE_ENTRY void CHILI_FastForward(u32_t ticks)
{
	AbsoluteTicks += ticks;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
__NONSECURE_ENTRY void BSP_WaitTicks(u32_t ticks)
{
	u32_t Start;

	Start = AbsoluteTicks;
	while ((AbsoluteTicks - Start) < ticks) BSP_Waiting();
} // End of BSP_WaitTicks()
