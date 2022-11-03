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
 * \brief This file contains API for THERMO Click driver.
 *
 * \addtogroup thermo THERMO Click Driver
 * @{
 */
// ----------------------------------------------------------------------------

#ifndef THERMO3_H
#define THERMO3_H

#include "cascoda-bm/cascoda_types.h"
#include "drv_digital_in.h"
#include "drv_digital_out.h"
#include "drv_i2c_master.h"

// -------------------------------------------------------------- PUBLIC MACROS
/**
 * \defgroup macros Macros
 * \{
 */

/**
 * \defgroup map_mikrobus MikroBUS
 * \{
 */
/* This is for code conventional purpose and 
	has no effect on anything apart from al pin */
#define THERMO3_MAP_MIKROBUS(cfg) \
	cfg.scl = 2;                  \
	cfg.sda = 4;                  \
	cfg.al  = 5
/** \} */

/**
 * \defgroup error_code Error Code
 * \{
 */
#define THERMO3_RETVAL uint8_t

#define THERMO3_OK 0x00
#define THERMO3_INIT_ERROR 0xFF
/** \} */

/**
 * \defgroup Device I2C address
 * \{
 */
#define THERMO3_I2C_ADDR 0x48 // Thermo3 I2C address (ADD0 pin is connected to ground)

//#define THERMO3_I2C_ADDR    0x49      // Thermo3 I2C address (ADD0 pin is connected to VCC)
/** \} */

/** \} */ // End group macro
// --------------------------------------------------------------- PUBLIC TYPES
/**
 * \defgroup type Types
 * \{
 */

/* configuration register bit mapping */
#define MIKROSDK_THERMO3_CONFIG_ONESHOT 0x80
#define MIKROSDK_THERMO3_CONFIG_SHUTDOWN 0x01

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
// ----------------------------------------------- PUBLIC FUNCTION DECLARATIONS

/**
 * \defgroup public_function Public function
 * \{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Config Object Initialization function.
 *
 * @param cfg  Click configuration structure.
 *
 * This function initializes click configuration structure to init state.
 * @note All used pins will be set to unconnected state.
 */
void thermo3_cfg_setup(thermo3_cfg_t *cfg);

/**
 * @brief Initialization function.
 *
 * @param ctx Click object.
 * @param cfg Click configuration structure.
 * 
 * This function initializes all necessary pins and peripherals used for this click.
 */
THERMO3_RETVAL thermo3_init(thermo3_t *ctx, thermo3_cfg_t *cfg);

/**
 * @brief Click Default Configuration function.
 *
 * @param ctx  Click object.
 *
 * This function executes default configuration for Thermo3 click.
 */
void thermo3_default_cfg(thermo3_t *ctx);

/**
 * @brief Generic write function.
 *
 * @param ctx          Click object.
 * @param reg          Register address.
 * @param data_buf     Data buf to be written.
 * @param len          Number of the bytes in data buf.
 *
 * This function writes data to the desired register.
 */
void thermo3_generic_write(thermo3_t *ctx, uint8_t reg, uint8_t *data_buf, uint8_t len);

/**
 * @brief Generic read function.
 *
 * @param ctx          Click object.
 * @param tx_buf       Input data buf.
 * @param wlen         Number of the bytes to be write.
 * @param rx_buf       Output data buf.
 * @param rlen         Number of the bytes to be read
 *
 * This function reads data from the desired register.
 */
void thermo3_generic_read(thermo3_t *ctx, uint8_t *tx_buf, uint8_t wlen, uint8_t *rx_buf, uint8_t rlen);

/**
 * @brief Gets temperature.
 *
 *
 * This function gets temperature data from TMP102 sensor.
 */
u16_t get_temperature();

/******************************************************************************/
/***************************************************************************/ /**
 * \brief THERMO3: Initialise the temperature sensor
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
uint8_t MIKROSDK_THERMO3_Initialise();

#ifdef __cplusplus
}
#endif
#endif // _THERMO3_H_

/** \} */ // End public_function group
/// \}    // End click Driver group
/*! @} */
// ------------------------------------------------------------------------- END
