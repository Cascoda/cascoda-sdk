/*
 * MikroSDK - MikroE Software Development Kit
 * Copyright� 2020 MikroElektronika d.o.o.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
 * \file
 *
 * \brief This file contains API for Fan Click driver.
 *
 * \addtogroup fan Fan Click Driver
 * @{
 */
// ----------------------------------------------------------------------------

#ifndef FAN_DRV_H
#define FAN_DRV_H

#include "drv_digital_in.h"
#include "drv_digital_out.h"
#include "drv_i2c_master.h"

// --------------------------------------------------------------- PUBLIC TYPES
/**
 * \defgroup type Types
 * \{
 */

/**
 * @brief Click context object definition.
 */
typedef struct
{
	digital_in_t int_pin;       /**< Interrupt event pin object. >*/
	i2c_master_t i2c;           /**< Communication module object. >*/
	uint8_t      slave_address; /**< Device slave address. >*/
} fan_t;

/**
 * @brief Click configuration structure definition.
 */
typedef struct
{
	pin_name_t scl;         /**< Module clock pin name. >*/
	pin_name_t sda;         /**< Module data pin name. >*/
	pin_name_t int_pin;     /**< Interrupt event pin name. >*/
	uint32_t   i2c_speed;   /**< Module speed. >*/
	uint8_t    i2c_address; /**< Device slave address. >*/
} fan_cfg_t;

#endif // FAN_DRV_H

/** \} */ // End public_function group
/// \}    // End click Driver group
/*! @} */
// ------------------------------------------------------------------------- END