/**
 * @file cascoda_isr_chili.c
 * @brief This file contains interrupt service routines
 * @date 04, September, 2012
 *//*
 * Copyright (C) 2012-2014 Nuvoton Technology Corp. All rights reserved.
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
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_usb.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_usb.h"

#include "Nano100Series.h"
#include "gpio.h"
#include "pwm.h"
#include "timer.h"
#if defined(USE_DEBUG)
#include "cascoda_debug_chili.h"
#endif // USE_DEBUG

extern u8_t asleep;

void HAL_IrqHandlerHardFault(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_HARDFAULT; // just in case this returns without reset
#endif                                     // USE_DEBUG

	while (1)
	{
	}
}

void TMR0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER0;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER0->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;

	if (asleep && USE_WATCHDOG_POWEROFF)
		WDTimeout = 1; // timer0 acts as watchdog
	else
		TIME_1msTick();
}

void TMR1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER1;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER1->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;
}

void TMR2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER2;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER2->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;

	CHILI_LEDBlink();
}

void TMR3_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER3;
	else
		Debug_IRQ_State = DEBUG_IRQ_TIMER3;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER3->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;

	printf("WD ISR\n");

	// timer3 acts as watchdog
	WDTimeout = 1;
}

/**
  * @brief  HIRC_IRQHandler, HIRC trim interrupt handler.
  * @param  None.
  * @retval None.
  */
void HIRC_IRQHandler(void)
{
	__IO uint32_t reg = SYS_GET_IRCTRIM_INT_FLAG();

#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_HIRC;
	else
		Debug_IRQ_State = DEBUG_IRQ_HIRC;
#endif // USE_DEBUG

	if (reg & BIT1)
	{
		printf("Trim Failure Interrupt\n");
		SYS_CLEAR_IRCTRIM_INT_FLAG(SYS_IRCTRIMINT_FAIL_INT);
	}
	if (reg & BIT2)
	{
		SYS_CLEAR_IRCTRIM_INT_FLAG(SYS_IRCTRIMINT_32KERR_INT);
		printf("LXT Clock Error Lock\n");
	}
}

/**
  * @brief  USBD_IRQHandler, USB interrupt handler.
  * @param  None.
  * @retval None.
  */
void USBD_IRQHandler(void)
{
#if defined(USE_USB)
	uint32_t u32INTSTS = USBD_GET_INT_FLAG();

#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_USBD;
#endif // USE_DEBUG

	if (u32INTSTS & USBD_INTSTS_FLDET)
	{
		/* Handle the USB attached/detached event */
		USB_FloatDetect();
	}
	else if (u32INTSTS & USBD_INTSTS_BUS)
	{
		/* Handle the USB bus event: Reset, Suspend, and Resume */
		USB_BusEvent();
	}
	else if (u32INTSTS & USBD_INTSTS_USB)
	{
		/* Handle the USB Protocol/Class event */
		USB_UsbEvent(u32INTSTS);
	}
	else if (u32INTSTS & USBD_INTSTS_WAKEUP)
	{
		/* Clear wakeup event. write one clear */
		USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP);
	}

#endif // USE_USB
}

/*---------------------------------------------------------------------------------------------------------*/
/* GPIO A,B,C ISR                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
/**
  * @brief  GPABC_IRQHandler, GPIO PortA, PortB, PortC interrupt handler.
  * @param  None.
  * @retval None.
  */
void GPABC_IRQHandler(void)
{
	uint32_t            u32Status;
	struct device_link *devlink;
	int                 i;

	/* Get Port PA status */
	u32Status = PA->ISRC;
	/* Clear all PA interrupts */
	GPIO_CLR_INT_FLAG(PA, u32Status);

#if defined(USE_DEBUG)
	if (u32Status)
	{
		if (asleep)
		{
			if (u32Status == BIT9)
				Debug_IRQ_State = DEBUG_IRQ_WKUP_PA_SW2;
			else if (u32Status == BIT10)
				Debug_IRQ_State = DEBUG_IRQ_WKUP_PA_RFIRQ;
			else
				Debug_IRQ_State = DEBUG_IRQ_WKUP_PA_UNKNOWN;
		}
		else
		{
			if (u32Status == BIT9)
				Debug_IRQ_State = DEBUG_IRQ_PA_SW2;
		}
	}
#endif // USE_DEBUG

	// PA.9
	if (u32Status & BIT9)
	{
		if (button1_pressed)
		{
			button1_pressed();
		}
	}

	// PA.10
	if (u32Status & BIT10)
	{
		if (asleep)
			return;

		i = 0;
		while (i < NUM_DEVICES)
		{
			devlink = &device_list[i];
			if (devlink->irq_gpio == &PA10)
				break;
			i++;
		}

		if (i < NUM_DEVICES)
		{
			EVBMEHandler(devlink->dev); /* Read downstream message */
		}
	}

	/* Get Port PB status */
	u32Status = USBP_PORT->ISRC;
	/* Clear all USBP_PORT interrupts */
	GPIO_CLR_INT_FLAG(USBP_PORT, u32Status);

#if defined(USE_DEBUG)
	if (u32Status)
	{
		if (asleep)
		{
			if (u32Status == USBP_PINMASK)
				Debug_IRQ_State = DEBUG_IRQ_WKUP_PBC_USBPRESENT;
			else
				Debug_IRQ_State = DEBUG_IRQ_WKUP_PBC_UNKNOWN;
		}
		else
		{
			if (u32Status == USBP_PINMASK)
				Debug_IRQ_State = DEBUG_IRQ_PBC_USBPRESENT;
		}
	}
#endif // USE_DEBUG

	// USBPresent GPIO
	if (u32Status & USBP_PINMASK)
	{
		// get USBPresent State and re-initialise System
		USBPresent = USBP_GPIO;
		if (!USBPresent)
		{
			CHILI_SystemReInit();
		}
		if (usb_present_changed)
		{
			usb_present_changed();
		}
	}
}

/**
  * @brief  PDWU_IRQ Handler.
  * @param  None.
  * @return None.
  */
void PDWU_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_PDWU;
	else
		Debug_IRQ_State = DEBUG_IRQ_PDWU;
#endif // USE_DEBUG

	CLK->WK_INTSTS = CLK_WK_INTSTS_IS; /* clear interrupt */
}

void BOD_IRQHandler(void) // Brownout low voltage detected interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_BOD;
#endif // USE_DEBUG
}

void WDT_IRQHandler(void) // Watch Dog Timer interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_WDT;
#endif // USE_DEBUG
}

void EINT0_IRQHandler(void) // External signal interrupt from PB.14 pin
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT0;
#endif // USE_DEBUG
}

void EINT1_IRQHandler(void) // External signal interrupt from PB.15 pin
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT1;
#endif // USE_DEBUG
}

void GPDEF_IRQHandler(void) // External interrupt from PD[15:0]/PE[15:0]/PF[7:0]
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_GPDEF;
#endif // USE_DEBUG
}

void PWM0_IRQHandler(void) // PWM 0 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM0;
#endif // USE_DEBUG
}

void PWM1_IRQHandler(void) // PWM 1 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM1;
#endif // USE_DEBUG
}

void UART0_IRQHandler(void) // UART0 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_UART0;
#endif // USE_DEBUG
}

void UART1_IRQHandler(void) // UART1 interrupt
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif // USE_DEBUG

#if defined(USE_UART)

	while (UART1->ISR & UART_ISR_RDA_IS_Msk)
	{
		if (SerialGetCommand())
		{
			SerialRxPending = true;
		}
	}

#endif // USE_UART
}

void SPI0_IRQHandler(void) // SPI0 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI0;
#endif // USE_DEBUG
}

void SPI1_IRQHandler(void) // SPI1 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI1;
#endif // USE_DEBUG
}

void SPI2_IRQHandler(void) // SPI2 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI2;
#endif // USE_DEBUG
}

void I2C0_IRQHandler(void) // I2C0 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_I2C0;
#endif // USE_DEBUG
}

void I2C1_IRQHandler(void) // I2C1 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_I2C1;
#endif // USE_DEBUG
}

void SC0_IRQHandler(void) // SC0 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SC0;
#endif // USE_DEBUG
}

void SC1_IRQHandler(void) // SC1 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SC1;
#endif // USE_DEBUG
}

void SC2_IRQHandler(void) // SC2 interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SC2;
#endif // USE_DEBUG
}

void LCD_IRQHandler(void) // LCD interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_LCD;
#endif // USE_DEBUG
}

void PDMA_IRQHandler(void) // PDMA interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PDMA;
#endif // USE_DEBUG
}

void I2S_IRQHandler(void) // I2S interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_I2S;
#endif // USE_DEBUG
}

void ADC_IRQHandler(void) // ADC interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_ADC;
#endif // USE_DEBUG
}

void DAC_IRQHandler(void) // DAC interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_DAC;
#endif // USE_DEBUG
}

void RTC_IRQHandler(void) // Real time clock interrupt
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_RTC;
#endif // USE_DEBUG
}
