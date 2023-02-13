/*
 * MikroSDK - MikroE Software Development Kit
 * Copyright© 2020 MikroElektronika d.o.o.
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
 * \brief This file contains API for Thermo 3 Click driver.
 *
 * \addtogroup thermo3 Thermo 3 Click Driver
 * @{
 */
// ----------------------------------------------------------------------------

#ifndef THERMO3_DRV_H
#define THERMO3_DRV_H

#include "drv_digital_in.h"
#include "drv_digital_out.h"
#include "drv_i2c_master.h"

// --------------------------------------------------------------- PUBLIC TYPES
/**
 * \defgroup type Types
 * \{
 */

/**
 * @brief Click ctx object definition.
 */
typedef struct
{
	// Input pins

	digital_in_t al;

	// Modules

	i2c_master_t i2c;

	// ctx variable

	uint8_t slave_address;

} thermo3_t;

/**
 * @brief Click configuration structure definition.
 */
typedef struct
{
	// Communication gpio pins

	pin_name_t scl;
	pin_name_t sda;

	// Additional gpio pins

	pin_name_t al;

	// static variable

	uint32_t i2c_speed;
	uint8_t  i2c_address;

} thermo3_cfg_t;

/** \} */ // End types group

#endif // THERMO3_DRV_H

/** \} */ // End public_function group
/// \}    // End click Driver group
/*! @} */
// ------------------------------------------------------------------------- END
