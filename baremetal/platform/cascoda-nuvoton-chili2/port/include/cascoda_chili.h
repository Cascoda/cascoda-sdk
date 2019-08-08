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
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * Internal (Non-Interface)
*/
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "M2351.h"
#include "cascoda_chili_config.h"
#include "spi.h"
#include "sys.h"

#ifndef CASCODA_CHILI_H
#define CASCODA_CHILI_H

/******************************************************************************/
/****** Global Variables defined in cascoda_bsp_*.c                      ******/
/******************************************************************************/
extern volatile u8_t     WDTimeout;            /* Nonzero watchdog timeout if used */
extern volatile u8_t     USBPresent;           /* 0: USB not active, 1: USB is active */
extern volatile u8_t     UseExternalClock;     /* 0: Use internal clock, 1: Use clock from CA-821x */
extern volatile fsys_mhz SystemFrequency;      /* system clock frequency [MHz] */
extern volatile u8_t     EnableCommsInterface; /* enable communications interface */

/* maximum count delay when switching clocks */
#define MAX_CLOCK_SWITCH_DELAY 100000
/* SPI Master Clock Rate [Hz] */
#define FCLK_SPI 2000000
/* Use Timer0 as watchdog for POWEROFF mode */
#define USE_WATCHDOG_POWEROFF 0

struct device_link
{
	uint32_t volatile *chip_select_gpio;
	uint32_t volatile *irq_gpio;
	uint32_t volatile *reset_gpio;
	struct ca821x_dev *dev;
};

#define NUM_DEVICES (1)
extern struct device_link device_list[NUM_DEVICES];

/* Non-Interface Functions */
u32_t    CHILI_ADCConversion(u32_t channel, u32_t reference);
void     CHILI_LEDBlink(void);
void     CHILI_GPIOInit(void);
void     CHILI_GPIOEnableInterrupts(void);
void     CHILI_SetClockExternalCFGXT1(u8_t clk_external);
ca_error CHILI_ClockInit(fsys_mhz fsys, u8_t enable_comms);
void     CHILI_CompleteClockInit(fsys_mhz fsys, u8_t enable_comms);
void     CHILI_EnableIntOscCal(void);
void     CHILI_DisableIntOscCal(void);
void     CHILI_GPIOPowerDown(u8_t tristateCax);
void     CHILI_GPIOPowerUp(void);
void     CHILI_TimersInit(void);
void     CHILI_SystemReInit(void);
void     TMR0_IRQHandler();
u8_t     CHILI_GetClockConfigMask(fsys_mhz fsys, u8_t enable_comms);
#if defined(USE_UART)
void  CHILI_UARTInit(void);
void  CHILI_UARTDeinit(void);
void  CHILI_UARTDeinit(void);
void  CHILI_UARTDMAWrite(u8_t *pBuffer, u32_t BufferSize);
void  CHILI_UARTFIFOWrite(u8_t *pBuffer, u32_t BufferSize);
u32_t CHILI_UARTFIFORead(u8_t *pBuffer, u32_t BufferSize);
void  CHILI_UARTDMASetupRead(u8_t *pBuffer, u32_t BufferSize);
void  CHILI_UARTFIFOIRQHandler(void);
void  CHILI_UARTDMAIRQHandler(void);
void  CHILI_UARTWaitWhileBusy(void);
#endif /* USE_UART */

#if (CASCODA_CHILI2_REV == 0) /* Chili 2.0 */
#define SPI SPI0
#define SPI_MODULE SPI0_MODULE
#elif (CASCODA_CHILI2_REV == -1) /* NuMaker-PFM-M2351 dev. board with arduino-style breakout */
#define SPI SPI1
#define SPI_MODULE SPI1_MODULE
#else
#error "Unsupported Chili 2 Revision"
#endif /* CASCODA_CHILI2_REV */

#if defined(USE_UART)
#define UART_FIFOSIZE 16
#define UART_RX_DMA_CH 0
#define UART_TX_DMA_CH 1
#if (UART_CHANNEL == 0)
#define UART UART0
#define UART_IRQn UART0_IRQn
#define UART_MODULE UART0_MODULE
#define UART_RST UART0_RST
#define UART_CLK_HXT CLK_CLKSEL1_UART0SEL_HXT
#define UART_CLK_HIRC CLK_CLKSEL1_UART0SEL_HIRC
#define UART_CLK_PLL CLK_CLKSEL1_UART0SEL_PLL
#define UART_CLKDIV CLK_CLKDIV0_UART0
#define PDMA_UART_TX PDMA_UART0_TX
#define PDMA_UART_RX PDMA_UART0_RX
#elif (UART_CHANNEL == 1)
#define UART UART1
#define UART_IRQn UART1_IRQn
#define UART_MODULE UART1_MODULE
#define UART_RST UART1_RST
#define UART_CLK_HXT CLK_CLKSEL1_UART1SEL_HXT
#define UART_CLK_HIRC CLK_CLKSEL1_UART1SEL_HIRC
#define UART_CLK_PLL CLK_CLKSEL1_UART1SEL_PLL
#define UART_CLKDIV CLK_CLKDIV0_UART1
#define PDMA_UART_TX PDMA_UART1_TX
#define PDMA_UART_RX PDMA_UART1_RX
#elif (UART_CHANNEL == 2)
#define UART UART2
#define UART_IRQn UART2_IRQn
#define UART_MODULE UART2_MODULE
#define UART_RST UART2_RST
#define UART_CLK_HXT CLK_CLKSEL3_UART2SEL_HXT
#define UART_CLK_HIRC CLK_CLKSEL3_UART2SEL_HIRC
#define UART_CLK_PLL CLK_CLKSEL3_UART2SEL_PLL
#define UART_CLKDIV CLK_CLKDIV4_UART2
#define PDMA_UART_TX PDMA_UART2_TX
#define PDMA_UART_RX PDMA_UART2_RX
/* UART3 not accessible on module */
#elif (UART_CHANNEL == 4)
#define UART UART4
#define UART_IRQn UART4_IRQn
#define UART_MODULE UART4_MODULE
#define UART_RST UART4_RST
#define UART_CLK_HXT CLK_CLKSEL3_UART4SEL_HXT
#define UART_CLK_HIRC CLK_CLKSEL3_UART4SEL_HIRC
#define UART_CLK_PLL CLK_CLKSEL3_UART4SEL_PLL
#define UART_CLKDIV CLK_CLKDIV4_UART4
#define PDMA_UART_TX PDMA_UART4_TX
#define PDMA_UART_RX PDMA_UART4_RX
#elif (UART_CHANNEL == 5)
#define UART UART5
#define UART_IRQn UART5_IRQn
#define UART_MODULE UART5_MODULE
#define UART_RST UART5_RST
#define UART_CLK_HXT CLK_CLKSEL3_UART5SEL_HXT
#define UART_CLK_HIRC CLK_CLKSEL3_UART5SEL_HIRC
#define UART_CLK_PLL CLK_CLKSEL3_UART5SEL_PLL
#define UART_CLKDIV CLK_CLKDIV4_UART5
#define PDMA_UART_TX PDMA_UART5_TX
#define PDMA_UART_RX PDMA_UART5_RX
#else
#error "UART Channel not available"
#endif /* UART_CHANNEL */
#endif /* USE_UART */

#endif /* CASCODA_CHILI_H */
