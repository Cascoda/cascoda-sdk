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
#endif /* USE_UART */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Perform ADC Conversion
 *******************************************************************************
 ******************************************************************************/
u32_t CHILI_ADCConversion(u32_t channel, u32_t reference);

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
/***************************************************************************/ /**
 * \brief Select System Clocks depending on Power Source
 *******************************************************************************
 ******************************************************************************/
ca_error CHILI_ClockInit(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Clock (Re-)Initialisation.
 *******************************************************************************
 ******************************************************************************/
void CHILI_CompleteClockInit(void);

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
 * \brief System Re-Initialisation
 *******************************************************************************
 ******************************************************************************/
void CHILI_SystemReInit(void);
void CHILI_FlashInit(void);
void CHILI_1msTick(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief FastForward time by the given amount
 *******************************************************************************
 * \param ticks - Time in Ticks (1ms/100ms)
 *******************************************************************************
 ******************************************************************************/
void CHILI_FastForward(u32_t ticks);

#endif /* CASCODA_CHILI_H */
