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
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * Internal (Non-Interface)
*/

#ifndef CASCODA_CHILI_H
#define CASCODA_CHILI_H

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili_config.h"

#include "M2351.h"
#include "spi.h"
#include "sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/****** Global Variables defined in cascoda_bsp_*.c                      ******/
/******************************************************************************/
extern volatile u8_t WDTimeout;  /* Nonzero watchdog timeout if used */
extern volatile u8_t USBPresent; /* 0: USB not active, 1: USB is active */

/* maximum count delay when switching clocks */
#define MAX_CLOCK_SWITCH_DELAY 100000
/* SPI Master Clock Rate [Hz] */
#define FCLK_SPI 2000000
/* Use Timer0 as watchdog for POWEROFF mode */
#define USE_WATCHDOG_POWEROFF 1
/* Use LXT instead of LIRC32 for RTC (more accurate, but higher power consumption) */
#define USE_RTC_LXT 0

struct device_link
{
	uint32_t volatile *chip_select_gpio;
	uint32_t volatile *irq_gpio;
	uint32_t volatile *reset_gpio;
	dispatch_read_t    dispatch_read;
	struct ca821x_dev *dev;
};

#define NUM_DEVICES (1)
extern struct device_link device_list[NUM_DEVICES];

/* Non-Interface Functions */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Perform ADC Conversion
 *******************************************************************************
 ******************************************************************************/
u32_t CHILI_ADCConversion(u32_t channel, u32_t reference);
void  CHILI_LEDBlink(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise Essential GPIOs for various Functions
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOInit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable GPIO Interrupts
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOEnableInterrupts(void);

/******************************************************************************/
/**************************************************************************/ /**
 * \brief Configure CFGXT1 bit for HXT mode in CONFIG0
 *
 * This isn't specifically related to dataflash, but in order to set the clock
 * on the M2351, the config is loaded into the flash.
 *******************************************************************************
 * \param clk_external - 1: external clock input on HXT, 0: HXT is crystal osc.
 *******************************************************************************
 ******************************************************************************/
void CHILI_SetClockExternalCFGXT1(u8_t clk_external);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Select System Clocks depending on Power Source
 *******************************************************************************
 ******************************************************************************/
ca_error CHILI_ClockInit(fsys_mhz fsys, u8_t enable_comms);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set the SysTick frequency in Hz
 * \param freqHz Frequency of the system tick in Hz, or 0 to disable
 *******************************************************************************
 ******************************************************************************/
void CHILI_SetSysTickFreq(uint32_t freqHz);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Clock (Re-)Initialisation.
 *******************************************************************************
 ******************************************************************************/
void CHILI_CompleteClockInit(fsys_mhz fsys, u8_t enable_comms);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_EnableIntOscCal(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_DisableIntOscCal(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for PowerDown
 * \param tristateCax - bool: set to 1 to tri-state the CA-821x interface
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerDown(u8_t tristateCax);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for active Mode after PowerDown
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerUp(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief (Re-)Initialise System Timers
 *******************************************************************************
 ******************************************************************************/
void CHILI_TimersInit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise PDMA
 *******************************************************************************
 ******************************************************************************/
void CHILI_PDMAInit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Re-Initialisation
 *******************************************************************************
 ******************************************************************************/
void CHILI_SystemReInit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 32: Timer0 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR0_IRQHandler();

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Clock
 *******************************************************************************
 ******************************************************************************/
u8_t CHILI_GetClockConfigMask(fsys_mhz fsys, u8_t enable_comms);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief RTC IRQ Handler
  *******************************************************************************
 ******************************************************************************/
void CHILI_RTCIRQHandler(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief 1 ms Tick for TMR0_IRQHandler ISR
 *******************************************************************************
 ******************************************************************************/
void CHILI_1msTick(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief FastForward time by the given amount
 *******************************************************************************
 * \param ticks - Time in Ticks (1ms/100ms)
 *******************************************************************************
 ******************************************************************************/
void CHILI_FastForward(u32_t ticks);

#if defined(USE_UART)

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise UART for Comms
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTInit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief De-Initialise UART
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTDeinit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Complete Buffer Write using DMA
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTDMAWrite(u8_t *pBuffer, u32_t BufferSize);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Complete Buffer Write using FIFO
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTFIFOWrite(u8_t *pBuffer, u32_t BufferSize);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Buffer FIFO Read
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 * \return Number of Characters placed in Buffer
 *******************************************************************************
 ******************************************************************************/
u32_t CHILI_UARTFIFORead(u8_t *pBuffer, u32_t BufferSize);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Set up DMA for UART Read
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTDMASetupRead(u8_t *pBuffer, u32_t BufferSize);

/**
 * Cancel a configured DMA Read. Can be used if a DMA transaction times out.
 */
void CHILI_UARTDMAReadCancel(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Interrupt Handler for UART FIFO
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTFIFOIRQHandler(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Interrupt Handler for UART DMA
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTDMAIRQHandler(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Waits While UART transfer is going on (DMA or FIFO)
 *******************************************************************************
 * \param pBuffer - Pointer to Data Buffer
 * \param BufferSize - Max. Characters to Read
 *******************************************************************************
 * \return Number of Characters placed in Buffer
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTWaitWhileBusy(void);
#endif /* USE_UART */

/**
 * Enable CRYPTO Module clock
 */
void CHILI_CRYPTOEnableClock(void);

/**
 * Disable CRYPTO Module clock
 */
void CHILI_CRYPTODisableClock(void);

/** function to kick the linker into including strong ISR overrides */
void cascoda_isr_secure_init(void);
/** function to kick the linker into including strong ISR overrides */
void cascoda_isr_chili_init(void);

#if (CASCODA_CHILI2_REV == 0) /* Chili 2.0 */
#define SPI SPI0
#define SPI_MODULE SPI0_MODULE
#define SPI_NUM 0
#elif (CASCODA_CHILI2_REV == -1) /* NuMaker-PFM-M2351 dev. board with arduino-style breakout */
#define SPI SPI1
#define SPI_MODULE SPI1_MODULE
#define SPI_NUM 1
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

#define SPI_RX_DMA_CH 2
#define SPI_TX_DMA_CH 3

#ifdef __cplusplus
}
#endif

#endif /* CASCODA_CHILI_H */
