/**
 * @file
 * @brief Chili temperature sensing EVBME functions
 */
/*
 *  Copyright (c) 2019, Cascoda Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

#include "tempsense_app.h"
#include "tempsense_evbme.h"
#if APP_USE_DEBUG
#include "tempsense_debug.h"
#endif /* APP_USE_DEBUG */

static int TEMPSENSE_mode_button_pressed(void);
static int TEMPSENSE_usb_present_changed(void);
static int TEMPSENSE_reinitialise(struct ca821x_dev *pDeviceRef);

void TEMPSENSE_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	wakeup_reason wkup;

	cascoda_reinitialise = TEMPSENSE_reinitialise;

	/* register USB_PRESENT */
	struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){special_pins.USB_PRESENT,
	                                                      MODULE_PIN_PULLUP_OFF,
	                                                      MODULE_PIN_DEBOUNCE_ON,
	                                                      MODULE_PIN_IRQ_BOTH,
	                                                      TEMPSENSE_usb_present_changed});

	/* register SWITCH */
	BSP_ModuleRegisterGPIOInput(&(struct gpio_input_args){special_pins.SWITCH,
	                                                      MODULE_PIN_PULLUP_OFF,
	                                                      MODULE_PIN_DEBOUNCE_ON,
	                                                      MODULE_PIN_IRQ_FALL,
	                                                      TEMPSENSE_mode_button_pressed});
	/* register LED_G */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_GREEN, MODULE_PIN_TYPE_LED);
	/* register LED_R */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_RED, MODULE_PIN_TYPE_LED);

	if (status == CA_ERROR_FAIL)
	{
		APP_INITIALISE = 0;
#if APP_USE_DEBUG
		APP_Debug_Error(0x10);
#endif /* APP_USE_DEBUG */
		return;
	}

	if (BSP_IsUSBPresent())
		APP_STATE = APP_STATE_new = APP_ST_NORMAL;
	else
		APP_STATE = APP_STATE_new = APP_ST_DEVICE;

	wkup = BSP_GetWakeupReason();
	if ((wkup == WAKEUP_DEEP_POWERDOWN) || (wkup == WAKEUP_RTCALARM))
	{
		TEMPSENSE_APP_Device_RestoreStateFromFlash(pDeviceRef);
		TEMPSENSE_RegisterCallbacks(pDeviceRef);
		TEMPSENSE_APP_InitPIB(pDeviceRef);
	}
	else
	{
		TEMPSENSE_APP_Initialise(pDeviceRef);
		TEMPSENSE_usb_present_changed();
	}

	if (APP_STATE == APP_ST_DEVICE)
		BSP_SystemConfig(FSYS_4MHZ, 0);

} // End of TEMPSENSE_Initialise()

void TEMPSENSE_Handler(struct ca821x_dev *pDeviceRef)
{
	/* call application handler */
	TEMPSENSE_APP_Handler(pDeviceRef);
} // End of TEMPSENSE_Handler()

int TEMPSENSE_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	int ret;
	/* call application dispatch */
	if ((ret = TEMPSENSE_APP_UpStreamDispatch(SerialRxBuffer, pDeviceRef)))
	{
		return ret;
	}
	return 0;
} // End of TEMPSENSE_UpStreamDispatch()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SWITCH callback called by GPIO ISR
 *******************************************************************************
 ******************************************************************************/
static int TEMPSENSE_mode_button_pressed(void)
{
	if (APP_STATE == APP_ST_NORMAL)
	{
		if (BSP_EnableUSB(), !BSP_IsUSBPresent())
			return 0;
		APP_STATE_new = APP_ST_COORDINATOR;
	}
	else if (APP_STATE == APP_ST_COORDINATOR)
	{
		BSP_DisableUSB();
		APP_STATE_new = APP_ST_DEVICE;
	}
	else
	{
		if (BSP_EnableUSB(), !BSP_IsUSBPresent())
			return 0;
		APP_STATE_new = APP_ST_NORMAL;
	}
	APP_INITIALISE = 1;
	return 0;
} // End of TEMPSENSE_mode_button_pressed()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief TEMPSENSE_usb_present_changed called by GPIO ISR
 *******************************************************************************
 ******************************************************************************/
static int TEMPSENSE_usb_present_changed(void)
{
	u8_t                     pinval;
	struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();

	BSP_ModuleSenseGPIOPin(special_pins.USB_PRESENT, &pinval);
	if (pinval)
		BSP_EnableUSB();
	else
		BSP_DisableUSB();

	if (BSP_IsUSBPresent())
	{
		APP_STATE_new = APP_ST_NORMAL;
	}
	else
	{
		APP_STATE_new = APP_ST_DEVICE;
	}
	APP_INITIALISE = 1;
	return 0;
} // End of TEMPSENSE_usb_present_changed()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief cascoda_reinitialise called by BSP
 *******************************************************************************
 ******************************************************************************/
static int TEMPSENSE_reinitialise(struct ca821x_dev *pDeviceRef)
{
	TEMPSENSE_APP_InitPIB(pDeviceRef);
	if (APP_STATE == APP_ST_COORDINATOR)
		TEMPSENSE_APP_Coordinator_SoftReinit(pDeviceRef);
	return 0;
} // End of TEMPSENSE_reinitialise()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MCPS_DATA_indication in TEMPSENSE COORDINATOR Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_COORD_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                         struct ca821x_dev                *pDeviceRef)
{
	TEMPSENSE_APP_Coordinator_ProcessDataInd(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of TEMPSENSE_APP_COORD_MCPS_DATA_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MCPS_DATA_indication in TEMPSENSE DEVICE Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_DEV_MCPS_DATA_indication(struct MCPS_DATA_indication_pset *params,
                                                       struct ca821x_dev                *pDeviceRef)
{
	TEMPSENSE_APP_Device_ProcessDataInd(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of TEMPSENSE_APP_COORD_MCPS_DATA_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MCPS_DATA_confirm in TEMPSENSE COORDINATOR Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_COORD_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params,
                                                      struct ca821x_dev             *pDeviceRef)
{
	TEMPSENSE_APP_Coordinator_ProcessDataCnf(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of TEMPSENSE_APP_COORD_MCPS_DATA_confirm()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MCPS_DATA_confirm in TEMPSENSE DEVICE Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_DEV_MCPS_DATA_confirm(struct MCPS_DATA_confirm_pset *params,
                                                    struct ca821x_dev             *pDeviceRef)
{
	TEMPSENSE_APP_Device_ProcessDataCnf(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of TEMPSENSE_APP_COORD_MCPS_DATA_confirm()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_ASSOCIATE_indication in TEMPSENSE COORDINATOR Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_COORD_MLME_ASSOCIATE_indication(struct MLME_ASSOCIATE_indication_pset *params,
                                                              struct ca821x_dev                     *pDeviceRef)
{
	if (APP_STATE == APP_ST_COORDINATOR)
	{
		TEMPSENSE_APP_Coordinator_AssociateResponse(params, pDeviceRef);
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_NOT_HANDLED;
} // End of TEMPSENSE_APP_COORD_MLME_ASSOCIATE_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_COMM_STATUS_indication in TEMPSENSE
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_MLME_COMM_STATUS_indication(struct MLME_COMM_STATUS_indication_pset *params,
                                                          struct ca821x_dev                       *pDeviceRef)
{
	u16_t panid;

	panid = GETLE16(params->PANId);

	/* supress message from other networks */
	if (panid != APP_PANId)
		return CA_ERROR_SUCCESS;
	/* supress message if expected */
	if ((params->Status == MAC_SUCCESS) || (params->Status == MAC_TRANSACTION_EXPIRED))
		return CA_ERROR_SUCCESS;

	return CA_ERROR_NOT_HANDLED;
} // End of TEMPSENSE_APP_COORD_COMM_STATUS_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for HWME_WAKEUP_indication in TEMPSENSE COORDINATOR Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_COORD_HWME_WAKEUP_indication(struct HWME_WAKEUP_indication_pset *params,
                                                           struct ca821x_dev                  *pDeviceRef)
{
	if (APP_STATE == APP_ST_COORDINATOR)
	{
		/* only suppress message */
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_NOT_HANDLED;
} // End of TEMPSENSE_APP_COORD_HWME_WAKEUP_indication()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_ASSOCIATE_confirm in TEMPSENSE DEVICE Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_DEV_MLME_ASSOCIATE_confirm(struct MLME_ASSOCIATE_confirm_pset *params,
                                                         struct ca821x_dev                  *pDeviceRef)
{
	TEMPSENSE_APP_Device_ProcessAssociateCnf(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of TEMPSENSE_APP_DEV_MLME_ASSOCIATE_confirm()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_SCAN_confirm in TEMPSENSE COORDINATOR Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_COORD_MLME_SCAN_confirm(struct MLME_SCAN_confirm_pset *params,
                                                      struct ca821x_dev             *pDeviceRef)
{
	TEMPSENSE_APP_Coordinator_ProcessScanCnf(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of TEMPSENSE_APP_COORD_MLME_SCAN_confirm()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for MLME_SCAN_confirm in TEMPSENSE DEVICE Mode
 *******************************************************************************
 ******************************************************************************/
static ca_error TEMPSENSE_APP_DEV_MLME_SCAN_confirm(struct MLME_SCAN_confirm_pset *params,
                                                    struct ca821x_dev             *pDeviceRef)
{
	TEMPSENSE_APP_Device_ProcessScanCnf(params, pDeviceRef);
	return CA_ERROR_SUCCESS;
} // End of TEMPSENSE_APP_DEV_MLME_SCAN_confirm()

void TEMPSENSE_RegisterCallbacks(struct ca821x_dev *pDeviceRef)
{
	if (APP_STATE == APP_ST_COORDINATOR)
	{
		pDeviceRef->callbacks.MCPS_DATA_indication        = &TEMPSENSE_APP_COORD_MCPS_DATA_indication;
		pDeviceRef->callbacks.MCPS_DATA_confirm           = &TEMPSENSE_APP_COORD_MCPS_DATA_confirm;
		pDeviceRef->callbacks.MLME_ASSOCIATE_indication   = &TEMPSENSE_APP_COORD_MLME_ASSOCIATE_indication;
		pDeviceRef->callbacks.MLME_COMM_STATUS_indication = &TEMPSENSE_APP_MLME_COMM_STATUS_indication;
		pDeviceRef->callbacks.HWME_WAKEUP_indication      = &TEMPSENSE_APP_COORD_HWME_WAKEUP_indication;
		pDeviceRef->callbacks.MLME_ASSOCIATE_confirm      = NULL;
		pDeviceRef->callbacks.MLME_SCAN_confirm           = &TEMPSENSE_APP_COORD_MLME_SCAN_confirm;
	}
	else if (APP_STATE == APP_ST_DEVICE)
	{
		pDeviceRef->callbacks.MCPS_DATA_indication        = &TEMPSENSE_APP_DEV_MCPS_DATA_indication;
		pDeviceRef->callbacks.MCPS_DATA_confirm           = &TEMPSENSE_APP_DEV_MCPS_DATA_confirm;
		pDeviceRef->callbacks.MLME_ASSOCIATE_indication   = NULL;
		pDeviceRef->callbacks.MLME_COMM_STATUS_indication = &TEMPSENSE_APP_MLME_COMM_STATUS_indication;
		pDeviceRef->callbacks.HWME_WAKEUP_indication      = NULL;
		pDeviceRef->callbacks.MLME_ASSOCIATE_confirm      = &TEMPSENSE_APP_DEV_MLME_ASSOCIATE_confirm;
		pDeviceRef->callbacks.MLME_SCAN_confirm           = &TEMPSENSE_APP_DEV_MLME_SCAN_confirm;
	}
	else
	{
		pDeviceRef->callbacks.MCPS_DATA_indication        = NULL;
		pDeviceRef->callbacks.MCPS_DATA_confirm           = NULL;
		pDeviceRef->callbacks.MLME_ASSOCIATE_indication   = NULL;
		pDeviceRef->callbacks.MLME_COMM_STATUS_indication = NULL;
		pDeviceRef->callbacks.HWME_WAKEUP_indication      = NULL;
		pDeviceRef->callbacks.MLME_ASSOCIATE_confirm      = NULL;
		pDeviceRef->callbacks.MLME_SCAN_confirm           = NULL;
	}
} // End of TEMPSENSE_RegisterCallbacks()
