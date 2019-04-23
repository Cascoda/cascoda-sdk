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
 * Interrupt Service Routines (ISRs)
*/
/* System */
#include <stdio.h>
/* Platform */
#include "Nano100Series.h"
#include "gpio.h"
#include "pwm.h"
#include "timer.h"
/* Cascoda */
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_usb.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_usb.h"
#if defined(USE_DEBUG)
#include "cascoda_debug_chili.h"
#endif /* USE_DEBUG */

extern volatile u8_t asleep;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Hard Fault interrupt
 *******************************************************************************
 ******************************************************************************/
void HAL_IrqHandlerHardFault(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_HARDFAULT; /* just in case this returns without reset */
#endif                                     /* USE_DEBUG */

	while (1)
	{
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Timer0 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER0;
#endif /* USE_DEBUG */

	/* clear all timer interrupt status bits */
	TIMER0->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;

	if (asleep && USE_WATCHDOG_POWEROFF)
		WDTimeout = 1; /* timer0 acts as watchdog */
	else
		TIME_1msTick(); /* timer0 for system ticks */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Timer1 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER1;
#endif /* USE_DEBUG */
	/* clear all timer interrupt status bits */
	TIMER1->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Timer2 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER2;
#endif /* USE_DEBUG */

	/* clear all timer interrupt status bits */
	TIMER2->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;
	CHILI_LEDBlink(); /* timer2 for LED control */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Timer3 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR3_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER3;
	else
		Debug_IRQ_State = DEBUG_IRQ_TIMER3;
#endif /* USE_DEBUG */

	/* clear all timer interrupt status bits */
	TIMER3->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;
	printf("WD ISR\n");
	WDTimeout = 1; /* timer3 acts as watchdog */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: HIRC trim interrupt handler
 *******************************************************************************
 ******************************************************************************/
void HIRC_IRQHandler(void)
{
	__IO uint32_t reg = SYS_GET_IRCTRIM_INT_FLAG();
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_HIRC;
	else
		Debug_IRQ_State = DEBUG_IRQ_HIRC;
#endif /* USE_DEBUG */

	if (reg & BIT1)
	{
		/* Display HIRC trim status */
		printf("Trim Failure Interrupt\n");
		/* Clear Trim Failure Interrupt */
		SYS_CLEAR_IRCTRIM_INT_FLAG(SYS_IRCTRIMINT_FAIL_INT);
	}
	if (reg & BIT2)
	{
		/* Display HIRC trim status */
		printf("LXT Clock Error Interrupt\n");
		/* Clear LXT Clock Error Interrupt */
		SYS_CLEAR_IRCTRIM_INT_FLAG(SYS_IRCTRIMINT_32KERR_INT);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: USB device interrupt
 *******************************************************************************
 ******************************************************************************/
void USBD_IRQHandler(void)
{
#if defined(USE_USB)
	uint32_t u32INTSTS = USBD_GET_INT_FLAG();

#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_USBD;
#endif /* USE_DEBUG */

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

#endif /* USE_USB */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: External interrupt from PA, PB and PC ports
 *******************************************************************************
 ******************************************************************************/
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
#endif /* USE_DEBUG */

	/* PA.9: SW2 */
	if (u32Status & BIT9)
	{
		if (button1_pressed)
		{
			button1_pressed();
		}
	}

	/* PA.10: RFIRQ */
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
#endif /* USE_DEBUG */

	/* USBPresent GPIO */
	if (u32Status & USBP_PINMASK)
	{
		/* get USBPresent State and re-initialise System */
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Clock controller interrupt for wake-up from power-down state
 *******************************************************************************
 ******************************************************************************/
void PDWU_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_PDWU;
	else
		Debug_IRQ_State = DEBUG_IRQ_PDWU;
#endif /* USE_DEBUG */

	CLK->WK_INTSTS = CLK_WK_INTSTS_IS; /* clear interrupt */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: UART1
 *******************************************************************************
 ******************************************************************************/
void UART1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif /* USE_DEBUG */

#if defined(USE_UART)

	while (UART1->ISR & UART_ISR_RDA_IS_Msk)
	{
		Serial_ReadInterface();
	}

#endif /* USE_UART */
}
