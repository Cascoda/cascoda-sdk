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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Gets List Index from Module Pin
 *******************************************************************************
 * \param mpin     - module pin number
 *******************************************************************************
 * \return list index or P_NA
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_ModuleGetIndexFromPin(u8_t mpin);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets MFP Functionality
 *******************************************************************************
 * \param portnum  - returned port number
 * \param portbit  - returned port b
 * \param func - function to set the pin to
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModuleSetMFP(enPortnum portnum, u8_t portbit, u8_t func);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-configure GPIOs for PowerDown
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePowerDownGPIOs(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-configure GPIOs for PowerUp
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePowerUpGPIOs(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Module Pin Interrupt Handling
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePinIRQHandler(enPortnum portnum);

#endif /* CASCODA_CHILI_GPIO_H */
