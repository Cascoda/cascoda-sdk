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
 * sensor/actuator I2C interface functions
*/

#ifndef CASCODA_SENSORIF_H
#define CASCODA_SENSORIF_H

#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* declarations */
#define SENSORIF_I2C_TIMEOUT 100           /* I2C bus access time-out time in [ms] */
#define SENSORIF_I2C_CLK_FREQUENCY 100000  /* I2C SCL frequency [Hz], 100kHz (full-speed) */
#define SENSORIF_INT_PULLUPS 0             /* I2C use internal pull-ups flag */
#define SENSORIF_SPI_CLK_FREQUENCY 4000000 /* SPI CLK frequency [Hz], 4MHz (max frequency for E-paper display) */
#define SENSORIF_SPI_DATA_WIDTH 8

/* Note: most internal GPIO pull-ups have a value of around 50kOhms which is usually
 * too high for I2C pull-up values, depending  on number of peripherals connected and
 * overall load capacitance. The SCL clock frequency might have to be lowered to below
 * full-speed if no external pull-ups on SDA and SCL are available.
 */

/** I2C master status enumerations */
enum sensorif_i2c_status
{
	SENSORIF_I2C_ST_START         = 0x08, /**< start */
	SENSORIF_I2C_ST_RSTART        = 0x10, /**< repeat start */
	SENSORIF_I2C_ST_TX_AD_ACK     = 0x18, /**< transmit address ACKed */
	SENSORIF_I2C_ST_TX_AD_NACK    = 0x20, /**< transmit address NACKed */
	SENSORIF_I2C_ST_TX_DT_ACK     = 0x28, /**< transmit data    ACKed */
	SENSORIF_I2C_ST_TX_DT_NACK    = 0x30, /**< transmit data    NACKed */
	SENSORIF_I2C_ST_ARB_LOST      = 0x38, /**< arbitration lost */
	SENSORIF_I2C_ST_RX_AD_ACK     = 0x40, /**< receive  address ACKed */
	SENSORIF_I2C_ST_RX_AD_NACK    = 0x48, /**< receive  address NACKed */
	SENSORIF_I2C_ST_RX_DT_ACK     = 0x50, /**< receive  data    ACKed */
	SENSORIF_I2C_ST_RX_DT_NACK    = 0x58, /**< receive  data    NACKed */
	SENSORIF_I2C_ST_BUS_ERROR_RAW = 0x00, /**< bus error code transmitted over wire*/
	SENSORIF_I2C_ST_STOP          = 0xF0, /**< on reset or stop */
	SENSORIF_I2C_ST_RELEASED      = 0xF8, /**< bus released */

	SENSORIF_I2C_ST_NOT_IMPLEMENTED = 0xFE, /**< not implemented */
	SENSORIF_I2C_ST_SUCCESS         = 0x00, /**< successful transfer */
	SENSORIF_I2C_ST_TIMEOUT         = 0xFF, /**< bus access time-out */
	SENSORIF_I2C_ST_BUS_ERROR       = 0xF1, /**< bus error re-mapped from 0x00 for status return*/
};

/* functions for platform implementation */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialises and enables I2C interface
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_I2C_Init(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disables I2C interface
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_I2C_Deinit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Writes bytes to I2C slave
 * \param slaveaddr - 6-Bit Slave Address
 * \param data - Pointer to Data Buffer
 * \param len - Pointer to Buffer Length (actual length is returned in plen)
 *******************************************************************************
 * \return Status. 0: success, other: either I2C status or re-mapped
 *******************************************************************************
 ******************************************************************************/
enum sensorif_i2c_status SENSORIF_I2C_Write(u8_t slaveaddr, u8_t *data, u32_t *len);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reads bytes from I2C slave
 *******************************************************************************
 * \param slaveaddr - 6-Bit Slave Address
 * \param pdata - Pointer to Data Buffer
 * \param plen - Pointer to Buffer Length (actual length is returned in plen)
 *******************************************************************************
 * \return Status. 0: success, other: either I2C status or re-mapped
 *******************************************************************************
 ******************************************************************************/
enum sensorif_i2c_status SENSORIF_I2C_Read(u8_t slaveaddr, u8_t *pdata, u32_t *plen);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialises and enables SPI interface
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_SPI_Init(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disables SPI interface
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_SPI_Deinit(void);

/************************************************************************************************************/
/*********************************************************************************************************/ /**
 * \brief Writes bytes to SPI slave
 * \param out_data - 8-bit data to send
 *************************************************************************************************************
 * \return Return CA_ERROR_SUCCESS = 0x00 if successful and CA_ERROR_FAIL = 0x01 if transmit FIFO is full
 *************************************************************************************************************
 ************************************************************************************************************/
ca_error SENSORIF_SPI_Write(u8_t out_data);

#ifdef __cplusplus
}
#endif

#endif // CASCODA_SENSORIF_H
