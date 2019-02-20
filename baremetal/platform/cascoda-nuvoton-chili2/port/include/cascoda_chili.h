/**
 * @file cascoda_chili.h
 * @brief Board Support Package (BSP)\n
 *        Micro: Nuvoton M2351
 * @author Wolfgang Bruchner
 * @date 09/09/15
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
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
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda_chili_config.h"

#include "M2351.h"
#include "spi.h"
#include "sys.h"

#ifndef CASCODA_CHILI_H
#define CASCODA_CHILI_H

/******************************************************************************/
/****** Global Variables defined in cascoda_bsp_*.c                      ******/
/******************************************************************************/
extern volatile u8_t WDTimeout;  //!< Nonzero watchdog timeout if used
extern volatile u8_t USBPresent; //!< 0: USB not active, 1: USB is active

//! maximum count delay when switching clocks
#define MAX_CLOCK_SWITCH_DELAY 100000
//! SPI Master Clock Rate [Hz]
#define FCLK_SPI 2000000
//! Use Timer0 as watchdog for POWEROFF mode
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

#if defined(USE_UART)
void CHILI_UARTInit(void);
#endif
void CHILI_GPIOInit(void);
void CHILI_GPIOEnableInterrupts(void);
void CHILI_ClockInit(void);
void CHILI_CompleteClockInit(void);
void CHILI_TimersInit(void);
void CHILI_DisableIntOscCal(void);
void CHILI_EnableIntOscCal(void);
void CHILI_GPIOPowerDown(u8_t tristateCax);
void CHILI_GPIOPowerUp(void);
void CHILI_SystemReInit(void);
void CHILI_LEDBlink(void);
void CHILI_SetClockExternalCFGXT1(u8_t clk_external);

//PIN CONFIGURATION LIST FOR DIFFERENT BOARDS *******************************
//For port, 0=A, 1=B, 2=C etc.
#if (CASCODA_CHILI2_REV == 0)
//Chili 2.0
#define RSTB_PIN 1
#define RSTB_PORT_NUM 2
#define RFIRQ_PIN 0
#define RFIRQ_PORT_NUM 2
#define RFSS_PIN 3
#define RFSS_PORT_NUM 0
#define SPI_MODULE_NUM 0
#define SPI_MISO_PIN 1
#define SPI_MISO_PORT_NUM 0
#define SPI_MOSI_PIN 0
#define SPI_MOSI_PORT_NUM 0
#define SPI_CLK_PIN 2
#define SPI_CLK_PORT_NUM 0
//These need manual config at the moment.
static inline void SET_MFP_GPIO()
{
	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA0MFP_Msk)) | SYS_GPA_MFPL_PA0MFP_GPIO;
	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA1MFP_Msk)) | SYS_GPA_MFPL_PA1MFP_GPIO;
	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA2MFP_Msk)) | SYS_GPA_MFPL_PA2MFP_GPIO;
}
static inline void SET_MFP_SPI()
{
	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA0MFP_Msk)) | SYS_GPA_MFPL_PA0MFP_SPI0_MOSI;
	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA1MFP_Msk)) | SYS_GPA_MFPL_PA1MFP_SPI0_MISO;
	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA2MFP_Msk)) | SYS_GPA_MFPL_PA2MFP_SPI0_CLK;
}
#elif (CASCODA_CHILI2_REV == -1)
//M2351 Development board with arduino-style breakout
#define RSTB_PIN 10
#define RSTB_PORT_NUM 2
#define RFIRQ_PIN 9
#define RFIRQ_PORT_NUM 2
#define RFSS_PIN 9
#define RFSS_PORT_NUM 7
#define SPI_MODULE_NUM 1
#define SPI_MISO_PIN 1
#define SPI_MISO_PORT_NUM 4
#define SPI_MOSI_PIN 0
#define SPI_MOSI_PORT_NUM 4
#define SPI_CLK_PIN 8
#define SPI_CLK_PORT_NUM 7
//These need manual config at the moment.
static inline void SET_MFP_GPIO()
{
	SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE0MFP_Msk)) | SYS_GPE_MFPL_PE0MFP_GPIO;
	SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE1MFP_Msk)) | SYS_GPE_MFPL_PE1MFP_GPIO;
	SYS->GPH_MFPH = (SYS->GPH_MFPH & (~SYS_GPH_MFPH_PH8MFP_Msk)) | SYS_GPH_MFPH_PH8MFP_GPIO;
}
static inline void SET_MFP_SPI()
{
	SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE0MFP_Msk)) | SYS_GPE_MFPL_PE0MFP_SPI1_MOSI;
	SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE1MFP_Msk)) | SYS_GPE_MFPL_PE1MFP_SPI1_MISO;
	SYS->GPH_MFPH = (SYS->GPH_MFPH & (~SYS_GPH_MFPH_PH8MFP_Msk)) | SYS_GPH_MFPH_PH8MFP_SPI1_CLK;
}
#endif

//helper macros
#define CONCAT_X(a, b) CONCAT(a, b)
#define CONCAT(a, b) a##b
#define BITMASK(x) (1UL << x)
#define GPIO_PORT(x) ((GPIO_T *)(GPIO_BASE + (x * 0x40UL)))

//AUTOMATIC - edit the fields above for pin config
#define RSTB GPIO_PIN_DATA(RSTB_PORT_NUM, RSTB_PIN)
#define RFIRQ GPIO_PIN_DATA(RFIRQ_PORT_NUM, RFIRQ_PIN)
#define RFSS GPIO_PIN_DATA(RFSS_PORT_NUM, RFSS_PIN)
static GPIO_T *const       RSTB_PORT     = (GPIO_PORT(RSTB_PORT_NUM));
static GPIO_T *const       RFIRQ_PORT    = (GPIO_PORT(RFIRQ_PORT_NUM));
static GPIO_T *const       RFSS_PORT     = (GPIO_PORT(RFSS_PORT_NUM));
static const unsigned long SPI_MODULE    = CONCAT_X(SPI, CONCAT_X(SPI_MODULE_NUM, _MODULE));
static SPI_T *const        SPI           = CONCAT_X(SPI, SPI_MODULE_NUM);
static GPIO_T *const       SPI_MISO_PORT = (GPIO_PORT(SPI_MISO_PORT_NUM));
static GPIO_T *const       SPI_MOSI_PORT = (GPIO_PORT(SPI_MOSI_PORT_NUM));
static GPIO_T *const       SPI_CLK_PORT  = (GPIO_PORT(SPI_CLK_PORT_NUM));

#if RFIRQ_PORT_NUM != 2
#error "Warning, GPIO interrupt is not currently automatically set - needs to be manually changed from C"
#endif

#endif // CASCODA_CHILI_H
