/**
 * @file cascoda_bsp_chili.c
 * @brief Board Support Package (BSP)\n
 *        Micro: Nuvoton Nano120\n
 *        Board: Chili Module
 * @author Wolfgang Bruchner
 * @date 21/01/16
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

//System
#include <stdio.h>

//Platform
#include "Nano100Series.h"
#include "adc.h"
#include "gpio.h"
#include "spi.h"
#include "sys.h"
#include "timer.h"
#include "uart.h"
#include "usbd.h"

//Cascoda
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif
#ifdef USE_DEBUG
#include "cascoda_debug_chili.h"
#endif // USE_DEBUG

#define HID_TX_RETRIES (1000)

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
u8_t WDTimeout  = 0;
u8_t USBPresent = 0; //!< 0: USB not active, 1: USB is active

// LED RGB Values
u8_t  LED_R_VAL            = 0;   //!< Red
u8_t  LED_G_VAL            = 0;   //!< Green
u8_t  LED_B_VAL            = 0;   //!< Blue
u16_t LED_R_BlinkTimeCount = 0;   //!< Red Blink Time Interval Counter
u16_t LED_G_BlinkTimeCount = 0;   //!< Green Blink Time Interval Counter
u16_t LED_B_BlinkTimeCount = 0;   //!< Blue Blink Time Interval Counter
u16_t LED_R_OnTime         = 100; //!< Red On Time [10 ms]
u16_t LED_G_OnTime         = 100; //!< Green On Time [10 ms]
u16_t LED_B_OnTime         = 100; //!< Blue On Time [10 ms]
u16_t LED_R_OffTime        = 0;   //!< Red Off Time [10 ms]
u16_t LED_G_OffTime        = 0;   //!< Green Off Time [10 ms]
u16_t LED_B_OffTime        = 0;   //!< Blue Off Time [10 ms]

u8_t asleep = 0;

struct device_link device_list[NUM_DEVICES];

static u8_t sUseExternalClock = 0; //!< 0: Use internal clock, 1: Use clock from CA-821x

/***************************************************************************/ /**
 * \defgroup LEDSetClrs LED Set/Clear Functions
 ************************************************************************** @{*/
void LED_SetRED(u16_t ton, u16_t toff)
{
	LED_R_VAL     = 1;
	LED_R_OnTime  = ton;
	LED_R_OffTime = toff;
}
void LED_SetGREEN(u16_t ton, u16_t toff)
{
	LED_G_VAL     = 1;
	LED_G_OnTime  = ton;
	LED_G_OffTime = toff;
}
void LED_ClrRED(void)
{
	LED_R_VAL = 0;
}
void LED_ClrGREEN(void)
{
	LED_G_VAL = 0;
}
/**@}*/

void BSP_ShowTitle(u8_t *pString)
{
} // End of BSP_ShowTitle()

void BSP_ShowTime(void)
{
} // End of BSP_ShowTime()

void BSP_WaitUs(u32_t us)
{
	volatile u32_t t1;
	volatile u32_t t2;

	// Note this is dependent on the TIMER0 prescaling in CHILI_TimersInit()
	// as it is pre-scaled to 1 us and counting to 1 ms, max. wait time is 1000 us
	if (us >= 1000)
		return;
	t1 = TIMER0->DR;
	do
	{
		t2 = TIMER0->DR;
		if (t1 > t2)
			t2 += 1000;
	} while ((t2 - t1) < us);
}

void BSP_ResetRF(u8_t ms)
{
	u32_t ticks;

	ticks = (u32_t)ms;

	// reset board with RSTB
	PA11 = 0; // RSTB is LOW
	if (ticks == 0)
		TIME_WaitTicks(10);
	else
		TIME_WaitTicks(ticks);
	PA11 = 1;
	TIME_WaitTicks(50);
} // End of BSP_ResetRF()

u8_t BSP_SenseRFIRQ(void)
{
	return PA10;
} // End of BSP_SenseRFIRQ()

void BSP_DisableRFIRQ()
{
	// note that this is temporarily disabling all GPIO A/B/C
	NVIC_DisableIRQ(GPABC_IRQn);
	__ASM volatile("" : : : "memory");

} // End of BSP_DisableRFIRQ()

void BSP_EnableRFIRQ()
{
	// note that this is re-enabling all GPIO A/B/C
	NVIC_EnableIRQ(GPABC_IRQn);
	__ASM volatile("" : : : "memory");

} // End of BSP_EnableRFIRQ()

void BSP_SetRFSSBHigh(void)
{
	PB3 = 1;
} // End of BSP_SetRFSSBHigh()

void BSP_SetRFSSBLow(void)
{
	PB3 = 0;
} // End of BSP_SetRFSSBLow()

void BSP_EnableSerialIRQ(void)
{
#if defined(USE_USB)
	NVIC_EnableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	NVIC_EnableIRQ(UART1_IRQn);
#endif
	__ASM volatile("" : : : "memory");
}

void BSP_DisableSerialIRQ(void)
{
#if defined(USE_USB)
	NVIC_DisableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	NVIC_DisableIRQ(UART1_IRQn);
#endif
	__ASM volatile("" : : : "memory");
}

#if defined(USE_USB)
void BSP_SerialWrite(u8_t *pBuffer)
{
	int i = 0;
	if (BSP_IsUSBPresent())
	{
		while (!HID_Transmit(pBuffer))
		{
			if (i++ > HID_TX_RETRIES)
			{
				HID_SetConnectedFlag(0);
				break;
			}
		}
	}
} // End of BSP_SerialWrite()

u8_t BSP_SerialRead(u8_t *pBuffer)
{
	return HID_Receive(pBuffer);
} // End of BSP_SerialRead()

#endif // USE_USB

#if defined(USE_UART)

void BSP_SerialWriteAll(u8_t *pBuffer, u32_t BufferSize)
{
	UART_Write(UART1, pBuffer, BufferSize);
} // End of BSP_SerialWriteAll()

u32_t BSP_SerialRead(u8_t *pBuffer, u32_t BufferSize)
{
	u32_t numBytes = 0;

	// return(0);

	// Note that this is basically redefining the vendor BSP function UART_Read()
	// which cannot read a previously unknown number of bytes.

	while ((!(UART1->FSR & UART_FSR_RX_EMPTY_F_Msk)) && (numBytes < BufferSize)) // Rx FIFO empty ?
	{
		pBuffer[numBytes] = UART1->RBR;
		++numBytes;
	}

	return (numBytes);
} // End of BSP_SerialRead()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise UART for Comms
 *******************************************************************************
 ******************************************************************************/
void CHILI_UARTInit(void)
{
	CLK->APBCLK |= CLK_APBCLK_UART1_EN; // UART1 Clock Enable
	if (sUseExternalClock)
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART_S_Msk) | CLK_CLKSEL1_UART_S_HXT;
	else
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART_S_Msk) | CLK_CLKSEL1_UART_S_HIRC;

	// Initialise UART
	UART_SetLine_Config(UART1, 115200, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);
	UART_Open(UART1, 115200);
	UART_EnableInt(UART1, UART_IER_RDA_IE_Msk);
	NVIC_EnableIRQ(UART1_IRQn);

} // End of CHILI_UARTInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief De-Initialise UART
 *******************************************************************************
 ******************************************************************************/
void BSP_UARTDeinit(void)
{
	NVIC_DisableIRQ(UART1_IRQn);
	UART_DisableInt(UART1, UART_IER_RDA_IE_Msk);
	UART_Close(UART1);
} // End of BSP_UARTDeinit()

#endif // USE_UART

u8_t BSP_GetChargeStat(void)
{
	u8_t charging;

	// CHARGE_STAT Input, MCP73831 STAT Output
	// High (Hi-Z):  Charging Complete, Shutdown or no Battery present
	// Low:          Charging / Preconditioning

	if (PA12 == 0)
		charging = 1;
	else
		charging = 0;

	return (charging);
} // End of BSP_GetChargeStat()

i32_t BSP_GetTemperature(void)
{
	u32_t adcval;
	i32_t tval;

	// enable references
	SYS_UnlockReg();
	SYS->Int_VREFCTL = SYS_VREFCTL_BGP_EN | SYS_VREFCTL_REG_EN; // 1.8V reference
	SYS->TEMPCTL     = 1;
	SYS_LockReg();
	if (sUseExternalClock)
		CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HXT, CLK_ADC_CLK_DIVIDER(1));
	else
		CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HIRC, CLK_ADC_CLK_DIVIDER(1));

	CLK_EnableModuleClock(ADC_MODULE);
	// wait 50 ms for references to settle
	TIME_WaitTicks(50);

	ADC_Open(ADC, ADC_INPUT_MODE_SINGLE_END, ADC_OPERATION_MODE_SINGLE, ADC_CH_14_MASK);
	ADC_SET_REF_VOLTAGE(ADC, ADC_REFSEL_INT_VREF);
	ADC_SetExtraSampleTime(ADC, 14, 0);

	ADC_POWER_ON(ADC);
	ADC_START_CONV(ADC);

	while (ADC_IS_BUSY())
		;

	adcval = ADC_GET_CONVERSION_DATA(ADC, 14);

	// disable adc and references
	ADC_POWER_DOWN(ADC);
	ADC_Close(ADC);
	SYS_UnlockReg();
	SYS->Int_VREFCTL = 0;
	SYS->TEMPCTL     = 0;
	SYS_LockReg();

	//Tenths of a degree celcius
	// Vadc         = adcval / 4096 * 1.8 = -1.73e-3 * T ['C] + 0.74
	// => T ['C]    = (0.74 - N*1.8/4096)/1.73e-3
	// => T ['C/10] = (0.74 - N*1.8/4096)/1.73e-4
	//              = (671561 - N*400)/157
	tval = (671561 - (adcval * 400)) / 157;

	return tval;
} // End of BSP_GetTemperature()

u32_t BSP_ADCGetVolts(void)
{
	u32_t adcval;

	PB12 = 1; // VOLTS_TEST high

	if (sUseExternalClock)
		CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HXT, CLK_ADC_CLK_DIVIDER(1));
	else
		CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HIRC, CLK_ADC_CLK_DIVIDER(1));

	CLK_EnableModuleClock(ADC_MODULE);
	ADC_Open(ADC, ADC_INPUT_MODE_SINGLE_END, ADC_OPERATION_MODE_SINGLE, ADC_CH_0_MASK);
	ADC_SET_REF_VOLTAGE(ADC, ADC_REFSEL_POWER);
	/* TODO */
	// ADC_SetSampleTime(0, ADC_SMPLCNT_64CLK);

	ADC_POWER_ON(ADC);
	ADC_START_CONV(ADC);

	while (ADC_IS_BUSY())
		;

	adcval = ADC_GET_CONVERSION_DATA(ADC, 0);

	// disable adc and references
	ADC_POWER_DOWN(ADC);
	ADC_Close(ADC);

	PB12 = 0; // VOLTS_TEST low

	return (adcval);
} // End of BSP_ADCGetVolts()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief LED Blinking for TMR2_IRQHandler ISR
 *******************************************************************************
 ******************************************************************************/
void CHILI_LEDBlink(void)
{
	if ((LED_R_BlinkTimeCount == 0) && (LED_R_VAL == 1))
		PA14 = 0;
	else if ((LED_R_BlinkTimeCount > LED_R_OnTime) || (LED_R_VAL == 0))
		PA14 = 1;

	if ((LED_G_BlinkTimeCount == 0) && (LED_G_VAL == 1))
		PA13 = 0;
	else if ((LED_G_BlinkTimeCount > LED_G_OnTime) || (LED_G_VAL == 0))
		PA13 = 1;

	if ((LED_R_BlinkTimeCount >= LED_R_OnTime + LED_R_OffTime) || (LED_R_VAL == 0))
		LED_R_BlinkTimeCount = 0;
	else
		LED_R_BlinkTimeCount += 1;

	if ((LED_G_BlinkTimeCount >= LED_G_OnTime + LED_G_OffTime) || (LED_G_VAL == 0))
		LED_G_BlinkTimeCount = 0;
	else
		LED_G_BlinkTimeCount += 1;

} // End of CHILI_LEDBlink()

void BSP_LEDSigMode(u8_t mode)
{
	// Mode   LEDs     BlinkIntervall  Comments
	//
	// 0        (G)    On              Clear R; G as is
	// 1       R       On              Set R, Clear G
	// 2      (R)G     On              R as is, Set G
	// 3      (R)G     50/50           R as is, Set G Blinking 50/50
	// 4       R G     1/1000          Set R, Set G, Blinking 1/1000
	// 5         G     1/1000          Clear R, Set G, Blinking 1/1000
	// 6               On              Clear all (R, G)
	// 7        RG     100             R and G blinking alternatively 100/100
	// 8        RG     35              R and G blinking alternatively 35/35
	// 9        RG     On              All on
	//
	// Note that On/Off times are in [10ms]

	if (mode == 0)
	{
		LED_ClrRED();
	}
	else if (mode == 1)
	{
		LED_SetRED(1, 0);
		LED_ClrGREEN();
	}
	else if (mode == 2)
	{
		LED_SetGREEN(1, 0);
	}
	else if (mode == 3)
	{
		LED_SetGREEN(50, 50);
	}
	else if (mode == 4)
	{
		LED_SetRED(1, 1000);
		LED_SetGREEN(1, 1000);
	}
	else if (mode == 5)
	{
		LED_ClrRED();
		LED_SetGREEN(1, 1000);
	}
	else if (mode == 6)
	{
		LED_ClrRED();
		LED_ClrGREEN();
	}
	else if (mode == 7)
	{
		LED_SetRED(100, 100);
		TIME_WaitTicks(1000);
		LED_SetGREEN(100, 100);
	}
	else if (mode == 8)
	{
		LED_SetRED(35, 35);
		TIME_WaitTicks(350);
		LED_SetGREEN(35, 35);
	}
	else
	{
		LED_SetRED(1, 0);
		LED_SetGREEN(1, 0);
	}

} // End of BSP_LEDSigMode()

void BSP_SPIInit(void)
{
	// Unlock protected registers
	SYS_UnlockReg();

	CLK_EnableModuleClock(SPI1_MODULE);

	// Lock protected registers
	SYS_LockReg();

	// SPI clock rate
	SPI_Open(SPI1, SPI_MASTER, SPI_MODE_2, 8, FCLK_SPI);

	SPI_DisableAutoSS(SPI1);
	BSP_SetRFSSBHigh();

	SPI_EnableFIFO(SPI1, 7, 7);

} // End of BSP_SPIInit()

u8_t BSP_SPIExchangeByte(u8_t OutByte)
{
	u8_t InByte;
	while (!BSP_SPIPushByte(OutByte))
		;
	while (!BSP_SPIPopByte(&InByte))
		;
	return InByte;
}

u8_t BSP_SPIPushByte(u8_t OutByte)
{
	if (!SPI_GET_TX_FIFO_FULL_FLAG(SPI1) && (SPI_GET_RX_FIFO_COUNT(SPI1) < 7))
	{
		SPI_WRITE_TX0(SPI1, OutByte);
		return 1;
	}
	return 0;
}
u8_t BSP_SPIPopByte(u8_t *InByte)
{
	if (!SPI_GET_RX_FIFO_EMPTY_FLAG(SPI1))
	{
		*InByte = SPI_READ_RX0(SPI1) & 0xFF;
		return 1;
	}
	return 0;
}

void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0)
{
	u8_t lxt_connected = 0;
	u8_t use_watchdog  = 0;

	// Note: GPIO wake-up only on falling edge, rising edge (IIE) does not perform wake-up contrary to datasheet
	//       This means that USBPresent cannot currently wake-up Nano120 as it is active high

	// Wake-Up Conditions:
	//
	// - Timeout Timer (Timer0 or CAX Sleep Timer) (if set)
	// - Falling Edge USBPresent            (PB.15) (Rev. 1.2 or lower)
	//                                      (PC.7)  (Rev. 1.3 or higher)
	// - Falling Edge SW2                   (PA.9)
	// - CAX Wakeup / Falling Edge SPI IRQ  (PA.10)
	//

	BSP_DisableUSB();

	//Disable GPIO - tristate the CAx if it is powered off
	CHILI_GPIOPowerDown(!use_timer0);

	//Disable HIRC calibration
	CHILI_DisableIntOscCal();

	if (USE_WATCHDOG_POWEROFF && !use_timer0)
		use_watchdog = 1;

	// use LF 32.768 kHz Crystal LXT for timer if present, switch off if not
	if ((CLK->CLKSTATUS & CLK_CLKSTATUS_LXT_STB) && use_timer0)
	{
		lxt_connected = 1;
		// Note: LIRC cannot be powered down as used for GPIOs
	}
	else
	{
		lxt_connected = 0;
		SYS_UnlockReg();
		CLK->PWRCTL &= ~(CLK_PWRCTL_LXT_EN);
		SYS_LockReg();
	}

	// reconfig GPIO debounce clock to LIRC
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_IRC10K, GPIO_DBCLKSEL_1);

	// enable wake-up for USBPresent PB.15/PC.7
	GPIO_DisableInt(USBP_PORT, USBP_PIN);
	GPIO_EnableInt(USBP_PORT, USBP_PIN, GPIO_INT_RISING);
	// enable wake-up for SW2 PA.9
	GPIO_DisableInt(PA, 9);
	GPIO_EnableInt(PA, 9, GPIO_INT_FALLING);
	// enable wake-up for SPI IRQ PA.10
	GPIO_DisableInt(PA, 10);
	GPIO_EnableInt(PA, 10, GPIO_INT_FALLING);

	TIMER_Stop(TIMER2);
	TIMER2->CTL |= TIMER_CTL_SW_RST_Msk;
	TIMER_DisableInt(TIMER2);
	NVIC_DisableIRQ(TMR2_IRQn);
	LED_R_BlinkTimeCount = 0;
	LED_G_BlinkTimeCount = 0;
	LED_B_BlinkTimeCount = 0;

	TIMER_Stop(TIMER0);
	TIMER0->CTL |= TIMER_CTL_SW_RST_Msk;
	while (TIMER0->CTL & TIMER_CTL_SW_RST_Msk)
		;

	if (use_timer0 || use_watchdog)
	{
		//  1s period - 1Hz
		TIMER_Open(TIMER0, TIMER_ONESHOT_MODE, 1);
		TIMER_EnableInt(TIMER0);
		TIMER_EnableWakeup(TIMER0);
		if (use_watchdog)
		{
			// always uses 10 kHz LIRC clock, 10 kHz Clock: prescaler 9 gives 1 ms units
			WDTimeout = 0;
			TIMER_SET_PRESCALE_VALUE(TIMER0, 9);
			TIMER_SET_CMP_VALUE(TIMER0, 2 * sleeptime_ms);
		}
		else if (lxt_connected)
		{
			if (sleeptime_ms < 1000)
			{
				//  32.768 kHz Clock: prescaler 32 gives roughly 1 ms units
				TIMER_SET_PRESCALE_VALUE(TIMER0, 32);
				TIMER_SET_CMP_VALUE(TIMER0, sleeptime_ms);
			}
			else
			{
				//  32.768 kHz Clock: prescaler 255 gives 7.8125 ms units, so 128 is 1 sec
				TIMER_SET_PRESCALE_VALUE(TIMER0, 255);
				TIMER_SET_CMP_VALUE(TIMER0, (128 * sleeptime_ms) / 1000);
			}
		}
		else
		{
			//  10 kHz Clock: prescaler 9 gives 1 ms units
			TIMER_SET_PRESCALE_VALUE(TIMER0, 9);
			TIMER_SET_CMP_VALUE(TIMER0, sleeptime_ms);
		}
		TIMER0->CTL = TIMER_ONESHOT_MODE;
		NVIC_EnableIRQ(TMR0_IRQn);
		TIMER_EnableInt(TIMER0);
		TIMER_EnableWakeup(TIMER0);
		if (use_watchdog)
			CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_LIRC;
		else if (lxt_connected)
			CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_LXT;
		else
			CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_LIRC;
		TIMER_Start(TIMER0);
	}

	asleep = 1;
	SYS_UnlockReg();

	CLK->PWRCTL |= (CLK_PWRCTL_PWRDOWN_EN | CLK_PWRCTL_DELY_EN); /* Set power down bit */
	SCB->SCR |= 0x04;                                            /* Sleep Deep */

	__WFI(); /* really enter power down here !!! */
	__DSB();
	__ISB();

	SYS_LockReg();

	TIMER_Stop(TIMER0);
	TIMER0->CTL |= TIMER_CTL_SW_RST_Msk;

	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_HCLK, GPIO_DBCLKSEL_8);

	asleep = 0;

	// re-configure system for operation
	LED_R_BlinkTimeCount = 0;
	LED_G_BlinkTimeCount = 0;
	LED_B_BlinkTimeCount = 0;
	// GPIO_DisableInt(PA, 10);
	// GPIO_EnableInt(PA,  10, GPIO_INT_FALLING);
	// USE SWITCH PA.9 FOR APP_STATE
	GPIO_DisableInt(PA, 9);
	GPIO_EnableInt(PA, 9, GPIO_INT_FALLING);
	// USB PRESENT
	GPIO_DisableInt(USBP_PORT, USBP_PIN);
	GPIO_EnableInt(USBP_PORT, USBP_PIN, GPIO_INT_BOTH_EDGE);
	CHILI_TimersInit();

	CHILI_GPIOPowerUp();
	CHILI_SystemReInit();

} // End of BSP_PowerDown()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialise Essential GPIOs for various Functions
 *******************************************************************************
 * Note that pull-ups are configured for each used GPIO regardless of default.
 *
 * I/O List for Nano120 LQPF48
 *
 * Pin    | Default        | Chili              | Usage
 * -----: | :------------- | :----------------- | :-----------------------------
 *   1    | PB.12    IN    | PB.12       OUT    | VOLTS_TEST
 *   4    | PA.11    IN    | PA.11       OUT    | ZIG_RESET (CA8210 NRESET, P27)
 *   5    | PA.10    IN    | PA.10       IN     | ZIG_IRQB (CA8210 NIRQ, P20)
 *   6    | PA.9     IN    | PA.9        IN     | SWITCH
 *   7    | PA.8     IN    | PA.8        OUT    | BATT_ON (Rev. 1.2 or lower only)
 *   8    | PB.4     IN    | UART1_RX    IN     | Only when USE_UART
 *   9    | PB.5     IN    | UART1_TX    OUT    | Only when USE_UART
 *  17    | PB.0     IN    | SPI1_MOSI0  OUT    | SPI_MOSI (CA8210 MOSI, P23)
 *  18    | PB.1     IN    | SPI1_MISO0  IN     | SPI_MISO (CA8210 MISO, P24)
 *  19    | PB.2     IN    | SPI1_SCLK   OUT    | SPI_CLK (CA8210 SCLK, P21)
 *  20    | PB.3     IN    | PB.3        OUT    | SPI_CS (CA8210 SSB, P22)
 *  26    | PA.14    IN    | PA.14       OUT    | LED_R
 *  27    | PA.13    IN    | PA.13       OUT    | LED_G
 *  28    | PA.12    IN    | PA.12       IN     | CHARGE_STAT
 *  29    | ICE_DAT  IN    | ICE_DAT     I/O    | TMS
 *  30    | ICE_CLK  IN    | ICE_CLK     IN     | TCK
 *  32    | PA.0     IN    | AD0         IN     | VOLTS
 *  43    | PB.15    IN    | PB.15       IN     | USB_PRESENT (Rev. 1.2 or lower)
 *  41    | PC.7     IN    | PC.7        IN     | USB_PRESENT (Rev. 1.3 or higher)
 *  44    | XT1_IN   I/O   | XT1_IN      IN     | CLK
 *  45    | XT1_OUT  I/O   | XT1_OUT     I/O    | -
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOInit(void)
{
	// Initialise and set Debounce to 8 HCLK cycles
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_HCLK, GPIO_DBCLKSEL_8);

	// PB.12: VOLTS_TEST
	// output, no pull-up, set to 0
	GPIO_SetMode(PB, BIT12, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PB, BIT12);
	PB12 = 0; // VOLTS_TEST is active high, so switch off to avoid unecessary power consumption

	// PA.11: ZIG_RESET
	// output, no pull-up, set to 1
	GPIO_SetMode(PA, BIT11, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT11);
	PA11 = 1; // RSTB is HIGH

	// PA.10: ZIG_IRQB
	// input, pull-up
	GPIO_SetMode(PA, BIT10, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT10);

	// PA.9: SWITCH
	// input, no pull-up, debounced
	GPIO_SetMode(PA, BIT9, GPIO_PMD_INPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT9);
	GPIO_ENABLE_DEBOUNCE(PA, BIT9);

#if (CASCODA_CHILI_REV == 2)
	// PA.8: BATT_ON
	// output, no pull-up, set to 1
	GPIO_SetMode(PA, BIT8, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT8);
	PA8 = 1; // BATT_ON is active high, so switch on for charging
#endif

	// SPI1: MFP but SS0 controlled separately as GPIO
	// PB.0: SPI1_MOSI0
	// MFP output
	// PB.1: SPI1_MISO0
	// MFP input
	// PB.2: SPI1_SCLK
	// MFP output
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB0_MFP_Msk)) | SYS_PB_L_MFP_PB0_MFP_SPI1_MOSI0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB1_MFP_Msk)) | SYS_PB_L_MFP_PB1_MFP_SPI1_MISO0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB2_MFP_Msk)) | SYS_PB_L_MFP_PB2_MFP_SPI1_SCLK;
	GPIO_DISABLE_PULL_UP(PB, BIT0);
	GPIO_DISABLE_PULL_UP(PB, BIT1);
	GPIO_DISABLE_PULL_UP(PB, BIT2);

	// PB.3: SPI_CS
	// output, no pull-up, set to 1
	GPIO_SetMode(PB, BIT3, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PB, BIT3);
	PB3 = 1;

	// PA.14: LED_R
	// output, no pull-up, set to 1
	GPIO_SetMode(PA, BIT14, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT14);
	PA14 = 1;

	// PA.13: LED_G
	// output, no pull-up, set to 1
	GPIO_SetMode(PA, BIT13, GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT13);
	PA13 = 1;

	// PA.12: CHARGE_STAT
	// input, pull-up, debounced
	GPIO_SetMode(PA, BIT12, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT12); // CHARGE_STAT is low or tri-state
	GPIO_ENABLE_DEBOUNCE(PA, BIT12);

	// PF.0: ICE_DAT MFP
	GPIO_DISABLE_PULL_UP(PF, BIT0);

	// PF.1: ICE_CLK MFP
	GPIO_DISABLE_PULL_UP(PF, BIT1);

	// TODO: This is causing some current leakage when sleeping:
	// PA.0: AD0 MFP (VOLTS)
	// Comment out for better power saving:
	SYS->PA_L_MFP = (SYS->PA_L_MFP & (~SYS_PA_L_MFP_PA0_MFP_Msk)) | SYS_PA_L_MFP_PA0_MFP_ADC_CH0;
	GPIO_DISABLE_DIGITAL_PATH(PA, BIT0);
	//Uncomment for better power saving:
	//GPIO_SetMode(PA, BIT0, GPIO_PMD_INPUT);
	GPIO_DISABLE_PULL_UP(PA, BIT0);

	// PB.15/PC.7: USB_PRESENT
	// input, no pull-up, debounced
	GPIO_SetMode(USBP_PORT, USBP_PINMASK, GPIO_PMD_INPUT);
	GPIO_DISABLE_PULL_UP(USBP_PORT, USBP_PINMASK);
	GPIO_ENABLE_DEBOUNCE(USBP_PORT, USBP_PINMASK);
	// Immediately get USBPresent State
	USBPresent = USBP_GPIO;

#if (CASCODA_CHILI_REV == 3)
	// PB.15: PROGRAM Pin
	// enable pull-up just in case
	GPIO_SetMode(PB, BIT15, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PB, BIT15);
#endif

	// PF.3: XT1_IN MFP
	GPIO_DISABLE_PULL_UP(PF, BIT3);

	// PF.2: XT1_OUT MFP
	GPIO_DISABLE_PULL_UP(PF, BIT2);

// UART1 if used
// PB.4: UART1_RX
// MFP input
// PB.5: UART1_TX
// MFP output
#if defined(USE_UART)
	SYS->PB_L_MFP &= ~(SYS_PB_L_MFP_PB4_MFP_Msk | SYS_PB_L_MFP_PB5_MFP_Msk);
	SYS->PB_L_MFP |= (SYS_PB_L_MFP_PB4_MFP_UART1_RX | SYS_PB_L_MFP_PB5_MFP_UART1_TX);
#endif // USE_UART

} // End of CHILI_GPIOInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for PowerDown
 * \params tristateCax - bool: set to 1 to tri-state the CA-821x interface
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerDown(u8_t tristateCax)
{
	// PA.11: ZIG_RESET
	// tri-state - output to input
	GPIO_SetMode(PA, BIT11, GPIO_PMD_INPUT);

	// PA.10: ZIG_IRQB
	// disable pull-up
	GPIO_DISABLE_PULL_UP(PA, BIT10);

	if (tristateCax)
	{
		// SPI1:
		// PB.0: tri-state - output to input
		// PB.1: tri-state - input  to input
		// PB.2: tri-state - output to input
		SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB0_MFP_Msk)) | SYS_PB_L_MFP_PB0_MFP_GPB0;
		SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB1_MFP_Msk)) | SYS_PB_L_MFP_PB1_MFP_GPB1;
		SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB2_MFP_Msk)) | SYS_PB_L_MFP_PB2_MFP_GPB2;
		GPIO_SetMode(PB, BIT2, GPIO_PMD_INPUT);
		GPIO_SetMode(PB, BIT1, GPIO_PMD_INPUT);
		GPIO_SetMode(PB, BIT0, GPIO_PMD_INPUT);
	}

	// PB.3: SPI_CS
	// tri-state - output to input
	GPIO_SetMode(PB, BIT3, GPIO_PMD_INPUT);

	// PA.14: LED_R
	// tri-state - output to input - use pull-up otherwise light-dependent photodiode effect!
	PA14 = 1;
	GPIO_SetMode(PA, BIT14, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT14);

	// PA.13: LED_G
	// tri-state - output to input - use pull-up otherwise light-dependent photodiode effect!
	PA13 = 1;
	GPIO_SetMode(PA, BIT13, GPIO_PMD_INPUT);
	GPIO_ENABLE_PULL_UP(PA, BIT13);

	// PA.12: CHARGE_STAT
	// disable pull-up
	GPIO_DISABLE_PULL_UP(PA, BIT12);

} // End of CHILI_GPIOPowerDown()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for active Mode after PowerDown
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerUp(void)
{
	// PA.11: ZIG_RESET
	// input to output
	GPIO_SetMode(PA, BIT11, GPIO_PMD_OUTPUT);

	// PA.10: ZIG_IRQB
	// enable pull-up
	GPIO_ENABLE_PULL_UP(PA, BIT10);

	// SPI1:
	// PB.0: tri-state - input to SPI1_MOSI0
	// PB.1: tri-state - input to SPI1_MISO0
	// PB.2: tri-state - input to SPI1_SCLK
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB0_MFP_Msk)) | SYS_PB_L_MFP_PB0_MFP_SPI1_MOSI0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB1_MFP_Msk)) | SYS_PB_L_MFP_PB1_MFP_SPI1_MISO0;
	SYS->PB_L_MFP = (SYS->PB_L_MFP & (~SYS_PB_L_MFP_PB2_MFP_Msk)) | SYS_PB_L_MFP_PB2_MFP_SPI1_SCLK;

	// PB.3: SPI_CS
	// input to output
	GPIO_SetMode(PB, BIT3, GPIO_PMD_OUTPUT);

	// PA.14: LED_R
	// input to output
	GPIO_DISABLE_PULL_UP(PA, BIT14);
	GPIO_SetMode(PA, BIT14, GPIO_PMD_OUTPUT);

	// PA.13: LED_G
	// input to output
	GPIO_DISABLE_PULL_UP(PA, BIT13);
	GPIO_SetMode(PA, BIT13, GPIO_PMD_OUTPUT);

	// PA.12: CHARGE_STAT
	// enable pull-up
	GPIO_ENABLE_PULL_UP(PA, BIT12);

} // End of CHILI_GPIOPowerUp()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable GPIO Interrupts
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOEnableInterrupts(void)
{
	// USE SWITCH PA.9 FOR APP_COORDINATOR
	// GPIOA->ISR = 0x0000FFFF;
	GPIO_EnableInt(PA, 9, GPIO_INT_FALLING);

	//RFIRQ
	GPIO_EnableInt(PA, 10, GPIO_INT_FALLING);

	// PB.15/PC.7: USBPresent
	// USBP_PORT->ISR = 0x0000FFFF;
	GPIO_EnableInt(USBP_PORT, USBP_PIN, GPIO_INT_BOTH_EDGE);

	NVIC_EnableIRQ(GPABC_IRQn);

} // End of CHILI_GPIOEnableInterrupts()

u8_t BSP_GPIOSenseSwitch(void)
{
	return PA9;
} // End of BSP_GPIOSenseSwitch()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Select System Clocks depending on Power Source
 *******************************************************************************
 ******************************************************************************/
void CHILI_ClockInit(void)
{
	uint32_t delayCnt;

	// reset CLKSTATUS CLK_SW_FAIL
	/* Cannot reset CLK_SW_FAIL as CLKSTATUS is defined as read only! */
	// CLK->CLKSTATUS |= CLK_CLKSTATUS_CLK_SW_FAIL;

	if (sUseExternalClock) // 4 MHz clock supplied externally via HXT
	{
		// HXT enabled
		SYS_UnlockReg();
		CLK->PWRCTL &= ~(CLK_PWRCTL_HXT_SELXT); // switch off HXT feedback for external clock
		CLK->PWRCTL |= (CLK_PWRCTL_HXT_GAIN | CLK_PWRCTL_HXT_EN | CLK_PWRCTL_LXT_EN | CLK_PWRCTL_LIRC_EN);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->CLKSTATUS & CLK_CLKSTATUS_HXT_STB)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			sUseExternalClock = 0;
			BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
			BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_1);
#endif // USE_DEBUG
		}
	}

	if (!sUseExternalClock) // 12 MHz clock from HIRC
	{
		// HIRC enabled
		SYS_UnlockReg();
		CLK->PWRCTL |= (CLK_PWRCTL_HIRC_EN | CLK_PWRCTL_LXT_EN | CLK_PWRCTL_LIRC_EN);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->CLKSTATUS & CLK_CLKSTATUS_HIRC_STB)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
			BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_2);
#endif // USE_DEBUG
		}
	}

	if (USBPresent) // USB connected and supplies power - switch on PLL
	{
		if (sUseExternalClock) /* 4MHz HXT -> 48MHz PLL */
		{
			if (CLK_EnablePLL(CLK_PLLCTL_PLL_SRC_HXT, FREQ_48MHZ) != FREQ_48MHZ)
			{
				BSP_LEDSigMode(LED_M_SETERROR);
			}
		}
		else
		{
			if (CLK_EnablePLL(CLK_PLLCTL_PLL_SRC_HIRC, FREQ_48MHZ) != FREQ_48MHZ)
			{
				BSP_LEDSigMode(LED_M_SETERROR);
			}
		}
		/* TODO: Use CLK_WaitClockReady()? */
		for (delayCnt = 0; delayCnt < MAX_CLOCK_SWITCH_DELAY; delayCnt++)
		{
			if (CLK->CLKSTATUS & CLK_CLKSTATUS_PLL_STB)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
			if ((!USBPresent) && (!sUseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_3);
			else if ((!USBPresent) && (sUseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_4);
			else if ((USBPresent) && (!sUseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_5);
			else if ((USBPresent) && (sUseExternalClock))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_6);
			else
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_7);
#endif // USE_DEBUG
		}
	}

	if (USBPresent) // USB connected and supplies power - HCLK and USB from PLL
	{
		SYS_UnlockReg();
		CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_PLL, CLK_HCLK_CLK_DIVIDER(3)); // divider is 3  -> HCLK = 16 MHz
		/* Select IP clock source */
		CLK_SetModuleClock(USBD_MODULE, 0, CLK_USB_CLK_DIVIDER(1));
		/* Enable IP clock */
		CLK_EnableModuleClock(USBD_MODULE);
		SYS_LockReg();
	}
	else // USB unplugged, power from battery
	{
		if (sUseExternalClock) // 4 MHz clock supplied externally via HXT
		{
			SYS_UnlockReg();
			CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HXT, CLK_HCLK_CLK_DIVIDER(1));
			SYS_LockReg();
		}
		else // 12 MHz clock from HIRC
		{
			SYS_UnlockReg();
			CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_HCLK_CLK_DIVIDER(3));
			SYS_LockReg();
		}
		CLK_DisablePLL(); // PLL Off
	}

	// Set HCLK back to HIRC if clock switching error happens
	if (CLK->CLKSTATUS & CLK_CLKSTATUS_CLK_SW_FAIL)
	{
		SYS_UnlockReg();
		CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_HCLK_CLK_DIVIDER(1)); // HCLK = 12MHz
		SYS_LockReg();
		BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
		BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_8);
#endif // USE_DEBUG
	}

	// check clock frequency and signal problems
	SystemCoreClockUpdate();
	if (((USBPresent) && (sUseExternalClock) && (SystemCoreClock != 16000000)) ||
	    ((USBPresent) && (!sUseExternalClock) && (SystemCoreClock != 16000000)) ||
	    ((!USBPresent) && (sUseExternalClock) && (SystemCoreClock != 4000000)) ||
	    ((!USBPresent) && (!sUseExternalClock) && (SystemCoreClock != 4000000)))
	{
		BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
		BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_9);
#endif // USE_DEBUG
	}

} // End of CHILI_ClockInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Completes Clock (Re-)Initialisation.
 *******************************************************************************
 ******************************************************************************/
void CHILI_CompleteClockInit(void)
{
	if (!(CLK->CLKSTATUS & CLK_CLKSTATUS_CLK_SW_FAIL))
	{
		if (sUseExternalClock)
		{
			// disable HIRC
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HIRC_EN);
			SYS_LockReg();
		}
		else
		{
			// disable HXT
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HXT_EN);
			SYS_LockReg();
		}
	}

} // End of CHILI_CompleteClockInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_EnableIntOscCal(void)
{
	// enable HIRC trimming to 12 MHz if HIRC is used and LXT is present
	SYS->IRCTRIMINT = 0xFFFF;
	if ((!sUseExternalClock) && (CLK->CLKSTATUS & CLK_CLKSTATUS_LXT_STB))
	{
		// trim HIRC to 12MHz
		SYS->IRCTRIMCTL = (SYS_IRCTRIMCTL_TRIM_12M | SYS_IRCTRIMCTL_LOOP_32CLK | SYS_IRCTRIMCTL_RETRY_512);
		// enable HIRC-trim interrupt
		NVIC_EnableIRQ(HIRC_IRQn);
		SYS->IRCTRIMIEN = (SYS_IRCTRIMIEN_FAIL_EN | SYS_IRCTRIMIEN_32KERR_EN);
	}

} // End of CHILI_EnableIntOscCal()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_DisableIntOscCal(void)
{
	SYS->IRCTRIMINT = 0xFFFF;
	SYS->IRCTRIMIEN = 0;
	SYS->IRCTRIMCTL = 0;

} // End of CHILI_DisableIntOscCal()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief (Re-)Initialise System Timers
 *******************************************************************************
 ******************************************************************************/
void CHILI_TimersInit(void)
{
	// TIMER0: millisecond periodic tick (AbsoluteTicks) and power-down wake-up
	// TIMER1: microsecond timer / counter
	// TIMER2: 10 millisecond periodic tick for LED Blink
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HXT, 0);
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1_S_HXT, 0);
	CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL2_TMR2_S_HXT, 0);
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);
	CLK_EnableModuleClock(TMR2_MODULE);

	// configure clock selects and dividers for timers 0 and 1
	if (sUseExternalClock)
	{
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_HXT;
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR1_S_Msk) | CLK_CLKSEL1_TMR1_S_HXT;
		CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_TMR2_S_Msk) | CLK_CLKSEL2_TMR2_S_HXT;
		//  4 MHZ Clock: prescaler 3 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER0, 3);
		//  4 MHZ Clock: prescalar 3 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER1, 3);
		//  4 MHZ Clock: prescaler 79 gives 20 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER2, 79);
	}
	else
	{
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR0_S_Msk) | CLK_CLKSEL1_TMR0_S_HIRC;
		CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_TMR1_S_Msk) | CLK_CLKSEL1_TMR1_S_HIRC;
		CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_TMR2_S_Msk) | CLK_CLKSEL2_TMR2_S_HIRC;
		// 12 MHZ Clock: prescaler 11 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER0, 11);
		// 12 MHZ Clock: prescalar 11 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER1, 11);
		// 12 MHZ Clock: prescaler 239 gives 20 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER2, 239);
	}
	TIMER0->CTL = TIMER_PERIODIC_MODE;
	TIMER1->CTL = TIMER_CONTINUOUS_MODE;
	TIMER2->CTL = TIMER_PERIODIC_MODE;
	//  1uSec units, so 1000 is 1ms
	TIMER_SET_CMP_VALUE(TIMER0, 1000);
	//  1uSec units, counts microseconds
	TIMER_SET_CMP_VALUE(TIMER1, 0xFFFFFF);
	//  20uSec units, so 500 is 10ms
	TIMER_SET_CMP_VALUE(TIMER2, 500);

	NVIC_EnableIRQ(TMR0_IRQn);
	TIMER_EnableInt(TIMER0);
	TIMER_Start(TIMER0);
	NVIC_EnableIRQ(TMR2_IRQn);
	TIMER_EnableInt(TIMER2);
	TIMER_Start(TIMER2);

} // End of CHILI_TimersInit()

void BSP_UseExternalClock(u8_t useExternalClock)
{
	if (sUseExternalClock == useExternalClock)
		return;
	sUseExternalClock = useExternalClock;

	if (useExternalClock)
	{
		//swap then disable
		CHILI_SystemReInit();
		CHILI_EnableIntOscCal();
	}
	else
	{
		//disable then swap
		CHILI_DisableIntOscCal();
		CHILI_SystemReInit();
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Re-Initialisation
 *******************************************************************************
 ******************************************************************************/
void CHILI_SystemReInit()
{
	static u8_t USBPresent_d = 0;

// switch off USB comms if USBPResent has changed and disconnected
#if defined(USE_USB)
	if (!USBPresent && USBPresent_d)
	{
		USBD_DISABLE_USB();
		CLK_DisableModuleClock(USBD_MODULE);
		NVIC_DisableIRQ(USBD_IRQn);
	}
#endif // USE_USB
#if defined(USE_UART)
	BSP_UARTDeinit();
#endif // USE_UART

	// configure clocks
	CHILI_ClockInit();
	WDT_Close();

// switch on USB comms if USBPResent has changed and connected
#if defined(USE_USB)
	if (USBPresent && !USBPresent_d)
		USB_Initialise();
#endif // USE_USB
#if defined(USE_UART)
	CHILI_UARTInit();
#endif // USE_UART

	// initialise timers
	CHILI_TimersInit();

	// finish clock initialisation
	// this has to be done AFTER all other system config changes !
	CHILI_CompleteClockInit();

	// re-initialise SPI clock rate */
	if (SPI_SetBusClock(SPI1, FCLK_SPI) != FCLK_SPI)
	{
		BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
		BSP_Debug_Error(DEBUG_ERR_SYSTEMINIT);
#endif // USE_DEBUG
	}

	// set interrupt priorities
	// 0: highest	timers
	// 1:          downstream dispatch and gpios
	// 2:          upstream   dispatch
	// 3: lowest
	NVIC_SetPriority(TMR0_IRQn, 0);
	NVIC_SetPriority(TMR1_IRQn, 0);
	NVIC_SetPriority(TMR2_IRQn, 0);
	NVIC_SetPriority(TMR3_IRQn, 0);
	NVIC_SetPriority(USBD_IRQn, 2);
	NVIC_SetPriority(GPABC_IRQn, 1);
	NVIC_SetPriority(GPDEF_IRQn, 1);
	NVIC_SetPriority(UART1_IRQn, 2);

	USBPresent_d = USBPresent;

} // End of CHILI_SystemReInit()

void BSP_Initialise(struct ca821x_dev *pDeviceRef)
{
	u32_t               i;
	struct device_link *device;

	// start up is on internal HIRC Oscillator, 12MHz, PLL disabled

	// initialise GPIOs
	CHILI_GPIOInit();

	// wait until system is stable after potential usb plug-in
	while (!(CLK->CLKSTATUS & CLK_CLKSTATUS_HIRC_STB))
		;
	for (i = 0; i < 500000; ++i)
		;

	// system  initialisation (clocks, timers ..)
	CHILI_SystemReInit();

	// enable GPIO interrupts
	CHILI_GPIOEnableInterrupts();

	//Initialise flash config
	CHILI_FlashInit();

	device                       = &device_list[0];
	device->chip_select_gpio     = &PB3;
	device->irq_gpio             = &PA10;
	device->dev                  = pDeviceRef;
	pDeviceRef->exchange_context = device;
} // End of BSP_Initialise()

void BSP_WatchdogEnable(u32_t timeout_ms)
{
	// uses TIMER3 instead of WDT for longer timeout intervals
	if (timeout_ms < 0x00FFFFFF)
	{
		// 10 kHz Clock: prescaler 9 gives 1 ms units
		TIMER_SET_PRESCALE_VALUE(TIMER3, 9);
		TIMER_SET_CMP_VALUE(TIMER3, timeout_ms);
		TIMER3->CTL |= TIMER_PERIODIC_MODE;
		TIMER_EnableInt(TIMER3);
		TIMER_EnableWakeup(TIMER3);
		NVIC_EnableIRQ(TMR3_IRQn);
		// always uses 10 kHz LIRC clock
		CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL2_TMR3_S_LIRC, 0);
		CLK_EnableModuleClock(TMR3_MODULE);
		TIME_WaitTicks(2);
		TIMER_Start(TIMER3);
		while (!TIMER_IS_ACTIVE(TIMER3))
			;
		WDTimeout = 0;
	}
	printf("WD Started,,,\n");
} // End of BSP_WatchdogEnable()

void BSP_WatchdogReset(u32_t timeout_ms)
{
	// uses TIMER3 instead of WDT for longer timeout intervals
	if (timeout_ms < 0x00FFFFFF)
	{
		TIMER3->CTL |= TIMER_CTL_SW_RST_Msk;
		while (TIMER3->CTL & TIMER_CTL_SW_RST_Msk)
			;
		BSP_WaitUs(500); // wait a additional 5 clock cycles to be sure
		TIMER_Start(TIMER3);
	}
} // End of BSP_WatchdogReset()

u8_t BSP_IsWatchdogTriggered(void)
{
	u8_t retval = WDTimeout;
	if (retval)
		WDTimeout = 0;
	return retval;
}

void BSP_WatchdogDisable(void)
{
	// uses TIMER3 instead of WDT for longer timeout intervals
	if (TIMER_IS_ACTIVE(TIMER3))
	{
		NVIC_DisableIRQ(TMR3_IRQn);
		TIMER3->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;
		TIMER_Close(TIMER3);
		while (TIMER_IS_ACTIVE(TIMER3))
			;
		CLK_DisableModuleClock(TMR3_MODULE);
	}
	WDTimeout = 0;
} // End of BSP_WatchdogDisable()

void BSP_EnableUSB(void)
{
#if defined(USE_USB)
	if (USBPresent || !USBP_GPIO)
		return;

	USBPresent = 1;
	CHILI_SystemReInit();
#endif
}

void BSP_DisableUSB(void)
{
#if defined(USE_USB)
	if (!USBPresent)
		return;

	USBPresent = 0;
	CHILI_SystemReInit();
#endif
}

u8_t BSP_IsUSBPresent(void)
{
	return USBPresent;
}
