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
/**
 * @file
 * Declaration of EVBME Functions
 */
/**
 * @ingroup bm-driver
 * @defgroup bm-evbme Baremetal EVBME abstraction
 * @brief  Evaluation Board Management Entity (EVBME) for core application-platform interactions
 *
 * @{
 */

#include "cascoda-bm/cascoda_bm.h"
#include "ca821x_api.h"

#ifndef CASCODA_EVBME_H
#define CASCODA_EVBME_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CA_TARGET_NAME
// Should be defined by the build system
#define CA_TARGET_NAME "UNKNOWN-TARGET"
#endif

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

/******************************************************************************/
/****** Global Parameters that can by set via EVBME_SET_request          ******/
/******************************************************************************/
extern u8_t EVBME_HasReset;
extern u8_t EVBME_UseMAC;

extern void (*EVBME_Message)(char *message, size_t len);
extern void (*MAC_Message)(u8_t CommandId, u8_t Count, const u8_t *pBuffer);

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
 * \param pDeviceRef - Cascoda device reference
 *******************************************************************************
 ******************************************************************************/
ca_error EVBME_NotHandled(const struct MAC_Message *msg, struct ca821x_dev *pDeviceRef);

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
 * \brief re-initialises CA821x / MAC PIB after powerdown. The function pointer
 *        should be populated at the top level
 *******************************************************************************
 ******************************************************************************/
extern int (*cascoda_reinitialise)(struct ca821x_dev *pDeviceRef);

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
 * Resets and re-initialises CAX
 *******************************************************************************
 ******************************************************************************/
void EVBME_CAX_Restart(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Get an openthread-specific EVBME attribute.
 *******************************************************************************
 * Only valid when openthread is being used, otherwise will return CA_ERROR_UNKNOWN.
 *
 * @param aAttrib The attribute to get
 * @param[in,out] aOutBufLen out:The length of the attribute data in octets, in: the maximum length of the attribute value in octets
 * @param[out] aOutBuf The buffer to fill with attribute data
 *
 * @returns Status of the command
 * @retval CA_ERROR_SUCCESS The output buffer is filled with the attribute data
 * @retval CA_ERROR_UNKNOWN The attribute was not recognised for this system
 * @retval CA_ERROR_NO_BUFFER The output buffer isn't big enough to store the attribute
 ******************************************************************************/
ca_error EVBME_GET_OT_Attrib(enum evbme_attribute aAttrib, uint8_t *aOutBufLen, uint8_t *aOutBuf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // CASCODA_EVBME_H
