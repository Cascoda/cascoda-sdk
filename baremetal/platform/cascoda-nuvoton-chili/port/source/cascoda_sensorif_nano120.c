/*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * sensor/actuator I2C interface functions
 * MODULE: Chili 1 (1.2, 1.3)
*/

#include <stdio.h>
/* Platform */
#include "Nano100Series.h"
#include "i2c.h"
#include "sys.h"
/* Cascoda */
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"

/* I2C Interface Module used:
 * -------------------------------------------------------------------------
 * Number  Module   SDA     SCL		Module Configuration
 * -------------------------------------------------------------------------
 * 0       I2C0     PA.4    PA.5    No restrictions, module pins 24 and 25
 * 1       I2C1                     No pins available on module
 */
/* set interface number (0/1/2) */
#define SENSORIF_I2CNUM 0

/* I2C module */
#if (SENSORIF_I2CNUM == 0)
#define SENSORIF_I2CIF I2C0
#else
#error "sensorif I2C module not valid"
#endif

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Writes bytes to I2C slave
 * \param slaveaddr - 6-Bit Slave Address
 * \param pdata - Pointer to Data Buffer
 * \param plen - Pointer to Buffer Length (actual length is returned in plen)
 *******************************************************************************
 * \return Status. 0: success, other: either I2C status or re-mapped
 *******************************************************************************
 ******************************************************************************/
enum sensorif_i2c_status SENSORIF_I2C_Write(u8_t slaveaddr, u8_t *pdata, u32_t *plen)
{
	u8_t                     transferring = 1;
	enum sensorif_i2c_status status;
	u32_t                    inlen;
	u8_t                     control;
	u32_t                    starttime = 0;

	inlen = *plen;
	*plen = 0;
	BSP_WaitUs(10);
	I2C_START(SENSORIF_I2CIF); /* send START bit */
	while (transferring)
	{
		/* wait until bus is ready */
		starttime = TIME_ReadAbsoluteTime();
		while (!(SENSORIF_I2CIF->CON & I2C_CON_I2C_STS_Msk))
		{
			if ((TIME_ReadAbsoluteTime() - starttime) > SENSORIF_I2C_TIMEOUT)
				return (SENSORIF_I2C_ST_TIMEOUT);
		}

		status = (enum sensorif_i2c_status)I2C_GET_STATUS(SENSORIF_I2CIF);
		switch (status)
		{
		case SENSORIF_I2C_ST_START: /* start: write SLA+W */
			I2C_SET_DATA(SENSORIF_I2CIF, (u8_t)(0x00 + (slaveaddr << 1)));
			control = I2C_SI;
			break;
		case SENSORIF_I2C_ST_TX_AD_ACK: /* slave has ACKed */
		case SENSORIF_I2C_ST_TX_DT_ACK:
			if (*plen < inlen) /* write data */
			{
				I2C_SET_DATA(SENSORIF_I2CIF, pdata[(*plen)++]);
			}
			else /* EOF transfer: send STOP */
			{
				control      = I2C_SI | I2C_STO;
				transferring = 0;
				status       = SENSORIF_I2C_ST_SUCCESS;
			}
			break;
		case SENSORIF_I2C_ST_TX_AD_NACK: /* slave has NACKed */
		case SENSORIF_I2C_ST_TX_DT_NACK:
			control      = I2C_SI | I2C_STO; /* send STOP */
			transferring = 0;
			break;
		case SENSORIF_I2C_ST_BUS_ERROR_RAW:
			status = SENSORIF_I2C_ST_BUS_ERROR; /* re-map bus error status */
			                                    // Fall through
		case SENSORIF_I2C_ST_ARB_LOST:          /* error or unknown status */
		default:
			control      = I2C_SI | I2C_STO; /* send STOP */
			transferring = 0;
			break;
		}
		I2C_SET_CONTROL_REG(SENSORIF_I2CIF, control);
	}

	return status;
}

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
enum sensorif_i2c_status SENSORIF_I2C_Read(u8_t slaveaddr, u8_t *pdata, u32_t *plen)
{
	u8_t                     transferring = 1;
	enum sensorif_i2c_status status;
	u32_t                    inlen;
	u8_t                     control;
	u32_t                    starttime = 0;

	inlen = *plen;
	*plen = 0;
	BSP_WaitUs(10);
	I2C_START(SENSORIF_I2CIF); /* send START bit */
	while (transferring)
	{
		/* wait until bus is ready */
		starttime = TIME_ReadAbsoluteTime();
		while (!(SENSORIF_I2CIF->CON & I2C_CON_I2C_STS_Msk))
		{
			if ((TIME_ReadAbsoluteTime() - starttime) > SENSORIF_I2C_TIMEOUT)
				return (SENSORIF_I2C_ST_TIMEOUT);
		}

		status = (enum sensorif_i2c_status)I2C_GET_STATUS(SENSORIF_I2CIF);
		switch (status)
		{
		case SENSORIF_I2C_ST_START: /* start: write SLA+RD */
			I2C_SET_DATA(SENSORIF_I2CIF, (u8_t)(0x01 + (slaveaddr << 1)));
			control = I2C_SI;
			break;
		case SENSORIF_I2C_ST_RX_AD_ACK: /* slave has ACKed SLA+RD */
			control = I2C_SI | I2C_AA;  /* set ACK */
			break;
		case SENSORIF_I2C_ST_RX_AD_NACK:     /* slave has NACKed SLA+RD */
			control      = I2C_SI | I2C_STO; /* send STOP */
			transferring = 0;
			break;
		case SENSORIF_I2C_ST_RX_DT_ACK: /* read data */
			pdata[(*plen)++] = (u8_t)I2C_GET_DATA(SENSORIF_I2CIF);
			if (*plen < (inlen - 1))
			{
				control = I2C_SI | I2C_AA; /* set ACK */
			}
			else
			{
				control = I2C_SI; /* clear ACK */
			}
			break;

		case SENSORIF_I2C_ST_RX_DT_NACK: /* read last data */
			pdata[(*plen)++] = (u8_t)I2C_GET_DATA(SENSORIF_I2CIF);
			control          = I2C_SI | I2C_STO; /* send STOP */
			transferring     = 0;
			status           = SENSORIF_I2C_ST_SUCCESS;
			break;
		case SENSORIF_I2C_ST_BUS_ERROR_RAW:
			status = SENSORIF_I2C_ST_BUS_ERROR; /* re-map bus error status */
			                                    // Fall through
		case SENSORIF_I2C_ST_ARB_LOST:          /* error or unknown status */
		default:
			control      = I2C_SI | I2C_STO; /* send STOP */
			transferring = 0;
			break;
		}
		I2C_SET_CONTROL_REG(SENSORIF_I2CIF, control);
	}

	return status;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialises and enables I2C interface
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_I2C_Init(void)
{
	/* enable I2C peripheral clock */
#if (SENSORIF_I2CNUM == 0)
	CLK_EnableModuleClock(I2C0_MODULE);
#endif

	/* SDA/SCL port configurations */
#if (SENSORIF_I2CNUM == 0)
	/* re-config PA.5 and PA.4 */
	GPIO_ENABLE_DIGITAL_PATH(PA, BIT5);
	GPIO_ENABLE_DIGITAL_PATH(PA, BIT4);
	GPIO_DISABLE_DEBOUNCE(PA, BIT5);
	GPIO_DISABLE_DEBOUNCE(PA, BIT4);
#if (SENSORIF_INT_PULLUPS)
	GPIO_ENABLE_PULL_UP(PA, BIT5);
	GPIO_ENABLE_PULL_UP(PA, BIT4);
#else
	GPIO_DISABLE_PULL_UP(PA, BIT5);
	GPIO_DISABLE_PULL_UP(PA, BIT4);
#endif
	/* initialise PA MFP for I2C0 SDA and SCL */
	/* PA.5 = I2C0 SCL */
	/* PA.4 = I2C0 SDA */
	SYS->PA_L_MFP &= ~(SYS_PA_L_MFP_PA4_MFP_Msk | SYS_PA_L_MFP_PA4_MFP_Msk);
	SYS->PA_L_MFP |= (SYS_PA_L_MFP_PA4_MFP_I2C0_SDA | SYS_PA_L_MFP_PA5_MFP_I2C0_SCL);
#endif

	/* reset I2C module */
#if (SENSORIF_I2CNUM == 0)
	SYS_ResetModule(I2C0_RST);
#endif

	/* enable I2C */
	I2C_Open(SENSORIF_I2CIF, SENSORIF_CLK_FREQUENCY);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disables I2C interface
 *******************************************************************************
 ******************************************************************************/
void SENSORIF_I2C_Deinit(void)
{
	I2C_DisableInt(SENSORIF_I2CIF);
	I2C_Close(SENSORIF_I2CIF);
#if (SENSORIF_I2CNUM == 0)
	CLK_DisableModuleClock(I2C0_MODULE);
#endif
}
