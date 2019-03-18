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
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0
*/

#include <stdio.h>
/* Platform */
#include "M2351.h"
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
 * 0       I2C0     PB.4    PB.5    Both USB-Stick and One-Sided (Solder-On)
 * 1       I2C1     PB.0    PB.1    One-Sided Population (Solder-On) only
 * 2       I2C2     PB.12   PB.13   One-Sided Population (Solder-On) only
 */
/* set interface number (0/1/2) */
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
	u8_t                     control0;
	u32_t                    starttime = 0;

	inlen = *plen;
	*plen = 0;
	BSP_WaitUs(10);
	I2C_START(SENSORIF_I2CIF); /* send START bit */
	while (transferring)
	{
		/* wait until bus is ready */
		starttime = TIME_ReadAbsoluteTime();
		while (!(SENSORIF_I2CIF->CTL0 & I2C_CTL0_SI_Msk))
		{
			if ((TIME_ReadAbsoluteTime() - starttime) > SENSORIF_I2C_TIMEOUT)
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
	u8_t                     control0;
	u32_t                    starttime = 0;

	inlen = *plen;
	*plen = 0;
	BSP_WaitUs(10);
	I2C_START(SENSORIF_I2CIF); /* send START bit */
	while (transferring)
	{
		/* wait until bus is ready */
		starttime = TIME_ReadAbsoluteTime();
		while (!(SENSORIF_I2CIF->CTL0 & I2C_CTL0_SI_Msk))
		{
			if ((TIME_ReadAbsoluteTime() - starttime) > SENSORIF_I2C_TIMEOUT)
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
			control0 = I2C_CTL_SI_AA;   /* set ACK */
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
#elif (SENSORIF_I2CNUM == 1)
	CLK_EnableModuleClock(I2C1_MODULE);
#else
	CLK_EnableModuleClock(I2C2_MODULE);
#endif

	/* SDA/SCL port configurations */
#if (SENSORIF_I2CNUM == 0)
	/* re-config PB.5 and PB.4 */
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT5);
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT4);
	GPIO_DISABLE_DEBOUNCE(PB, BIT5);
	GPIO_DISABLE_DEBOUNCE(PB, BIT4);
#if (SENSORIF_INT_PULLUPS)
	GPIO_SetPullCtl(PB, BIT5, GPIO_PUSEL_PULL_UP);
	GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_PULL_UP);
#else
	GPIO_SetPullCtl(PB, BIT5, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_DISABLE);
#endif
	/* initialise PB MFP for I2C0 SDA and SCL */
	/* PB.5 = I2C0 SCL */
	/* PB.4 = I2C0 SDA */
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB4MFP_I2C0_SDA | SYS_GPB_MFPL_PB5MFP_I2C0_SCL);
#elif (SENSORIF_I2CNUM == 1)
	/* re-config PB.1 and PB.0 */
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT1);
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT0);
	GPIO_DISABLE_DEBOUNCE(PB, BIT1);
	GPIO_DISABLE_DEBOUNCE(PB, BIT0);
#if (SENSORIF_INT_PULLUPS)
	GPIO_SetPullCtl(PB, BIT1, GPIO_PUSEL_PULL_UP);
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_PULL_UP);
#else
	GPIO_SetPullCtl(PB, BIT1, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_DISABLE);
#endif
	/* initialise PB MFP for I2C1 SDA and SCL */
	/* PB.1 = I2C1 SCL */
	/* PB.0 = I2C1 SDA */
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB0MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB0MFP_I2C1_SDA | SYS_GPB_MFPL_PB1MFP_I2C1_SCL);
#else
	/* re-config PB.13 and PB.12 */
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT13);
	GPIO_ENABLE_DIGITAL_PATH(PB, BIT12);
	GPIO_DISABLE_DEBOUNCE(PB, BIT13);
	GPIO_DISABLE_DEBOUNCE(PB, BIT12);
#if (SENSORIF_INT_PULLUPS)
	GPIO_SetPullCtl(PB, BIT13, GPIO_PUSEL_PULL_UP);
	GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_PULL_UP);
#else
	GPIO_SetPullCtl(PB, BIT13, GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_DISABLE);
#endif
	/* initialise PB MFP for I2C2 SDA and SCL */
	/* PB.13 = I2C2 SCL */
	/* PB.12 = I2C2 SDA */
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPH_PB12MFP_I2C2_SDA | SYS_GPB_MFPH_PB13MFP_I2C2_SCL);
#endif

	/* reset I2C module */
#if (SENSORIF_I2CNUM == 0)
	SYS_ResetModule(I2C0_RST);
#elif (SENSORIF_I2CNUM == 1)
	SYS_ResetModule(I2C1_RST);
#else
	SYS_ResetModule(I2C2_RST);
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
#elif (SENSORIF_I2CNUM == 1)
	CLK_DisableModuleClock(I2C1_MODULE);
#else
	CLK_DisableModuleClock(I2C2_MODULE);
#endif
}
