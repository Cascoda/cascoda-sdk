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
 * \brief This file contains API for Relay Click driver.
 *
 * \addtogroup relay Relay Click Driver
 * @{
 */
// ----------------------------------------------------------------------------

#ifndef RELAY_DRV_H
#define RELAY_DRV_H

#include "drv_digital_out.h"

/**
 * \defgroup relay_state Relay state
 * \{
 */
#define RELAY_STATE_ON 1
#define RELAY_STATE_OFF 0
/** \} */

/**
 * \defgroup select_relay Select Relay
 * \{
 */
#define RELAY_NUM_1 1
#define RELAY_NUM_2 2
/** \} */

/** \} */ // End group macro
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
	// Output pins

	digital_out_t rel2;
	digital_out_t rel1;

} relay_t;

/**
 * @brief Click configuration structure definition.
 */
typedef struct
{
	// Additional gpio pins

	pin_name_t rel2;
	pin_name_t rel1;

} relay_cfg_t;

/** \} */ // End types group

#endif // RELAY_DRV_H

/** \} */ // End public_function group
/// \}    // End click Driver group
/*! @} */
// ------------------------------------------------------------------------- END
