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
	PN_NA = 255 /* not available */
} enPortnum;

/* MFP definitions (need to be checked when changing pins or package!) */
#define PMFP_GPIO 0
#define PMFP_SPI 3
#define PMFP_UART 1
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
/* ZIG_RESET: PA.11 */
#define ZIG_RESET_PORT PA
#define ZIG_RESET_PNUM PN_A
#define ZIG_RESET_PIN 11
#define ZIG_RESET_PVAL PA11
/* ZIG_IRQB: PA.10 */
#define ZIG_IRQB_PORT PA
#define ZIG_IRQB_PNUM PN_A
#define ZIG_IRQB_PIN 10
#define ZIG_IRQB_PVAL PA10
#define ZIG_IRQB_IRQn GPABC_IRQn
/* SPI_MOSI: PB.0 */
#define SPI_MOSI_PORT PB
#define SPI_MOSI_PNUM PN_B
#define SPI_MOSI_PIN 0
#define SPI_MOSI_PVAL PB0
/* SPI_MISO: PB.1 */
#define SPI_MISO_PORT PB
#define SPI_MISO_PNUM PN_B
#define SPI_MISO_PIN 1
#define SPI_MISO_PVAL PB1
/* SPI_SCLK: PB.2 */
#define SPI_SCLK_PORT PB
#define SPI_SCLK_PNUM PN_B
#define SPI_SCLK_PIN 2
#define SPI_SCLK_PVAL PB2
/* SPI_CS: PB.3 */
#define SPI_CS_PORT PB
#define SPI_CS_PNUM PN_B
#define SPI_CS_PIN 3
#define SPI_CS_PVAL PB3
/* CHARGE_STAT: PA.12 */
#define CHARGE_STAT_PORT PA
#define CHARGE_STAT_PNUM PN_A
#define CHARGE_STAT_PIN 12
#define CHARGE_STAT_PVAL PA12
/* USB_PRESENT: PC.7 */
#if (CASCODA_CHILI_REV == 3)
#define USB_PRESENT_PORT PC
#define USB_PRESENT_PNUM PN_C
#define USB_PRESENT_PIN 7
#define USB_PRESENT_PVAL PC7
#define USB_PRESENT_IRQn GPABC_IRQn
#endif
/* USB_PRESENT: PB.15 */
#if (CASCODA_CHILI_REV == 2)
#define USB_PRESENT_PORT PB
#define USB_PRESENT_PNUM PN_B
#define USB_PRESENT_PIN 15
#define USB_PRESENT_PVAL PB15
#define USB_PRESENT_IRQn GPABC_IRQn
#endif
/* PROGRAM: PB.15 */
#if (CASCODA_CHILI_REV == 3)
#define PROGRAM_PORT PB
#define PROGRAM_PNUM PN_B
#define PROGRAM_PIN 15
#define PROGRAM_PVAL PB15
#endif
/* VOLTS_TEST: PB.12 */
#define VOLTS_TEST_PORT PB
#define VOLTS_TEST_PNUM PN_B
#define VOLTS_TEST_PIN 12
#define VOLTS_TEST_PVAL PB12
/* VOLTS: PA.0 */
#define VOLTS_PORT PA
#define VOLTS_PNUM PN_A
#define VOLTS_PIN 0
#define VOLTS_PVAL PA0
/* UART_TXD: PB.5 */
#if defined(USE_UART)
#define UART_TXD_PORT PB
#define UART_TXD_PNUM PN_B
#define UART_TXD_PIN 5
#define UART_TXD_PVAL PB5
#endif
/* UART_RXD: PB.4 */
#if defined(USE_UART)
#define UART_RXD_PORT PB
#define UART_RXD_PNUM PN_B
#define UART_RXD_PIN 4
#define UART_RXD_PVAL PB4
#endif
/* BATT_ON: PA.8 */
#if (CASCODA_CHILI_REV == 2)
#define BATT_ON_PORT PA
#define BATT_ON_PNUM PN_A
#define BATT_ON_PIN 8
#define BATT_ON_PVAL PA8
#endif

#if (CASCODA_CHILI_REV != 2) && (CASCODA_CHILI_REV != 3)
#error "Unsupported Chili Revision"
#endif

/******************************************************************************/
/****** Functions                                                        ******/
/******************************************************************************/
u8_t CHILI_ModuleGetIndexFromPin(u8_t mpin);
void CHILI_ModuleSetMFP(enPortnum portnum, u8_t portbit, u8_t func);
void CHILI_ModulePowerDownGPIOs(void);
void CHILI_ModulePowerUpGPIOs(void);
void CHILI_ModulePinIRQHandler(enPortnum portnum);

#endif /* CASCODA_CHILI_GPIO_H */
