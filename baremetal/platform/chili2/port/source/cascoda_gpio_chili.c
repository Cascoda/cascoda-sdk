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
 * Module Pin and GPIO Mapping
*/
/* System */
#include <stdio.h>
#include <string.h>
/* Platform */
#include "M2351.h"
#include "eadc.h"
#include "gpio.h"
#include "spi.h"
#include "sys.h"
#include "timer.h"
#include "uart.h"
#include "usbd.h"
/* Cascoda */
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_config.h"
#include "cascoda_chili_gpio.h"
#include "cascoda_secure.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */

#ifndef CASCODA_CHILI2_CONFIG
#error CASCODA_CHILI2_CONFIG has to be defined! Please include the file "cascoda_chili_config.h"
#endif

/******************************************************************************/
/****** Dynamically configurable Ports / Pins                            ******/
/******************************************************************************/

enum
{
	PIN_USB_PRESENT = 103,
};

struct pinlist
{
	u8_t      pin;         /* module pin number */
	enPortnum portnum;     /* mcu port number (0=PA, 1=PB, ..) */
	u8_t      portbit;     /* mcu port bit (0 - 15) */
	u8_t      adc_channel; /* adc channel available on pin */
};

struct pinstatus
{
	u8_t                blocked : 1;   /* pin is blocked by other functionality */
	module_pin_dir      io : 1;        /* pin type (input/output) */
	module_pin_pullup   pullup : 1;    /* inputs only: 0: none, 1: pullup */
	module_pin_debounce debounce : 1;  /* inputs only:	0: no debounce, 1: debounce */
	module_pin_type     isled : 1;     /* outputs only: 1: pin has led attached */
	u8_t                permanent : 1; /* outputs only: 1: keep as driven output (don't tristate) in power-down */
	module_pin_irq irq : 2; /* inputs only: 0: no irq, 1: irq falling edge, 2: irq rising edge, 3: irq both edges */
};

/******************************************************************************/
/****** Static Variables                                                 ******/
/******************************************************************************/
static const struct pinlist ModulePinList[] = {
/* module pin */
/* port PX.Y */
/* adc channel */
#if (CASCODA_CHILI2_CONFIG == 0 || CASCODA_CHILI2_CONFIG == 2)
    {5, PN_B, 12, 12},    /* PB.12 */
    {6, PN_B, 13, 13},    /* PB.13 */
    {11, PN_A, 13, P_NA}, /* PA.13 */
    {12, PN_A, 14, P_NA}, /* PA.14 */
#else
    {PIN_USB_PRESENT, PN_B, 12, P_NA}, /* PB.12:(USB_PRESENT), virtual */
#endif /* CASCODA_CHILI2_CONFIG */
#if (CASCODA_EXTERNAL_FLASHCHIP_PRESENT == 0)
    {15, PN_A, 15, P_NA}, /* PA.15 */
#endif
#if (CASCODA_CHILI2_CONFIG == 0 || CASCODA_CHILI2_CONFIG == 2)
    {17, PN_A, 12, P_NA}, /* PA.12 */
#endif                    /* CASCODA_CHILI2_CONFIG */
#if (!CASCODA_CHILI_DISABLE_CA821x)
    {31, PN_B, 5, 5}, /* PB.5 */
#endif
    {32, PN_B, 4, 4}, /* PB.4 */
    {33, PN_B, 3, 3}, /* PB.3 */
    {34, PN_B, 2, 2}, /* PB.2 */
#if (CASCODA_CHILI2_CONFIG == 0 || CASCODA_CHILI2_CONFIG == 2)
    {35, PN_B, 1, 1}, /* PB.1 */
    {36, PN_B, 0, 0}, /* PB.0 */
#endif                /* CASCODA_CHILI2_CONFIG */
};

/* number of dynamically available module i/o pins */
enum
{
	NUM_MODULEPINS = sizeof(ModulePinList) / sizeof(ModulePinList[0]),
};

static struct pinstatus ModulePinStatus[NUM_MODULEPINS] = {0};
static int (*ModulePinCallbacks[NUM_MODULEPINS])(void);

struct ModuleSpecialPins BSP_GetModuleSpecialPins(void)
{
	struct ModuleSpecialPins rval;
	memset(&rval, P_NA, sizeof(rval));
#if (CASCODA_CHILI2_CONFIG == 1 || CASCODA_CHILI2_CONFIG == 2)
	rval.USB_PRESENT = PIN_USB_PRESENT;
#endif
	return rval;
}

u8_t CHILI_ModuleGetPinFromPort(enPortnum portnum, u8_t portbit)
{
	int i;
	for (i = 0; i < NUM_MODULEPINS; i++)
	{
		if (ModulePinList[i].portnum == portnum && ModulePinList[i].portbit == portbit)
		{
			return ModulePinList[i].pin;
		}
	}
	return (P_NA);
}

u8_t CHILI_ModuleGetPortBitFromPin(u8_t mpin)
{
	int i;
	for (i = 0; i < NUM_MODULEPINS; i++)
	{
		if (ModulePinList[i].pin == mpin)
		{
			return ModulePinList[i].portbit;
		}
	}
	return (P_NA);
}

u8_t CHILI_ModuleGetPortNumFromPin(u8_t mpin)
{
	int i;
	for (i = 0; i < NUM_MODULEPINS; i++)
	{
		if (ModulePinList[i].pin == mpin)
		{
			return ModulePinList[i].portnum;
		}
	}
	return (P_NA);
}

u8_t CHILI_ModuleGetIndexFromPin(u8_t mpin)
{
	u8_t i;

	for (i = 0; i < NUM_MODULEPINS; ++i)
	{
		if (ModulePinList[i].pin == mpin)
		{
			return (i);
		}
	}
	return (P_NA);
}

void CHILI_ModulePowerDownGPIOs(u8_t useGPIOforWakeup)
{
	u8_t    i;
	GPIO_T *port;

	for (i = 0; i < NUM_MODULEPINS; ++i)
	{
		if ((ModulePinStatus[i].io == MODULE_PIN_DIR_OUT) && (ModulePinStatus[i].permanent != 1))
		{
			port = MGPIO_PORT(ModulePinList[i].portnum);
			GPIO_SetMode(port, BITMASK(ModulePinList[i].portbit), GPIO_MODE_INPUT);
			/* use pull-up for LEDs otherwise light-dependent photodiode effect! */
			if (ModulePinStatus[i].isled == MODULE_PIN_TYPE_LED)
			{
				GPIO_SetPullCtl(port, BITMASK(ModulePinList[i].portbit), GPIO_PUSEL_PULL_UP);
			}
		}
		else if ((ModulePinStatus[i].io == MODULE_PIN_DIR_IN) && (!useGPIOforWakeup))
		{
			port = MGPIO_PORT(ModulePinList[i].portnum);
			if (ModulePinStatus[i].irq != MODULE_PIN_IRQ_OFF)
				GPIO_DisableInt(port, ModulePinList[i].portbit);
		}
	}
}

void CHILI_ModulePowerUpGPIOs(u8_t useGPIOforWakeup)
{
	u8_t    i;
	GPIO_T *port;

	for (i = 0; i < NUM_MODULEPINS; ++i)
	{
		if ((ModulePinStatus[i].io == MODULE_PIN_DIR_OUT) && (ModulePinStatus[i].permanent != 1))
		{
			port = MGPIO_PORT(ModulePinList[i].portnum);
			if (ModulePinStatus[i].isled == MODULE_PIN_TYPE_LED)
			{
				GPIO_SetPullCtl(port, BITMASK(ModulePinList[i].portbit), GPIO_PUSEL_DISABLE);
			}
			GPIO_SetMode(port, BITMASK(ModulePinList[i].portbit), GPIO_MODE_OUTPUT);
		}
		else if ((ModulePinStatus[i].io == MODULE_PIN_DIR_IN) && (!useGPIOforWakeup))
		{
			port = MGPIO_PORT(ModulePinList[i].portnum);
			if (ModulePinStatus[i].irq != MODULE_PIN_IRQ_OFF)
			{
				if (ModulePinStatus[i].irq == MODULE_PIN_IRQ_BOTH)
					GPIO_EnableInt(port, ModulePinList[i].portbit, GPIO_INT_BOTH_EDGE);
				else if (ModulePinStatus[i].irq == MODULE_PIN_IRQ_RISE)
					GPIO_EnableInt(port, ModulePinList[i].portbit, GPIO_INT_RISING);
				else if (ModulePinStatus[i].irq == MODULE_PIN_IRQ_FALL)
					GPIO_EnableInt(port, ModulePinList[i].portbit, GPIO_INT_FALLING);
			}
		}
	}
}

void CHILI_ModulePinIRQHandler(enPortnum portnum)
{
	GPIO_T *port;
	u8_t    i;

	port = MGPIO_PORT(portnum);

	for (i = 0; i < NUM_MODULEPINS; ++i)
	{
		if ((ModulePinStatus[i].irq) && (ModulePinList[i].portnum == portnum))
		{
			if (port->INTSRC & BITMASK(ModulePinList[i].portbit))
			{
				/* clear interrupt */
				GPIO_CLR_INT_FLAG(port, BITMASK(ModulePinList[i].portbit));
				/* call callback if registered */
				if (ModulePinCallbacks[i])
				{
					ModulePinCallbacks[i]();
					CHILI_SetWakeup(WUP_GPIO);
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleRegisterGPIOInput(struct gpio_input_args *args)
{
	u8_t                mpin     = args->mpin;
	module_pin_pullup   pullup   = args->pullup;
	module_pin_debounce debounce = args->debounce;
	module_pin_irq      irq      = args->irq;
	int (*callback)(void)        = args->callback;

	u8_t    index;
	GPIO_T *port;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;
	/* pin already registered or used ? */
	if (ModulePinStatus[index].blocked)
		return CA_ERROR_NO_ACCESS;
	/* irq but no callback function defined ? */
	if ((irq != MODULE_PIN_IRQ_OFF) && (callback == NULL))
		return CA_ERROR_INVALID_ARGS;

	port = MGPIO_PORT(ModulePinList[index].portnum);

	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_MODE_INPUT);
	if (pullup == MODULE_PIN_PULLUP_ON)
		GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_PULL_UP);
	else
		GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_DISABLE);
	if (debounce == MODULE_PIN_DEBOUNCE_ON)
		GPIO_ENABLE_DEBOUNCE(port, BITMASK(ModulePinList[index].portbit));
	else
		GPIO_DISABLE_DEBOUNCE(port, BITMASK(ModulePinList[index].portbit));
	if (irq != MODULE_PIN_IRQ_OFF)
	{
		if (irq == MODULE_PIN_IRQ_BOTH)
			GPIO_EnableInt(port, ModulePinList[index].portbit, GPIO_INT_BOTH_EDGE);
		else if (irq == MODULE_PIN_IRQ_RISE)
			GPIO_EnableInt(port, ModulePinList[index].portbit, GPIO_INT_RISING);
		else if (irq == MODULE_PIN_IRQ_FALL)
			GPIO_EnableInt(port, ModulePinList[index].portbit, GPIO_INT_FALLING);
		if (ModulePinList[index].portnum == PN_A)
		{
			NVIC_EnableIRQ(GPA_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_B)
		{
			NVIC_EnableIRQ(GPB_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_C)
		{
			NVIC_EnableIRQ(GPC_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_D)
		{
			NVIC_EnableIRQ(GPD_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_E)
		{
			NVIC_EnableIRQ(GPE_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_F)
		{
			NVIC_EnableIRQ(GPF_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_G)
		{
			NVIC_EnableIRQ(GPG_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_H)
		{
			NVIC_EnableIRQ(GPH_IRQn);
		}
	}
	else
	{
		GPIO_DisableInt(port, ModulePinList[index].portbit);
		/* don't disable NVIC otherwise other interrupts might be compromised */
	}

	ModulePinStatus[index].blocked   = 1;
	ModulePinStatus[index].io        = MODULE_PIN_DIR_IN;
	ModulePinStatus[index].pullup    = pullup;
	ModulePinStatus[index].debounce  = debounce;
	ModulePinStatus[index].isled     = MODULE_PIN_TYPE_GENERIC;
	ModulePinStatus[index].permanent = 0;
	ModulePinStatus[index].irq       = irq;
	ModulePinCallbacks[index]        = callback;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleRegisterGPIOOutput(u8_t mpin, module_pin_type isled)
{
	u8_t    index;
	GPIO_T *port;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;

	/* pin already registered or used ? */
	if (ModulePinStatus[index].blocked)
		return CA_ERROR_NO_ACCESS;

	port = MGPIO_PORT(ModulePinList[index].portnum);

	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_MODE_OUTPUT);
	GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_DISABLE);

	ModulePinStatus[index].blocked   = 1;
	ModulePinStatus[index].io        = MODULE_PIN_DIR_OUT;
	ModulePinStatus[index].pullup    = MODULE_PIN_PULLUP_OFF;
	ModulePinStatus[index].debounce  = MODULE_PIN_DEBOUNCE_OFF;
	ModulePinStatus[index].isled     = isled;
	ModulePinStatus[index].permanent = 0;
	ModulePinStatus[index].irq       = MODULE_PIN_IRQ_OFF;
	ModulePinCallbacks[index]        = NULL;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleRegisterGPIOOutputOD(u8_t mpin, module_pin_type isled)
{
	u8_t    index;
	GPIO_T *port;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;

	/* pin already registered or used ? */
	if (ModulePinStatus[index].blocked)
		return CA_ERROR_NO_ACCESS;

	port = MGPIO_PORT(ModulePinList[index].portnum);

	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_MODE_OPEN_DRAIN);
	GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_DISABLE);

	ModulePinStatus[index].blocked   = 1;
	ModulePinStatus[index].io        = MODULE_PIN_DIR_OUT;
	ModulePinStatus[index].pullup    = MODULE_PIN_PULLUP_OFF;
	ModulePinStatus[index].debounce  = MODULE_PIN_DEBOUNCE_OFF;
	ModulePinStatus[index].isled     = isled;
	ModulePinStatus[index].permanent = 0;
	ModulePinStatus[index].irq       = MODULE_PIN_IRQ_OFF;
	ModulePinCallbacks[index]        = NULL;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleRegisterGPIOSharedInputOutputOD(struct gpio_input_args *args, module_pin_type isled)
{
	u8_t                mpin     = args->mpin;
	module_pin_pullup   pullup   = args->pullup;
	module_pin_debounce debounce = args->debounce;
	module_pin_irq      irq      = args->irq;
	int (*callback)(void)        = args->callback;

	u8_t    index;
	GPIO_T *port;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;
	/* pin already registered or used ? */
	if (ModulePinStatus[index].blocked)
		return CA_ERROR_NO_ACCESS;
	/* irq but no callback function defined ? */
	if ((irq != MODULE_PIN_IRQ_OFF) && (callback == NULL))
		return CA_ERROR_INVALID_ARGS;

	port = MGPIO_PORT(ModulePinList[index].portnum);

	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_MODE_OPEN_DRAIN);
	if (pullup == MODULE_PIN_PULLUP_ON)
		GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_PULL_UP);
	else
		GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_DISABLE);
	if (debounce == MODULE_PIN_DEBOUNCE_ON)
		GPIO_ENABLE_DEBOUNCE(port, BITMASK(ModulePinList[index].portbit));
	else
		GPIO_DISABLE_DEBOUNCE(port, BITMASK(ModulePinList[index].portbit));
	if (irq != MODULE_PIN_IRQ_OFF)
	{
		if (irq == MODULE_PIN_IRQ_BOTH)
			GPIO_EnableInt(port, ModulePinList[index].portbit, GPIO_INT_BOTH_EDGE);
		else if (irq == MODULE_PIN_IRQ_RISE)
			GPIO_EnableInt(port, ModulePinList[index].portbit, GPIO_INT_RISING);
		else if (irq == MODULE_PIN_IRQ_FALL)
			GPIO_EnableInt(port, ModulePinList[index].portbit, GPIO_INT_FALLING);
		if (ModulePinList[index].portnum == PN_A)
		{
			NVIC_EnableIRQ(GPA_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_B)
		{
			NVIC_EnableIRQ(GPB_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_C)
		{
			NVIC_EnableIRQ(GPC_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_D)
		{
			NVIC_EnableIRQ(GPD_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_E)
		{
			NVIC_EnableIRQ(GPE_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_F)
		{
			NVIC_EnableIRQ(GPF_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_G)
		{
			NVIC_EnableIRQ(GPG_IRQn);
		}
		else if (ModulePinList[index].portnum == PN_H)
		{
			NVIC_EnableIRQ(GPH_IRQn);
		}
	}
	else
	{
		GPIO_DisableInt(port, ModulePinList[index].portbit);
		/* don't disable NVIC otherwise other interrupts might be compromised */
	}

	ModulePinStatus[index].blocked   = 1;
	ModulePinStatus[index].io        = MODULE_PIN_DIR_OUT;
	ModulePinStatus[index].pullup    = pullup;
	ModulePinStatus[index].debounce  = debounce;
	ModulePinStatus[index].isled     = isled;
	ModulePinStatus[index].permanent = 0;
	ModulePinStatus[index].irq       = irq;
	ModulePinCallbacks[index]        = callback;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleDeregisterGPIOPin(u8_t mpin)
{
	u8_t    index;
	GPIO_T *port;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;

	port = MGPIO_PORT(ModulePinList[index].portnum);

	/* input, no pullup, no debounce, no irq */
	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_MODE_INPUT);
	GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_DISABLE);
	GPIO_DISABLE_DEBOUNCE(port, BITMASK(ModulePinList[index].portbit));

	ModulePinStatus[index].blocked   = 0;
	ModulePinStatus[index].io        = MODULE_PIN_DIR_IN;
	ModulePinStatus[index].pullup    = MODULE_PIN_PULLUP_OFF;
	ModulePinStatus[index].debounce  = MODULE_PIN_DEBOUNCE_OFF;
	ModulePinStatus[index].isled     = MODULE_PIN_TYPE_GENERIC;
	ModulePinStatus[index].permanent = 0;
	ModulePinStatus[index].irq       = MODULE_PIN_IRQ_OFF;
	ModulePinCallbacks[index]        = NULL;
	GPIO_DisableInt(port, ModulePinList[index].portbit);

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
u8_t BSP_ModuleIsGPIOPinRegistered(u8_t mpin)
{
	u8_t index;
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return 1; /* doesn't exist */
	return ModulePinStatus[index].blocked;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleSetGPIOPin(u8_t mpin, u8_t val)
{
	u8_t index;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;

	/* pin not registered or not output ? */
	if ((!ModulePinStatus[index].blocked) || (ModulePinStatus[index].io != MODULE_PIN_DIR_OUT))
		return CA_ERROR_NO_ACCESS;

	MGPIO_PORTPIN(ModulePinList[index].portnum, ModulePinList[index].portbit) = val;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleSenseGPIOPin(u8_t mpin, u8_t *val)
{
	u8_t index;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;
	/* pin not registered ? */
	if (!ModulePinStatus[index].blocked)
		return CA_ERROR_NO_ACCESS;

	*val = MGPIO_PORTPIN(ModulePinList[index].portnum, ModulePinList[index].portbit);

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleSenseGPIOPinOutput(u8_t mpin, u8_t *val)
{
	u8_t index;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;
	/* pin not registered ? */
	if (!ModulePinStatus[index].blocked)
		return CA_ERROR_NO_ACCESS;
	GPIO_T  *port = MGPIO_PORT(ModulePinList[index].portnum);
	uint32_t mode = port->MODE;
	/* pin not an output? */
	if (((mode >> (2 * ModulePinList[index].portbit)) & 0x03) == 0)
		return CA_ERROR_NO_ACCESS;

	*val = ((port->DOUT) >> ModulePinList[index].portbit) & 1;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleSetGPIOOutputPermanent(u8_t mpin)
{
	u8_t index;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;

	/* pin not registered or not output ? */
	if ((!ModulePinStatus[index].blocked) || (ModulePinStatus[index].io != MODULE_PIN_DIR_OUT))
		return CA_ERROR_NO_ACCESS;

	ModulePinStatus[index].permanent = 1;

	return CA_ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------*
 * See cascoda-bm/cascoda_interface.h for docs                               *
 *---------------------------------------------------------------------------*/
ca_error BSP_ModuleReadVoltsPin(u8_t mpin, u32_t *val)
{
	u8_t    index;
	GPIO_T *port;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;
	/* pin already registered or used ? */
	if (ModulePinStatus[index].blocked)
		return CA_ERROR_NO_ACCESS;
	/* pin has no ADC channel attached to it ? */
	if (ModulePinList[index].adc_channel == P_NA)
		return CA_ERROR_NO_ACCESS;

	port = MGPIO_PORT(ModulePinList[index].portnum);

	CHILI_ModuleSetMFP(ModulePinList[index].portnum, ModulePinList[index].portbit, PMFP_ADC);
	GPIO_DISABLE_DIGITAL_PATH(port, BITMASK(ModulePinList[index].portbit));
	GPIO_SetPullCtl(port, BITMASK(ModulePinList[index].portbit), GPIO_PUSEL_DISABLE);

	/* ADC value for channel = 0, reference = AVDD 3.3V */
	*val = CHILI_ADCConversion(ModulePinList[index].adc_channel, SYS_VREFCTL_VREF_AVDD);

	CHILI_ModuleSetMFP(ModulePinList[index].portnum, ModulePinList[index].portbit, PMFP_GPIO);
	GPIO_ENABLE_DIGITAL_PATH(port, BITMASK(ModulePinList[index].portbit));
	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_MODE_INPUT);

	return CA_ERROR_SUCCESS;
}
