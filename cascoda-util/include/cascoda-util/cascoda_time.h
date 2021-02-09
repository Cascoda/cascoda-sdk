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
/**
 *  @file
 *  @brief A simple time interface that records absolute time to millisecond resolution
 *  and allows waiting for a set number of milliseconds.
 */
/**
 * @ingroup cascoda-util
 * @defgroup ca-time Time
 * @brief  Simple time interface for getting time since application start with millisecond resolution
 *
 * @{
 */

#ifndef INCLUDE_CASCODA_BM_CASCODA_TIME_H_
#define INCLUDE_CASCODA_BM_CASCODA_TIME_H_

#include <stdint.h>

/**
 * Get the number of milliseconds since program start.
 *
 * @returns The number of milliseconds since program start
 */
uint32_t TIME_ReadAbsoluteTime(void);

/**
 * Compare the time arguments, including the loop-around nature of the time value.
 * Note that this function will be inaccurate if there is more than 2^31 milliseconds difference.
 *
 * @param aT1 T1
 * @param aT2 T2
 *
 * @retval 0  T1 == T2
 * @retval 1  T1 > T2
 * @retval -1 T1 < T2
 */
int TIME_Cmp(uint32_t aT1, uint32_t aT2);

#endif /* INCLUDE_CASCODA_BM_CASCODA_TIME_H_ */

/**
 * @}
 */
