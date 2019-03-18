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
 * Low-Level Debug Functions
*/

#if defined(USE_DEBUG)

#include "cascoda_debug_chili.h"
#include "cascoda-bm/cascoda_types.h"

/******************************************************************************/
/****** Global Variables                                                 ******/
/******************************************************************************/
u8_t Debug_IRQ_State = 0; /*  IRQ State (occasional or wakeup) */
u8_t Debug_BSP_Error = 0; /*  BSP Error Status */

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset all Low Level (BSP/IRQ) Debug Codes
 *******************************************************************************
 ******************************************************************************/
void BSP_Debug_Reset(void)
{
	Debug_IRQ_State = DEBUG_IRQ_CLEAR;
	Debug_BSP_Error = DEBUG_ERR_CLEAR;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Assign Error Code and Source
 *******************************************************************************
 * \param code - error code
 *******************************************************************************
 ******************************************************************************/
void BSP_Debug_Error(u8_t code)
{
	/* store first call until message has been sent */
	if (Debug_BSP_Error == DEBUG_ERR_CLEAR)
	{
		Debug_BSP_Error = code;
	}
}

#endif /* USE_DEBUG */
