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

void TMR0_IRQHandler(void)
{
	/* clear all timer interrupt status bits */
	TIMER0->ISR = TIMER_ISR_TMR_IS_Msk | TIMER_ISR_TCAP_IS_Msk | TIMER_ISR_TMR_WAKE_STS_Msk;

	if (asleep && USE_WATCHDOG_POWEROFF)
		WDTimeout = 1; /* timer0 acts as watchdog */
	else
		CHILI_1msTick(); /* timer0 for system ticks */
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
	ca_log_warn("WD ISR");
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
		ca_log_warn("Trim Failure Interrupt");
		/* Clear Trim Failure Interrupt */
		SYS_CLEAR_IRCTRIM_INT_FLAG(SYS_IRCTRIMINT_FAIL_INT);
	}
	if (reg & BIT2)
	{
		/* Display HIRC trim status */
		ca_log_warn("LXT Clock Error Interrupt");
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
	uint32_t            ZIG_IRQB_status;
	struct device_link *devlink;
	int                 i;
	enPortnum           portnum;

	/* Get Port status */
	ZIG_IRQB_status = ZIG_IRQB_PORT->ISRC;

	/* ZIG_IRQB */
	if (ZIG_IRQB_status & BITMASK(ZIG_IRQB_PIN))
	{
		GPIO_CLR_INT_FLAG(ZIG_IRQB_PORT, BITMASK(ZIG_IRQB_PIN));

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
