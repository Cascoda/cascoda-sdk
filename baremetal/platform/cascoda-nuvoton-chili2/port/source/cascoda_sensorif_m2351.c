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
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0
*/

#include <stdio.h>
/* Platform */
#include "M2351.h"
#include "i2c.h"
#include "spi.h"
#include "sys.h"
/* Cascoda */
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"

/* I2C Interface Module used:
 * -------------------------------------------------------------------------
 * Number  Module   SDA     SCL		Module Configuration
 * -------------------------------------------------------------------------
 * 0       I2C0     PB.4    PB.5    Both USB-Stick and One-Sided (Solder-On)
 * 1       I2C1     PB.0    PB.1    One-Sided Population (Solder-On) only
 * 2       I2C2     PB.12   PB.13   One-Sided Population (Solder-On) only
 */

/* SPI Interface Module used:
 * -------------------------------------------------------------------------
 * Number  Module   MISO     MOSI     CLK     SS
 * -------------------------------------------------------------------------
 * 1       SPI1     PB.5     PB.4     PB.3    PB.2
 * 2       SPI2     PA.14    PA.15    PA.13   PA.12
 */

/****************************************************************************/
/* These values must also be updated in cascoda_sensorif_secure.c           */
/****************************************************************************/

/* set I2C interface number (0/1/2) */
#define SENSORIF_I2CNUM 1

/* I2C module */
#if (SENSORIF_I2CNUM == 0)
#define SENSORIF_I2CIF I2C0
#elif (SENSORIF_I2CNUM == 1)
#define SENSORIF_I2CIF I2C1
#elif (SENSORIF_I2CNUM == 2)
#define SENSORIF_I2CIF I2C2
#else
#error "sensorif I2C module not valid"
#endif

/* set SPI interface number (1/2)*/
#define SENSORIF_SPINUM 1

/* SPI module */
#if (SENSORIF_SPINUM == 1)
#define SENSORIF_SPIIF SPI1
#elif (SENSORIF_SPINUM == 2)
#define SENSORIF_SPIIF SPI2
#else
#error "sensorif SPI module not valid"
#endif

enum sensorif_i2c_status SENSORIF_I2C_Write(u8_t slaveaddr, u8_t *pdata, u32_t *plen)
{
	u8_t                     transferring = 1;
	enum sensorif_i2c_status status;
	u32_t                    inlen;
	u8_t                     control0  = I2C_CTL_STO_SI;
	u32_t                    starttime = 0;

	inlen = *plen;
	*plen = 0;
	BSP_WaitUs(10);
	I2C_START(SENSORIF_I2CIF); /* send START bit */
	while (transferring)
	{
		/* wait until bus is ready */
		starttime = BSP_ReadAbsoluteTime();
		while (!(SENSORIF_I2CIF->CTL0 & I2C_CTL0_SI_Msk))
		{
			if ((BSP_ReadAbsoluteTime() - starttime) > SENSORIF_I2C_TIMEOUT)
				return (SENSORIF_I2C_ST_TIMEOUT);
		}

		status = (enum sensorif_i2c_status)I2C_GET_STATUS(SENSORIF_I2CIF);
		switch (status)
		{
		case SENSORIF_I2C_ST_START: /* start: write SLA+W */
			I2C_SET_DATA(SENSORIF_I2CIF, (u8_t)(0x00 + (slaveaddr << 1)));
			control0 = I2C_CTL_SI;
			break;
		case SENSORIF_I2C_ST_TX_AD_ACK: /* slave has ACKed */
		case SENSORIF_I2C_ST_TX_DT_ACK:
			if (*plen < inlen) /* write data */
			{
				I2C_SET_DATA(SENSORIF_I2CIF, pdata[(*plen)++]);
			}
			else /* EOF transfer: send STOP */
			{
				control0     = I2C_CTL_STO_SI;
				transferring = 0;
				status       = SENSORIF_I2C_ST_SUCCESS;
			}
			break;
		case SENSORIF_I2C_ST_TX_AD_NACK: /* slave has NACKed */
		case SENSORIF_I2C_ST_TX_DT_NACK:
			control0     = I2C_CTL_STO_SI; /* send STOP */
			transferring = 0;
			break;
		case SENSORIF_I2C_ST_BUS_ERROR_RAW:
			status = SENSORIF_I2C_ST_BUS_ERROR; /* re-map bus error status */
			                                    // Fall through
		case SENSORIF_I2C_ST_ARB_LOST:          /* error or unknown status */
		default:
			control0     = I2C_CTL_STO_SI; /* send STOP */
			transferring = 0;
			break;
		}
		I2C_SET_CONTROL_REG(SENSORIF_I2CIF, control0);
	}

	return status;
}

enum sensorif_i2c_status SENSORIF_I2C_Read(u8_t slaveaddr, u8_t *pdata, u32_t *plen)
{
	u8_t                     transferring = 1;
	enum sensorif_i2c_status status;
	u32_t                    inlen;
	u8_t                     control0;
	u32_t                    starttime = 0;

	inlen = *plen;
	*plen = 0;
	BSP_WaitUs(10);
	I2C_START(SENSORIF_I2CIF); /* send START bit */
	while (transferring)
	{
		/* wait until bus is ready */
		starttime = BSP_ReadAbsoluteTime();
		while (!(SENSORIF_I2CIF->CTL0 & I2C_CTL0_SI_Msk))
		{
			if ((BSP_ReadAbsoluteTime() - starttime) > SENSORIF_I2C_TIMEOUT)
				return (SENSORIF_I2C_ST_TIMEOUT);
		}

		status = (enum sensorif_i2c_status)I2C_GET_STATUS(SENSORIF_I2CIF);
		switch (status)
		{
		case SENSORIF_I2C_ST_START: /* start: write SLA+RD */
			I2C_SET_DATA(SENSORIF_I2CIF, (u8_t)(0x01 + (slaveaddr << 1)));
			control0 = I2C_CTL_SI;
			break;
		case SENSORIF_I2C_ST_RX_AD_ACK: /* slave has ACKed SLA+RD */
			if (inlen > 1)
			{
				control0 = I2C_CTL_SI_AA; /* set ACK */
			}
			else
			{
				control0 = I2C_CTL_SI; /* clear ACK as only one byte will be read */
			}
			break;
		case SENSORIF_I2C_ST_RX_AD_NACK:   /* slave has NACKed SLA+RD */
			control0     = I2C_CTL_STO_SI; /* send STOP */
			transferring = 0;
			break;
		case SENSORIF_I2C_ST_RX_DT_ACK: /* read data */
			pdata[(*plen)++] = (u8_t)I2C_GET_DATA(SENSORIF_I2CIF);
			if (*plen < (inlen - 1))
			{
				control0 = I2C_CTL_SI_AA; /* set ACK */
			}
			else
			{
				control0 = I2C_CTL_SI; /* clear ACK */
			}
			break;

		case SENSORIF_I2C_ST_RX_DT_NACK: /* read last data */
			pdata[(*plen)++] = (u8_t)I2C_GET_DATA(SENSORIF_I2CIF);
			control0         = I2C_CTL_STO_SI; /* send STOP */
			transferring     = 0;
			status           = SENSORIF_I2C_ST_SUCCESS;
			break;
		case SENSORIF_I2C_ST_BUS_ERROR_RAW:
			status = SENSORIF_I2C_ST_BUS_ERROR; /* re-map bus error status */
			                                    // Fall through
		case SENSORIF_I2C_ST_ARB_LOST:          /* error or unknown status */
		default:
			control0     = I2C_CTL_STO_SI; /* send STOP */
			transferring = 0;
			break;
		}
		I2C_SET_CONTROL_REG(SENSORIF_I2CIF, control0);
	}

	return status;
}

__NONSECURE_ENTRY ca_error SENSORIF_SPI_Write(u8_t out_data)
{
	if (!SPI_GET_TX_FIFO_FULL_FLAG(SENSORIF_SPIIF))
	{
		SPI_WRITE_TX(SENSORIF_SPIIF, out_data);
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_FAIL;
}
