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
 *
 * @ingroup bm-sensorif
 * @defgroup bm-sensorif-si7021 Silicon Labs Si7021 temperature / humidity sensor driver
 * @brief Sensor interface for Silicon Labs Si7021 temperature / humidity sensor.
 *
 * @{
*/

#ifndef SIF_SI7021_H
#define SIF_SI7021_H

#ifdef __cplusplus
extern "C" {
#endif

/* slave addresses */
#define SIF_SAD_SI7021 0x40 /* Si7021 temperature / humidity sensor */

/* master measurement (read access) modes */
enum sif_si7021_mode
{
	SIF_SI7021_MODE_HOLD_MASTER,
	/* master hold mode (clock streching, clock suspend) */ //!< SIF_SI7021_MODE_HOLD_MASTER
	SIF_SI7021_MODE_NACK_WAIT,
	/* no-hold master hold mode (wait while NACKs) */ //!< SIF_SI7021_MODE_NACK_WAIT
	SIF_SI7021_MODE_TCONV_WAIT,
	/* wait for maximum conversion time */ //!< SIF_SI7021_MODE_TCONV_WAIT
};

/* measurement mode */
#define SIF_SI7021_MODE SIF_SI7021_MODE_HOLD_MASTER

/* max. conversion times for measurement [ms] */
#define SIF_SI7021_TCONV_MAX_TEMP 10 /* temperature */
#define SIF_SI7021_TCONV_MAX_HUM 20  /* humidity */

/* functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Read Temperature
 *******************************************************************************
 * \return Temperature in 'C, 1s complement (-128 to +127 'C)
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_SI7021_ReadTemperature(void);                                        /* measure temperature, -128 to +127 'C */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Read Humidity
 *******************************************************************************
 * \return Humidity in % (0 to 100 %)
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_SI7021_ReadHumidity(void);                                           /* measure humidity, 0 to 100 % */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Soft Reset Command
 *******************************************************************************
 ******************************************************************************/
void SIF_SI7021_Reset(void);                                                  /* device soft-reset */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SI7021: Read ID byte of Electronic Serial Number
 *******************************************************************************
 * \return Sensr ID
 *******************************************************************************
 ******************************************************************************/
u8_t SIF_SI7021_ReadID(void);                                                 /* device read device identification */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // SIF_SI7021_H
