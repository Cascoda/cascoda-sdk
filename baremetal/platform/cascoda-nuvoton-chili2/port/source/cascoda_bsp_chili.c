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
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
volatile u8_t     WDTimeout            = 0;
volatile u8_t     USBPresent           = 1;
volatile u8_t     UseExternalClock     = 0;
volatile fsys_mhz SystemFrequency      = DEFAULT_FSYS;
volatile u8_t     EnableCommsInterface = 1;

volatile u8_t asleep = 0;

struct device_link device_list[NUM_DEVICES];

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_WaitUs(u32_t us)
{
	volatile u32_t t1;
	volatile u32_t t2;

	/* Note this is dependent on the TIMER0 prescaling in CHILI_TimersInit() */
	/* as it is pre-scaled to 1 us and counting to 1 ms, max. wait time is 1000 us */
	if (us >= 1000)
		return;
	t1 = TIMER_GetCounter(TIMER0);
	do
	{
		t2 = TIMER_GetCounter(TIMER0);
		if (t1 > t2)
			t2 += 1000;
	} while (t2 - t1 < us);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_ResetRF(u8_t ms)
{
	u32_t ticks;

	ticks = (u32_t)ms;

	/* reset board with RSTB */
	ZIG_RESET_PVAL = 0; /* RSTB is LOW */
	if (ticks == 0)
		TIME_WaitTicks(10);
	else
		TIME_WaitTicks(ticks);
	ZIG_RESET_PVAL = 1;
	TIME_WaitTicks(50);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
u8_t BSP_SenseRFIRQ(void)
{
	return ZIG_IRQB_PVAL;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_DisableRFIRQ()
{
	/* note that this is temporarily disabling all GPIO C ports */
	NVIC_DisableIRQ(ZIG_IRQB_IRQn);
	__ASM volatile("" : : : "memory");
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_EnableRFIRQ()
{
	/* note that this is re-enabling all GPIO C ports */
	NVIC_EnableIRQ(ZIG_IRQB_IRQn);
	__ASM volatile("" : : : "memory");
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_SetRFSSBHigh(void)
{
	SPI_CS_PVAL = 1;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_SetRFSSBLow(void)
{
	SPI_CS_PVAL = 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_EnableSerialIRQ(void)
{
#if defined(USE_USB)
	NVIC_EnableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	NVIC_EnableIRQ(UART_IRQn);
#endif
	__ASM volatile("" : : : "memory");
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_USBSerialWrite(u8_t *pBuffer)
{
	int i = 0;
	if (BSP_IsUSBPresent())
	{
		while (!USB_Transmit(pBuffer))
		{
			TIME_WaitTicks(2);
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_SerialWriteAll(u8_t *pBuffer, u32_t BufferSize)
{
	if (BufferSize < UART_FIFOSIZE)
		CHILI_UARTFIFOWrite(pBuffer, BufferSize);
	else
		CHILI_UARTDMAWrite(pBuffer, BufferSize);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
u64_t BSP_GetUniqueId(void)
{
	u64_t rval = 0;
	u32_t extra_byte;

	SYS_UnlockReg();
	FMC_Open();
	extra_byte = FMC_ReadUID(2);
	rval       = FMC_ReadUID(0) ^ extra_byte;
	rval       = rval << 32ULL;
	rval |= FMC_ReadUID(1) ^ extra_byte;
	FMC_Close();
	SYS_LockReg();

	return rval;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
i32_t BSP_GetTemperature(void)
{
	u32_t adcval;
	i32_t tempval;

	/* enable temperature sensor */
	SYS_UnlockReg();
	SYS->IVSCTL |= SYS_IVSCTL_VTEMPEN_Msk;
	SYS_LockReg();

	/* Vref source is from 3.3V AVDD - note that other settings make no difference to ADC Output */
	/* ADC value for channel = 17 (temperature sensor), reference = AVDD 3.3V */
	adcval = CHILI_ADCConversion(17, SYS_VREFCTL_VREF_AVDD);

	/* disable temperature sensor */
	SYS_UnlockReg();
	SYS->IVSCTL &= ~SYS_IVSCTL_VTEMPEN_Msk;
	SYS_LockReg();

	/* Tenths of a degree celcius */
	/* Vadc         = adcval / 4096 * 3.3 = -1.82e-3 * T ['C] + 0.72 */
	/* => T ['C]    = (0.72 - N*3.3/4096)/1.82e-3 */
	/* => T ['C/10] = (0.72 - N*3.3/4096)/1.82e-4 */
	/*              = (894066 - N*1000)/226 */
	tempval = (894066 - adcval * 1000) / 226;

	return tempval;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_SPIInit(void)
{
	/* Unlock protected registers */
	SYS_UnlockReg();
	CLK_EnableModuleClock(SPI_MODULE);
	/* Lock protected registers */
	SYS_LockReg();

	/* SPI clock rate */
	SPI_Open(SPI, SPI_MASTER, SPI_MODE_2, 8, FCLK_SPI);

	/* Disable Auto-Slave-Select, SSB controlled by GPIO */
	SPI_DisableAutoSS(SPI);
	BSP_SetRFSSBHigh();

	SPI_SetFIFO(SPI, 7, 7);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
u8_t BSP_SPIExchangeByte(u8_t OutByte)
{
	u8_t InByte;
	while (!BSP_SPIPushByte(OutByte))
		;
	while (!BSP_SPIPopByte(&InByte))
		;
	return InByte;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
u8_t BSP_SPIPushByte(u8_t OutByte)
{
	if (!SPI_GET_TX_FIFO_FULL_FLAG(SPI) && (SPI_GET_RX_FIFO_COUNT(SPI) < 7))
	{
		SPI_WRITE_TX(SPI, OutByte);
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
u8_t BSP_SPIPopByte(u8_t *InByte)
{
	if (!SPI_GET_RX_FIFO_EMPTY_FLAG(SPI))
	{
		*InByte = SPI_READ_RX(SPI) & 0xFF;
		return 1;
	}
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0, u8_t dpd)
{
	u8_t lxt_connected = 0;
	u8_t use_watchdog  = 0;

	/* Note: GPIO wake-up only on falling edge, rising edge (IIE) does not perform wake-up contrary to datasheet */
	/*       This means that USBPresent cannot currently wake-up micro as it is active high */
	/* Wake-Up Conditions: */
	/* - Timeout Timer (Timer0 or CAX Sleep Timer) (if set) */
	/* - GPIO Interrupts (not disabled here) */

	BSP_DisableUSB();

	/* Disable GPIO - tristate the CAx if it is powered off */
	CHILI_GPIOPowerDown(!use_timer0);

	/* Disable HIRC calibration */
	CHILI_DisableIntOscCal();

	if (USE_WATCHDOG_POWEROFF && !use_timer0)
		use_watchdog = 1;

	/* use LF 32.768 kHz Crystal LXT for timer if present, switch off if not */
	if ((CLK->STATUS & CLK_STATUS_LXTSTB_Msk) && use_timer0)
	{
		lxt_connected = 1;
		/* Note: LIRC cannot be powered down as used for GPIOs */
	}
	else
	{
		lxt_connected = 0;
		SYS_UnlockReg();
		CLK->PWRCTL &= ~(CLK_PWRCTL_LXTEN_Msk);
		SYS_LockReg();
	}

	/* reconfig GPIO debounce clock to LIRC */
	GPIO_SET_DEBOUNCE_TIME(PA, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PC, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PD, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PE, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PF, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PG, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(PH, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);

	TIMER_Stop(TIMER0);
	SYS_ResetModule(TMR0_RST);
	TIMER_ResetCounter(TIMER0);

	if ((use_timer0 || use_watchdog) && (!dpd))
	{
		CLK_EnableModuleClock(TMR0_MODULE);
		if (use_watchdog)
			CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LIRC, 0);
		else if (lxt_connected)
			CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LXT, 0);
		else
			CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LIRC, 0);

		/*  1s period - 1Hz */
		TIMER_Open(TIMER0, TIMER_ONESHOT_MODE, 1);
		if (use_watchdog)
		{
			/* always uses 10 kHz LIRC clock, 10 kHz Clock: prescaler 9 gives 1 ms units */
			WDTimeout = 0;
			TIMER_SET_PRESCALE_VALUE(TIMER0, 9);
			TIMER_SET_CMP_VALUE(TIMER0, 2 * sleeptime_ms);
		}
		else if (lxt_connected)
		{
			if (sleeptime_ms < 1000)
			{
				/* 32.768 kHz Clock: prescaler 32 gives roughly 1 ms units */
				TIMER_SET_PRESCALE_VALUE(TIMER0, 32);
				TIMER_SET_CMP_VALUE(TIMER0, sleeptime_ms);
			}
			else
			{
				/* 32.768 kHz Clock: prescaler 255 gives 7.8125 ms units, so 128 is 1 sec */
				TIMER_SET_PRESCALE_VALUE(TIMER0, 255);
				TIMER_SET_CMP_VALUE(TIMER0, (128 * sleeptime_ms) / 1000);
			}
		}
		else
		{
			/*  10 kHz Clock: prescaler 9 gives 1 ms units */
			TIMER_SET_PRESCALE_VALUE(TIMER0, 9);
			TIMER_SET_CMP_VALUE(TIMER0, sleeptime_ms);
		}

		NVIC_EnableIRQ(TMR0_IRQn);
		TIMER_EnableInt(TIMER0);
		TIMER_EnableWakeup(TIMER0);
		TIMER_Start(TIMER0);
	}

	asleep = 1;
	SYS_UnlockReg();

	if (dpd)
	{
		CLK->PWRCTL &= ~(CLK_PWRCTL_HXTEN_Msk); /* turn off all clocks */
		CLK->PWRCTL &= ~(CLK_PWRCTL_LXTEN_Msk);
		CLK->PWRCTL &= ~(CLK_PWRCTL_HIRCEN_Msk);
		CLK->PWRCTL &= ~(CLK_PWRCTL_LIRCEN_Msk);
		CLK->PWRCTL &= ~(CLK_PWRCTL_HIRC48EN_Msk);
		CLK_EnableDPDWKPin(CLK_DPDWKPIN_FALLING);    /* Enable DPD wakeup from pin */
		CLK_SetPowerDownMode(CLK_PMUCTL_PDMSEL_DPD); /* DPD, no data retention, from reset on wake-up */
	}
	else
	{
		CLK_SetPowerDownMode(CLK_PMUCTL_PDMSEL_ULLPD); /* ULLPD */
	}

	CLK->PWRCTL |= CLK_PWRCTL_PDEN_Msk; /* Set power down bit */
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  /* Sleep Deep */

	__WFI(); /* really enter power down here !!! */
	__DSB();
	__ISB();

	SYS_LockReg();

	TIMER_Stop(TIMER0);
	SYS_ResetModule(TMR0_RST);

	GPIO_SET_DEBOUNCE_TIME(PA, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PC, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PD, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PE, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PF, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PG, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PH, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);

	asleep = 0;

	CHILI_TimersInit();
	CHILI_GPIOPowerUp();
	CHILI_SystemReInit();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
wakeup_reason BSP_GetWakeupReason(void)
{
	if (CLK_GetPMUWKSrc() & CLK_PMUSTS_PINWK_Msk)
	{
		CLK->PMUSTS &= ~CLK_PMUSTS_CLRWK_Msk;
		return WAKEUP_DEEP_POWERDOWN;
	}
	if (SYS_GetResetSrc() & SYS_RSTSTS_SYSRF_Msk)
	{
		SYS_ClearResetSrc(SYS_RSTSTS_SYSRF_Msk);
		return WAKEUP_SYSRESET;
	}
	return WAKEUP_POWERON;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_UseExternalClock(u8_t useExternalClock)
{
	if (UseExternalClock == useExternalClock)
		return;
	UseExternalClock = useExternalClock;

#ifdef USE_UART
	CHILI_UARTWaitWhileBusy();
#endif

	if (useExternalClock)
	{
		/* swap then disable */
		CHILI_SystemReInit();
		CHILI_EnableIntOscCal();
	}
	else
	{
		/* disable then swap */
		CHILI_DisableIntOscCal();
		CHILI_SystemReInit();
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_SystemReset()
{
	NVIC_SystemReset();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_Waiting(void)
{
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_Initialise(struct ca821x_dev *pDeviceRef)
{
	struct device_link *device;

	/* start up is on internal HIRC Oscillator, 12MHz, PLL disabled */

	/* initialise GPIOs */
	CHILI_GPIOInit();

	/* wait until system is stable after potential usb plug-in */
	while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk))
		;

	/* system  initialisation (clocks, timers ..) */
	CHILI_SystemReInit();

	/* configure HXT to external clock input */
	CHILI_SetClockExternalCFGXT1(1);

	/* enable GPIO interrupts */
	CHILI_GPIOEnableInterrupts();

	device                       = &device_list[0];
	device->chip_select_gpio     = &SPI_CS_PVAL;
	device->irq_gpio             = &ZIG_IRQB_PVAL;
	device->dev                  = pDeviceRef;
	pDeviceRef->exchange_context = device;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_WatchdogEnable(u32_t timeout_ms)
{
	/* uses TIMER3 instead of WDT for longer timeout intervals */
	if (timeout_ms < 0x00FFFFFF)
	{
		/* always uses 10 kHz LIRC clock */
		CLK_EnableModuleClock(TMR3_MODULE);
		CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_LIRC, 0);

		/* 10 kHz Clock: prescaler 9 gives 1 ms units */
		TIMER_Open(TIMER3, TIMER_PERIODIC_MODE, 1000000);
		TIMER_SET_PRESCALE_VALUE(TIMER3, 9);
		TIMER_SET_CMP_VALUE(TIMER3, timeout_ms);
		TIMER_EnableInt(TIMER3);
		TIMER_EnableWakeup(TIMER3);
		NVIC_EnableIRQ(TMR3_IRQn);
		TIME_WaitTicks(2);
		TIMER_Start(TIMER3);
		while (!TIMER_IS_ACTIVE(TIMER3))
			;
		WDTimeout = 0;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_WatchdogReset(u32_t timeout_ms)
{
	/* uses TIMER3 instead of WDT for longer timeout intervals */
	if (timeout_ms < 0x00FFFFFF)
	{
		SYS_ResetModule(TMR3_RST);
		BSP_WaitUs(500); /* wait additional 5 clock cycles to be sure */
		TIMER_Start(TIMER3);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
u8_t BSP_IsWatchdogTriggered(void)
{
	u8_t retval = WDTimeout;
	if (retval)
		WDTimeout = 0;
	return retval;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_WatchdogDisable(void)
{
	/* uses TIMER3 instead of WDT for longer timeout intervals */
	if (TIMER_IS_ACTIVE(TIMER3))
	{
		NVIC_DisableIRQ(TMR3_IRQn);
		TIMER3->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
		TIMER_Close(TIMER3);
		while (TIMER_IS_ACTIVE(TIMER3))
			;
		CLK_DisableModuleClock(TMR3_MODULE);
	}
	WDTimeout = 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_EnableUSB(void)
{
#if defined(USE_USB)
	if (USBPresent)
		return;
	USBPresent = 1;
	CHILI_SystemReInit();
#endif /* USE_USB */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_DisableUSB(void)
{
#if defined(USE_USB)
	if (!USBPresent)
		return;
	USBPresent = 0;
	CHILI_SystemReInit();
#endif /* USE_USB */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
u8_t BSP_IsUSBPresent(void)
{
	return USBPresent;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief See cascoda-bm/cascoda_interface.h
 *******************************************************************************
 ******************************************************************************/
void BSP_SystemConfig(fsys_mhz fsys, u8_t enable_comms)
{
	if (enable_comms)
	{
		/* check frequency settings */
#if defined(USE_USB)
		if (fsys == FSYS_4MHZ)
		{
			printf("SystemConfig Error: Minimum clock frequency for USB is 12MHz\n");
			return;
		}
#endif
#if defined(USE_UART)
		if ((fsys == FSYS_4MHZ) && (UART_BAUDRATE > 115200))
		{
			printf("SystemConfig Error: Minimum clock frequency for UART @%u is 12MHz\n", UART_BAUDRATE);
			return;
		}
		if ((fsys == FSYS_64MHZ) && (UART_BAUDRATE == 6000000))
		{
			printf("SystemConfig Error: UART @%u cannot be used at 64 MHz\n", UART_BAUDRATE);
			return;
		}
#endif
	}

#ifdef USE_UART
	CHILI_UARTWaitWhileBusy();
#endif

	SystemFrequency      = fsys;
	EnableCommsInterface = enable_comms;

	/* system  initialisation (clocks, timers ..) */
	CHILI_SystemReInit();
}
