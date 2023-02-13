/**
 * @file
 * @brief mikrosdk interface
 */
/*
 *  Copyright (c) 2022, Cascoda Ltd.
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
/*
 * Example click interface driver
*/

#ifndef MOTION_CLICK_H
#define MOTION_CLICK_H

#include <stdint.h>

/* use detection pin as interrupt (1) instead of polling (0) */
#define MOTION_USE_INTERRUPT 0

/* new state inluding changes or static behaviour */
typedef enum
{
	MOTION_CH_NO_DETECT   = 0,
	MOTION_CH_DETECTED    = 1,
	MOTION_CH_NOT_PRESENT = 2,
	MOTION_CH_PRESENT     = 3,
	MOTION_CH_ERROR       = 4
} motion_detect_state_changes_t;

enum motion_status
{
	MOTION_ST_OK   = 0, /* read values ok */
	MOTION_ST_FAIL = 1  /* acquisition failed */
};

/* timing parameters [ms] */
#define MOTION_T_TONMIN 1000 /* 1 second minimum on time */

/* new functions */
motion_detect_state_changes_t MIKROSDK_MOTION_get_detected(void);
uint8_t                       MIKROSDK_MOTION_alarm_triggered(void);
void                          MIKROSDK_MOTION_Enable(void);
void                          MIKROSDK_MOTION_Disable(void);
void                          MIKROSDK_MOTION_pin_mapping(uint8_t enable, uint8_t output);
uint8_t                       MIKROSDK_MOTION_Initialise(void);
uint8_t                       MIKROSDK_MOTION_Reinitialise(void);
uint8_t                       MIKROSDK_MOTION_Acquire(uint8_t *detection_state, uint32_t *detection_time);

#endif // MOTION_CLICK_H
