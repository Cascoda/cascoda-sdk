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
 * Internal (Non-Interface)
*/
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili_config.h"

#ifndef CASCODA_CHILI_H
#define CASCODA_CHILI_H

/******************************************************************************/
/****** Global Variables defined in cascoda_bsp_*.c                      ******/
/******************************************************************************/
extern volatile u8_t WDTimeout;        /* Nonzero watchdog timeout if used */
extern volatile u8_t USBPresent;       /* 0: USB not active, 1: USB is active */
extern volatile u8_t UseExternalClock; /* 0: Use internal clock, 1: Use clock from CA-821x */

/* maximum count delay when switching clocks */
#define MAX_CLOCK_SWITCH_DELAY 100000
/* SPI Master Clock Rate [Hz] */
#define FCLK_SPI 2000000
/* Use Timer0 as watchdog for POWEROFF mode */
#define USE_WATCHDOG_POWEROFF 1

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
void CHILI_LED_SetRED(u16_t ton, u16_t toff);
void CHILI_LED_SetGREEN(u16_t ton, u16_t toff);
void CHILI_LED_ClrRED(void);
void CHILI_LED_ClrGREEN(void);
#if defined(USE_UART)
void CHILI_UARTInit(void);
void CHILI_UARTDeinit(void);
#endif /* USE_UART */
u32_t CHILI_ADCConversion(u32_t channel, u32_t reference);
void  CHILI_LEDBlink(void);
void  CHILI_GPIOInit(void);
void  CHILI_GPIOEnableInterrupts(void);
void  CHILI_ClockInit(void);
void  CHILI_CompleteClockInit(void);
void  CHILI_EnableIntOscCal(void);
void  CHILI_DisableIntOscCal(void);
void  CHILI_GPIOPowerDown(u8_t tristateCax);
void  CHILI_GPIOPowerUp(void);
void  CHILI_TimersInit(void);
void  CHILI_SystemReInit(void);
void  CHILI_FlashInit(void);

/******************************************************************************/
/****** Version-specific pin-out changes                                 ******/
/****** Use CASCODA_CHILI_REV to specify Chili Rev. 1.x                  ******/
/******************************************************************************/
#if CASCODA_CHILI_REV == 2
#define USBP_PORT (PB) /* USB Present port */
#define USBP_PIN (15)  /* USB Present pin */
#define USBP_PINMASK (BIT15)
#define USBP_GPIO (PB15)
#define USBP_ISR (GPIO_ISR_ISR_15)
#define USBP_PMD_INPUT (GPIO_PMD_PMD15_INPUT)
#define USBP_PMD_MASK (GPIO_PMD_PMD15_MASK)
#define USBP_IER_IR (GPIO_IER_IR_EN_15)
#define USBP_IER_IF (GPIO_IER_IF_EN_15)
#define USBP_IMD_EDGE (GPIO_IMD_EDGE_15)
#elif CASCODA_CHILI_REV == 3
#define USBP_PORT (PC) /* USB Present port */
#define USBP_PIN (7)   /* USB Present pin */
#define USBP_PINMASK (BIT7)
#define USBP_GPIO (PC7)
#define USBP_ISR (GPIO_ISR_ISR_7)
#define USBP_PMD_INPUT (GPIO_PMD_PMD7_INPUT)
#define USBP_PMD_MASK (GPIO_PMD_PMD7_MASK)
#define USBP_IER_IR (GPIO_IER_IR_EN_7)
#define USBP_IER_IF (GPIO_IER_IF_EN_7)
#define USBP_IMD_EDGE (GPIO_IMD_EDGE_7)
#else
#error "Unsupported Chili Revision"
#endif

#endif /* CASCODA_CHILI_H */
