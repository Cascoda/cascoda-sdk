/****************************************************************************
**
** Copyright (C) 2022 MikroElektronika d.o.o.
** Contact: https://www.mikroe.com/contact
**
** This file is part of the mikroSDK package
**
** Commercial License Usage
**
** Licensees holding valid commercial NECTO compilers AI licenses may use this
** file in accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The MikroElektronika Company.
** For licensing terms and conditions see
** https://www.mikroe.com/legal/software-license-agreement.
** For further information use the contact form at
** https://www.mikroe.com/contact.
**
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used for
** non-commercial projects under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** OF MERCHANTABILITY, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
** DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
** OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
/*!
 * @file  hal_gpio.c
 * @brief This file contains all the functions prototypes for the GPIO library.
 */

#include "hal_gpio.h"
#include <stdint.h>
#include <stdio.h>
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "M2351.h"
#include "cascoda_chili_config.h"
#include "cascoda_chili_gpio.h"

void hal_gpio_configure_pin(hal_gpio_pin_t *pin, hal_pin_name_t name, hal_gpio_direction_t direction)
{
	u8_t status;
	status    = BSP_ModuleIsGPIOPinRegistered(name);
	pin->base = CHILI_ModuleGetPortNumFromPin(name);
	pin->mask = CHILI_ModuleGetPortBitFromPin(name);

	if (direction == HAL_GPIO_DIGITAL_INPUT)
	{
		status = BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){
		    (u8_t)name, MODULE_PIN_PULLUP_OFF, MODULE_PIN_DEBOUNCE_ON, MODULE_PIN_IRQ_OFF, NULL});
		if (status)
		{
			ca_log_warn("hal_gpio_configure_pin() input Error; write status: %02X", status);
		}
	}

	else if (direction == HAL_GPIO_DIGITAL_OUTPUT)
	{
		status = BSP_ModuleRegisterGPIOOutput(name, MODULE_PIN_TYPE_GENERIC);
		if (status)
		{
			ca_log_warn("hal_gpio_configure_pin() output Error; write status: %02X", status);
		}
	}
}

void hal_gpio_deregister_pin(hal_gpio_pin_t *pin)
{
	u8_t status;
	u8_t mpin = CHILI_ModuleGetPinFromPort(pin->base, pin->mask);
	status    = BSP_ModuleIsGPIOPinRegistered(mpin);

	if (status)
	{
		status = BSP_ModuleDeregisterGPIOPin(mpin);
	}
	else
	{
		ca_log_warn("hal_gpio_deregister_pin() Error; write status: %02X", status);
	}
}

uint8_t hal_gpio_read_pin_input(hal_gpio_pin_t *pin)
{
	uint8_t val  = 0;
	u8_t    mpin = CHILI_ModuleGetPinFromPort(pin->base, pin->mask);
	u8_t    status;

	status = BSP_ModuleSenseGPIOPin(mpin, &val);

	if (status)
	{
		ca_log_warn("hal_gpio_read_pin_input() Error; write status: %02X", status);
	}
	return val;
}

uint8_t hal_gpio_read_pin_output(hal_gpio_pin_t *pin)
{
	uint8_t *val;
	u8_t     mpin = CHILI_ModuleGetPinFromPort(pin->base, pin->mask);

	u8_t status;

	status = BSP_ModuleSenseGPIOPin(mpin, val);

	if (status)
	{
		ca_log_warn("hal_gpio_read_pin_output() Error; write status: %02X", status);
	}
	return *val;
}

void hal_gpio_write_pin_output(hal_gpio_pin_t *pin, uint8_t value)
{
	u8_t mpin = CHILI_ModuleGetPinFromPort(pin->base, pin->mask);

	u8_t status;
	status = BSP_ModuleSetGPIOPin(mpin, value);

	if (status)
	{
		ca_log_warn("hal_gpio_write_pin_output() Error; write status: %02X", status);
	}
}

void hal_gpio_toggle_pin_output(hal_gpio_pin_t *pin)
{
	uint8_t *val;
	u8_t     mpin = CHILI_ModuleGetPinFromPort(pin->base, pin->mask);

	u8_t status;
	status = BSP_ModuleSenseGPIOPin(mpin, val);
	if (status)
	{
		ca_log_warn("hal_gpio_toggle_pin_output() Error; write status: %02X", status);
	}

	status = BSP_ModuleSetGPIOPin(mpin, ~*val);

	if (status)
	{
		ca_log_warn("hal_gpio_toggle_pin_output() Error; write status: %02X", status);
	}
}

void hal_gpio_set_pin_output(hal_gpio_pin_t *pin)
{
	u8_t status;

	u8_t *val;

	u8_t mpin = CHILI_ModuleGetPinFromPort(pin->base, pin->mask);

	status = BSP_ModuleSetGPIOPin(mpin, 1); //clear pin output to 1

	if (status)
	{
		ca_log_warn("hal_gpio_set_pin_output() Error; write status: %02X", status);
	}
}

void hal_gpio_clear_pin_output(hal_gpio_pin_t *pin)
{
	u8_t status;

	u8_t mpin = CHILI_ModuleGetPinFromPort(pin->base, pin->mask);

	status = BSP_ModuleSetGPIOPin(mpin, 0); //clear pin output to 0

	if (status)
	{
		ca_log_warn("hal_gpio_clear_pin_output() Error; write status: %02X", status);
	}
}

/*---------------------Unused Functions--------------------------------*/

void hal_gpio_configure_port(hal_gpio_port_t *    port,
                             hal_port_name_t      name,
                             hal_gpio_mask_t      mask,
                             hal_gpio_direction_t direction)
{
	//hal_ll_gpio_configure_port( port, name, mask, direction );
}

hal_port_size_t hal_gpio_read_port_input(hal_gpio_port_t *port)
{
	//return hal_ll_gpio_read_port_input( port );
}

hal_port_size_t hal_gpio_read_port_output(hal_gpio_port_t *port)
{
	//return hal_ll_gpio_read_port_output( port );
}

void hal_gpio_write_port_output(hal_gpio_port_t *port, hal_port_size_t value)
{
	//hal_ll_gpio_write_port_output( port, value );
}

// ------------------------------------------------------------------------- END
