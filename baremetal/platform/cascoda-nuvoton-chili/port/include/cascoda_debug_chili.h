/**
 * @file cascoda_debug_chili.h
 * @brief Board Support Package (BSP) Low-Level Debug (Over Air)\n
 *        Micro: Nuvoton Nano120 Chili Module
 * @author Wolfgang Bruchner
 * @date 18/07/17
 *//*
 * Copyright (C) 2017  Cascoda, Ltd.
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
#if defined(USE_DEBUG)

#include "cascoda-bm/cascoda_types.h"

#ifndef CASCODA_DEBUG_CHILI_H
#define CASCODA_DEBUG_CHILI_H

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
extern u8_t Debug_IRQ_State; //!<  IRQ State (wakeup, occasional or invalid)
extern u8_t Debug_BSP_Error; //!<  BSP Error Status

/******************************************************************************/
/****** Enumerations                                                     ******/
/******************************************************************************/
// Debug_IRQ_State
enum enIRQState
{
	DEBUG_IRQ_CLEAR               = 0x00, // no irq
	DEBUG_IRQ_WKUP_TIMER0         = 0x01, // valid wakeup
	DEBUG_IRQ_WKUP_TIMER1         = 0xD2, // should not happen
	DEBUG_IRQ_WKUP_TIMER2         = 0xD3, // should not happen
	DEBUG_IRQ_WKUP_TIMER3         = 0xD4, // should not happen
	DEBUG_IRQ_WKUP_HIRC           = 0xD5, // should not happen
	DEBUG_IRQ_WKUP_USBD           = 0xD6, // should not happen
	DEBUG_IRQ_WKUP_PA_SW2         = 0x07, // valid wakeup
	DEBUG_IRQ_WKUP_PA_RFIRQ       = 0x08, // valid wakeup
	DEBUG_IRQ_WKUP_PA_UNKNOWN     = 0xD9, // should not happen
	DEBUG_IRQ_WKUP_PBC_USBPRESENT = 0x0A, // valid wakeup
	DEBUG_IRQ_WKUP_PBC_UNKNOWN    = 0xDB, // should not happen
	DEBUG_IRQ_WKUP_PDWU           = 0xDC, // should not happen
	DEBUG_IRQ_WKUP_UART           = 0xDD, // should not happen
	DEBUG_IRQ_PA_SW2              = 0x0E, // valid
	DEBUG_IRQ_PBC_USBPRESENT      = 0x0F, // valid
	DEBUG_IRQ_TIMER3              = 0xE0, // should not happen
	DEBUG_IRQ_HARDFAULT           = 0xE1, // should not happen
	DEBUG_IRQ_HIRC                = 0xE2, // should not happen
	DEBUG_IRQ_PDWU                = 0xE3, // should not happen
	DEBUG_IRQ_BOD                 = 0xE4, // should not happen
	DEBUG_IRQ_WDT                 = 0xE5, // should not happen
	DEBUG_IRQ_EINT0               = 0xE6, // should not happen
	DEBUG_IRQ_EINT1               = 0xE7, // should not happen
	DEBUG_IRQ_GPDEF               = 0xE8, // should not happen
	DEBUG_IRQ_PWM0                = 0xE9, // should not happen
	DEBUG_IRQ_PWM1                = 0xEA, // should not happen
	DEBUG_IRQ_UART0               = 0xEB, // should not happen
	DEBUG_IRQ_SPI0                = 0xEC, // should not happen
	DEBUG_IRQ_SPI1                = 0xED, // should not happen
	DEBUG_IRQ_SPI2                = 0xEE, // should not happen
	DEBUG_IRQ_I2C0                = 0xEF, // should not happen
	DEBUG_IRQ_I2C1                = 0xF0, // should not happen
	DEBUG_IRQ_SC0                 = 0xF1, // should not happen
	DEBUG_IRQ_SC1                 = 0xF2, // should not happen
	DEBUG_IRQ_SC2                 = 0xF3, // should not happen
	DEBUG_IRQ_LCD                 = 0xF4, // should not happen
	DEBUG_IRQ_PDMA                = 0xF5, // should not happen
	DEBUG_IRQ_I2S                 = 0xF6, // should not happen
	DEBUG_IRQ_ADC                 = 0xF7, // should not happen
	DEBUG_IRQ_DAC                 = 0xF8, // should not happen
	DEBUG_IRQ_RTC                 = 0xF9  // should not happen
};

/* Debug_BSP_Error */
enum enBSPError
{
	DEBUG_ERR_CLEAR             = 0x00, // no error
	DEBUG_ERR_SYSTEMINIT        = 0xE1,
	DEBUG_ERR_CLOCKINIT_1       = 0xE2,
	DEBUG_ERR_CLOCKINIT_2       = 0xE3,
	DEBUG_ERR_CLOCKINIT_3       = 0xE4,
	DEBUG_ERR_CLOCKINIT_4       = 0xE5,
	DEBUG_ERR_CLOCKINIT_5       = 0xE6,
	DEBUG_ERR_CLOCKINIT_6       = 0xE7,
	DEBUG_ERR_CLOCKINIT_7       = 0xE8,
	DEBUG_ERR_CLOCKINIT_8       = 0xE9,
	DEBUG_ERR_CLOCKINIT_9       = 0xEA,
	DEBUG_ERR_CAX_EXTERNALCLOCK = 0xEB,
	DEBUG_ERR_CAX_POWERDOWN     = 0xEC,
	DEBUG_ERR_CAX_WAKEUP_1      = 0xED,
	DEBUG_ERR_CAX_WAKEUP_2      = 0xEE,
	DEBUG_ERR_UART1             = 0xEF,
	DEBUG_ERR_RESTART_1         = 0xF0,
	DEBUG_ERR_RESTART_2         = 0xF1,
	DEBUG_ERR_RESTART           = 0xAA
};

/******************************************************************************/
/****** Debug Functions                                                  ******/
/******************************************************************************/
void BSP_Debug_Reset(void);
void BSP_Debug_Error(u8_t code);

#endif // CASCODA_DEBUG_CHILI_H

#endif // USE_DEBUG
