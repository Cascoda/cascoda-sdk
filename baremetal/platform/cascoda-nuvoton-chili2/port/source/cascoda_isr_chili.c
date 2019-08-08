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
 * \brief ISR 1: IRC TRIM interrupt
 *******************************************************************************
 ******************************************************************************/
void IRC_IRQHandler(void)
{
	/* 12 MHz HIRC */
	if (SYS->TISTS12M & SYS_TISTS12M_TFAILIF_Msk) /* Get Trim Failure Interrupt */
	{
		printf("HIRC Trim Failure Interrupt\n");
		SYS->TISTS12M = SYS_TISTS12M_TFAILIF_Msk;
	}
	if (SYS->TISTS12M & SYS_TISTS12M_CLKERRIF_Msk) /* Get Clock Error Interrupt */
	{
		printf("HIRC Clock Error Interrupt\n");
		SYS->TISTS12M = SYS_TISTS12M_CLKERRIF_Msk;
	}

	/* 48 MHz HIRC48 */
	if (SYS->TISTS48M & SYS_TISTS48M_TFAILIF_Msk) /* Get Trim Failure Interrupt */
	{
		printf("HIRC48 Trim Failure Interrupt\n");
		SYS->TISTS48M = SYS_TISTS48M_TFAILIF_Msk;
	}
	if (SYS->TISTS48M & SYS_TISTS48M_CLKERRIF_Msk) /* Get Clock Error Interrupt */
	{
		printf("HIRC48 Clock Error Interrupt\n");
		SYS->TISTS48M = SYS_TISTS48M_CLKERRIF_Msk;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 2: Clock controller interrupt for wake-up from power-down state
 *******************************************************************************
 ******************************************************************************/
void PWRWU_IRQHandler(void)
{
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 4: Clock fail detected interrupt
 *******************************************************************************
 ******************************************************************************/
void CLKFAIL_IRQHandler(void)
{
	uint32_t u32Reg;

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
 * \brief ISR 16: External interrupt from PA ports
 *******************************************************************************
 ******************************************************************************/
void GPA_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_A);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 17: External interrupt from PB ports
 *******************************************************************************
 ******************************************************************************/
void GPB_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_B);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 18: External interrupt from PC ports
 *******************************************************************************
 ******************************************************************************/
void GPC_IRQHandler(void)
{
	uint32_t            ZIG_IRQB_status;
	struct device_link *devlink;
	int                 i;

	/* Get Port status */
	ZIG_IRQB_status = ZIG_IRQB_PORT->INTSRC;

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

	CHILI_ModulePinIRQHandler(PN_C);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 19: External interrupt from PD ports
 *******************************************************************************
 ******************************************************************************/
void GPD_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_D);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 20: External interrupt from PE ports
 *******************************************************************************
 ******************************************************************************/
void GPE_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_E);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 21: External interrupt from PF ports
 *******************************************************************************
 ******************************************************************************/
void GPF_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_F);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 72: External interrupt from PG ports
 *******************************************************************************
 ******************************************************************************/
void GPG_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_G);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 88: External interrupt from PH ports
 *******************************************************************************
 ******************************************************************************/
void GPH_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_H);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 32: Timer 0 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR0_IRQHandler(void)
{
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
	/* clear all timer interrupt status bits */
	TIMER2->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 35: Timer 3 interrupt
 *******************************************************************************
 ******************************************************************************/
void TMR3_IRQHandler(void)
{
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
#if defined(USE_UART)
#if (UART_CHANNEL == 0)
	CHILI_UARTFIFOIRQHandler();
#endif
#endif /* USE_UART */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 37: UART1 interrupt
 *******************************************************************************
 ******************************************************************************/
void UART1_IRQHandler(void)
{
#if defined(USE_UART)
#if (UART_CHANNEL == 1)
	CHILI_UARTFIFOIRQHandler();
#endif
#endif /* USE_UART */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 40: PDMA0 interrupt
 *******************************************************************************
 ******************************************************************************/
void PDMA0_IRQHandler(void)
{
	/* Get PDMA interrupt status */
	uint32_t u32Status = PDMA_GET_INT_STATUS(PDMA0);
	if (u32Status & PDMA_INTSTS_ABTIF_Msk) /* Target Abort */
	{
		printf("DMA ABORT\n");
		PDMA0->ABTSTS = PDMA0->ABTSTS;
		return;
	}
#if defined(USE_UART)
	if (u32Status & PDMA_INTSTS_TDIF_Msk) /* Transfer Done */
		CHILI_UARTDMAIRQHandler();
#endif /* USE_UART */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 48: UART2 interrupt
 *******************************************************************************
 ******************************************************************************/
void UART2_IRQHandler(void)
{
#if defined(USE_UART)
#if (UART_CHANNEL == 2)
	CHILI_UARTFIFOIRQHandler();
#endif
#endif /* USE_UART */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 49: UART3 interrupt
 *******************************************************************************
 ******************************************************************************/
void UART3_IRQHandler(void)
{
#if defined(USE_UART)
#if (UART_CHANNEL == 3)
	CHILI_UARTFIFOIRQHandler();
#endif
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
 * \brief ISR 74: UART4 interrupt
 *******************************************************************************
 ******************************************************************************/
void UART4_IRQHandler(void)
{
#if defined(USE_UART)
#if (UART_CHANNEL == 4)
	CHILI_UARTFIFOIRQHandler();
#endif
#endif /* USE_UART */
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 75: UART5 interrupt
 *******************************************************************************
 ******************************************************************************/
void UART5_IRQHandler(void)
{
#if defined(USE_UART)
#if (UART_CHANNEL == 5)
	CHILI_UARTFIFOIRQHandler();
#endif
#endif /* USE_UART */
}
