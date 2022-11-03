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

#ifndef SHT_H
#define SHT_H

#include "cascoda-bm/cascoda_sensorif.h"
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
 *  Only RST and INT pins matter  **/
#define SHT_MAP_MIKROBUS(cfg) \
	cfg.scl     = 2;          \
	cfg.sda     = 4;          \
	cfg.rst     = 5;          \
	cfg.int_pin = 6;
/** \} */

/**
 * \defgroup error_code Error Code
 * \{
 */
#define SHT_RETVAL uint8_t

#define SHT_OK 0x00
#define SHT_INIT_ERROR 0xFF
/** \} */

#define SHT_I2C_ADDR0 0x44
#define SHT_I2C_ADDR1 0x45

/**
 * \defgroup measu_freq Meausurement frequency
 * \{
 */

// 0.5 measurements per second
#define SHT_MPS_05 0x20

// 1 measurements per second
#define SHT_MPS_1 0x21

// 2 measurements per second
#define SHT_MPS_2 0x22

// 4 measurements per second
#define SHT_MPS_4 0x23

// 10 measurements per second
#define SHT_MPS_10 0x27
/** \} */

/**
 * \defgroup repeat Repeatability
 * \{
 */

// High repeatability
#define SHT_RPT_HIGH 0

// Medium repeatability
#define SHT_RPT_MEDIUM 1

// Low repeatability
#define SHT_RPT_LOW 2

// Stretching enabled
#define SHT_STR_ENABLE 0x2C

// Stretching disabled
#define SHT_STR_DISABLE 0x24
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Config Object Initialization function.
 *
 * @param cfg  Click configuration structure.
 *
 * Description: This function initializes click configuration structure to init state.
 * @note All used pins will be set to unconnected state.
 */
void sht_cfg_setup(sht_cfg_t *cfg);

/**
 * @brief Initialization function.
 * 
 * @param ctx Click object.
 * @param cfg Click configuration structure.
 * 
 * Description: This function initializes all necessary pins and peripherals used for this click.
 */
SHT_RETVAL sht_init(sht_t *ctx, sht_cfg_t *cfg);

/**
 * @brief Generic write function.
 *
 * @param ctx          Click object.
 * @param reg          Register address.
 * @param data_buf     Data buf to be written.
 * @param len          Number of the bytes in data buf.
 *
 * Description: This function writes data to the desired register.
 */
void sht_generic_write(sht_t *ctx, uint8_t reg, uint8_t *data_buf, uint8_t len);

/**
 * @brief Generic read function.
 *
 * @param ctx          Click object.
 * @param reg          Register address.
 * @param data_buf     Output data buf
 * @param len          Number of the bytes to be read
 *
 * Description: This function reads data from the desired register.
 */
void sht_generic_read(sht_t *ctx, uint8_t *reg, uint8_t *data_buf, uint8_t len);

/**
  * @brief Resets settings
  *
  * @param ctx          Click object.
  * 
  * Description: Calling of this function will reset all of the settings to defaults.
  */
void sht_reset(sht_t *ctx);

/**
  * @brief Resets device
  *
  * @param ctx          Click object.
  * 
  * Description: Hardware reset of the device. 
  */
void sht_hw_reset(sht_t *ctx);

/**
  * @brief Int status
  *
  * @param ctx          Click object.
  * 
  * Description: Gets int_pin status. 
  */
uint8_t sht_int_get(sht_t *ctx);

/**
  * @brief Int status
  *
  * @param ctx          Click object.
  * @param state        Pin state value.
  * 
  * Description: sets rst pin to the desired value. 
  */
void sht_rst_set(sht_t *ctx, uint8_t state);

/**
 * @brief Sets the clock stretching state
 *
 * @param ctx                 Click object.
 * @param clk_stretching      ( true / false )
 * 
 * Description: When a clock stretching is disabled, the sensor responds to a read header
 * with a not acknowledge (NACK), if no data is present. Otherwise sensor
 * responds to a read header with an ACK and subsequently pulls down the SCL
 * line. The SCL line is pulled down until the measurement is complete. As soon
 * as the measurement is complete, the sensor releases the SCL line and sends
 * the measurement results. By default stretching is disabled after
 * initialization.
 *
 * @note
 * This setting have influence on measurmet in single shot mode.
 */
void sht_set_clk_strecth(sht_t *ctx, uint8_t clk_stretching);

/**
 * @brief <h3>Sets the repeatability value</h3>
 *
 * @param ctx                 Click object.
 * @param repeatability       ( RPT_HIGH / RPT_MEDIUM / RPT_LOW )
 * 
 * 
 * Description: Repeatability setting influences the measurement duration and thus the
 * overall energy consumption of the sensor. RPT_MEDIUM is default value after
 * initialization.
 */
void sht_set_repeats(sht_t *ctx, uint8_t repeatability);

/**
 * @brief Measurements per Second
 *
 * @param ctx                     Click object.
 * @param measure_per_second      ( MPS_05 / MPS_1 / MPS_2 / MPS_4 / MPS_10 )
 * 
 * Description: Sets the measurement per second value used for periodic type of
 * measurement.
 * 
 *    MPS_05 - 1 measurement on every 2 seconds
 *    MPS_1 - 1 measurement per second
 *    MPS_2 - 2 measurements per second
 *    MPS_4 - 4 measurements per second
 *    MPS_10 - 10 measurements per second
 * 
 * @note
 * Initialization sets the default value to MPS_1
 *
 * @warning
 * At the highest mps setting self-heating of the sensor might occur.
 */
void sht_set_mps(sht_t *ctx, uint8_t measure_per_second);

/**
 * @brief Single Shot Temperature Measurement
 * 
 * @return Temperature in ( C )
 * 
 * Description: Returns temperature measurement in single shot mode.
 */
float sht_temp_ss();

/**
 * @brief Single Shot Humidity Measurement
 *
 * @return Humidity in ( % ) 
 * 
 * Description: Returns humidity measurement in single shot mode.
 */
float sht_hum_ss();

/**
 * @brief Start Periodic Measurement
 *
 * @param ctx                     Click object.
 * 
 * Description: Starts periodic measurement with current settings.
 */
void sht_start_pm(sht_t *ctx);

/**
 * @brief Periodic Mode Temperature
 *
 * @param ctx                     Click object.
 *
 * @return Temperature in ( C )
 * 
 * Description: Returns temperature measurement in periodic mode.
 * 
 * @note
 * Before function call, periodic measurement must be started by calling
 * @link sht_start_pm @endlink
 * or communication error can occurrs.
 */
float sht_temp_pm(sht_t *ctx);

/**
 * @brief Periodic Mode Humidity
 *
 * @param ctx                     Click object.
 * 
 * @return Hunidity in ( % )
 * 
 * Description: Returns humidity measurement in periodic mode.
 *
 * @note
 * Before function call, periodic measurement must be started by calling
 * @link sht_start_pm @endlink
 * or communication error can occurrs.
 */
float sht_hum_pm(sht_t *ctx);

/**
 * @brief Stop Periodic Measurement
 *
 * @param ctx                     Click object. 
 * 
 * Description: Stops periodic measurement instantly. This must be called before call of
 * any other function except the periodic measurement read functions.
 */
void sht_stop_pm(sht_t *ctx);

/**
 * @brief Software Reset
 *
 * @param ctx                     Click object. 
 * 
 * Description: Forces the device into a well-defined state without removing the power
 * supply. This triggers the sensor to reset its system controller and
 * reloads calibration data from the memory.
 */
void sht_software_rst(sht_t *ctx);

/**
 * @brief Heater State
 *
 * @param ctx                     Click object. 
 * @param state                   ( true / false )
 * 
 * Description: Sets the heater state.
 *
 * @note
 * By default heater is disabled.
 */
void sht_heater_control(sht_t *ctx, uint8_t state);

/**
 * @brief Clears Status Register
 *
 * @param ctx                     Click object. 
 * 
 * Description: The status register contains informations about the operational status of
 * the heater, the alert mode and on the execution status of the last command
 * and the last write sequence.
 */
void sht_clear_status(sht_t *ctx);

/**
 * @brief Alert Status
 *
 * @param ctx                     Click object. 
 * 
 * @retval FALSE - no pending status
 * @retval TRUE - at least one pending alert
 * 
 * Description: Returns does device have pending status about any of posibile error.
 */
uint8_t sht_alert_status(sht_t *ctx);

/**
 * @brief Heater State
 *
 * @param ctx                     Click object. 
 * 
 * @retval FALSE - heater off
 * @retval TRUE - heater on 
 *
 * Description: Returns information about current heater state.
 */
uint8_t sht_heater_status(sht_t *ctx);

/**
 * @brief Humidity Alert
 *
 * @param ctx                     Click object. 
 * 
 * @retval FALSE - no alert
 * @retval TRUE - alert
 * 
 * Description Returns does humididty tracking alert exists.
 */
uint8_t sht_hum_status(sht_t *ctx);

/**
 * @brief Returns does temperature tracking alert exists.
 *
 * @param ctx                     Click object. 
 * 
 * @retval    FALSE - no_alert
 *            TRUE - alert
 *         
 * 
 */
uint8_t sht_temp_status(sht_t *ctx);

/**
 * @brief Returns information does module detected any kind of reset during current
 * power on time.
 *
 * @param ctx                     Click object. 
 *
 * @retval    FALSE - no reset detected since last clear
 *            TRUE - some kind of ( soft or hard ) reset detected
 * 
 */
uint8_t sht_reset_status(sht_t *ctx);

/**
 * @brief Returns information about last command execution.
 *
 * @param ctx                     Click object.
 * 
 * @retval    FALSE - last command executed successfully
 *            TRUE - last command not processed
 *         
 * 
 */
uint8_t sht_cmd_status(sht_t *ctx);

/**
 * @brief Returns information about last cheksum
 *
 * @param ctx                     Click object.
 *
 * @retval    FALSE - checksum of last write was correct
 *            TRUE - checksum of last write failed
 *          
 * 
 */
uint8_t sht_wr_chksum_status(sht_t *ctx);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief sht: Initialise the sht temp-humidity sensor
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
uint8_t MIKROSDK_SHT_Initialise(void);

#ifdef __cplusplus
}
#endif
#endif // _SHT_H_

/** \} */ // End public_function group
/// \}    // End click Driver group
/*! @} */
// ------------------------------------------------------------------------- END
