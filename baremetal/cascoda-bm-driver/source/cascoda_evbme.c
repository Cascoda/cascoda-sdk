/**
 * @file
 * @brief  EvaBoard Management Entity (EVBME) functions
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
#include <string.h>

#include "cascoda-bm/cascoda_bm.h"
#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_wait.h"
#include "ca821x_api.h"
#include "mac_messages.h"

/******************************************************************************/
/****** Global Parameters that can by set via EVBME_SET_request          ******/
/******************************************************************************/
uint8_t EVBME_HasReset = 0; //!< Used to notify apps that radio has been reset
uint8_t EVBME_UseMAC   = 0; //!< Use MAC functionality during phy tests

static const char *app_name;    //!< String describing initialised app
static u8_t        CLKExternal; //!< Nonzero if the CA821x is supplying a 4MHz clock

/** Function pointer for sending ASCII reporting messages upstream */
void (*EVBME_Message)(char *message, size_t len, struct ca821x_dev *pDeviceRef);
/** Function pointer for sending API commands upstream. */
void (*MAC_Message)(u8_t CommandId, u8_t Count, const u8_t *pBuffer);
/** Function pointer for reinitialising the app upon a restart **/
int (*app_reinitialise)(struct ca821x_dev *pDeviceRef);

// serial dispatch function
int (*cascoda_serial_dispatch)(u8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

// static function prototypes
static ca_error EVBME_WakeupCallback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_ResetRF(uint8_t ms, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_Connect(const char *aAppName, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_ExternalClock(u8_t on_offb, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_Wakeup_callback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef);
static ca_error EVBME_CAX_Wakeup(enum powerdown_mode mode, int timeout_ms, struct ca821x_dev *pDeviceRef);
static void     EVBME_WakeUpRF(void);

#if defined(USE_USB) || defined(USE_UART)
static void     EVBME_COMM_CHECK_request(struct SerialBuffer *SerialRxBuffer);
static int      EVBMEUpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
static void     EVBMESendDownStream(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);
static int      EVBMECheckSerialCommand(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef);
static void     EVBME_Disconnect(void);
static ca_error EVBME_SET_request(uint8_t            Attribute,
                                  uint8_t            AttributeLength,
                                  uint8_t *          AttributeValue,
                                  struct ca821x_dev *pDeviceRef);

static void EVBME_COMM_CHECK_request(struct SerialBuffer *SerialRxBuffer)
{
	struct EVBME_COMM_CHECK_request *msg = (struct EVBME_COMM_CHECK_request *)(SerialRxBuffer->Data);

	if (msg->mIndSize == 0 || SerialRxBuffer->CmdLen == 3)
		msg->mIndSize = 1;

	uint8_t buf[msg->mIndSize];

	TIME_WaitTicks(msg->mDelay);

	memset(buf, 0, sizeof(buf));
	buf[0] = msg->mHandle;

	if (!MAC_Message)
		return;

	for (uint8_t i = 0; i < msg->mIndCount; i++)
	{
		MAC_Message(EVBME_COMM_INDICATION, msg->mIndSize, buf);
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch upstream messages for EVBME
 *******************************************************************************
 * \param SerialRxBuffer - Serial message to process
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return  0: Message was not consumed by EVBME (should go downstream)
 *         >0: Message was consumed by EVBME
 *******************************************************************************
 ******************************************************************************/
static int EVBMEUpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	ca_error status;
	int      ret = 0;

	switch (SerialRxBuffer->CmdId)
	{
	case EVBME_SET_REQUEST:
		ret = 1;
		status =
		    EVBME_SET_request(SerialRxBuffer->Data[0], SerialRxBuffer->Data[1], SerialRxBuffer->Data + 2, pDeviceRef);
		if (status == CA_ERROR_INVALID)
			ca_log_warn("EVBME_SET INVALID ATTRIBUTE");
		else if (status == CA_ERROR_UNKNOWN)
			ca_log_warn("EVBME_SET UNKNOWN ATTRIBUTE");
		break;
	case EVBME_GUI_CONNECTED:
		ret = 1;
		EVBME_Connect(app_name, pDeviceRef);
		break;
	case EVBME_GUI_DISCONNECTED:
		ret = 1;
		EVBME_Disconnect();
		break;
	case EVBME_MESSAGE_INDICATION:
		ret = 1;
		break;
	case EVBME_COMM_CHECK:
		ret = 1;
		EVBME_COMM_CHECK_request(SerialRxBuffer);
		break;
	default:
		// check if MAC/API straight-through command requires additional action
		ret = EVBMECheckSerialCommand(SerialRxBuffer, pDeviceRef);
		break;
	}

	return ret;
} // End of EVBMEUpStreamDispatch()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends UpStream Command from Serial DownStream to SPI
 *******************************************************************************
 * \param buf - Message to send DownStream
 * \param len - Length of buf
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
static void EVBMESendDownStream(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	struct MAC_Message response;
	ca_error           status;

	status = DISPATCH_ToCA821x(buf, len, &response.CommandId, pDeviceRef);

	if (status == CA_ERROR_SUCCESS)
	{
#if CASCODA_CA_VER == 8210
		struct MAC_Message *cmd = (struct MAC_Message *)buf;
		switch (cmd->CommandId)
		{
		case SPI_MLME_SET_REQUEST:
			if (cmd->PData.SetReq.PIBAttribute == macShortAddress)
				pDeviceRef->shortaddr = GETLE16(cmd->PData.SetReq.PIBAttributeValue);
			else if (cmd->PData.SetReq.PIBAttribute == nsIEEEAddress)
				memcpy(pDeviceRef->extaddr, cmd->PData.SetReq.PIBAttributeValue, 8);
			break;
		case SPI_MLME_RESET_REQUEST:
			if (cmd->PData.u8Param)
				pDeviceRef->shortaddr = 0xFFFF;
			break;
		case SPI_HWME_SET_REQUEST:
			if (cmd->PData.HWMESetReq.HWAttribute == HWME_LQIMODE)
				pDeviceRef->lqi_mode = cmd->PData.HWMESetReq.HWAttributeValue[0];
			break;
		}
#endif
		// Synchronous Confirms have to be sent upstream for interrupt driven handlers
		if ((buf[0] != SPI_IDLE) && (buf[0] != SPI_NACK) && (buf[0] & SPI_SYN))
		{
			DISPATCH_NotHandled(&response);
		}
	}
} // End of EVBMESendDownStream()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks if MAC/API Command from Serial requires Action
 *******************************************************************************
 * \param SerialRxBuffer - Serial message to process
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return  0: Message was not consumed by EVBME (should go downstream)
 *         >0: Message was consumed by EVBME
 *******************************************************************************
 ******************************************************************************/
static int EVBMECheckSerialCommand(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	uint8_t            status, val;
	struct MAC_Message response;
	int                ret = 0;

	if (SerialRxBuffer->CmdId == SPI_MLME_SET_REQUEST)
	{
		if (TDME_CheckPIBAttribute(SerialRxBuffer->Data[0], SerialRxBuffer->Data[2], SerialRxBuffer->Data + 3))
		{
			ret                       = 1;
			response.CommandId        = SPI_MLME_SET_CONFIRM;
			response.Length           = 3;
			response.PData.Payload[0] = MAC_INVALID_PARAMETER;
			response.PData.Payload[1] = SerialRxBuffer->Data[0];
			response.PData.Payload[2] = SerialRxBuffer->Data[1];
			DISPATCH_NotHandled(&response);
		}
	}

	// MLME set / get phyTransmitPower
	if ((SerialRxBuffer->CmdId == SPI_MLME_SET_REQUEST) && (SerialRxBuffer->Data[0] == phyTransmitPower))
	{
		status                    = TDME_SetTxPower(SerialRxBuffer->Data[3], pDeviceRef);
		ret                       = 1;
		response.CommandId        = SPI_MLME_SET_CONFIRM;
		response.Length           = 3;
		response.PData.Payload[0] = status;
		response.PData.Payload[1] = SerialRxBuffer->Data[0];
		response.PData.Payload[2] = SerialRxBuffer->Data[1];
		DISPATCH_NotHandled(&response);
	}
	if ((SerialRxBuffer->CmdId == SPI_MLME_GET_REQUEST) && (SerialRxBuffer->Data[0] == phyTransmitPower))
	{
		status                    = TDME_GetTxPower(&val, pDeviceRef);
		ret                       = 1;
		response.CommandId        = SPI_MLME_GET_CONFIRM;
		response.Length           = 5;
		response.PData.Payload[0] = status;
		response.PData.Payload[1] = SerialRxBuffer->Data[0];
		response.PData.Payload[2] = SerialRxBuffer->Data[1];
		response.PData.Payload[3] = 1;
		response.PData.Payload[4] = val;
		DISPATCH_NotHandled(&response);
	}

	if (SerialRxBuffer->CmdId == SPI_MLME_ASSOCIATE_REQUEST)
		TDME_ChannelInit(SerialRxBuffer->Data[0], pDeviceRef);
	else if (SerialRxBuffer->CmdId == SPI_MLME_START_REQUEST)
		TDME_ChannelInit(SerialRxBuffer->Data[2], pDeviceRef);
	else if ((SerialRxBuffer->CmdId == SPI_MLME_SET_REQUEST) && (SerialRxBuffer->Data[0] == phyCurrentChannel))
		TDME_ChannelInit(SerialRxBuffer->Data[3], pDeviceRef);
	else if ((SerialRxBuffer->CmdId == SPI_TDME_SET_REQUEST) && (SerialRxBuffer->Data[0] == TDME_CHANNEL))
		TDME_ChannelInit(SerialRxBuffer->Data[2], pDeviceRef);

	return ret;
} // End of EVBMECheckSerialCommand()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset Test Modes and RF when GUI is disconnected
 *******************************************************************************
 ******************************************************************************/
static void EVBME_Disconnect(void)
{
	EVBME_HasReset = 1;
	BSP_ResetRF(50); // reset RF for 50 ms
} // End of EVBME_Disconnect()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief EVBME_SET_request according to EVBME Spec
 *******************************************************************************
 * \param Attribute - Attribute Specifier
 * \param AttributeLength - Attribute Length
 * \param AttributeValue - Pointer to Attribute Value
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_SET_request(uint8_t            Attribute,
                                  uint8_t            AttributeLength,
                                  uint8_t *          AttributeValue,
                                  struct ca821x_dev *pDeviceRef)
{
	ca_error status;

	/* branch for EVBME-SET Attributes */
	switch (Attribute)
	{
	case EVBME_RESETRF:
		if (AttributeLength != 1)
		{
			status = CA_ERROR_INVALID;
		}
		else
		{
			status         = EVBME_ResetRF(AttributeValue[0], pDeviceRef);
			EVBME_HasReset = AttributeValue[0];
		}
		break;

	case EVBME_CFGPINS:
		if ((AttributeLength != 1) || (AttributeValue[0] > 0x03))
		{
			status = CA_ERROR_INVALID;
		}
		else
		{
			EVBME_UseMAC = AttributeValue[0];
			status       = CA_ERROR_SUCCESS;
		}
		break;

	case EVBME_WAKEUPRF:
		status = CA_ERROR_SUCCESS;
		EVBME_WakeUpRF();
		break;

	default:
		status = CA_ERROR_UNKNOWN; /* what's that ??? */
		break;
	}

	return status;
} // End of EVBME_SET_request()

#else // defined(USE_USB) || defined(USE_UART)

/** dummy stub function if no serial interface is defined */
static void EVBME_Message_DUMMY(char *pBuffer, size_t Count, struct ca821x_dev *pDeviceRef)
{
	(void)pBuffer;
	(void)Count;
	(void)pDeviceRef;
}

/** dummy stub function if no serial interface is defined */
static void MAC_Message_DUMMY(u8_t CommandId, u8_t Count, const u8_t *pBuffer)
{
	(void)CommandId;
	(void)Count;
	(void)pBuffer;
}

#endif // defined(USE_USB) || defined(USE_UART)

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for Wakeup indication from CA-821X after reset
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_WakeupCallback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	ca_error  status;
	ca_error *context = (ca_error *)WAIT_GetContext();
	uint8_t   chipid[2];
	uint8_t   attlen;

	if (params->WakeUpCondition != HWME_WAKEUP_POWERUP)
	{
		ca_log_warn("CA-821X wakeup condition not power-up, check status");
		status = CA_ERROR_FAIL;
		goto exit;
	}

	if (TDME_ChipInit(pDeviceRef))
	{
		ca_log_crit("CA-821X initialisation failed\n");
		status = CA_ERROR_FAIL;
		goto exit;
	}

	if (HWME_GET_request_sync(HWME_CHIPID, &attlen, chipid, pDeviceRef))
	{
		ca_log_warn("CA-821X ID Failed");
		status = CA_ERROR_FAIL;
	}
	else
	{
		status = CA_ERROR_SUCCESS;
		ca_log_info("CA-821X V%d.%d", chipid[0], chipid[1]); // product id.version number
		// MAC_MPW for V0.x
		if (chipid[0] == 0)
			pDeviceRef->MAC_MPW = 1;
		else
			pDeviceRef->MAC_MPW = 0;
	}

exit:
	if (context)
		*context = status;
	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset RF device and check connection to RF
 *******************************************************************************
 * \param ms - Number of milliseconds to hold reset pin low for
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_ResetRF(uint8_t ms, struct ca821x_dev *pDeviceRef)
{
	ca_error status   = CA_ERROR_SUCCESS;
	ca_error cbStatus = CA_ERROR_FAIL;

	if (ms > 1) // reset RF
		BSP_ResetRF(ms);
	else
		BSP_ResetRF(2);

	status = WAIT_CallbackSwap(
	    SPI_HWME_WAKEUP_INDICATION, (ca821x_generic_callback)&EVBME_WakeupCallback, 100, &cbStatus, pDeviceRef);

	if (status == CA_ERROR_SUCCESS)
		status = cbStatus;

	if (status == CA_ERROR_SPI_WAIT_TIMEOUT)
	{
		ca_log_crit("CA-821X connection timed out waiting for wakeup indication, check hardware");
		goto exit;
	}

exit:
	return status;
} // End of EVBME_ResetRF()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset RF device and check connection to RF when GUI is connected
 *******************************************************************************
 * \param aAppName - App name string
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_Connect(const char *aAppName, struct ca821x_dev *pDeviceRef)
{
	ca_error    status;
	u64_t       device_id = BSP_GetUniqueId();
	const char *appname   = aAppName ? aAppName : "UNKNAPP";

	EVBME_HasReset = 1; // initialise/reset PIBs on higher layers
	ca_log_note("EVBME connected, %s, %s", appname, ca821x_get_version());

	if (device_id)
	{
		ca_log_note("Device ID: %08x%08x", (uint32_t)(device_id >> 32ULL), (uint32_t)device_id);
	}

	status = EVBME_ResetRF(50, pDeviceRef); // reset RF for 50 ms

	return status;
} // End of EVBME_Connect()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Switch External Clock from CAX on or off
 *******************************************************************************
 * \param on_offb - 0: Off 1: On
 * \param pDeviceRef   Pointer to initialised ca821x_device_ref struct
 * \retval CA_ERROR_SUCCESS for success, CA_ERROR_FAIL for failure
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_ExternalClock(u8_t on_offb, struct ca821x_dev *pDeviceRef)
{
	u8_t clkparam[2];

	if (on_offb)
	{
		clkparam[0] = 0x03; // 4 MHz
		clkparam[1] = 0x02; // GPIO2
	}
	else
	{
		clkparam[0] = 0x00; // turn clock off/
		clkparam[1] = 0x00;
	}
	if (HWME_SET_request_sync(HWME_SYSCLKOUT, 2, clkparam, pDeviceRef))
	{
		return CA_ERROR_FAIL;
	}
	return CA_ERROR_SUCCESS;
} // End of EVBME_CAX_ExternalClock()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Program CAX Low-Power Mode
 *******************************************************************************
 * \param mode - the required power down mode
 * \param sleeptime_ms - milliseconds to sleep for
 * \param pDeviceRef - a pointer to the ca821x_dev struct
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef)
{
	u8_t          pdparam[5];
	ca_mac_status status;
	u8_t          i;

	if ((mode == PDM_POWEROFF) || (mode == PDM_DPD))
		pdparam[0] = 0x1C; // power-off mode 1, wake-up by sleep timer
	else if (mode == PDM_POWERDOWN)
		pdparam[0] = 0x2A; // power-off mode 0, wake-up by gpio (ssb)
	else if (mode == PDM_STANDBY)
		pdparam[0] = 0x24; // standby mode,     wake-up by gpio (ssb)
	else
		return CA_ERROR_SUCCESS; // nothing to do, remain active

	pdparam[1] = LS0_BYTE(sleeptime_ms);
	pdparam[2] = LS1_BYTE(sleeptime_ms);
	pdparam[3] = LS2_BYTE(sleeptime_ms);
	pdparam[4] = LS3_BYTE(sleeptime_ms);

	if ((status = HWME_SET_request_sync(HWME_POWERCON, 5, pdparam, pDeviceRef)))
	{
		ca_log_crit("CA-821X PowerDown: %02X", status);
		return CA_ERROR_FAIL;
	}

	/* check max. 10 times if device has gone into power-down mode */
	for (i = 0; i < 10; ++i)
	{
		BSP_WaitUs(600);
		if (BSP_SenseRFIRQ()) /* device in power-down ? */
			return CA_ERROR_SUCCESS;
	}

	printf("CA-821X No PowerDown\n");
	return CA_ERROR_FAIL;
} // End of EVBME_CAX_PowerDown()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Callback for Wakeup indication from CA-821X after wakeup
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_Wakeup_callback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	u8_t condition = params->WakeUpCondition;
	u8_t mode;

	if (!WAIT_GetContext())
		return 0;
	mode = *((u8_t *)WAIT_GetContext());

	if ((((mode == PDM_STANDBY) && (condition != HWME_WAKEUP_STBY_GPIO)) ||
	     ((mode == PDM_POWERDOWN) && (condition != HWME_WAKEUP_POFF_GPIO)) ||
	     ((mode == PDM_POWEROFF) && (condition != HWME_WAKEUP_POFF_SLT))) &&
	    (condition != HWME_WAKEUP_POWERUP))
	{
		ca_log_warn("CA-821X wakeup: mode: %02X; condition: %02X", mode, condition);
	}

	if ((mode == PDM_POWEROFF) || (mode == PDM_POWERDOWN))
	{
		TDME_ChipInit(pDeviceRef);
	}

	return CA_ERROR_SUCCESS;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wait for CAX wakeup after power-down
 *******************************************************************************
 * CAX GPIO wake-up by sending dummy SPI packet, wait for wakeup message.
 *******************************************************************************
 * \param mode - Power-Down Mode
 * \param timeout_ms - Timeout for the wakeup indication (ms)
 * \param pDeviceRef - Pointer to initialised ca821x_device_ref struct
 *******************************************************************************
 ******************************************************************************/
static ca_error EVBME_CAX_Wakeup(enum powerdown_mode mode, int timeout_ms, struct ca821x_dev *pDeviceRef)
{
	ca_error status = CA_ERROR_SUCCESS;

	// wake up device by gpio by toggling chip select
	if ((mode == PDM_STANDBY) || (mode == PDM_POWERDOWN))
	{
		EVBME_WakeUpRF();
	}
	DISPATCH_ReadCA821x(pDeviceRef);

	status = WAIT_CallbackSwap(
	    SPI_HWME_WAKEUP_INDICATION, (ca821x_generic_callback)&EVBME_CAX_Wakeup_callback, timeout_ms, &mode, pDeviceRef);

	if (status)
	{
		ca_log_crit("CA-821X wakeup: status: %02X", status);
	}

	return status;
} // End of EVBME_CAX_Wakeup()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wake Up CAX RF Chip by toggling GPIO
 *******************************************************************************
 * requires several 32kHz cycles on SPI CSB
 *******************************************************************************
 ******************************************************************************/
static void EVBME_WakeUpRF(void)
{
	BSP_SetRFSSBLow();
	BSP_WaitUs(100);
	BSP_SetRFSSBHigh();
}

void cascoda_io_handler(struct ca821x_dev *pDeviceRef)
{
	DISPATCH_FromCA821x(pDeviceRef);

#if defined(USE_USB) || defined(USE_UART)
	SerialGetCommand();
	if (SerialRxPending && !SPI_IsFifoAlmostFull())
	{
		bool handled = false;
		if (cascoda_serial_dispatch)
		{
			if (cascoda_serial_dispatch(&SerialRxBuffer.CmdId, SerialRxBuffer.CmdLen + 2, pDeviceRef) > 0)
			{
				handled = true;
			}
		}
		if (EVBMEUpStreamDispatch(&SerialRxBuffer, pDeviceRef) > 0)
			handled = true;
		if (!handled)
		{
			EVBMESendDownStream(&SerialRxBuffer.CmdId, SerialRxBuffer.CmdLen + 2, pDeviceRef);
		}
#if defined(USE_UART)
		/* send RX_RDY */
		SerialSendRxRdy();
#endif /* USE_UART */
		SerialRxPending = false;
	}
#endif /* USE_UART || USE_USB */
}

void DISPATCH_NotHandled(struct MAC_Message *msg)
{
	if (MAC_Message)
		MAC_Message(msg->CommandId, msg->Length, msg->PData.Payload);

} // End of DISPATCH_NotHandled()

void EVBME_SwitchClock(struct ca821x_dev *pDeviceRef, u8_t useExternalClock)
{
	if (CLKExternal == useExternalClock)
		return;
	CLKExternal = useExternalClock;

	if (!CLKExternal)
	{
		// switch before turning external clock off
		BSP_UseExternalClock(CLKExternal);
	}

	if (EVBME_CAX_ExternalClock(CLKExternal, pDeviceRef))
	{
		//Failed
		CLKExternal = 0;
		BSP_UseExternalClock(CLKExternal);
		return;
	}

	if (CLKExternal)
	{
		// switch after turning external clock on
		BSP_UseExternalClock(CLKExternal);
	}

} // End of EVBME_SwitchClock()

void EVBME_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef)
{
	u8_t CLKExternal_saved;

	// save external clock status
	CLKExternal_saved = CLKExternal;

	// switch off external clock
	EVBME_SwitchClock(pDeviceRef, 0);

	// power down CAX
	EVBME_CAX_PowerDown(mode, sleeptime_ms, pDeviceRef);

	// power down
	if (mode == PDM_DPD) // mode has to use CAX sleep timer
		BSP_PowerDown(sleeptime_ms, 0, 1, pDeviceRef);
	else if (mode == PDM_POWEROFF) // mode has to use CAX sleep timer
		BSP_PowerDown(sleeptime_ms, 0, 0, pDeviceRef);
	else if (mode == PDM_ALLON)
		TIME_WaitTicks(sleeptime_ms);
	else
		BSP_PowerDown(sleeptime_ms, 1, 0, pDeviceRef);

	// wake up CAX
	if ((mode == PDM_POWEROFF) && BSP_IsWatchdogTriggered())
	{
		EVBME_CAX_Restart(pDeviceRef);
	}
	else
	{
		if ((mode != PDM_ACTIVE) && (mode != PDM_ALLON))
		{
			if (EVBME_CAX_Wakeup(mode, sleeptime_ms, pDeviceRef) != CA_ERROR_SUCCESS)
			{
				EVBME_CAX_Restart(pDeviceRef);
			}
		}
	}

	// restore external clock status
	EVBME_SwitchClock(pDeviceRef, CLKExternal_saved);

} // End of EVBME_PowerDown()

void EVBME_CAX_Restart(struct ca821x_dev *pDeviceRef)
{
	ca_error status = CA_ERROR_SUCCESS;

	EVBME_SwitchClock(pDeviceRef, 0);

	BSP_ResetRF(5);

	status = WAIT_Callback(SPI_HWME_WAKEUP_INDICATION, 20, NULL, pDeviceRef);

	if (status)
	{
		ca_log_crit("CA-821X restart: status: %02X", status);
		return;
	}

	if (TDME_ChipInit(pDeviceRef))
	{
		ca_log_crit("CA-821X restart: ChipInit fail");
		return;
	}

	// call application specific re-initialisation
	if (app_reinitialise)
		app_reinitialise(pDeviceRef);
}

ca_error EVBMEInitialise(const char *aAppName, struct ca821x_dev *pDeviceRef)
{
	ca_error status;

	app_name = aAppName;
#ifdef stdout
	setbuf(stdout, NULL);
#endif

	BSP_Initialise(pDeviceRef, DISPATCH_ReadCA821x); // initialise chip

	SPI_Initialise();

	EVBME_HasReset = 1; // initialise/reset PIBs on higher layers

	// function pointer assignments
	pDeviceRef->ca821x_api_downstream = &DISPATCH_ToCA821x;

#if defined(USE_USB)
	EVBME_Message = EVBME_Message_USB;
	MAC_Message   = MAC_Message_USB;
#elif defined(USE_UART)
	EVBME_Message = EVBME_Message_UART;
	MAC_Message   = MAC_Message_UART;
#else
	EVBME_Message = EVBME_Message_DUMMY;
	MAC_Message   = MAC_Message_DUMMY;
#endif

	status = EVBME_Connect(aAppName, pDeviceRef); // reset and connect RF

	return status;
} // End of EVBMEInitialise()

const char *EVBME_GetAppName(void)
{
	return app_name;
} // End of EVBMEInitialise()
