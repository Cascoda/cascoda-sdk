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
 * Interrupt Service Routines (ISRs)
*/
/* System */
#include <stdio.h>
/* Platform */
#include "M2351.h"
#include "gpio.h"
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
 * \brief ISR 1: IRC TRIM interrupt
 *******************************************************************************
 ******************************************************************************/
void IRC_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_HIRC;
	else
		Debug_IRQ_State = DEBUG_IRQ_HIRC;
#endif /* USE_DEBUG */

	if (SYS->TISTS12M & SYS_TISTS12M_TFAILIF_Msk) /* Get Trim Failure Interrupt */
	{
		/* Display HIRC trim status */
		printf("HIRC Trim Failure Interrupt\n");
		/* Clear Trim Failure Interrupt */
		SYS->TISTS12M = SYS_TISTS12M_TFAILIF_Msk;
	}
	if (SYS->TISTS12M & SYS_TISTS12M_CLKERRIF_Msk) /* Get LXT Clock Error Interrupt */
	{
		/* Display HIRC trim status */
		printf("LXT Clock Error Interrupt\n");
		/* Clear LXT Clock Error Interrupt */
		SYS->TISTS12M = SYS_TISTS12M_CLKERRIF_Msk;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 2: Clock controller interrupt for wake-up from power-down state
 *******************************************************************************
 ******************************************************************************/
void PWRWU_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_PDWU;
	else
		Debug_IRQ_State = DEBUG_IRQ_PDWU;
#endif /* USE_DEBUG */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 4: Clock fail detected interrupt
 *******************************************************************************
 ******************************************************************************/
void CLKFAIL_IRQHandler(void)
{
	uint32_t u32Reg;

#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif /* USE_DEBUG */

	/* Unlock protected registers */
	SYS_UnlockReg();

	u32Reg = CLK->CLKDSTS;

	if (u32Reg & CLK_CLKDSTS_HXTFIF_Msk)
	{
		/* HCLK is switched to HIRC automatically if HXT clock fail interrupt is happened */
		printf("HXT Clock is stopped! HCLK is switched to HIRC.\n");
		/* Disable HXT clock fail interrupt */
		CLK->CLKDCTL &= ~(CLK_CLKDCTL_HXTFDEN_Msk | CLK_CLKDCTL_HXTFIEN_Msk);
		/* Write 1 to clear HXT Clock fail interrupt flag */
		CLK->CLKDSTS = CLK_CLKDSTS_HXTFIF_Msk;
	}

	if (u32Reg & CLK_CLKDSTS_LXTFIF_Msk)
	{
		/* LXT clock fail interrupt is happened */
		printf("LXT Clock is stopped!\n");
		/* Disable LXT clock fail interrupt */
		CLK->CLKDCTL &= ~(CLK_CLKDCTL_LXTFIEN_Msk | CLK_CLKDCTL_LXTFDEN_Msk);
		/* Write 1 to clear LXT Clock fail interrupt flag */
		CLK->CLKDSTS = CLK_CLKDSTS_LXTFIF_Msk;
	}

	if (u32Reg & CLK_CLKDSTS_HXTFQIF_Msk)
	{
		/* HCLK should be switched to HIRC if HXT clock frequency monitor interrupt is happened */
		CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
		printf("HXT Frequency is abnormal! HCLK is switched to HIRC.\n");
		/* Disable HXT clock frequency monitor interrupt */
		CLK->CLKDCTL &= ~(CLK_CLKDCTL_HXTFQDEN_Msk | CLK_CLKDCTL_HXTFQIEN_Msk);
		/* Write 1 to clear HXT Clock frequency monitor interrupt */
		CLK->CLKDSTS = CLK_CLKDSTS_HXTFQIF_Msk;
	}

	/* Lock protected registers */
	SYS_LockReg();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 17: External interrupt from PB ports
 *******************************************************************************
 ******************************************************************************/
void GPB_IRQHandler(void)
{
	uint32_t u32Status;
	/* Get Port PB status */
	u32Status = PB->INTSRC;
	/* Clear all PB interrupts */
	GPIO_CLR_INT_FLAG(PB, u32Status);

#if defined(USE_DEBUG)
	if (u32Status)
	{
		if (asleep)
		{
			if (u32Status == BIT4)
				Debug_IRQ_State = DEBUG_IRQ_WKUP_SW2;
			else if (u32Status == BIT12)
				Debug_IRQ_State = DEBUG_IRQ_WKUP_USBPRESENT;
			else
				Debug_IRQ_State = DEBUG_IRQ_WKUP_P_UNKNOWN;
		}
		else
		{
			if (u32Status == BIT4)
				Debug_IRQ_State = DEBUG_IRQ_SW2;
			if (u32Status == BIT12)
				Debug_IRQ_State = DEBUG_IRQ_USBPRESENT;
		}
	}
#endif /* USE_DEBUG */

	/* PB.4: SW2 */
	if (u32Status & BIT4)
	{
		/* only switch if usb connected */
		if (PB12 == 1 && button1_pressed)
		{
			button1_pressed();
		}
	}

#if (CASCODA_CHILI2_CONFIG == 1)
	/* PB.12: USBPresent */
	if (u32Status & BIT12)
	{
		/* get USBPresent State and re-initialise System */
		USBPresent = PB12;
		if (!USBPresent)
		{
			CHILI_SystemReInit();
		}
		if (usb_present_changed)
		{
			usb_present_changed();
		}
	}
#endif
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 18: External interrupt from PC ports
 *******************************************************************************
 ******************************************************************************/
void GPC_IRQHandler(void)
{
	uint32_t            u32Status;
	struct device_link *devlink;
	int                 i;

	/* Get Port PC status */
	u32Status = PC->INTSRC;
	/* Clear all PC interrupts */
	GPIO_CLR_INT_FLAG(PC, u32Status);

#if defined(USE_DEBUG)
	if (u32Status)
	{
		if (asleep)
		{
			if (u32Status == BIT0)
				Debug_IRQ_State = DEBUG_IRQ_WKUP_RFIRQ;
			else
				Debug_IRQ_State = DEBUG_IRQ_WKUP_P_UNKNOWN;
		}
	}
#endif /* USE_DEBUG */

	/* RFIRQ */
	if (u32Status & BITMASK(RFIRQ_PIN))
	{
		if (asleep)
			return;

		i = 0;
		while (i < NUM_DEVICES)
		{
			devlink = &device_list[i];
			if (devlink->irq_gpio == &RFIRQ)
				break;
			i++;
		}

		if (i < NUM_DEVICES)
		{
			EVBMEHandler(devlink->dev); /* Read downstream message */
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 32: Timer 0 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER0;
#endif /* USE_DEBUG */

	/* clear all timer interrupt status bits */
	TIMER0->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;

	if (asleep && USE_WATCHDOG_POWEROFF)
		WDTimeout = 1; /* timer0 acts as watchdog */
	else
		TIME_1msTick(); /* timer0 for system ticks */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 33: Timer 1 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER1;
#endif /* USE_DEBUG */
	/* clear all timer interrupt status bits */
	TIMER1->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 34: Timer 2 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER2;
#endif /* USE_DEBUG */

	/* clear all timer interrupt status bits */
	TIMER2->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
	/* insert blink service routine CHILI_LEDBlink() here if LEDs are populated */
	CHILI_LEDBlink();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 35: Timer 3 interrupt
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
	TIMER3->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
	/* timer3 acts as watchdog */
	WDTimeout = 1;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 36: UART0 interrupt
 *******************************************************************************
 ******************************************************************************/
void UART0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif /* USE_DEBUG */

#if defined(USE_UART)

	while (UART0->INTSTS & UART_INTSTS_RDAINT_Msk)
	{
		if (SerialGetCommand())
		{
			SerialRxPending = true;
		}
	}

#else
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_UART;
#endif /* USE_DEBUG */
#endif /* USE_UART */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 53: USB device interrupt
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
