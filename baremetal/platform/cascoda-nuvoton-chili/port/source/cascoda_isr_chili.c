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
#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_usb.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#include "cascoda_chili_usb.h"

extern volatile u8_t asleep;

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Hard Fault interrupt
 *******************************************************************************
 ******************************************************************************/
void HAL_IrqHandlerHardFault(void)
{
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
	/* clear all timer interrupt status bits */
	TIMER2->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Timer3 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR3_IRQHandler(void)
{
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
	uint32_t            ZIG_IRQB_status, USB_PRESENT_status;
	struct device_link *devlink;
	int                 i;
	enPortnum           portnum;

	/* Get Port status */
	ZIG_IRQB_status    = ZIG_IRQB_PORT->ISRC;
	USB_PRESENT_status = USB_PRESENT_PORT->ISRC;

	/* clear all interrupts */
	GPIO_CLR_INT_FLAG(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN));
	GPIO_CLR_INT_FLAG(USB_PRESENT_PORT, BITMASK(USB_PRESENT_PIN));

	/* ZIG_IRQB */
	if (ZIG_IRQB_status & BITMASK(ZIG_IRQB_PIN))
	{
		if (asleep)
			return;

		i = 0;
		while (i < NUM_DEVICES)
		{
			devlink = &device_list[i];
			if (devlink->irq_gpio == &ZIG_IRQB_PVAL)
				break;
			i++;
		}

		if (i < NUM_DEVICES)
		{
			DISPATCH_ReadCA821x(devlink->dev); /* Read downstream message */
		}
		return; /* should make handling quicker if no other interrupts are triggered */
	}

	/* USB_PRESENT */
	if (USB_PRESENT_status & BITMASK(USB_PRESENT_PIN))
	{
		/* get state and re-initialise System */
		USBPresent = USB_PRESENT_PVAL;
		if (!USBPresent)
		{
			CHILI_SystemReInit();
		}
		if (usb_present_changed)
		{
			usb_present_changed();
		}
		return; /* should make handling quicker if no other interrupts are triggered */
	}

	for (portnum = PN_A; portnum <= PN_C; ++portnum)
	{
		CHILI_ModulePinIRQHandler(portnum);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: External interrupt from PD, PE and PF ports
 *******************************************************************************
 ******************************************************************************/
void GPDEF_IRQHandler(void)
{
	enPortnum portnum;

	for (portnum = PN_D; portnum <= PN_F; ++portnum)
	{
		CHILI_ModulePinIRQHandler(portnum);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Clock controller interrupt for wake-up from power-down state
 *******************************************************************************
 ******************************************************************************/
void PDWU_IRQHandler(void)
{
	CLK->WK_INTSTS = CLK_WK_INTSTS_IS; /* clear interrupt */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: UART1
 *******************************************************************************
 ******************************************************************************/
void UART1_IRQHandler(void)
{
#if defined(USE_UART)

	while (UART1->ISR & UART_ISR_RDA_IS_Msk)
	{
		Serial_ReadInterface();
	}

#endif /* USE_UART */
}
