/**
 * @file cascoda_bsp_chili.c
 * @brief Board Support Package (BSP)\n
 *        Micro: Nuvoton M2351\n
 *        Board: Chili Module
 * @author Wolfgang Bruchner
 * @date 25/10/18
 *//*
 * Copyright (C) 2018  Cascoda, Ltd.
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

#include "M2351.h"
#include "gpio.h"
#include "timer.h"
#if defined(USE_DEBUG)
#include "cascoda_debug_chili.h"
#endif // USE_DEBUG

extern volatile u8_t asleep;

/**
  * @brief  ISR 0: Brown-out low voltage detected interrupt
  * @param  None.
  * @return None.
  */
void BOD_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_BOD;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 1: IRC TRIM interrupt/*
  * @param  None.
  * @retval None.
  */
void IRC_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_HIRC;
	else
		Debug_IRQ_State = DEBUG_IRQ_HIRC;
#endif // USE_DEBUG

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

/**
  * @brief  ISR 2: Clock controller interrupt for chip wake-up from power-down state.
  * @param  None.
  * @return None.
  */
void PWRWU_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_PDWU;
	else
		Debug_IRQ_State = DEBUG_IRQ_PDWU;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 3: SRAM parity check error interrupt
  * @param  None.
  * @return None.
  */
void SRAM_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 4: Clock fail detected interrupt
  * @param  None.
  * @return None.
  */
void CLKFAIL_IRQHandler(void)
{
	uint32_t u32Reg;

#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG

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

/**
  * @brief  ISR 6: Real time clock interrupt
  * @param  None.
  * @return None.
  */
void RTC_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_RTC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 7: Backup register tamper interrupt
  * @param  None.
  * @return None.
  */
void TAMPER_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 8: Watchdog Timer interrupt
  * @param  None.
  * @return None.
  */
void WDT_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_WDT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 9: Window Watchdog Timer interrupt
  * @param  None.
  * @return None.
  */
void WWDT_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_WDT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 10: External interrupt pins 0
  * @param  None.
  * @return None.
  */
void EINT0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 11: External interrupt pins 1
  * @param  None.
  * @return None.
  */
void EINT1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 12: External interrupt pins 2
  * @param  None.
  * @return None.
  */
void EINT2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 13: External interrupt pins 3
  * @param  None.
  * @return None.
  */
void EINT3_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 14: External interrupt pins 4
  * @param  None.
  * @return None.
  */
void EINT4_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 15: External interrupt pins 5
  * @param  None.
  * @return None.
  */
void EINT5_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 16: External interrupt from PA pins
  * @param  None.
  * @retval None
  */
void GPA_IRQHandler(void)
{
/* no interrupts mapped */
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_GPINV;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 17: External interrupt from PB pins
  * @param  None.
  * @retval None.
  */
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
#endif // USE_DEBUG

	// PB.4: SW2
	if (u32Status & BIT4)
	{
		// only switch if usb connected
		if (PB12 == 1 && button1_pressed)
		{
			button1_pressed();
		}
	}

	// PB.12: USBPresent
	if (u32Status & BIT12)
	{
		// get USBPresent State and re-initialise System
		USBPresent = 1;
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
  * @brief  ISR 18: External interrupt from PC pins
  * @param  None.
  * @retval None.
  */
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
#endif // USE_DEBUG

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

/**
  * @brief  ISR 19: External interrupt from PD pins
  * @param  None.
  * @retval None
  */
void GPD_IRQHandler(void)
{
/* no interrupts mapped */
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_GPINV;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 20: External interrupt from PE pins
  * @param  None.
  * @retval None
  */
void GPE_IRQHandler(void)
{
/* no interrupts mapped */
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_GPINV;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 21: External interrupt from PF pins
  * @param  None.
  * @retval None
  */
void GPF_IRQHandler(void)
{
/* no interrupts mapped */
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_GPINV;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 22: QSPI0 interrupt (Quad SPI)
  * @param  None.
  * @retval None
  */
void QSPI0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 23: SPI0 interrupt
  * @param  None.
  * @retval None
  */
void SPI0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 24: EPWM0 brake interrupt
  * @param  None.
  * @retval None
  */
void BRAKE0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 25: EPWM0 pair 0 interrupt
  * @param  None.
  * @retval None
  */
void EPWM0_P0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 26: EPWM0 pair 1 interrupt
  * @param  None.
  * @retval None
  */
void EPWM0_P1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 27: EPWM0 pair 2 interrupt
  * @param  None.
  * @retval None
  */
void EPWM0_P2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 28: EPWM1 brake interrupt
  * @param  None.
  * @retval None
  */
void BRAKE1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 29: EPWM1 pair 0 interrupt
  * @param  None.
  * @retval None
  */
void EPWM1_P0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 30: EPWM1 pair 1 interrupt
  * @param  None.
  * @retval None
  */
void EPWM1_P1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 31: EPWM1 pair 2 interrupt
  * @param  None.
  * @retval None
  */
void EPWM1_P2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 32: Timer 0 interrupt
  * @param  None.
  * @retval None
  */
void TMR0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER0;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER0->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;

	if (asleep && USE_WATCHDOG_POWEROFF)
		WDTimeout = 1; // timer0 acts as watchdog
	else
		TIME_1msTick();
}

/**
  * @brief  ISR 33: Timer 1 interrupt
  * @param  None.
  * @retval None
  */
void TMR1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER1;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER1->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;
}

/**
  * @brief  ISR 34: Timer 2 interrupt
  * @param  None.
  * @retval None
  */
void TMR2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER2;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER2->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;

	//x	BSP_LEDBlink();
}

/**
  * @brief  ISR 35: Timer 3 interrupt
  * @param  None.
  * @retval None
  */
void TMR3_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_TIMER3;
	else
		Debug_IRQ_State = DEBUG_IRQ_TIMER3;
#endif // USE_DEBUG

	// clear all timer interrupt status bits
	TIMER3->INTSTS = TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk;

	printf("WD ISR\n");

	// timer3 acts as watchdog
	WDTimeout = 1;
}

/**
  * @brief  ISR 36: UART0 interrupt
  * @param  None.
  * @retval None
  */
void UART0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif // USE_DEBUG

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
#endif // USE_DEBUG
#endif // USE_UART
}

/**
  * @brief  ISR 37: UART1 interrupt
  * @param  None.
  * @retval None
  */
void UART1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif // USE_DEBUG

#if defined(USE_UART)

	while (UART1->INTSTS & UART_INTSTS_RDAINT_Msk)
	{
		if (SerialGetCommand())
		{
			SerialRxPending = true;
		}
	}

#else
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_UART;
#endif // USE_DEBUG
#endif // USE_UART
}

/**
  * @brief  ISR 38: I2C0 interrupt
  * @param  None.
  * @retval None
  */
void I2C0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_I2C;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 39: I2C1 interrupt
  * @param  None.
  * @retval None
  */
void I2C1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_I2C;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 40: PDMA0 interrupt (Peripheral DMA)
  * @param  None.
  * @retval None
  */
void PDMA0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PDMA;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 41: DAC interrupt
  * @param  None.
  * @retval None
  */
void DAC_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_DAC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 42: EADC interrupt source 0
  * @param  None.
  * @retval None
  */
void EADC0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_ADC;
#endif // USE_DEBUG
	/* Clear the A/D ADINT0 interrupt flag */
	EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);
}

/**
  * @brief  ISR 43: EADC interrupt source 1
  * @param  None.
  * @retval None
  */
void EADC1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_ADC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 44: ACMP0 and ACMP1 interrupt
  * @param  None.
  * @retval None
  */
void ACMP01_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 46: EADC interrupt source 2
  * @param  None.
  * @retval None
  */
void EADC2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_ADC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 47: EADC interrupt source 3
  * @param  None.
  * @retval None
  */
void EADC3_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_ADC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 48: UART2 interrupt
  * @param  None.
  * @retval None
  */
void UART2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif // USE_DEBUG

#if defined(USE_UART)

	while (UART2->INTSTS & UART_INTSTS_RDAINT_Msk)
	{
		if (SerialGetCommand())
		{
			SerialRxPending = true;
		}
	}

#else
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_UART;
#endif // USE_DEBUG
#endif // USE_UART
}

/**
  * @brief  ISR 49: UART3 interrupt
  * @param  None.
  * @retval None
  */
void UART3_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif // USE_DEBUG

#if defined(USE_UART)

	while (UART3->INTSTS & UART_INTSTS_RDAINT_Msk)
	{
		if (SerialGetCommand())
		{
			SerialRxPending = true;
		}
	}

#else
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_UART;
#endif // USE_DEBUG
#endif // USE_UART
}

/**
  * @brief  ISR 51: SPI1 interrupt
  * @param  None.
  * @retval None
  */
void SPI1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 52: SPI2 interrupt
  * @param  None.
  * @retval None
  */
void SPI2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 53: USB device interrupt
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

/**
  * @brief  ISR 54: USB host interrupt
  * @param  None.
  * @retval None
  */
void USBH_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 55: USB OTG interrupt
  * @param  None.
  * @retval None
  */
void USBOTG_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 56: CAN0 interrupt
  * @param  None.
  * @retval None
  */
void CAN0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 58: Smart card host 0 interrupt
  * @param  None.
  * @retval None
  */
void SC0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 59: Smart card host 1 interrupt
  * @param  None.
  * @retval None
  */
void SC1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 60: Smart card host 2 interrupt
  * @param  None.
  * @retval None
  */
void SC2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SC;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 62: SPI3 interrupt
  * @param  None.
  * @retval None
  */
void SPI3_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_SPI;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 64: SD host 0 interrupt (Secure Digital Host Controller)
  * @param  None.
  * @retval None
  */
void SDH0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 68: I2S0 interrupt
  * @param  None.
  * @retval None
  */
void I2S0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_I2S;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 70: OPA interrupt
  * @param  None.
  * @retval None
  */
void OPA0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 71: Crypto interrupt
  * @param  None.
  * @retval None
  */
void CRPT_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 72: External interrupt from PG pins
  * @param  None.
  * @retval None
  */
void GPG_IRQHandler(void)
{
/* no interrupts mapped */
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_GPINV;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 73: External interrupt pins 6
  * @param  None.
  * @return None.
  */
void EINT6_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 74: UART4 interrupt
  * @param  None.
  * @retval None
  */
void UART4_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif // USE_DEBUG

#if defined(USE_UART)

	while (UART4->INTSTS & UART_INTSTS_RDAINT_Msk)
	{
		if (SerialGetCommand())
		{
			SerialRxPending = true;
		}
	}

#else
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_UART;
#endif // USE_DEBUG
#endif // USE_UART
}

/**
  * @brief  ISR 75: UART5 interrupt
  * @param  None.
  * @retval None
  */
void UART5_IRQHandler(void)
{
#if defined(USE_DEBUG)
	if (asleep)
		Debug_IRQ_State = DEBUG_IRQ_WKUP_UART;
#endif // USE_DEBUG

#if defined(USE_UART)

	while (UART5->INTSTS & UART_INTSTS_RDAINT_Msk)
	{
		if (SerialGetCommand())
		{
			SerialRxPending = true;
		}
	}

#else
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_UART;
#endif // USE_DEBUG
#endif // USE_UART
}

/**
  * @brief  ISR 76: USCI0 interrupt (Universal Serial Control Interface)
  * @param  None.
  * @retval None
  */
void USCI0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 77: USCI1 interrupt (Universal Serial Control Interface)
  * @param  None.
  * @retval None
  */
void USCI1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 78: BPWM0 interrupt
  * @param  None.
  * @retval None
  */
void BPWM0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 79: BPWM1 interrupt
  * @param  None.
  * @retval None
  */
void BPWM1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PWM;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 82: I2C2 interrupt
  * @param  None.
  * @retval None
  */
void I2C2_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_I2C;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 84: QEI0 interrupt (Quadrature Encoder Interface)
  * @param  None.
  * @retval None
  */
void QEI0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 85: QEI1 interrupt (Quadrature Encoder Interface)
  * @param  None.
  * @retval None
  */
void QEI1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 86: ECAP0 interrupt (Enhanced Input Capture Timer)
  * @param  None.
  * @retval None
  */
void ECAP0_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 87: ECAP1 interrupt (Enhanced Input Capture Timer)
  * @param  None.
  * @retval None
  */
void ECAP1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 88: External interrupt from PH pins
  * @param  None.
  * @retval None
  */
void GPH_IRQHandler(void)
{
/* no interrupts mapped */
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_GPINV;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 89: External interrupt pins 7
  * @param  None.
  * @return None.
  */
void EINT7_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_EINT;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 98: PDMA1 interrupt (Peripheral DMA)
  * @param  None.
  * @retval None
  */
void PDMA1_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_PDMA;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 99: SCU interrupt (Security Configuration Unit)
  * @param  None.
  * @retval None
  */
void SCU_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}

/**
  * @brief  ISR 101: TRNG interrupt (True Random Number Generator)
  * @param  None.
  * @retval None
  */
void TRNG_IRQHandler(void)
{
#if defined(USE_DEBUG)
	Debug_IRQ_State = DEBUG_IRQ_OTHERS;
#endif // USE_DEBUG
}
