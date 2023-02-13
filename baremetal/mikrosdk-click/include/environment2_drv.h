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
 * @file environment2.h
 * @brief This file contains API for Environment 2 Click Driver.
 */

#ifndef ENVIRONMENT2_DRV_H
#define ENVIRONMENT2_DRV_H

#include "drv_digital_in.h"
#include "drv_digital_out.h"
#include "drv_i2c_master.h"

/* added for tvoc sampling interval */
extern uint32_t sgp40_sampling_interval;

/**
 * @brief Environment 2 SGP40 description commands.
 * @details Specified SGP40 commands for description of Environment 2 Click driver.
 */
#define ENVIRONMENT2_SGP40_CMD_MEASURE_RAW 0x260F
#define ENVIRONMENT2_SGP40_CMD_MEASURE_TEST 0x280E
#define ENVIRONMENT2_SGP40_CMD_HEATER_OFF 0x3615
#define ENVIRONMENT2_SGP40_CMD_SOFT_RESET 0x0006

/**
 * @brief Environment 2 SHT40 description commands.
 * @details Specified SHT40 commands for description of Environment 2 Click driver.
 */
#define ENVIRONMENT2_SHT40_CMD_MEASURE_T_RH_HIGH_PRECISION 0xFD
#define ENVIRONMENT2_SHT40_CMD_MEASURE_T_RH_MEDIUM_PRECISION 0xF6
#define ENVIRONMENT2_SHT40_CMD_MEASURE_T_RH_LOWEST_PRECISION 0xE0
#define ENVIRONMENT2_SHT40_CMD_READ_SERIAL 0x89
#define ENVIRONMENT2_SHT40_CMD_SOFT_RESET 0x94
#define ENVIRONMENT2_SHT40_CMD_ACTIVATE_HIGHEST_HEATER_1SEC 0x39
#define ENVIRONMENT2_SHT40_CMD_ACTIVATE_HIGHEST_HEATER_0_1SEC 0x32
#define ENVIRONMENT2_SHT40_CMD_ACTIVATE_MEDIUM_HEATER_1SEC 0x2F
#define ENVIRONMENT2_SHT40_CMD_ACTIVATE_MEDIUM_HEATER_0_1SEC 0x24
#define ENVIRONMENT2_SHT40_CMD_ACTIVATE_LOWEST_HEATER_1SEC 0x1E
#define ENVIRONMENT2_SHT40_CMD_ACTIVATE_LOWEST_HEATER_0_1SEC 0x15

/**
 * @brief Environment 2 device address setting.
 * @details Specified setting for device slave address selection of
 * Environment 2 Click driver.
 */
#define ENVIRONMENT2_SGP40_SET_DEV_ADDR 0x59
#define ENVIRONMENT2_SHT40_SET_DEV_ADDR 0x44

/**
 * @brief Environment 2 device selection.
 * @details Specified selection for device slave address of
 * Environment 2 Click driver.
 */
#define ENVIRONMENT2_SEL_SGP40 0x00
#define ENVIRONMENT2_SEL_SHT40 0x01

/**
 * @brief Environment 2 fixed point arithmetic parts.
 * @details Specified the fixed point arithmetic parts for VOC algorithm of
 * Environment 2 Click driver.
 */
#define F16(x) ((fix16_t)(((x) >= 0) ? ((x)*65536.0 + 0.5) : ((x)*65536.0 - 0.5)))
// MODIFIED: replaced
// #define VocAlgorithm_SAMPLING_INTERVAL (5.) // MODIFIED: (1.)
#define VocAlgorithm_SAMPLING_INTERVAL ((float)(sgp40_sampling_interval) / 1000.0)
#define VocAlgorithm_INITIAL_BLACKOUT (45.) // MODIFIED: (45.)
#define VocAlgorithm_VOC_INDEX_GAIN (230.)
#define VocAlgorithm_SRAW_STD_INITIAL (50.)
#define VocAlgorithm_SRAW_STD_BONUS (220.)
#define VocAlgorithm_TAU_MEAN_VARIANCE_HOURS (12.)
#define VocAlgorithm_TAU_INITIAL_MEAN (20.)
#define VocAlgorithm_INIT_DURATION_MEAN ((3600. * 0.75))
#define VocAlgorithm_INIT_TRANSITION_MEAN (0.01)
#define VocAlgorithm_TAU_INITIAL_VARIANCE (2500.)
#define VocAlgorithm_INIT_DURATION_VARIANCE ((3600. * 1.45))
#define VocAlgorithm_INIT_TRANSITION_VARIANCE (0.01)
#define VocAlgorithm_GATING_THRESHOLD (340.)
#define VocAlgorithm_GATING_THRESHOLD_INITIAL (510.)
#define VocAlgorithm_GATING_THRESHOLD_TRANSITION (0.09)
#define VocAlgorithm_GATING_MAX_DURATION_MINUTES ((60. * 3.))
#define VocAlgorithm_GATING_MAX_RATIO (0.3)
#define VocAlgorithm_SIGMOID_L (500.)
#define VocAlgorithm_SIGMOID_K (-0.0065)
#define VocAlgorithm_SIGMOID_X0 (213.)
#define VocAlgorithm_VOC_INDEX_OFFSET_DEFAULT (100.)
#define VocAlgorithm_LP_TAU_FAST (20.0)
#define VocAlgorithm_LP_TAU_SLOW (500.0)
#define VocAlgorithm_LP_ALPHA (-0.2)
#define VocAlgorithm_PERSISTENCE_UPTIME_GAMMA ((3. * 3600.))
#define VocAlgorithm_MEAN_VARIANCE_ESTIMATOR__GAMMA_SCALING (64.)
#define VocAlgorithm_MEAN_VARIANCE_ESTIMATOR__FIX16_MAX (32767.)

/**
 * @brief Environment 2 SGP40 description setting.
 * @details Specified SGP40 setting for description of Environment 2 Click driver.
 */
#define ENVIRONMENT2_SGP40_TEST_PASSED 0xD400
#define ENVIRONMENT2_SGP40_TEST_FAILED 0x4B00

/**
 * @brief Environment 2 Click context object.
 * @details Context object definition of Environment 2 Click driver.
 */
typedef struct
{
	// Modules

	i2c_master_t i2c; /**< I2C driver object. */

	// I2C slave address

	uint8_t slave_address; /**< Device slave address (used for I2C driver). */

} environment2_t;

/**
 * @brief Environment 2 Click configuration object.
 * @details Configuration object definition of Environment 2 Click driver.
 */
typedef struct
{
	pin_name_t scl; /**< Clock pin descriptor for I2C driver. */
	pin_name_t sda; /**< Bidirectional data pin descriptor for I2C driver. */

	uint32_t i2c_speed;   /**< I2C serial speed. */
	uint8_t  i2c_address; /**< I2C slave address. */

} environment2_cfg_t;

typedef int32_t fix16_t;

/**
 * @brief Environment 2 Click VOC algorithm object.
 * @details Struct to hold all the states of the VOC algorithm.
 */
typedef struct
{
	fix16_t mVoc_Index_Offset;
	fix16_t mTau_Mean_Variance_Hours;
	fix16_t mGating_Max_Duration_Minutes;
	fix16_t mSraw_Std_Initial;
	fix16_t mUptime;
	fix16_t mSraw;
	fix16_t mVoc_Index;
	fix16_t m_Mean_Variance_Estimator__Gating_Max_Duration_Minutes;
	bool    m_Mean_Variance_Estimator___Initialized;
	fix16_t m_Mean_Variance_Estimator___Mean;
	fix16_t m_Mean_Variance_Estimator___Sraw_Offset;
	fix16_t m_Mean_Variance_Estimator___Std;
	fix16_t m_Mean_Variance_Estimator___Gamma;
	fix16_t m_Mean_Variance_Estimator___Gamma_Initial_Mean;
	fix16_t m_Mean_Variance_Estimator___Gamma_Initial_Variance;
	fix16_t m_Mean_Variance_Estimator__Gamma_Mean;
	fix16_t m_Mean_Variance_Estimator__Gamma_Variance;
	fix16_t m_Mean_Variance_Estimator___Uptime_Gamma;
	fix16_t m_Mean_Variance_Estimator___Uptime_Gating;
	fix16_t m_Mean_Variance_Estimator___Gating_Duration_Minutes;
	fix16_t m_Mean_Variance_Estimator___Sigmoid__L;
	fix16_t m_Mean_Variance_Estimator___Sigmoid__K;
	fix16_t m_Mean_Variance_Estimator___Sigmoid__X0;
	fix16_t m_Mox_Model__Sraw_Std;
	fix16_t m_Mox_Model__Sraw_Mean;
	fix16_t m_Sigmoid_Scaled__Offset;
	fix16_t m_Adaptive_Lowpass__A1;
	fix16_t m_Adaptive_Lowpass__A2;
	bool    m_Adaptive_Lowpass___Initialized;
	fix16_t m_Adaptive_Lowpass___X1;
	fix16_t m_Adaptive_Lowpass___X2;
	fix16_t m_Adaptive_Lowpass___X3;
} environment2_voc_algorithm_params;

/* voc functions */
void environment2_voc_config(void);
void environment2_voc_algorithm(int32_t sraw, int32_t *voc_index);

#endif // ENVIRONMENT2_DRV_H

/*! @} */ // environment2

// ------------------------------------------------------------------------ END
