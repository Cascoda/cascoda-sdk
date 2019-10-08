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
 * MCU:    Nuvoton M2351
 * MODULE: Chili 2.0 (and NuMaker-PFM-M2351 Development Board)
 * Interface Functions, see cascoda-bm/cascoda_interface.h for descriptions
*/
/* System */
#include <stdio.h>
/* Platform */
#include "M2351.h"
#include "eadc.h"
#include "fmc.h"
#include "gpio.h"
#include "spi.h"
#include "sys.h"
#include "timer.h"
#include "uart.h"
#include "usbd.h"
/* Cascoda */
#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#include "cascoda_secure.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
volatile u8_t USBPresent = 1;

struct device_link device_list[NUM_DEVICES];

/* The pointer to the correct (secure or non-secure) CRPT module for use by MBEDTLS is determined at
 *link-time. This declaration * is how the symbol gets passed along. The alternative to this is moving
 * MBEDTLS higher up in the dependency chain, near the BSP, or finding a way to inject the partition
 * file from the secure binaries into the library.
 */
CRPT_T *CRPT_dyn = CRPT;


/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0, u8_t dpd, struct ca821x_dev *pDeviceRef)
{
	u32_t timeout_sec;

	/* Note: GPIO wake-up only on falling edge, rising edge (IIE) does not perform wake-up contrary to datasheet */
	/*       This means that USBPresent cannot currently wake-up micro as it is active high */
	/* Wake-Up Conditions: */
	/* - Timeout Timer (Timer0 or CAX Sleep Timer) (if set) */
	/* - GPIO Interrupts (not disabled here) */

	if (USE_WATCHDOG_POWEROFF)
	{
		/* set RTC timeout alarm as watchdog */
		if (sleeptime_ms > 10000)
			timeout_sec = (6 * sleeptime_ms) / 5000; /* > 10 seconds -> x 1.2 */
		else if (sleeptime_ms > 1000)
			timeout_sec = (2 * sleeptime_ms) / 1000; /* >  1 seconds -> x 2.0 */
		else
			timeout_sec = 2; /* min. 2 seconds */
		if (BSP_RTCSetAlarmSeconds(timeout_sec))
		{
			return;
		}
	}

	BSP_DisableUSB();
#if defined(USE_UART)
	CHILI_UARTDeinit();
#endif /* USE_UART */

	/* Disable GPIO - tristate the CAx if it is powered off */
	CHILI_GPIOPowerDown(!use_timer0);

	/* Disable HIRC calibration */
	CHILI_DisableIntOscCal();

	/* use LF 32.768 kHz Crystal LXT for timer if present, switch off if not */
	CHILI_PowerDownSelectClock(use_timer0);

	/* reconfig GPIO debounce clock to LIRC */
	GPIO_SET_DEBOUNCE_TIME(PA, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PC, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PD, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PE, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PF, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PG, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PH, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);

	/* set before TIMER0 is disabled */
	CHILI_SetAsleep(1);

	CHILI_PowerDownSecure(sleeptime_ms, use_timer0, dpd);

	CHILI_GPIOPowerUp();
	CHILI_SystemReInit();

	/* reset after TIMER0 is re-enabled */
	CHILI_SetAsleep(0);
	CHILI_FastForward(sleeptime_ms);

	/* read downstream message that has woken up device */
	if (!use_timer0)
		device_list[0].dispatch_read(pDeviceRef);
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_ResetRF(u8_t ms)
{
	u32_t ticks;

	ticks = (u32_t)ms;

	/* reset board with RSTB */
	ZIG_RESET_PVAL = 0; /* RSTB is LOW */
	if (ticks == 0)
		BSP_WaitTicks(10);
	else
		BSP_WaitTicks(ticks);
	ZIG_RESET_PVAL = 1;
	BSP_WaitTicks(50);
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
u8_t BSP_SenseRFIRQ(void)
{
	return ZIG_IRQB_PVAL;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_DisableRFIRQ()
{
	/* note that this is temporarily disabling all GPIO C ports */
	NVIC_DisableIRQ(ZIG_IRQB_IRQn);
	__ASM volatile("" : : : "memory");
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_EnableRFIRQ()
{
	/* note that this is re-enabling all GPIO C ports */
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
	TZ_NVIC_EnableIRQ_NS(ZIG_IRQB_IRQn);
#endif
	NVIC_EnableIRQ(ZIG_IRQB_IRQn);
	__ASM volatile("" : : : "memory");
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_SetRFSSBHigh(void)
{
	SPI_CS_PVAL = 1;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_SetRFSSBLow(void)
{
	SPI_CS_PVAL = 0;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_EnableSerialIRQ(void)
{
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#if defined(USE_USB)
	TZ_NVIC_EnableIRQ_NS(USBD_IRQn);
	NVIC_EnableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	TZ_NVIC_EnableIRQ_NS(UART_IRQn);
	NVIC_EnableIRQ(UART_IRQn);
#endif
#else
#if defined(USE_USB)
	NVIC_EnableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	NVIC_EnableIRQ(UART_IRQn);
#endif
#endif
	__ASM volatile("" : : : "memory");
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_DisableSerialIRQ(void)
{
#if defined(USE_USB)
	NVIC_DisableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	NVIC_DisableIRQ(UART_IRQn);
#endif
	__ASM volatile("" : : : "memory");
}

#if defined(USE_USB)

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_USBSerialWrite(u8_t *pBuffer)
{
	int i = 0;
	if (BSP_IsUSBPresent())
	{
		while (!USB_Transmit(pBuffer))
		{
			BSP_WaitTicks(2);
			if (i++ > USB_TX_RETRIES)
			{
				USB_SetConnectedFlag(0);
				break;
			}
		}
	}
}

#endif /* USE_USB */

#if defined(USE_UART)

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_SerialWriteAll(u8_t *pBuffer, u32_t BufferSize)
{
	if (BufferSize < UART_FIFOSIZE)
		CHILI_UARTFIFOWrite(pBuffer, BufferSize);
	else
		CHILI_UARTDMAWrite(pBuffer, BufferSize);
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
u32_t BSP_SerialRead(u8_t *pBuffer, u32_t BufferSize)
{
	u32_t numBytes = 0;
	if (BufferSize > 1)
		CHILI_UARTDMASetupRead(pBuffer, BufferSize);
	else
		numBytes = CHILI_UARTFIFORead(pBuffer, BufferSize);
	return (numBytes);
}

#endif /* USE_UART */

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
i32_t BSP_GetTemperature(void)
{
	u32_t adcval;
	i32_t tempval;

	/* enable temperature sensor */
	CHILI_EnableTemperatureSensor();

	/* Vref source is from 3.3V AVDD - note that other settings make no difference to ADC Output */
	/* ADC value for channel = 17 (temperature sensor), reference = AVDD 3.3V */
	adcval = CHILI_ADCConversion(17, SYS_VREFCTL_VREF_AVDD);

	/* disable temperature sensor */
	CHILI_DisableTemperatureSensor();

	/* Tenths of a degree celcius */
	/* Vadc         = adcval / 4096 * 3.3 = -1.82e-3 * T ['C] + 0.72 */
	/* => T ['C]    = (0.72 - N*3.3/4096)/1.82e-3 */
	/* => T ['C/10] = (0.72 - N*3.3/4096)/1.82e-4 */
	/*              = (894066 - N*1000)/226 */
	tempval = (894066 - adcval * 1000) / 226;

	return tempval;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
#if (CASCODA_CHILI2_CONFIG == 1)
u8_t BSP_GetChargeStat(void)
{
	u8_t charging;

	/* CHARGE_STAT Input, MCP73831 STAT Output */
	/* High (Hi-Z):  Charging Complete, Shutdown or no Battery present */
	/* Low:          Charging / Preconditioning */

	if (CHARGE_STAT_PVAL == 0)
		charging = 1;
	else
		charging = 0;

	return (charging);
}
#else
u8_t BSP_GetChargeStat(void)
{
	return (0);
}
#endif

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
#if (CASCODA_CHILI2_CONFIG == 1)
u32_t BSP_ADCGetVolts(void)
{
	u32_t voltsval;

	CHILI_ModuleSetMFP(VOLTS_PNUM, VOLTS_PIN, PMFP_ADC);
	GPIO_DISABLE_DIGITAL_PATH(VOLTS_PORT, BITMASK(VOLTS_PIN));
	GPIO_SetPullCtl(VOLTS_PORT, BITMASK(VOLTS_PIN), GPIO_PUSEL_DISABLE);

	VOLTS_TEST_PVAL = 1; /* VOLTS_TEST high */

	/* ADC value for channel = 1, reference = AVDD 3.3V */
	voltsval = CHILI_ADCConversion(1, SYS_VREFCTL_VREF_AVDD);

	VOLTS_TEST_PVAL = 0; /* VOLTS_TEST low */

	CHILI_ModuleSetMFP(VOLTS_PNUM, VOLTS_PIN, PMFP_GPIO);
	GPIO_ENABLE_DIGITAL_PATH(VOLTS_PORT, BITMASK(VOLTS_PIN));
	GPIO_SetMode(VOLTS_PORT, BITMASK(VOLTS_PIN), GPIO_MODE_INPUT);

	return (voltsval);
}
#else
u32_t BSP_ADCGetVolts(void)
{
	return (0);
}
#endif

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_SPIInit(void)
{
	CHILI_EnableSpiModuleClock();
	/* SPI clock rate */
	SPI_Open(SPI, SPI_MASTER, SPI_MODE_2, 8, FCLK_SPI);

	/* Disable Auto-Slave-Select, SSB controlled by GPIO */
	SPI_DisableAutoSS(SPI);
	BSP_SetRFSSBHigh();

	SPI_SetFIFO(SPI, 7, 7);
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
u8_t BSP_SPIExchangeByte(u8_t OutByte)
{
	u8_t InByte;
	while (!BSP_SPIPushByte(OutByte))
		;
	while (!BSP_SPIPopByte(&InByte))
		;
	return InByte;
}

/**
 * Get the number of bytes in the tx fifo.
 * For some reason this isn't in the nuvoton vendorcode.
 */
static inline uint8_t getTxFifoCount()
{
	return ((SPI->STATUS & SPI_STATUS_TXCNT_Msk) >> SPI_STATUS_TXCNT_Pos) & 0x0f;
}

static uint8_t fifo_imbalance = 0;
/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
u8_t BSP_SPIPushByte(u8_t OutByte)
{
	if (!SPI_GET_TX_FIFO_FULL_FLAG(SPI) && (fifo_imbalance < 7))
	{
		fifo_imbalance++;
		SPI_WRITE_TX(SPI, OutByte);
		return 1;
	}
	return 0;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
u8_t BSP_SPIPopByte(u8_t *InByte)
{
	if (!SPI_GET_RX_FIFO_EMPTY_FLAG(SPI))
	{
		fifo_imbalance--;
		*InByte = SPI_READ_RX(SPI) & 0xFF;
		return 1;
	}
	return 0;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_SystemReset()
{
	NVIC_SystemReset();
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_Initialise(struct ca821x_dev *pDeviceRef, dispatch_read_t pDispatchReadCallback)
{
	struct device_link *device;

	/* start up is on internal HIRC Oscillator, 12MHz, PLL disabled */

	/* initialise GPIOs */
	CHILI_GPIOInit();

	CHILI_WaitForSystemStable();

	/* system  initialisation (clocks, timers ..) */
	CHILI_SystemReInit();
	cascoda_isr_chili_init();

	/* configure HXT to external clock input */
	CHILI_SetClockExternalCFGXT1(1);

	/* enable GPIO interrupts */
	CHILI_GPIOEnableInterrupts();

	if (USE_WATCHDOG_POWEROFF)
		BSP_RTCInitialise();

	device                       = &device_list[0];
	device->chip_select_gpio     = &SPI_CS_PVAL;
	device->irq_gpio             = &ZIG_IRQB_PVAL;
	device->dispatch_read        = pDispatchReadCallback;
	device->dev                  = pDeviceRef;
	pDeviceRef->exchange_context = device;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_EnableUSB(void)
{
#if defined(USE_USB)
	if (USBPresent)
		return;
	USBPresent = 1;
	CHILI_SystemReInit();
#endif /* USE_USB */
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
void BSP_DisableUSB(void)
{
#if defined(USE_USB)
	if (!USBPresent)
		return;
	USBPresent = 0;
	CHILI_SystemReInit();
#endif /* USE_USB */
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
u8_t BSP_IsUSBPresent(void)
{
	return USBPresent;
}
