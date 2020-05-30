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
#include "cascoda-bm/cascoda_usb.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#include "cascoda_chili_usb.h"
#include "cascoda_secure.h"

#if (__ARM_FEATURE_CMSE == 3U)
// Nonsecure function type, when called by a TrustZone enabled environment
// the hardware switches to the nonsecure world
typedef void __attribute__((cmse_nonsecure_call)) (*nsc_dispatch_read_t)(struct ca821x_dev *pDeviceRef);
#else
// If not a TrustZone environment, this is a normal function pointer
typedef void (*nsc_dispatch_read_t)(struct ca821x_dev *pDeviceRef);
#endif

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 16: External interrupt from PA ports
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY void GPA_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_A);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR 17: External interrupt from PB ports
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY void GPB_IRQHandler(void)
{
	CHILI_ModulePinIRQHandler(PN_B);
}

void cascoda_isr_chili_init(void)
{
	//This function is here to kick the linker, do not remove!
	return;
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

		CHILI_SetWakeup(1);

		if (CHILI_GetAsleep())
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
			((nsc_dispatch_read_t)(devlink->dispatch_read))(devlink->dev); /* Read downstream message */
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
 * \brief ISR 48: UART2 interrupt
 *******************************************************************************
 ******************************************************************************/
__NONSECURE_ENTRY void UART2_IRQHandler(void)
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
__NONSECURE_ENTRY void UART5_IRQHandler(void)
{
#if defined(USE_UART)
#if (UART_CHANNEL == 5)
	CHILI_UARTFIFOIRQHandler();
#endif
#endif /* USE_UART */
}
