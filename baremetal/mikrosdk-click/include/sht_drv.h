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
 * \brief This file contains API for SHT Click driver.
 *
 * \addtogroup sht SHT Click Driver
 * @{
 */
// ----------------------------------------------------------------------------

#ifndef SHT_DRV_H
#define SHT_DRV_H

#include "drv_digital_in.h"
#include "drv_digital_out.h"
#include "drv_i2c_master.h"

#define SHT_I2C_ADDR0 0x44
#define SHT_I2C_ADDR1 0x45

/**
 * \defgroup measu_freq Meausurement frequency
 * \{
 */

#define SHT_MPS_05 0x20 // 0.5 measurements per second
#define SHT_MPS_1 0x21  // 1 measurements per second
#define SHT_MPS_2 0x22  // 2 measurements per second
#define SHT_MPS_4 0x23  // 4 measurements per second
#define SHT_MPS_10 0x27 // 10 measurements per second
/** \} */

/**
 * \defgroup repeat Repeatability
 * \{
 */

#define SHT_RPT_HIGH 0   // High repeatability
#define SHT_RPT_MEDIUM 1 // Medium repeatability
#define SHT_RPT_LOW 2    // Low repeatability

#define SHT_STR_ENABLE 0x2C  // Stretching enabled
#define SHT_STR_DISABLE 0x24 // Stretching disabled
/** \} */

/**
 * \defgroup comms Commands
 * \{
 */

#define SHT_CRC_POLYNOMIAL 0x31
#define SHT_FETCH_DATA 0xE000
#define SHT_PERIODIC_ART 0x2B32
#define SHT_BREAK 0x3093
#define SHT_SOFT_RESET 0x30A2
#define SHT_HEATER 0x30
#define SHT_READ_STATUS 0xF32D
#define SHT_CLEAR_STATUS1 0x3041
/** \} */

/** \} */ // End group macro
// --------------------------------------------------------------- PUBLIC TYPES
/**
 * \defgroup type Types
 * \{
 */

typedef struct
{
	uint8_t clk_stretching;
	uint8_t repeatability;
	uint8_t mps;

} drv_variables_t;

/**
 * @brief Click ctx object definition.
 */
typedef struct
{
	// Output pins

	digital_out_t rst;
	// Input pins

	digital_in_t int_pin;

	// Modules

	i2c_master_t i2c;

	// ctx variable

	uint8_t slave_address;

	// driver variables

	drv_variables_t vars;

} sht_t;

/**
 * @brief Click configuration structure definition.
 */
typedef struct
{
	// Communication gpio pins

	pin_name_t scl;
	pin_name_t sda;

	// Additional gpio pins

	pin_name_t rst;
	pin_name_t int_pin;

	// static variable

	uint32_t i2c_speed;
	uint8_t  i2c_address;

	//driver variable's config

	drv_variables_t vars_cfg;

} sht_cfg_t;

/** \} */ // End types group
// ----------------------------------------------- PUBLIC FUNCTION DECLARATIONS

/**
 * \defgroup public_function Public function
 * \{
 */

#endif // SHT_DRV_H

/** \} */ // End public_function group
/// \}    // End click Driver group
/*! @} */
// ------------------------------------------------------------------------- END
