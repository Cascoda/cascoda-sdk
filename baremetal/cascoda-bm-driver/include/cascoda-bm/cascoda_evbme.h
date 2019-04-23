/**
 * @file   cascoda_evbme.h
 * @brief  EvaBoard Management Entity (EVBME) Definitions/Declarations
 * @author Wolfgang Bruchner
 * @date   19/07/14
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
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
#include "cascoda-bm/cascoda_bm.h"
#include "ca821x_api.h"

#ifndef CASCODA_EVBME_H
#define CASCODA_EVBME_H

/***************************************************************************/ /**
 * \defgroup EVBMEEnums EVBME Enumerations
 ************************************************************************** @{*/
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
	EVBME_ERROR_INDICATION    = 0xF0
};

/** EVBME attribute ids */
enum evbme_attribute
{
	EVBME_RESETRF  = 0x00,
	EVBME_CFGPINS  = 0x01,
	EVBME_WAKEUPRF = 0x02
};

/** Definitions for Powerdown Modes */
enum PDMode
{
	PDM_ALLON     = 0, //!< Mainly for Testing
	PDM_ACTIVE    = 1, //!< CAX Full Data Retention, MAC Running
	PDM_STANDBY   = 2, //!< CAX Full Data Retention
	PDM_POWERDOWN = 3, //!< No CAX Retention, PIB has to be re-initialised
	PDM_POWEROFF  = 4  //!< No CAX Retention, PIB has to be re-initialised. Timer-Wakeup only
};
/**@}*/

/******************************************************************************/
/****** Message Definitions for UpStream (Serial) and  DownStream (SPI)  ******/
/******************************************************************************/
#define SERIAL_TX_CMD_ID (SerialTxBuffer.Header[SERIAL_CMD_ID])
#define SERIAL_TX_CMD_LEN (SerialTxBuffer.Header[SERIAL_CMD_LEN])
#define SERIAL_TX_DATA (SerialTxBuffer.Data)

struct MAC_Message;
struct SerialBuffer;
struct ca821x_dev;

/******************************************************************************/
/****** Global Parameters that can by set via EVBME_SET_request          ******/
/******************************************************************************/
extern u8_t EVBME_HasReset;
extern u8_t EVBME_UseMAC;

extern void (*EVBME_Message)(char *message, size_t len, struct ca821x_dev *pDeviceRef);
extern void (*MAC_Message)(u8_t CommandId, u8_t Count, u8_t *pBuffer);
extern int (*app_reinitialise)(struct ca821x_dev *pDeviceRef);

/******************************************************************************/
/****** EVBME API Functions                                              ******/
/******************************************************************************/
ca_error EVBMEInitialise(const char *aAppName, struct ca821x_dev *dev);
void     EVBMEHandler(struct ca821x_dev *pDeviceRef);
int      EVBMEUpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
void     EVBMESendDownStream(const uint8_t *buf, size_t len, uint8_t *response, struct ca821x_dev *pDeviceRef);
void     EVBMESendUpStream(struct MAC_Message *msg);
int      EVBMECheckSerialCommand(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
ca_error EVBME_SET_request(u8_t Attribute, u8_t AttributeLength, u8_t *AttributeValue, struct ca821x_dev *pDeviceRef);
ca_error EVBME_ResetRF(u8_t ms, struct ca821x_dev *pDeviceRef);
ca_error EVBME_Connect(const char *aAppName, struct ca821x_dev *pDeviceRef);
void     EVBME_Disconnect(void);
void     cascoda_io_handler(struct ca821x_dev *pDeviceRef);

void     EVBME_PowerDown(enum PDMode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef);
ca_error EVBME_CAX_ExternalClock(u8_t on_offb, struct ca821x_dev *pDeviceRef);
void     EVBME_SwitchClock(struct ca821x_dev *pDeviceRef, u8_t useExternalClock);
void     EVBME_CAX_PowerDown(enum PDMode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef);
void     EVBME_CAX_Wakeup(enum PDMode mode, int timeout_ms, struct ca821x_dev *pDeviceRef);
void     EVBME_CAX_Restart(struct ca821x_dev *pDeviceRef);
void     EVBME_WakeUpRF(void);
void     EVBME_ERROR_Indication(ca_error error_code, u8_t has_restarted, struct ca821x_dev *pDeviceRef);

#endif // CASCODA_EVBME_H
