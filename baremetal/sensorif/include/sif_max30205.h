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
/*
 * Sensor interface for Maxim MAX30205 human body temperature sensor
*/

#ifndef SIF_MAX30205_H
#define SIF_MAX30205_H

#ifdef __cplusplus
extern "C" {
#endif

/* slave addresses */
/* Note that the slave address is hardware progammable by 3 bits (A2,A1,A0)
 * according to table 1 in the datasheet
 * SIF_SAD_MAX30205 has to match table 1 (bits 7:1 in address+r/w transfer byte,
 * not address(6:0)).
 */
#define SIF_SAD_MAX30205 0x90 /* A2/A1/A0 = 0/0/0 */

/* master measurement (read access) modes */
enum sif_max30205_mode
{
	SIF_MAX30205_MODE_POLL_ONE_SHOT, /* poll D7 in configuration register */
	SIF_MAX30205_MODE_TCONV_WAIT,    /* wait for maximum conversion time */
};

/* measurement mode */
#define SIF_MAX30205_MODE SIF_MAX30205_MODE_TCONV_WAIT

/* max. conversion times for measurement [ms] */
#define SIF_MAX30205_TCONV_MAX_TEMP 60 /* temperature */

/* configuration register bit mapping */
#define SIF_MAX30205_CONFIG_ONESHOT 0x80
#define SIF_MAX30205_CONFIG_SHUTDOWN 0x01

/* functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MAX30205: Read Temperature
 *******************************************************************************
 * \return Temperature as defined in MAX30205 Datasheet, normal format
 *******************************************************************************
 ******************************************************************************/
u16_t SIF_MAX30205_ReadTemperature(void);                                     /* measure temperature */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief MAX30205: Initialise Sensor
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_MAX30205_Initialise(void);                                           /* initialise sensor, shutdown mode */

#ifdef __cplusplus
}
#endif

#endif // SIF_MAX30205_H
