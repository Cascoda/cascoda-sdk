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
 * Module Pin and GPIO Mapping
*/
/* System */
#include <stdio.h>
/* Platform */
#include "Nano100Series.h"
#include "adc.h"
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
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"
#ifdef USE_USB
#include "cascoda-bm/cascoda_usbhid.h"
#include "cascoda_chili_usb.h"
#endif /* USE_USB */

/******************************************************************************/
/****** Dynamically configurable Ports / Pins                            ******/
/******************************************************************************/

enum
{
	PIN_SWITCH      = 9,
	PIN_LED_GREEN   = 101,
	PIN_LED_RED     = 102,
	PIN_USB_PRESENT = 103,

/** number of dynamically available module i/o pins */
#if defined(USE_UART)
	NUM_MODULEPINS = 11,
#else
	NUM_MODULEPINS = 13,
#endif /* USE_UART */
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
	u8_t                blocked : 1;  /* pin is blocked by other functionality */
	module_pin_dir      io : 1;       /* pin type (input/output) */
	module_pin_pullup   pullup : 1;   /* inputs only: 0: none, 1: pullup */
	module_pin_debounce debounce : 1; /* inputs only:	0: no debounce, 1: debounce */
	module_pin_type     isled : 1;    /* 1: pin has led attached */
	module_pin_irq irq : 2; /* inputs only:	0: no irq, 1: irq falling edge, 2: irq rising edge, 3: irq both edges */
};

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
const struct ModuleSpecialPins BSP_ModuleSpecialPins = {
    MSP_DEFAULT,
    .SWITCH      = PIN_SWITCH,
    .LED_GREEN   = PIN_LED_GREEN,
    .LED_RED     = PIN_LED_RED,
    .USB_PRESENT = PIN_USB_PRESENT,
};

/******************************************************************************/
/****** Static Variables                                                 ******/
/******************************************************************************/
static struct pinstatus ModulePinStatus[NUM_MODULEPINS] = {0};
static int (*ModulePinCallbacks[NUM_MODULEPINS])(void);

static const struct pinlist ModulePinList[NUM_MODULEPINS] = {
    /* module pin */
    /* port PX.Y */
    /* adc channel */
    {9, PN_A, 9, P_NA}, /* PA.9 (SWITCH) */
#ifndef USE_UART
    {17, PN_B, 4, P_NA}, /* PB.4 */
#endif
    {18, PN_A, 15, P_NA}, /* PA.15 */
    {19, PN_B, 8, P_NA},  /* PB.8 */
#ifndef USE_UART
    {20, PN_B, 5, P_NA}, /* PB.5 */
#endif
    {21, PN_A, 1, 1},                /* PA.1 */
    {22, PN_A, 2, 2},                /* PA.2 */
    {23, PN_A, 3, 3},                /* PA.3 */
    {24, PN_A, 4, 4},                /* PA.4 (I2C0_SDA) */
    {25, PN_A, 5, 5},                /* PA.5 (I2C0_SCL) */
    {PIN_LED_GREEN, PN_A, 13, P_NA}, /* PA.13 (LED_G), virtual */
    {PIN_LED_RED, PN_A, 14, P_NA},   /* PA.14:(LED_R), virtual */
#if (CASCODA_CHILI_REV == 3)
    {PIN_USB_PRESENT, PN_C, 7, P_NA}, /* PC.7:(USB_PRESENT), virtual */
#else
    {PIN_USB_PRESENT, PN_B, 15, P_NA}, /* PB.15:(USB_PRESENT), virtual */
#endif
};

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Gets List Index from Module Pin
 *******************************************************************************
 * \param mpin     - module pin number
 *******************************************************************************
 * \return list index or P_NA
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets MFP Functionality
 *******************************************************************************
 * \param portnum  - returned port number
 * \param portbit  - returned port b
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModuleSetMFP(enPortnum portnum, u8_t portbit, u8_t func)
{
	u32_t mask, data;
	if (portbit < 8)
	{
		mask = (0x7 << 4 * portbit);
		data = (func << 4 * portbit);
		if (portnum == PN_A)
			SYS->PA_L_MFP = (SYS->PA_L_MFP & (~mask)) | data;
		else if (portnum == PN_B)
			SYS->PB_L_MFP = (SYS->PB_L_MFP & (~mask)) | data;
		else if (portnum == PN_C)
			SYS->PC_L_MFP = (SYS->PC_L_MFP & (~mask)) | data;
		else if (portnum == PN_D)
			SYS->PD_L_MFP = (SYS->PD_L_MFP & (~mask)) | data;
		else if (portnum == PN_E)
			SYS->PE_L_MFP = (SYS->PE_L_MFP & (~mask)) | data;
		else if (portnum == PN_F)
			SYS->PF_L_MFP = (SYS->PF_L_MFP & (~mask)) | data;
	}
	else if (portbit < 16)
	{
		mask = (0x7 << 4 * (portbit - 8));
		data = (func << 4 * (portbit - 8));
		if (portnum == PN_A)
			SYS->PA_H_MFP = (SYS->PA_H_MFP & (~mask)) | data;
		else if (portnum == PN_B)
			SYS->PB_H_MFP = (SYS->PB_H_MFP & (~mask)) | data;
		else if (portnum == PN_C)
			SYS->PC_H_MFP = (SYS->PC_H_MFP & (~mask)) | data;
		else if (portnum == PN_D)
			SYS->PD_H_MFP = (SYS->PD_H_MFP & (~mask)) | data;
		else if (portnum == PN_E)
			SYS->PE_H_MFP = (SYS->PE_H_MFP & (~mask)) | data;
		/* no PF_H_MFP */
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-configure GPIOs for PowerDown
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePowerDownGPIOs(void)
{
	u8_t    i;
	GPIO_T *port;

	for (i = 0; i < NUM_MODULEPINS; ++i)
	{
		if (ModulePinStatus[i].io == MODULE_PIN_DIR_OUT)
		{
			port = MGPIO_PORT(ModulePinList[i].portnum);
			GPIO_SetMode(port, BITMASK(ModulePinList[i].portbit), GPIO_PMD_INPUT);
			/* use pull-up for LEDs otherwise light-dependent photodiode effect! */
			if (ModulePinStatus[i].isled == MODULE_PIN_TYPE_LED)
			{
				MGPIO_PORTPIN(ModulePinList[i].portnum, ModulePinList[i].portbit) = 1;
				GPIO_ENABLE_PULL_UP(port, BITMASK(ModulePinList[i].portbit));
			}
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Re-configure GPIOs for PowerUp
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePowerUpGPIOs(void)
{
	u8_t    i;
	GPIO_T *port;

	for (i = 0; i < NUM_MODULEPINS; ++i)
	{
		if (ModulePinStatus[i].io == MODULE_PIN_DIR_OUT)
		{
			port = MGPIO_PORT(ModulePinList[i].portnum);
			if (ModulePinStatus[i].isled == MODULE_PIN_TYPE_LED)
			{
				GPIO_DISABLE_PULL_UP(port, BITMASK(ModulePinList[i].portbit));
			}
			GPIO_SetMode(port, BITMASK(ModulePinList[i].portbit), GPIO_PMD_OUTPUT);
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief ISR: Module Pin Interrupt Handling
 *******************************************************************************
 ******************************************************************************/
void CHILI_ModulePinIRQHandler(enPortnum portnum)
{
	GPIO_T *port;
	u8_t    i;

	port = MGPIO_PORT(portnum);

	for (i = 0; i < NUM_MODULEPINS; ++i)
	{
		if ((ModulePinStatus[i].irq) && (ModulePinList[i].portnum == portnum))
		{
			if (port->ISRC & BITMASK(ModulePinList[i].portbit))
			{
				/* clear interrupt */
				GPIO_CLR_INT_FLAG(port, BITMASK(ModulePinList[i].portbit));
				/* call callback if registered */
				if (ModulePinCallbacks[i])
					ModulePinCallbacks[i]();
			}
		}
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Registers GPIO Functionality for Module Pin Input
 *******************************************************************************
 * \param mpin  - module pin number
 * \param pullup - none/pullup/pulldown for input
 * \param debounce - enable debounce for input
 * \param irq - enable interupt for input
 * \param callback - pointer to ISR for input
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
ca_error BSP_ModuleRegisterGPIOInput(u8_t                mpin,
                                     module_pin_pullup   pullup,
                                     module_pin_debounce debounce,
                                     module_pin_irq      irq,
                                     int (*callback)(void))
{
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

	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_PMD_INPUT);
	if (pullup == MODULE_PIN_PULLUP_ON)
		GPIO_ENABLE_PULL_UP(port, BITMASK(ModulePinList[index].portbit));
	else
		GPIO_DISABLE_PULL_UP(port, BITMASK(ModulePinList[index].portbit));
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
		if (ModulePinList[index].portnum > PN_C)
		{
			NVIC_EnableIRQ(GPDEF_IRQn);
		}
		else
		{
			NVIC_EnableIRQ(GPABC_IRQn);
		}
	}
	else
	{
		GPIO_DisableInt(port, ModulePinList[index].portbit);
		/* don't disable NVIC otherwise other interupts might be compromised */
	}

	ModulePinStatus[index].blocked  = 1;
	ModulePinStatus[index].io       = MODULE_PIN_DIR_IN;
	ModulePinStatus[index].pullup   = pullup;
	ModulePinStatus[index].debounce = debounce;
	ModulePinStatus[index].isled    = MODULE_PIN_TYPE_GENERIC;
	ModulePinStatus[index].irq      = irq;
	ModulePinCallbacks[index]       = callback;

	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Registers GPIO Functionality for Module Pin Output
 *******************************************************************************
 * \param mpin  - module pin number
 * \param isled - pin is attached to led
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
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

	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_PMD_OUTPUT);
	GPIO_DISABLE_PULL_UP(port, BITMASK(ModulePinList[index].portbit));

	ModulePinStatus[index].blocked  = 1;
	ModulePinStatus[index].io       = MODULE_PIN_DIR_OUT;
	ModulePinStatus[index].pullup   = MODULE_PIN_PULLUP_OFF;
	ModulePinStatus[index].debounce = MODULE_PIN_DEBOUNCE_OFF;
	ModulePinStatus[index].isled    = isled;
	ModulePinStatus[index].irq      = MODULE_PIN_IRQ_OFF;
	ModulePinCallbacks[index]       = NULL;

	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Unregisters GPIO Functionality for Module Pin to Default Settings
 *******************************************************************************
 * \param mpin  - module pin number
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
ca_error BSP_ModuleDeregisterGPIOPin(u8_t mpin)
{
	u8_t    index;
	GPIO_T *port;

	/* pin in dynamic list ? */
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return CA_ERROR_NO_ACCESS;

	port = MGPIO_PORT(ModulePinList[index].portnum);

	/* input, no pullup, no debounce, no irq */
	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_PMD_INPUT);
	GPIO_DISABLE_PULL_UP(port, BITMASK(ModulePinList[index].portbit));
	GPIO_DISABLE_DEBOUNCE(port, BITMASK(ModulePinList[index].portbit));

	ModulePinStatus[index].blocked  = 0;
	ModulePinStatus[index].io       = MODULE_PIN_DIR_IN;
	ModulePinStatus[index].pullup   = MODULE_PIN_PULLUP_OFF;
	ModulePinStatus[index].debounce = MODULE_PIN_DEBOUNCE_OFF;
	ModulePinStatus[index].isled    = MODULE_PIN_TYPE_GENERIC;
	ModulePinStatus[index].irq      = MODULE_PIN_IRQ_OFF;
	ModulePinCallbacks[index]       = NULL;
	GPIO_DisableInt(port, ModulePinList[index].portbit);

	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks if a Module Pin is already registered / used
 *******************************************************************************
 * \param mpin  - module pin number
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
u8_t BSP_ModuleIsGPIOPinRegistered(u8_t mpin)
{
	u8_t index;
	if (((index = CHILI_ModuleGetIndexFromPin(mpin))) == P_NA)
		return 1; /* doesn't exist */
	return ModulePinStatus[index].blocked;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sets Module Pin GPIO Output Value
 *******************************************************************************
 * \param mpin  - module pin
 * \param val - output value
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Senses GPIO Input Value of Module Pin
 *******************************************************************************
 * \param mpin  - module pin
 * \param val - output value
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reads ADC Conversion Value on Module Pin
 *******************************************************************************
 * \param mpin  - module pin
 * \param val - 32-bit conversion value
 *******************************************************************************
 * \return status
 *******************************************************************************
 ******************************************************************************/
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
	GPIO_DISABLE_PULL_UP(port, BITMASK(ModulePinList[index].portbit));

	/* ADC value for channel = 0, reference = AVDD 3.3V */
	*val = CHILI_ADCConversion(ModulePinList[index].adc_channel, ADC_REFSEL_POWER);

	CHILI_ModuleSetMFP(ModulePinList[index].portnum, ModulePinList[index].portbit, PMFP_GPIO);
	GPIO_ENABLE_DIGITAL_PATH(port, BITMASK(ModulePinList[index].portbit));
	GPIO_SetMode(port, BITMASK(ModulePinList[index].portbit), GPIO_PMD_INPUT);

	return CA_ERROR_SUCCESS;
}
