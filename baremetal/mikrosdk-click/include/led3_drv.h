/****************************************************************************
** Copyright (C) 2020 MikroElektronika d.o.o.
** Contact: https://www.mikroe.com/contact
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
** DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
**  USE OR OTHER DEALINGS IN THE SOFTWARE.
****************************************************************************/

/*!
 * @file led3_drv.h
 * @brief This file contains API for LED3 Click Driver.
 */

#ifndef LED3_DRV_H
#define LED3_DRV_H

#include "drv_digital_in.h"
#include "drv_digital_out.h"
#include "drv_i2c_master.h"

/**
 * @brief CLICK interface port for I2C
 * 
 */
#define I2C_PORTNUM 1 // CLICK interface port for I2C

/**
 * @brief LED3 device address setting.
 * @details Specified setting for device slave address selection of
 * LED3 Click driver.
 */
#define LED3_SLAVE_ADDR 0x38

/**
 * @brief LED3 Click context object.
 * @details Context object definition of LED3 Click driver.
 */
typedef struct
{
	// Modules
	i2c_master_t i2c; /**< I2C driver object. */
	                  // MODIFIED: excluded uart
	                  //    uart_t uart;           /**< UART driver object. */

	// I2C slave address
	uint8_t slave_address; /**< Device slave address (used for I2C driver). */

} led3_t;

/**
 * @brief LED3 Click configuration object.
 * @details Configuration object definition of LED3 Click driver.
 */
typedef struct
{
	pin_name_t scl; /**< Clock pin descriptor for I2C driver. */
	pin_name_t sda; /**< Bidirectional data pin descriptor for I2C driver. */

	uint32_t i2c_speed;   /**< I2C serial speed. */
	uint8_t  i2c_address; /**< I2C slave address. */

} led3_cfg_t;

#endif // LED3_DRV_H

/*! @} */ // led3

// ------------------------------------------------------------------------ END