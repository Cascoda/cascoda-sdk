/**
 * @file
 * @brief  EvaBoard Management Entity (EVBME) Definitions/Declarations
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
#include "cascoda-bm/cascoda_bm.h"
#include "ca821x_api.h"

#ifndef CASCODA_EVBME_H
#define CASCODA_EVBME_H

#ifdef __cplusplus
extern "C" {
#endif

/** EVBME message id codes */
enum evbme_msg_id_code
{
	EVBME_SET_REQUEST         = 0x5F,
	EVBME_GUI_CONNECTED       = 0x81,
	EVBME_GUI_DISCONNECTED    = 0x82,
	EVBME_MESSAGE_INDICATION  = 0xA0,
	EVBME_COMM_CHECK          = 0xA1,
	EVBME_COMM_INDICATION     = 0xA2,
	EVBME_TERMINAL_INDICATION = 0xFE,
};

/** EVBME attribute ids */
enum evbme_attribute
{
	EVBME_RESETRF  = 0x00,
	EVBME_CFGPINS  = 0x01,
	EVBME_WAKEUPRF = 0x02
};

/** Definitions for Powerdown Modes */
enum powerdown_mode
{
	PDM_ALLON     = 0, //!< Mainly for Testing
	PDM_ACTIVE    = 1, //!< CAX Full Data Retention, MAC Running
	PDM_STANDBY   = 2, //!< CAX Full Data Retention
	PDM_POWERDOWN = 3, //!< No CAX Retention, PIB has to be re-initialised
	PDM_POWEROFF  = 4, //!< No CAX Retention, PIB has to be re-initialised. Timer-Wakeup only
	PDM_DPD       = 5  //!< No CAX Retention or MCU Retention (Data saved in NVM)
};

/** Structure of the EVBME_COMM_CHECK message that can be used to test comms by host. */
struct EVBME_COMM_CHECK_request
{
	uint8_t mHandle;   //!< Handle identifying this comm check
	uint8_t mDelay;    //!< Delay before sending responses
	uint8_t mIndCount; //!< Number of indications to send up
	uint8_t mIndSize;  //!< Size of the indications to send
};

/******************************************************************************/
/****** Global Parameters that can by set via EVBME_SET_request          ******/
/******************************************************************************/
extern u8_t EVBME_HasReset;
extern u8_t EVBME_UseMAC;

extern void (*EVBME_Message)(char *message, size_t len, struct ca821x_dev *pDeviceRef);
extern void (*MAC_Message)(u8_t CommandId, u8_t Count, const u8_t *pBuffer);
extern int (*app_reinitialise)(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** EVBME API Functions                                              ******/
/******************************************************************************/

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialises EVBME after Reset
 *******************************************************************************
 * Initialises low level interfaces, resets and initialises CA-8210.
 *******************************************************************************
 * \param aAppName - App name string
 * \param dev - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return Status of initialisation
 *******************************************************************************
 ******************************************************************************/
ca_error EVBMEInitialise(const char *aAppName, struct ca821x_dev *dev);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Returns the app name string.
 *******************************************************************************
 * \return app_name
 *******************************************************************************
 ******************************************************************************/
const char *EVBME_GetAppName(void);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends DownStream Command from API UpStream to Serial
 *******************************************************************************
 * \param msg - Message to send upstream
 *******************************************************************************
 ******************************************************************************/
void DISPATCH_NotHandled(struct MAC_Message *msg);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Processes messages received over available interfaces. This function
 *        should be called regularly from application context eg the main loop.
 * 		  Alternatively, you may use cascoda_io_signal() to notify the
 *		  application that it must call cascoda_io_handler().
 *******************************************************************************
 ******************************************************************************/
void cascoda_io_handler(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
* \brief Put the system into a state of power-down for a given time
*******************************************************************************
* \param mode - Power-Down Mode
* \param sleeptime_ms - milliseconds to sleep for
* \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
*******************************************************************************
* Power-Down Modes
*
* Enum              | MCU             | CAX              | Notes
* ----------------- | --------------- | ---------------- | -------------------------
* PDM_ALLON         | Active          | Active           | Mainly for Testing
* PDM_ACTIVE        | Power-Down      | Active           | CAX Full Data Retention, MAC Running
* PDM_STANDBY       | Power-Down      | Standby          | CAX Full Data Retention
* PDM_POWERDOWN     | Power-Down      | Power-Down 0     | No CAX Retention, PIB has to be re-initialised
* PDM_POWEROFF      | Power-Down      | Power-Down 1     | No CAX Retention, PIB has to be re-initialised. Timer-Wakeup only
* PDM_DPD           | Deep-Power-Down | Power-Down 1     | No CAX Retention or MCU Retention (Data saved in NVM)
*******************************************************************************
******************************************************************************/
void EVBME_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief System Clock Switch
 ********************************************************************************
 * \param pDeviceRef - a pointer to the ca821x_dev struct
 * \param useExternalClock - boolean (1: use external clock from ca821x, 0: use internal clock)
 *******************************************************************************
 ******************************************************************************/
void EVBME_SwitchClock(struct ca821x_dev *pDeviceRef, u8_t useExternalClock);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Restarts Air Interface
 *******************************************************************************
 * Resets and re-initialises CAX, calls app_reinitialise callback.
 *******************************************************************************
 ******************************************************************************/
void EVBME_CAX_Restart(struct ca821x_dev *pDeviceRef);

#ifdef __cplusplus
}
#endif

#endif // CASCODA_EVBME_H
