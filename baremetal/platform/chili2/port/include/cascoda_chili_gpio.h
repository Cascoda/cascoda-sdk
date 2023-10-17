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
 * @file
 * @brief Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton Nano120
 * MODULE: Chili 1 (1.2, 1.3)
 * Module Pin and GPIO Mapping
 */

#ifndef CASCODA_CHILI_GPIO_H
#define CASCODA_CHILI_GPIO_H

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Helper Macros for Port Handling                                       ******/
/******************************************************************************/
/* gives port (GPIO_T *), i.e. PA */
#define MGPIO_PORT(portnum) ((GPIO_T *)(GPIO_BASE + ((int)PA & NS_OFFSET) + (portnum * 0x40UL)))
/* gives port pin, i.e. PA0 */
#define MGPIO_PORTPIN(portnum, pin) \
	(*((volatile uint32_t *)((GPIO_PIN_DATA_BASE + ((int)PA & NS_OFFSET) + (0x40 * (portnum))) + ((pin) << 2))))
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
#define PMFP_ADC 1
/* Chili2 */
#if ((CASCODA_CHILI2_CONFIG == 0) || (CASCODA_CHILI2_CONFIG == 1))
#define PMFP_SPI 4
/* Chili2 devboard */
#elif (CASCODA_CHILI2_CONFIG == 2)
#define PMFP_SPI 4
/* NuMaker-PFM-M2351 dev. board with arduino-style breakout - center SPI connection */
#elif (CASCODA_CHILI2_CONFIG == 3)
#define PMFP_SPI 6
/* NuMaker-PFM-M2351 dev. board with arduino-style breakout - SIP connect D10-D13 */
#elif (CASCODA_CHILI2_CONFIG == 4)
#define PMFP_SPI 5
#endif /* CASCODA_CHILI2_CONFIG */
#if defined(USE_UART)
#if (UART_CHANNEL == 0)
#define PMFP_UART 6
#elif (UART_CHANNEL == 1)
#define PMFP_UART 6
#elif (UART_CHANNEL == 2)
#define PMFP_UART 7
#elif (UART_CHANNEL == 4)
#define PMFP_UART 3
#elif (UART_CHANNEL == 5)
#define PMFP_UART 7
#endif /* UART_CHANNEL */
#else
#define PMFP_UART 6
#endif /* USE_UART */
#define PMFP_X32 10
#if ((CASCODA_CHILI2_CONFIG == 3) || (CASCODA_CHILI2_CONFIG == 4))
#define PMFP_XT1 10
#endif

/******************************************************************************/
/****** Hard Connected Ports / Pins                                      ******/
/******************************************************************************/
/****** Use CASCODA_CHILI2_CONFIG to specify Chili2 platform             ******/
/******************************************************************************/
/*
 * PORTNAME_PORT:	port module, i.e. PA
 * PORTNAME_PNUM:	port numbers, 0=PA, 1=PB, 2=PC etc.
 * PORTNAME_PIN:	port pin, i.e. 20 for PA.20
 * PORTNAME_PVAL: 	port value for r/w, i.e. PA20
 */

/* Chili2 */
#if ((CASCODA_CHILI2_CONFIG == 0) || (CASCODA_CHILI2_CONFIG == 1))
#if (CASCODA_CHILI_DISABLE_CA821x == 0)
/* ZIG_RESET: PC.1 */
#define ZIG_RESET_PORT PC
#define ZIG_RESET_PNUM PN_C
#define ZIG_RESET_PIN 1
#define ZIG_RESET_PVAL PC1
#else
/* ZIG_INTERNAL_RESET: PC.1 */
#define ZIG_INTERNAL_RESET_PORT PC
#define ZIG_INTERNAL_RESET_PNUM PN_C
#define ZIG_INTERNAL_RESET_PIN 1
#define ZIG_INTERNAL_RESET_PVAL PC1
/* ZIG_RESET: PB.5 */
#define ZIG_RESET_PORT PB
#define ZIG_RESET_PNUM PN_B
#define ZIG_RESET_PIN 5
#define ZIG_RESET_PVAL PB5
#endif
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
/* Chili2 devboard */
#elif (CASCODA_CHILI2_CONFIG == 2)
#if (CASCODA_CHILI_DISABLE_CA821x == 0)
/* ZIG_RESET: PC.1 */
#define ZIG_RESET_PORT PC
#define ZIG_RESET_PNUM PN_C
#define ZIG_RESET_PIN 1
#define ZIG_RESET_PVAL PC1
#else
/* ZIG_INTERNAL_RESET: PC.1 */
#define ZIG_INTERNAL_RESET_PORT PC
#define ZIG_INTERNAL_RESET_PNUM PN_C
#define ZIG_INTERNAL_RESET_PIN 1
#define ZIG_INTERNAL_RESET_PVAL PC1
/* ZIG_RESET: PB.5 */
#define ZIG_RESET_PORT PB
#define ZIG_RESET_PNUM PN_B
#define ZIG_RESET_PIN 5
#define ZIG_RESET_PVAL PB5
#endif
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
/* NuMaker-PFM-M2351 dev. board with arduino-style breakout - center SPI connection */
#elif (CASCODA_CHILI2_CONFIG == 3)
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
/* XT1I: PF.3 */
#define XT1I_PORT PF
#define XT1I_PNUM PN_F
#define XT1I_PIN 3
#define XT1I_PVAL PF3
/* XT1O: PF.2 */
#define XT1O_PORT PF
#define XT1O_PNUM PN_F
#define XT1O_PIN 2
#define XT1O_PVAL PF2
/* NuMaker-PFM-M2351 dev. board with arduino-style breakout - SIP connect D10-D13 */
#elif (CASCODA_CHILI2_CONFIG == 4)
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
/* SPI_MOSI: PF.6 */
#define SPI_MOSI_PORT PF
#define SPI_MOSI_PNUM PN_F
#define SPI_MOSI_PIN 6
#define SPI_MOSI_PVAL PF6
/* SPI_MISO: PF.7 */
#define SPI_MISO_PORT PF
#define SPI_MISO_PNUM PN_F
#define SPI_MISO_PIN 7
#define SPI_MISO_PVAL PF7
/* SPI_SCLK: PF.8 */
#define SPI_SCLK_PORT PF
#define SPI_SCLK_PNUM PN_F
#define SPI_SCLK_PIN 8
#define SPI_SCLK_PVAL PF8
/* SPI_CS: PF.9 */
#define SPI_CS_PORT PF
#define SPI_CS_PNUM PN_F
#define SPI_CS_PIN 9
#define SPI_CS_PVAL PF9
/* XT1I: PF.3 */
#define XT1I_PORT PF
#define XT1I_PNUM PN_F
#define XT1I_PIN 3
#define XT1I_PVAL PF3
/* XT1O: PF.2 */
#define XT1O_PORT PF
#define XT1O_PNUM PN_F
#define XT1O_PIN 2
#define XT1O_PVAL PF2
#endif /* CASCODA_CHILI2_CONFIG */
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
/* X32I: PF.5 */
#define X32I_PORT PF
#define X32I_PNUM PN_F
#define X32I_PIN 5
#define X32I_PVAL PF5
/* X32O: PF.4 */
#define X32O_PORT PF
#define X32O_PNUM PN_F
#define X32O_PIN 4
#define X32O_PVAL PF4
/* VBUS: PA.12 */
#define VBUS_PORT PA
#define VBUS_PNUM PN_A
#define VBUS_PIN 12
#define VBUS_PVAL PA12
/* VBUS_CONNECTED */
#define VBUS_CONNECTED_PORT PB
#define VBUS_CONNECTED_PNUM PN_B
#define VBUS_CONNECTED_PIN 12
#define VBUS_CONNECTED_PVAL PB12
/* EXTERNAL_SPI_FLASH_CS */
#if CASCODA_EXTERNAL_FLASHCHIP_PRESENT
#define EXTERNAL_SPI_FLASH_CS_PORT PA
#define EXTERNAL_SPI_FLASH_CS_PNUM PN_A
#define EXTERNAL_SPI_FLASH_CS_PIN 15
#define EXTERNAL_SPI_FLASH_CS_PVAL PA15
#endif

/******************************************************************************/
/****** Functions                                                        ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Gets Module Pin from List index
 *******************************************************************************
 * \param portnum     - port number
 * \param portbit     - pin number
 *******************************************************************************
 * \return list index or P_NA
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_ModuleGetPinFromPort(enPortnum portnum, u8_t portbit);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Gets pin number from module pin
 *******************************************************************************
 * \param mpin     - module pin number
 *******************************************************************************
 * \return list index or P_NA
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_ModuleGetPortBitFromPin(u8_t mpin);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Gets port number from module pin
 *******************************************************************************
 * \param mpin     - module pin number
 *******************************************************************************
 * \return list index or P_NA
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_ModuleGetPortNumFromPin(u8_t mpin);

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
void CHILI_ModulePowerDownGPIOs(u8_t useGPIOforWakeup);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-configure GPIOs for PowerUp
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePowerUpGPIOs(u8_t useGPIOforWakeup);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Module Pin Interrupt Handling
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePinIRQHandler(enPortnum portnum);

#ifdef __cplusplus
}
#endif

#endif /* CASCODA_CHILI_GPIO_H */
