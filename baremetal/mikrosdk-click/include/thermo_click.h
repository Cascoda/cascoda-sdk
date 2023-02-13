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

#ifndef THERMO_CLICK_H
#define THERMO_CLICK_H

#include <stdint.h>

/* data acquisition status */
enum thermo_status
{
	THERMO_ST_OK   = 0, /* read values ok */
	THERMO_ST_FAIL = 1  /* acquisition failed */
};

/* timing parameters [ms] */
#define THERMO_T_POWERUP 200 /* max31855 200 ms power-up time */

/* new functions */
uint8_t MIKROSDK_THERMO_get_thermo_temperature(uint16_t *temperature);
uint8_t MIKROSDK_THERMO_get_junction_temperature(uint16_t *temperature);
uint8_t MIKROSDK_THERMO_get_all_temperatures(uint16_t *thermo_temperature, uint16_t *junction_temperature);
uint8_t MIKROSDK_THERMO_Initialise(void);
uint8_t MIKROSDK_THERMO_Reinitialise(void);
uint8_t MIKROSDK_THERMO_Acquire(uint16_t *thermo_temperature, uint16_t *junction_temperature);

#endif // THERMO_CLICK_H
