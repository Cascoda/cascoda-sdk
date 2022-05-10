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
 * @ingroup bm-sensorif
 * @defgroup bm-sensorif-tmp102 Texas TMP102 human body temperature sensor driver
 * @brief Sensor interface for Texas TMP102 human body temperature sensor.
 *
 * @{
*/

#ifndef SIF_TMP102_H
#define SIF_TMP102_H

#ifdef __cplusplus
extern "C" {
#endif

/* slave addresses */
/* Note that the slave address is hardware progammable by 7 bits and a direction bit
 * according to table 4 in the datasheet
 * SIF_SAD_TMP102 has to match table 4 (bits 7:1 in address+r/w transfer byte,
 * not address(6:0)).
 */

#define SIF_SAD_TMP102 0x48 /* ADD0 pin connection = GND */

/* Pin configuration */
#define SIF_TMP102_ALERT_PIN 33

/* master measurement (read access) modes */
enum sif_TMP102_mode
{
	SIF_TMP102_MODE_POLL_ONE_SHOT, /* poll D7 in configuration register */
	SIF_TMP102_MODE_TCONV_WAIT,    /* wait for maximum conversion time */
};

/* measurement mode */
#define SIF_TMP102_MODE SIF_TMP102_MODE_TCONV_WAIT

/* max. conversion times for measurement [ms] */
#define SIF_TMP102_TCONV_MAX_TEMP 60 /* temperature */

/* configuration register bit mapping */
#define SIF_TMP102_CONFIG_ONESHOT 0x80
#define SIF_TMP102_CONFIG_INTERRUPT 0x02 /* The AL pin is activated until being any pin being read */

/* functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Read Temperature
 *******************************************************************************
 * \return Temperature as defined in TMP102 Datasheet, normal format. It is a fixed
 * point number. Refer to TMP102 Datasheet Table 2 for more information on the conversion of temperature.
 *******************************************************************************
 ******************************************************************************/
u16_t SIF_TMP102_ReadTemperature(void);                                       /* measure temperature */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Initialise Sensor
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_TMP102_Initialise(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TMP102: Soft Reset Command
 *******************************************************************************
 ******************************************************************************/
void SIF_TMP102_Reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // SIF_TMP102_H