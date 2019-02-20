/**
 * @file cascoda_bsp_chili.c
 * @brief Board Support Package (BSP)\n
 *        Micro: Nuvoton M2351\n
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
#include "M2351.h"
#include "eadc.h"
#include "fmc.h"
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
volatile u8_t WDTimeout  = 0;
volatile u8_t USBPresent = 1; //!< 0: USB not active, 1: USB is active

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

volatile u8_t asleep = 0;

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
	u32_t t1, t2;

	// Note this is dependent on the TIMER0 prescaling in CHILI_TimersInit()
	// as it is pre-scaled to 1 us and counting to 1 ms, max. wait time is 1000 us
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

void BSP_ResetRF(u8_t ms)
{
	u32_t ticks;

	ticks = (u32_t)ms;

	// reset board with RSTB
	RSTB = 0; // RSTB is LOW
	if (ticks == 0)
		TIME_WaitTicks(10);
	else
		TIME_WaitTicks(ticks);
	RSTB = 1;
	TIME_WaitTicks(50);
} // End of BSP_ResetRF()

u8_t BSP_SenseRFIRQ(void)
{
	return RFIRQ;
} // End of BSP_SenseRFIRQ()

void BSP_DisableRFIRQ()
{
	// note that this is temporarily disabling all GPIO C
	NVIC_DisableIRQ(GPC_IRQn);
	__ASM volatile("" : : : "memory");

} // End of BSP_DisableRFIRQ()

void BSP_EnableRFIRQ()
{
	// note that this is re-enabling all GPIO C
	NVIC_EnableIRQ(GPC_IRQn);
	__ASM volatile("" : : : "memory");

} // End of BSP_EnableRFIRQ()

void BSP_SetRFSSBHigh(void)
{
	RFSS = 1;
} // End of BSP_SetRFSSBHigh()

void BSP_SetRFSSBLow(void)
{
	RFSS = 0;
} // End of BSP_SetRFSSBLow()

void BSP_EnableSerialIRQ(void)
{
#if defined(USE_USB)
	NVIC_EnableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	NVIC_EnableIRQ(UART0_IRQn);
#endif
	__ASM volatile("" : : : "memory");
}

void BSP_DisableSerialIRQ(void)
{
#if defined(USE_USB)
	NVIC_DisableIRQ(USBD_IRQn);
#elif defined(USE_UART)
	NVIC_DisableIRQ(UART0_IRQn);
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
	UART_Write(UART0, pBuffer, BufferSize);
} // End of BSP_SerialWriteAll()

u32_t BSP_SerialRead(u8_t *pBuffer, u32_t BufferSize)
{
	u32_t numBytes = 0;

	// return(0);

	// Note that this is basically redefining the vendor BSP function UART_Read()
	// which cannot read a previously unknown number of bytes.

	while ((!(UART0->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk)) && (numBytes < BufferSize)) // Rx FIFO empty ?
	{
		pBuffer[numBytes] = UART0->DAT;
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
	/* Set multi-function pins for UART0 RXD and TXD */
	SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | UART0_RXD_PB12 | UART0_TXD_PB13;

	if (CLKExternal)
		CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
	else
		CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(3));
	CLK_EnableModuleClock(UART0_MODULE);

	// Initialise UART
	SYS_ResetModule(UART0_RST);
	UART_SetLineConfig(UART0, 115200, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);
	UART_Open(UART0, 115200);
	UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk);
	NVIC_EnableIRQ(UART0_IRQn);
} // End of BSP_UARTInit()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief De-Initialise UART
 *******************************************************************************
 ******************************************************************************/
void BSP_UARTDeinit(void)
{
	NVIC_DisableIRQ(UART0_IRQn);
	UART_DisableInt(UART0, UART_INTEN_RDAIEN_Msk);
	UART_Close(UART0);
} // End of BSP_UARTDeinit()

#endif // USE_UART

u8_t BSP_GetChargeStat(void)
{
	u8_t charging;

	// CHARGE_STAT Input, MCP73831 STAT Output
	// High (Hi-Z):  Charging Complete, Shutdown or no Battery present
	// Low:          Charging / Preconditioning

	if (PB0 == 0)
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
	/* Enable temperature sensor */
	SYS->IVSCTL |= SYS_IVSCTL_VTEMPEN_Msk;
	/* Vref source is from 3.3V AVDD - note that other settings make no difference to ADC Output */
	SYS->VREFCTL = (SYS->VREFCTL & ~ SYS_VREFCTL_VREFCTL_Msk) | SYS_VREFCTL_VREF_AVDD;
	SYS_LockReg();

	/* Enable EADC module clock */
	CLK_EnableModuleClock(EADC_MODULE);
	/* EADC clock source is 16MHz, set divider to 16 -> 1 MHz */
	CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(16));

	/* Reset EADC module */
	SYS_ResetModule(EADC_RST);

	// wait 50 ms for references to settle
	TIME_WaitTicks(50);

	/* Set input mode as single-end and enable the A/D converter */
	EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);

	/* Set sample module 17 external sampling time to 0xFF */
	EADC_SetExtendSampleTime(EADC, 17, 0xFF);

	/* Clear the A/D ADINT0 interrupt flag for safe */
	EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);

	/* Enable the sample module 17 interrupt.  */
	EADC_ENABLE_INT(EADC, BIT0); //Enable sample module A/D ADINT0 interrupt.
	EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 0, BIT17);
	NVIC_EnableIRQ(EADC0_IRQn);

	/* Reset the ADC interrupt indicator and trigger sample module 17 to start A/D conversion */
	EADC_START_CONV(EADC, BIT17);

	/* Wait until EADC conversion is done */
	while (!EADC_GET_INT_FLAG(EADC, EADC_STATUS2_AVALID_Msk)) /* Disable the ADINT0 interrupt */
		EADC_DISABLE_INT(EADC, BIT0);
	NVIC_DisableIRQ(EADC0_IRQn);

	/* Get the conversion result of the sample module 0 */
	adcval = EADC_GET_CONV_DATA(EADC, 17);

	// disable adc and references
	/* Disable EADC module clock */
	EADC_Close(EADC);
	CLK_DisableModuleClock(EADC_MODULE);
	SYS_UnlockReg();
	SYS->IVSCTL &= ~SYS_IVSCTL_VTEMPEN_Msk;
	SYS_LockReg();

	//Tenths of a degree celcius
	// Vadc         = adcval / 4096 * 3.3 = -1.82e-3 * T ['C] + 0.72
	// => T ['C]    = (0.72 - N*3.3/4096)/1.82e-3
	// => T ['C/10] = (0.72 - N*3.3/4096)/1.82e-4
	//              = (894066 - N*1000)/226
	tval = (894066 - adcval * 1000)/226;

	return tval;
} // End of BSP_GetTemperature()

u32_t BSP_ADCGetVolts(void)
{
	u32_t adcval;

	PB13 = 1; // VOLTS_TEST high

	// enable references
	SYS_UnlockReg();
	/* Enable temperature sensor */
	SYS->VREFCTL =
	    (SYS->VREFCTL & ~SYS_VREFCTL_VREFCTL_Msk) | SYS_VREFCTL_VREF_AVDD; //Vref source is from internal 2.0V
	SYS_LockReg();

	/* Enable EADC module clock */
	CLK_EnableModuleClock(EADC_MODULE);
	/* EADC clock source is 16MHz, set divider to 16 -> 1 MHz */
	CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(16));

	/* Reset EADC module */
	SYS_ResetModule(EADC_RST);

	// wait 50 ms for references to settle
	TIME_WaitTicks(50);

	/* Set input mode as single-end and enable the A/D converter */
	EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);

	/* Set sample module 1 external sampling time to 0xFF */
	EADC_SetExtendSampleTime(EADC, 1, 0xFF);

	/* Clear the A/D ADINT0 interrupt flag for safe */
	EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);

	/* Enable the sample module 1 interrupt.  */
	EADC_ENABLE_INT(EADC, BIT0); //Enable sample module A/D ADINT0 interrupt.
	EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 0, BIT1);
	NVIC_EnableIRQ(EADC0_IRQn);

	/* Reset the ADC interrupt indicator and trigger sample module 1 to start A/D conversion */
	EADC_START_CONV(EADC, BIT1);

	/* Wait until EADC conversion is done */
	while (!EADC_GET_INT_FLAG(EADC, EADC_STATUS2_AVALID_Msk)) /* Disable the ADINT0 interrupt */
		EADC_DISABLE_INT(EADC, BIT0);
	NVIC_DisableIRQ(EADC0_IRQn);

	/* Get the conversion result of the sample module 1 */
	adcval = EADC_GET_CONV_DATA(EADC, 1);

	// disable adc and references
	/* Disable EADC module clock */
	EADC_Close(EADC);
	CLK_DisableModuleClock(EADC_MODULE);

	PB13 = 0; // VOLTS_TEST low

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

	CLK_EnableModuleClock(SPI_MODULE);
	SET_MFP_SPI();

	// Lock protected registers
	SYS_LockReg();

	// SPI clock rate
	SPI_Open(SPI, SPI_MASTER, SPI_MODE_2, 8, FCLK_SPI);

	SPI_DisableAutoSS(SPI);
	BSP_SetRFSSBHigh();

	SPI_SetFIFO(SPI, 7, 7);

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
	if (!SPI_GET_TX_FIFO_FULL_FLAG(SPI) && (SPI_GET_RX_FIFO_COUNT(SPI) < 7))
	{
		SPI_WRITE_TX(SPI, OutByte);
		return 1;
	}
	return 0;
}
u8_t BSP_SPIPopByte(u8_t *InByte)
{
	if (!SPI_GET_RX_FIFO_EMPTY_FLAG(SPI))
	{
		*InByte = SPI_READ_RX(SPI) & 0xFF;
		return 1;
	}
	return 0;
}

void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0)
{
	u8_t lxt_connected = 0;
	u8_t use_watchdog  = 0;

	// Note: GPIO wake-up only on falling edge, rising edge (IIE) does not perform wake-up contrary to datasheet
	//       This means that USBPresent cannot currently wake-up micro as it is active high

	// Wake-Up Conditions:
	//
	// - Timeout Timer (Timer0 or CAX Sleep Timer) (if set)
	// - Falling Edge USBPresent            (PB.12)
	// - Falling Edge SW2                   (PB.4)
	// - CAX Wakeup / Falling Edge SPI IRQ  (PC.0)
	//

	BSP_DisableUSB();

	//Disable GPIO - tristate the CAx if it is powered off
	CHILI_GPIOPowerDown(!use_timer0);

	//Disable HIRC calibration
	CHILI_DisableIntOscCal();

	if (USE_WATCHDOG_POWEROFF && !use_timer0)
		use_watchdog = 1;

	// use LF 32.768 kHz Crystal LXT for timer if present, switch off if not
	if ((CLK->STATUS & CLK_STATUS_LXTSTB_Msk) && use_timer0)
	{
		lxt_connected = 1;
		// Note: LIRC cannot be powered down as used for GPIOs
	}
	else
	{
		lxt_connected = 0;
		SYS_UnlockReg();
		CLK->PWRCTL &= ~(CLK_PWRCTL_LXTEN_Msk);
		SYS_LockReg();
	}

	// reconfig GPIO debounce clock to LIRC
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(RFIRQ_PORT, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(RFSS_PORT, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);
	GPIO_SET_DEBOUNCE_TIME(SPI_MISO_PORT, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_2);

	// enable wake-up for USBPresent PB.12
	GPIO_DisableInt(PB, 12);
	GPIO_EnableInt(PB, 12, GPIO_INT_RISING);
	// enable wake-up for SW2 PB.4
	GPIO_DisableInt(PB, 4);
	GPIO_EnableInt(PB, 4, GPIO_INT_FALLING);
	// enable wake-up for SPI IRQ PC.0
	GPIO_DisableInt(RFIRQ_PORT, RFIRQ_PIN);
	GPIO_EnableInt(RFIRQ_PORT, RFIRQ_PIN, GPIO_INT_FALLING);

	TIMER_Stop(TIMER2);
	SYS_ResetModule(TMR2_RST);
	TIMER_DisableInt(TIMER2);
	NVIC_DisableIRQ(TMR2_IRQn);
	TIMER2->CNT          = 0;
	LED_R_BlinkTimeCount = 0;
	LED_G_BlinkTimeCount = 0;
	LED_B_BlinkTimeCount = 0;

	TIMER_Stop(TIMER0);
	SYS_ResetModule(TMR0_RST);
	TIMER_ResetCounter(TIMER0);

	if (use_timer0 || use_watchdog)
	{
		CLK_EnableModuleClock(TMR0_MODULE);
		if (use_watchdog)
			CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LIRC, 0);
		else if (lxt_connected)
			CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LXT, 0);
		else
			CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LIRC, 0);

		//  1s period - 1Hz
		TIMER_Open(TIMER0, TIMER_ONESHOT_MODE, 1);
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

		NVIC_EnableIRQ(TMR0_IRQn);
		TIMER_EnableInt(TIMER0);
		TIMER_EnableWakeup(TIMER0);
		TIMER_Start(TIMER0);
	}

	asleep = 1;
	SYS_UnlockReg();

	CLK_SetPowerDownMode(CLK_PMUCTL_PDMSEL_ULLPD); /* ULLPD, any lower requires modifications */
	CLK->PWRCTL |= CLK_PWRCTL_PDEN_Msk;            /* Set power down bit */
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;             /* Sleep Deep */

	__WFI(); /* really enter power down here !!! */
	__DSB();
	__ISB();

	SYS_LockReg();

	TIMER_Stop(TIMER0);
	SYS_ResetModule(TMR0_RST);

	GPIO_SET_DEBOUNCE_TIME(RFIRQ_PORT, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_ENABLE_DEBOUNCE(RFIRQ_PORT, BITMASK(RFIRQ_PIN));

	asleep = 0;

	// re-configure system for operation
	LED_R_BlinkTimeCount = 0;
	LED_G_BlinkTimeCount = 0;
	LED_B_BlinkTimeCount = 0;
	// USE SWITCH PB.4 FOR APP_STATE
	GPIO_DisableInt(PB, 4);
	GPIO_EnableInt(PB, 4, GPIO_INT_FALLING);
	// USB PRESENT
	GPIO_DisableInt(PB, 12);
	GPIO_EnableInt(PB, 12, GPIO_INT_BOTH_EDGE);
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
 * I/O List for M2351 QFN33 (M2351ZIAAE)
 *
 * Pin    | Default        | Chili              | Usage
 * -----: | :------------- | :----------------- | :-----------------------------
 *   1    | PB.5     IN    | PB.5        IN     | Connector   (UART5 TX)
 *   2    | PB.4     IN    | PB.4        IN     | Connector   (UART5 RX) (SWITCH)
 *   3    | PB.3     IN    | PB.3        IN     | Connector   (UART1 TX) (LED_G)
 *   4    | PB.2     IN    | PB.2        IN     | Connector   (UART1 RX) (LED_R)
 *   5    | PB.1     IN    | PB.1        IN     | VOLTS       (UART2 TX)
 *   6    | PB.0     IN    | PB.0        IN     | CHARGE_STAT (UART2 RX)
 *   7    | PF.5     IN    | X32_IN      IN     | 32 kHz Xtal
 *   8    | PF.4     IN    | X32_OUT     I/O    | 32 kHz Xtal
 *   9    | PF.3     IN    | XT1_IN      IN     | CLK
 *  10    | PF.2     IN    | XT1_OUT     I/O    | NC
 *  11    | PA.3     IN    | PA.3        I/O    | SPI_CS      (CA8210 SSB,    P22)
 *  12    | PA.2     IN    | SPI0_SCLK   OUT    | SPI_CLK     (CA8210 SCLK,   P21)
 *  13    | PA.1     IN    | SPI0_MISO   IN     | SPI_MISO    (CA8210 MISO,   P24)
 *  14    | PA.0     IN    | SPI0_MOSI   OUT    | SPI_MOSI    (CA8210 MOSI,   P23)
 *  15    | VDD33    SUP   | VDD33       SUP    | -
 *  16    | nRESET   IN    | nRESET      IN     | -
 *  17    | PF.0     IN    | ICE_DAT     I/O    | TMS         (UART1 TX)
 *  18    | PF.1     IN    | ICE_CLK     IN     | TCK         (UART1 RX)
 *  19    | PC.1     IN    | PC.1        OUT    | ZIG_RESET   (CA8210 NRESET, P27)
 *  20    | PC.0     IN    | PC.0        IN     | ZIG_IRQB    (CA8210 NIRQ,   P20)
 *  21    | PA.12    IN    | USB_5V      SUP    | USB VBUS
 *  22    | PA.13    IN    | USB_D-      I/O    | USB D-
 *  23    | PA.14    IN    | USB_D+      I/O    | USB D+
 *  24    | PA.15    IN    | PA.15       IN     | Connector
 *  25    | VSS      SUP   | VSS         SUP    | -
 *  26    | VSW      SUP   | VSW         SUP    | -
 *  27    | VDD      SUP   | VDD         SUP    | -
 *  28    | LDO_CAP  SUP   | LDO_CAP     SUP    | -
 *  29    | PB.14    IN    | PB.14       IN     | ZIG_APP     (CA8210 DIG3,   P26, GPIO9)
 *  30    | PB.13    IN    | PB.13       OUT    | VOLTS_TEST  (UART0 TX)
 *  31    | PB.12    IN    | PB.12       IN     | USB_PRESENT (UART0 RX)  PROGRAM
 *  32    | AVDD      SUP  | AVDD        SUP    | -
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOInit(void)
{
	// Initialise and set Debounce to 8 HCLK cycles
	GPIO_SET_DEBOUNCE_TIME(PA, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(PB, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(RFIRQ_PORT, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(RFSS_PORT, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);
	GPIO_SET_DEBOUNCE_TIME(SPI_MISO_PORT, GPIO_DBCTL_DBCLKSRC_HCLK, GPIO_DBCTL_DBCLKSEL_8);

	// PB.13: VOLTS_TEST
	// output, no pull-up, set to 0
	GPIO_SetMode(PB, BIT13, GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(PB, BIT13, GPIO_PUSEL_DISABLE);
	PB13 = 0; // VOLTS_TEST is active high, so switch off to avoid unecessary power consumption

	// PC.1: ZIG_RESET
	// output, no pull-up, set to 1
	GPIO_SetMode(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_PUSEL_DISABLE);
	RSTB = 1; // RSTB is HIGH

	// PC.0: ZIG_IRQB
	// input, pull-up
	GPIO_SetMode(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_MODE_INPUT);
	GPIO_SetPullCtl(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_PUSEL_PULL_UP);
	GPIO_EnableInt(RFIRQ_PORT, RFIRQ_PIN, GPIO_INT_FALLING);

	// PB.4: SWITCH
	// input, no pull-up, debounced
	//x GPIO_SetMode(PB, BIT4, GPIO_MODE_INPUT);
	//x GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_DISABLE);
	//x GPIO_ENABLE_DEBOUNCE(PB, BIT4);
	// currently no switch so pull up
	GPIO_SetPullCtl(PB, BIT4, GPIO_PUSEL_PULL_UP);

	// SPI: MFP but SS0 controlled separately as GPIO
	// SPI_MOSI
	// MFP output
	// SPI_MISO
	// MFP input
	// SPI_SCLK
	// MFP output
	SET_MFP_SPI();
	GPIO_SetPullCtl(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_PUSEL_DISABLE);
	GPIO_SetPullCtl(SPI_CLK_PORT, BITMASK(SPI_CLK_PIN), GPIO_PUSEL_DISABLE);

	// PA.3: SPI_CS
	// output, no pull-up, set to 1
	GPIO_SetMode(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_PUSEL_DISABLE);
	RFSS = 1;

	// PB.2: LED_R
	// output, no pull-up, set to 1
	//x GPIO_SetMode(PB, BIT2, GPIO_MODE_OUTPUT);
	//x GPIO_SetPullCtl(PB, BIT2, GPIO_PUSEL_DISABLE);
	//x PB2 = 1;

	// PB.3: LED_G
	// output, no pull-up, set to 1
	//x GPIO_SetMode(PB, BIT3, GPIO_MODE_OUTPUT);
	//x GPIO_SetPullCtl(PB, BIT3, GPIO_PUSEL_DISABLE);
	//x PB3 = 1;

	// PB.0: CHARGE_STAT
	// input, pull-up, debounced
	GPIO_SetMode(PB, BIT0, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_PULL_UP); // CHARGE_STAT is low or tri-state
	GPIO_ENABLE_DEBOUNCE(PB, BIT0);

	// PF.0: ICE_DAT MFP
	//x GPIO_SetPullCtl(PF, BIT0, GPIO_PUSEL_DISABLE);

	// PF.1: ICE_CLK MFP
	//x GPIO_SetPullCtl(PF, BIT1, GPIO_PUSEL_DISABLE);

	// PB.1: EADC0 MFP (VOLTS)
	SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB1MFP_Msk)) | SYS_GPB_MFPL_PB1MFP_EADC0_CH1;
	GPIO_DISABLE_DIGITAL_PATH(PB, BIT1);
	GPIO_SetPullCtl(PB, BIT1, GPIO_PUSEL_DISABLE);

	// PB.12: USB_PRESENT
	// input, no pull-up, debounced
	GPIO_SetMode(PB, BIT12, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_DISABLE);
	GPIO_ENABLE_DEBOUNCE(PB, BIT12);
	// Immediately get USBPresent State
	USBPresent = 1;

	// PB.12: PROGRAM Pin
	// enable pull-up just in case
	//x GPIO_SetMode(PB, BIT12, GPIO_MODE_INPUT);
	//x GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_PULL_UP);

	// PF.3: XT1_IN MFP
	GPIO_SetPullCtl(PF, BIT3, GPIO_PUSEL_DISABLE);

	// PF.2: XT1_OUT MFP
	GPIO_SetPullCtl(PF, BIT2, GPIO_PUSEL_DISABLE);

	// PF.5: XT32_IN MFP
	GPIO_SetPullCtl(PF, BIT5, GPIO_PUSEL_DISABLE);

	// PF.4: XT32_OUT MFP
	GPIO_SetPullCtl(PF, BIT4, GPIO_PUSEL_DISABLE);

// UART0 if used
// PB.12: UART0_RX
// MFP input
// PB.13: UART0_TX
// MFP output
#if defined(USE_UART)
	/* Set multi-function pins for UART0 RXD and TXD */
	SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | UART0_RXD_PB12 | UART0_TXD_PB13;
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
	// PC.1: ZIG_RESET
	// tri-state - output to input
	GPIO_SetMode(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_MODE_INPUT);

	// PC.0: ZIG_IRQB
	// disable pull-up
	GPIO_SetPullCtl(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_PUSEL_DISABLE);

	if (tristateCax)
	{
		// SPI:
		// SPI_MOSI: tri-state - output to input
		// SPI_MISO: tri-state - input  to input
		// SPI_CLK:  tri-state - output to input
		SET_MFP_GPIO();
		GPIO_SetMode(SPI_MOSI_PORT, BITMASK(SPI_MOSI_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_MISO_PORT, BITMASK(SPI_MISO_PIN), GPIO_MODE_INPUT);
		GPIO_SetMode(SPI_CLK_PORT, BITMASK(SPI_CLK_PIN), GPIO_MODE_INPUT);
	}

	// PA.3: SPI_CS
	// tri-state - output to input
	GPIO_SetMode(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_MODE_INPUT);

	// PB.2: LED_R
	// tri-state - output to input - use pull-up otherwise light-dependent photodiode effect!
	PB2 = 1;
	GPIO_SetMode(PB, BIT2, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT2, GPIO_PUSEL_PULL_UP);

	// PB.3: LED_G
	// tri-state - output to input - use pull-up otherwise light-dependent photodiode effect!
	PB3 = 1;
	GPIO_SetMode(PB, BIT3, GPIO_MODE_INPUT);
	GPIO_SetPullCtl(PB, BIT3, GPIO_PUSEL_PULL_UP);

	// PB.0: CHARGE_STAT
	// disable pull-up
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_DISABLE);

} // End of CHILI_GPIOPowerDown()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-program GPIOs for active Mode after PowerDown
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOPowerUp(void)
{
	// PC.1: ZIG_RESET
	// input to output
	GPIO_SetMode(RSTB_PORT, BITMASK(RSTB_PIN), GPIO_MODE_OUTPUT);

	// PC.0: ZIG_IRQB
	// enable pull-up
	GPIO_SetPullCtl(RFIRQ_PORT, BITMASK(RFIRQ_PIN), GPIO_PUSEL_PULL_UP);

	// SPI1:
	// PA.0: tri-state - input to SPI1_MOSI0
	// PA.1: tri-state - input to SPI1_MISO0
	// PA.2: tri-state - input to SPI1_SCLK
	SET_MFP_SPI();

	// PA.3: SPI_CS
	// input to output
	GPIO_SetMode(RFSS_PORT, BITMASK(RFSS_PIN), GPIO_MODE_OUTPUT);

	// PB.2: LED_R
	// input to output
	GPIO_SetPullCtl(PB, BIT2, GPIO_PUSEL_DISABLE);
	GPIO_SetMode(PB, BIT2, GPIO_MODE_OUTPUT);

	// PB.3: LED_G
	// input to output
	GPIO_SetPullCtl(PB, BIT3, GPIO_PUSEL_DISABLE);
	GPIO_SetMode(PB, BIT3, GPIO_MODE_OUTPUT);

	// PB.0: CHARGE_STAT
	// enable pull-up
	GPIO_SetPullCtl(PB, BIT0, GPIO_PUSEL_PULL_UP);

} // End of CHILI_GPIOPowerUp()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Enable GPIO Interrupts
 *******************************************************************************
 ******************************************************************************/
void CHILI_GPIOEnableInterrupts(void)
{
	// USE SWITCH PB.4 FOR APP_COORDINATOR
	//x GPIO_EnableInt(PB, 4, GPIO_INT_FALLING);

	// PB.12: USBPresent
	//x GPIO_EnableInt(PB, 12, GPIO_INT_BOTH_EDGE);

	//x NVIC_EnableIRQ(GPB_IRQn);

} // End of CHILI_GPIOEnableInterrupts()

u8_t BSP_GPIOSenseSwitch(void)
{
	return PB4;
} // End of BSP_GPIOSenseSwitch()

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
void CHILI_SetClockExternalCFGXT1(u8_t clk_external)
{
	const uint32_t DATA_FLASH_CONFIG0_CFGXT1_MASK = 0x08000000; //!< mask for CONFIG0 CFGXT1 bit
	uint32_t       cfg0;

	SYS_UnlockReg();
	FMC_Open();
	cfg0 = FMC_Read(FMC_USER_CONFIG_0);
	if (clk_external)
	{
		if (cfg0 & DATA_FLASH_CONFIG0_CFGXT1_MASK)
		{
			FMC_ENABLE_CFG_UPDATE();
			FMC_Write(FMC_USER_CONFIG_0, cfg0 & ~DATA_FLASH_CONFIG0_CFGXT1_MASK);
			FMC_DISABLE_CFG_UPDATE();
		}
	}
	else
	{
		if (!(cfg0 & DATA_FLASH_CONFIG0_CFGXT1_MASK))
		{
			FMC_ENABLE_CFG_UPDATE();
			FMC_Erase(FMC_CONFIG_BASE);
			FMC_Write(FMC_USER_CONFIG_0, cfg0 | DATA_FLASH_CONFIG0_CFGXT1_MASK);
			FMC_DISABLE_CFG_UPDATE();
		}
	}
	FMC_Close();
	SYS_LockReg();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Select System Clocks depending on Power Source
 *******************************************************************************
 ******************************************************************************/
void CHILI_ClockInit(void)
{
	uint32_t delayCnt;

	// NOTE: __HXT in system_M2351.h has to be changed to correct input clock frequency !!
#if __HXT != 4000000UL
#error "__HXT Not set correctly in system_M2351.h"
#endif

	if (sUseExternalClock) // 4 MHz clock supplied externally via HXT
	{
		// HXT enabled
		SYS_UnlockReg();
		CLK->PWRCTL |= (CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->STATUS & CLK_STATUS_HXTSTB_Msk)
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
		CLK->PWRCTL |= (CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk);
		SYS_LockReg();
		for (delayCnt = 0; delayCnt <= MAX_CLOCK_SWITCH_DELAY; ++delayCnt)
		{
			if (CLK->STATUS & CLK_STATUS_HIRCSTB_Msk)
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
		SYS_UnlockReg();
		// *****************************************************************************************
		// PLL Setup
		// *****************************************************************************************
		// FREF =  2 -   8 MHz
		// FVCO = 96 - 200 MHz
		// FOUT = 24 - 144 MHz
		// *****************************************************************************************
		// 						FIN	FREF FVCO FOUT	NR NF NO	INDIV FBDIV OUTDIV		PLLCTL
		// *****************************************************************************************
		//	CLKExternal = 0		 12    4   96   48	 3 12  2		2    10      1		0x0008440A
		//	CLKExternal = 1		  4	   4   96   48   1 12  2	    0	 10      1		0x0000400A
		// *****************************************************************************************
		// Note: Using CLK_EnablePLL() does not always give correct results, so avoid!
		if (sUseExternalClock)
			CLK->PLLCTL = 0x0000400A;
		else
			CLK->PLLCTL = 0x0008440A;
		SYS_LockReg();
		/* TODO: Use CLK_WaitClockReady()? */
		for (delayCnt = 0; delayCnt < MAX_CLOCK_SWITCH_DELAY; delayCnt++)
		{
			if (CLK->STATUS & CLK_STATUS_PLLSTB_Msk)
				break;
		}
		if (delayCnt >= MAX_CLOCK_SWITCH_DELAY)
		{
			BSP_LEDSigMode(LED_M_SETERROR);
#if defined(USE_DEBUG)
			if ((!USBPresent) && (!CLKExternal))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_3);
			else if ((!USBPresent) && (CLKExternal))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_4);
			else if ((USBPresent) && (!CLKExternal))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_5);
			else if ((USBPresent) && (CLKExternal))
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_6);
			else
				BSP_Debug_Error(DEBUG_ERR_CLOCKINIT_7);
#endif // USE_DEBUG
		}
	}

	if (USBPresent) // USB connected and supplies power - HCLK and USB from PLL
	{
		SYS_UnlockReg();
		CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(3)); // divider is 3  -> HCLK = 16 MHz
		/* Select IP clock source */
		CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(1));
		/* Enable IP clock */
		CLK_EnableModuleClock(USBD_MODULE);
		SYS_LockReg();
	}
	else // USB unplugged, power from battery
	{
		if (sUseExternalClock) // 4 MHz clock supplied externally via HXT
		{
			SYS_UnlockReg();
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));
			SYS_LockReg();
		}
		else // 12 MHz clock from HIRC
		{
			SYS_UnlockReg();
			CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(3));
			SYS_LockReg();
		}
		CLK_DisablePLL(); // PLL Off
	}

	// Set HCLK back to HIRC if clock switching error happens
	if (CLK->STATUS & CLK_STATUS_CLKSFAIL_Msk)
	{
		SYS_UnlockReg();
		CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1)); // HCLK = 12MHz
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
	if (!(CLK->STATUS & CLK_STATUS_CLKSFAIL_Msk))
	{
		if (sUseExternalClock)
		{
			// disable HIRC
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HIRCEN_Msk);
			SYS_LockReg();
		}
		else
		{
			// disable HXT
			SYS_UnlockReg();
			CLK->PWRCTL &= ~(CLK_PWRCTL_HXTEN_Msk);
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
	SYS->TISTS12M = 0xFFFF;
	if ((!sUseExternalClock) && (CLK->STATUS & CLK_STATUS_LXTSTB_Msk))
	{
		// trim HIRC to 12MHz
		SYS->TCTL12M = 0xF1;
		// enable HIRC-trim interrupt
		NVIC_EnableIRQ(CKFAIL_IRQn);
		SYS->TIEN12M = (SYS_TIEN12M_TFAILIEN_Msk | SYS_TIEN12M_CLKEIEN_Msk);
	}

} // End of CHILI_EnableIntOscCal()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Disable Internal Oscillator Calibration
 *******************************************************************************
 ******************************************************************************/
void CHILI_DisableIntOscCal(void)
{
	SYS->TISTS12M = 0xFFFF;
	SYS->TIEN12M  = 0;
	SYS->TCTL12M  = 0;

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
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);
	CLK_EnableModuleClock(TMR2_MODULE);

	// configure clock selects and dividers for timers 0 and 1
	if (sUseExternalClock)
	{
		CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);
		CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
		CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_HXT, 0);
	}
	else
	{
		CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);
		CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
		CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_HIRC, 0);
	}

	TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1000000);
	TIMER_Open(TIMER1, TIMER_CONTINUOUS_MODE, 1000000);
	TIMER_Open(TIMER2, TIMER_PERIODIC_MODE, 50000);

	// prescale value has to be set after TIMER_Open() is called!
	if (sUseExternalClock)
	{
		//  4 MHZ Clock: prescaler 3 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER0, 3);
		//  4 MHZ Clock: prescalar 3 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER1, 3);
		//  4 MHZ Clock: prescaler 79 gives 20 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER2, 79);
	}
	else
	{
		// 12 MHZ Clock: prescaler 11 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER0, 11);
		// 12 MHZ Clock: prescalar 11 gives 1 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER1, 11);
		// 12 MHZ Clock: prescaler 239 gives 20 uSec units
		TIMER_SET_PRESCALE_VALUE(TIMER2, 239);
	}

	// note timers are 24-bit (+ 8-bit prescale)
	// 1uSec units, so 1000 is 1ms
	TIMER_SET_CMP_VALUE(TIMER0, 1000);
	// 1uSec units, counts microseconds
	TIMER_SET_CMP_VALUE(TIMER1, 0xFFFFFF);
	// 20uSec units, so 500 is 10ms
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
#if defined(USE_USB)
	static u8_t USBPresent_d = 0;
#endif // USE_USB

// switch off USB comms if USBPResent has changed and disconnected
#if defined(USE_USB)
	if (!USBPresent && USBPresent_d)
		USB_Deinitialise();
#endif // USE_USB
#if defined(USE_UART)
	BSP_UARTDeinit();
#endif // USE_UART

	// configure clocks
	CHILI_ClockInit();

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
	NVIC_SetPriority(GPA_IRQn, 1);
	NVIC_SetPriority(GPB_IRQn, 1);
	NVIC_SetPriority(GPC_IRQn, 1);
	NVIC_SetPriority(GPD_IRQn, 1);
	NVIC_SetPriority(GPE_IRQn, 1);
	NVIC_SetPriority(GPF_IRQn, 1);
	NVIC_SetPriority(UART0_IRQn, 2);

#if defined(USE_USB)
	USBPresent_d = USBPresent;
#endif // USE_USB

} // End of CHILI_SystemReInit()

void BSP_Initialise(struct ca821x_dev *pDeviceRef)
{
	u32_t               i;
	struct device_link *device;

	// start up is on internal HIRC Oscillator, 12MHz, PLL disabled

	// initialise GPIOs
	CHILI_GPIOInit();

	// wait until system is stable after potential usb plug-in
	while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk))
		;
	for (i = 0; i < 500000; ++i)
		;

	// system  initialisation (clocks, timers ..)
	CHILI_SystemReInit();

	// configure HXT to external clock input
	CHILI_SetClockExternalCFGXT1(1);

	// enable GPIO interrupts
	CHILI_GPIOEnableInterrupts();

	device                       = &device_list[0];
	device->chip_select_gpio     = &RFSS;
	device->irq_gpio             = &RFIRQ;
	device->dev                  = pDeviceRef;
	pDeviceRef->exchange_context = device;
} // End of BSP_Initialise()

void BSP_WatchdogEnable(u32_t timeout_ms)
{
	// uses TIMER3 instead of WDT for longer timeout intervals
	if (timeout_ms < 0x00FFFFFF)
	{
		// always uses 10 kHz LIRC clock
		CLK_EnableModuleClock(TMR3_MODULE);
		CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_LIRC, 0);

		// 10 kHz Clock: prescaler 9 gives 1 ms units
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
	printf("WD Started,,,\n");
} // End of BSP_WatchdogEnable()

void BSP_WatchdogReset(u32_t timeout_ms)
{
	// uses TIMER3 instead of WDT for longer timeout intervals
	if (timeout_ms < 0x00FFFFFF)
	{
		SYS_ResetModule(TMR3_RST);
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
		TIMER3->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
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
	if (USBPresent)
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

int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t *pn32Out_R0)
{
	return 0;
}
