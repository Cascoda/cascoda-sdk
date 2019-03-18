/*
 * cascoda_time.h
 *
 *  Created on: 22 Nov 2018 (Moved out of BSP)
 *      Author: ciaran
 *
 *  A simple time interface that records absolute time to millisecond resolution
 *  and allows waiting for a set number of milliseconds.
 */

#ifndef INCLUDE_CASCODA_BM_CASCODA_TIME_H_
#define INCLUDE_CASCODA_BM_CASCODA_TIME_H_

#include "cascoda-bm/cascoda_types.h"

void TIME_1msTick(void);            //The BSP Should call this at 1ms Intervals
void TIME_FastForward(u32_t ticks); //Used by bm-driver to fast-forward time after sleeping

u32_t TIME_ReadAbsoluteTime(void);
void  TIME_WaitTicks(u32_t ticks);

#endif /* INCLUDE_CASCODA_BM_CASCODA_TIME_H_ */
