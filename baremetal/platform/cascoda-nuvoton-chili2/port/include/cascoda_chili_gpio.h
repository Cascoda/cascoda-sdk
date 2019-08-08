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
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton Nano120
 * MODULE: Chili 1 (1.2, 1.3)
 * Module Pin and GPIO Mapping
 */
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili_config.h"

#ifndef CASCODA_CHILI_GPIO_H
#define CASCODA_CHILI_GPIO_H

/******************************************************************************/
/* Helper Macros for Port Handling                                       ******/
/******************************************************************************/
#ifndef GPIO_BASE
#define GPIO_BASE GPIOA_BASE
#endif
/* gives port (GPIO_T *), i.e. PA */
#define MGPIO_PORT(portnum) ((GPIO_T *)(GPIO_BASE + (portnum * 0x40UL)))
/* gives port pin, i.e. PA0 */
#define MGPIO_PORTPIN(portnum, pin) (*((volatile uint32_t *)((GPIO_PIN_DATA_BASE + (0x40 * (portnum))) + ((pin) << 2))))
/* gives register bit mask for port pin x */
#define BITMASK(x) (1UL << x)

/* port numbers */
typedef enum enPortnum
{
	PN_A  = 0,
	PN_B  = 1,
	PN_C  = 2,
	PN_D  = 3,
	PN_E  = 4,
	PN_F  = 5,
	PN_G  = 6,
	PN_H  = 7,
	PN_NA = 255 /* not available */
} enPortnum;

/* MFP definitions (need to be checked when changing pins or package!) */
#define PMFP_GPIO 0
#if (CASCODA_CHILI2_REV == 0) /* Chili 2.0 */
#define PMFP_SPI 4
#elif (CASCODA_CHILI2_REV == -1) /* NuMaker-PFM-M2351 dev. board with arduino-style breakout */
#define PMFP_SPI 6
#endif /* CASCODA_CHILI2_REV */
#define PMFP_UART 6
#define PMFP_ADC 1

/******************************************************************************/
/****** Hard Connected Ports / Pins                                      ******/
/******************************************************************************/
/****** Use CASCODA_CHILI_REV to specify Chili Rev. 1.x                  ******/
/******************************************************************************/
/*
 * PORTNAME_PORT:	port module, i.e. PA
 * PORTNAME_PNUM:	port numbers, 0=PA, 1=PB, 2=PC etc.
 * PORTNAME_PIN:	port pin, i.e. 20 for PA.20
 * PORTNAME_PVAL: 	port value for r/w, i.e. PA20
 */
#if (CASCODA_CHILI2_REV == 0) /* Chili 2.0 */
/* ZIG_RESET: PC.1 */
#define ZIG_RESET_PORT PC
#define ZIG_RESET_PNUM PN_C
#define ZIG_RESET_PIN 1
#define ZIG_RESET_PVAL PC1
/* ZIG_IRQB: PC.0 */
#define ZIG_IRQB_PORT PC
#define ZIG_IRQB_PNUM PN_C
#define ZIG_IRQB_PIN 0
#define ZIG_IRQB_PVAL PC0
#define ZIG_IRQB_IRQn GPC_IRQn
/* SPI_MOSI: PA.0 */
#define SPI_MOSI_PORT PA
#define SPI_MOSI_PNUM PN_A
#define SPI_MOSI_PIN 0
#define SPI_MOSI_PVAL PA0
/* SPI_MISO: PA.1 */
#define SPI_MISO_PORT PA
#define SPI_MISO_PNUM PN_A
#define SPI_MISO_PIN 1
#define SPI_MISO_PVAL PA1
/* SPI_SCLK: PA.2 */
#define SPI_SCLK_PORT PA
#define SPI_SCLK_PNUM PN_A
#define SPI_SCLK_PIN 2
#define SPI_SCLK_PVAL PA2
/* SPI_CS: PA.3 */
#define SPI_CS_PORT PA
#define SPI_CS_PNUM PN_A
#define SPI_CS_PIN 3
#define SPI_CS_PVAL PA3
#elif (CASCODA_CHILI2_REV == -1) /* NuMaker-PFM-M2351 dev. board with arduino-style breakout */
/* ZIG_RESET: PC.10 */
#define ZIG_RESET_PORT PC
#define ZIG_RESET_PNUM PN_C
#define ZIG_RESET_PIN 10
#define ZIG_RESET_PVAL PC10
/* ZIG_IRQB: PC.9 */
#define ZIG_IRQB_PORT PC
#define ZIG_IRQB_PNUM PN_C
#define ZIG_IRQB_PIN 9
#define ZIG_IRQB_PVAL PC9
#define ZIG_IRQB_IRQn GPC_IRQn
/* SPI_MOSI: PE.0 */
#define SPI_MOSI_PORT PE
#define SPI_MOSI_PNUM PN_E
#define SPI_MOSI_PIN 0
#define SPI_MOSI_PVAL PE0
/* SPI_MISO: PE.1 */
#define SPI_MISO_PORT PE
#define SPI_MISO_PNUM PN_E
#define SPI_MISO_PIN 1
#define SPI_MISO_PVAL PE1
/* SPI_SCLK: PH.8 */
#define SPI_SCLK_PORT PH
#define SPI_SCLK_PNUM PN_H
#define SPI_SCLK_PIN 8
#define SPI_SCLK_PVAL PH8
/* SPI_CS: PH.9 */
#define SPI_CS_PORT PH
#define SPI_CS_PNUM PN_H
#define SPI_CS_PIN 9
#define SPI_CS_PVAL PH9
#endif /* CASCODA_CHILI2_REV */
/* CHARGE_STAT: PB.0 */
#define CHARGE_STAT_PORT PB
#define CHARGE_STAT_PNUM PN_B
#define CHARGE_STAT_PIN 0
#define CHARGE_STAT_PVAL PB0
/* VOLTS_TEST: PB.13 */
#define VOLTS_TEST_PORT PB
#define VOLTS_TEST_PNUM PN_B
#define VOLTS_TEST_PIN 13
#define VOLTS_TEST_PVAL PB13
/* VOLTS: PB.1 */
#define VOLTS_PORT PB
#define VOLTS_PNUM PN_B
#define VOLTS_PIN 1
#define VOLTS_PVAL PB1
/* UART_RXD/UART_TXD */
#if defined(USE_UART)
#if (UART_CHANNEL == 0)
#define UART_RXD_PORT PB
#define UART_RXD_PNUM PN_B
#define UART_RXD_PIN 12
#define UART_RXD_PVAL PB12
#define UART_TXD_PORT PB
#define UART_TXD_PNUM PN_B
#define UART_TXD_PIN 13
#define UART_TXD_PVAL PB13
#elif (UART_CHANNEL == 1)
#define UART_RXD_PORT PB
#define UART_RXD_PNUM PN_B
#define UART_RXD_PIN 2
#define UART_RXD_PVAL PB2
#define UART_TXD_PORT PB
#define UART_TXD_PNUM PN_B
#define UART_TXD_PIN 3
#define UART_TXD_PVAL PB3
#elif (UART_CHANNEL == 2)
#define UART_RXD_PORT PB
#define UART_RXD_PNUM PN_B
#define UART_RXD_PIN 0
#define UART_RXD_PVAL PB0
#define UART_TXD_PORT PB
#define UART_TXD_PNUM PN_B
#define UART_TXD_PIN 1
#define UART_TXD_PVAL PB1
#elif (UART_CHANNEL == 4)
#define UART_RXD_PORT PA
#define UART_RXD_PNUM PN_A
#define UART_RXD_PIN 13
#define UART_RXD_PVAL PA13
#define UART_TXD_PORT PA
#define UART_TXD_PNUM PN_A
#define UART_TXD_PIN 12
#define UART_TXD_PVAL PA12
#elif (UART_CHANNEL == 5)
#define UART_RXD_PORT PB
#define UART_RXD_PNUM PN_B
#define UART_RXD_PIN 4
#define UART_RXD_PVAL PB4
#define UART_TXD_PORT PB
#define UART_TXD_PNUM PN_B
#define UART_TXD_PIN 5
#define UART_TXD_PVAL PB5
#endif /* UART_CHANNEL */
#endif /* USE_UART */

/******************************************************************************/
/****** Functions                                                        ******/
/******************************************************************************/
u8_t CHILI_ModuleGetIndexFromPin(u8_t mpin);
void CHILI_ModuleSetMFP(enPortnum portnum, u8_t portbit, u8_t func);
void CHILI_ModulePowerDownGPIOs(void);
void CHILI_ModulePowerUpGPIOs(void);
void CHILI_ModulePinIRQHandler(enPortnum portnum);

#endif /* CASCODA_CHILI_GPIO_H */
