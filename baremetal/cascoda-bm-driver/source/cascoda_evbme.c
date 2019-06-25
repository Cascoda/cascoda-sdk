/**
 * @file   cascoda_evbme.c
 * @brief  EvaBoard Management Entity (EVBME) functions
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

/** Function pointer that the BSP should call when USB present status changes **/
int (*usb_present_changed)(void);

// serial dispatch function
int (*cascoda_serial_dispatch)(u8_t *buf, size_t len, struct ca821x_dev *pDeviceRef);

/* dummy stub functions if no serial interface is defined */
void EVBME_Message_DUMMY(char *pBuffer, size_t Count, struct ca821x_dev *pDeviceRef)
{
	(void)pBuffer;
	(void)Count;
	(void)pDeviceRef;
}
void MAC_Message_DUMMY(u8_t CommandId, u8_t Count, const u8_t *pBuffer)
{
	(void)CommandId;
	(void)Count;
	(void)pBuffer;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialises EVBME after Reset
 *******************************************************************************
 * Initialises low level interfaces, resets and initialises CA-8210.
 *******************************************************************************
 * \param aAppName - App name string
 *******************************************************************************
 * \return Status of initialisation
 *******************************************************************************
 ******************************************************************************/
ca_error EVBMEInitialise(const char *aAppName, struct ca821x_dev *pDeviceRef)
{
	ca_error status;

	app_name = aAppName;
#ifdef stdout
	setbuf(stdout, NULL);
#endif

	BSP_Initialise(pDeviceRef); // initialise chip

	SPI_Initialise();

	EVBME_HasReset = 1; // initialise/reset PIBs on higher layers

	// function pointer assignments
	pDeviceRef->ca821x_api_downstream = &DISPATCH_ToCA821x;
	ca821x_wait_for_message           = &WAIT_Legacy;

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

struct EVBME_COMM_CHECK_request
{
	uint8_t mHandle;   //!< Handle identifying this comm check
	uint8_t mDelay;    //!< Delay before sending responses
	uint8_t mIndCount; //!< Number of indications to send up
};

void EVBME_COMM_CHECK_request(struct EVBME_COMM_CHECK_request *msg)
{
	TIME_WaitTicks(msg->mDelay);

	if (!MAC_Message)
		return;

	for (uint8_t i = 0; i < msg->mIndCount; i++)
	{
		MAC_Message(EVBME_COMM_INDICATION, 1, &(msg->mHandle));
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Dispatch upstream messages for EVBME
 *******************************************************************************
 * \param SerialRxBuffer - Serial message to process
 *******************************************************************************
 * \return  0: Message was not consumed by EVBME (should go downstream)
 *         >0: Message was consumed by EVBME
 *******************************************************************************
 ******************************************************************************/
int EVBMEUpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
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
			printf("INVALID ATTRIBUTE\n");
		else if (status == CA_ERROR_UNKNOWN)
			printf("UNKNOWN ATTRIBUTE\n");
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
		EVBME_COMM_CHECK_request((struct EVBME_COMM_CHECK_request *)(SerialRxBuffer->Data));
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
 * \param response - Container for synchronous response
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
void EVBMESendDownStream(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
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
			EVBMESendUpStream(&response);
		}
	}
} // End of EVBMESendDownStream()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Sends DownStream Command from API UpStream to Serial
 *******************************************************************************
 * \param msg - Message to send upstream
 *******************************************************************************
 ******************************************************************************/
void EVBMESendUpStream(struct MAC_Message *msg)
{
	if (MAC_Message)
		MAC_Message(msg->CommandId, msg->Length, msg->PData.Payload);

} // End of EVBMESendUpStream()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks if MAC/API Command from Serial requires Action
 *******************************************************************************
 * \param SerialRxBuffer - Serial message to process
 *******************************************************************************
 * \return  0: Message was not consumed by EVBME (should go downstream)
 *         >0: Message was consumed by EVBME
 *******************************************************************************
 ******************************************************************************/
int EVBMECheckSerialCommand(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
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
			EVBMESendUpStream(&response);
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
		EVBMESendUpStream(&response);
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
		EVBMESendUpStream(&response);
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
 * \brief EVBME_SET_request according to EVBME Spec
 *******************************************************************************
 * \param Attribute - Attribute Specifier
 * \param AttributeLength - Attribute Length
 * \param AttributeValue - Pointer to Attribute Value
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
ca_error EVBME_SET_request(uint8_t            Attribute,
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

int EVBME_WakeupCallback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	ca_error  status;
	ca_error *context = (ca_error *)WAIT_GetContext();
	uint8_t   chipid[2];
	uint8_t   attlen;

	if (params->WakeUpCondition != HWME_WAKEUP_POWERUP)
	{
		printf("CA-821X wakeup condition not power-up, check status\n");
		status = CA_ERROR_FAIL;
		goto exit;
	}

	if (TDME_ChipInit(pDeviceRef))
	{
		printf("CA-821X initialisation failed\n");
		status = CA_ERROR_FAIL;
		goto exit;
	}

	printf("CA-821X connected, ");
	if (HWME_GET_request_sync(HWME_CHIPID, &attlen, chipid, pDeviceRef))
	{
		printf("ID Failed");
		status = CA_ERROR_FAIL;
	}
	else
	{
		status = CA_ERROR_SUCCESS;
		printf("V%d.%d", chipid[0], chipid[1]); // product id.version number
		// MAC_MPW for V0.x
		if (chipid[0] == 0)
			pDeviceRef->MAC_MPW = 1;
		else
			pDeviceRef->MAC_MPW = 0;
	}
	printf("\n");

exit:
	if (context)
		*context = status;
	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset RF device and check connection to RF
 *******************************************************************************
 * \param ms - Number of milliseconds to hold reset pin low for
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
ca_error EVBME_ResetRF(uint8_t ms, struct ca821x_dev *pDeviceRef)
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
		printf("CA-821X connection timed out waiting for wakeup indication, check hardware\n");
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
 *******************************************************************************
 * \return EVBME Status
 *******************************************************************************
 ******************************************************************************/
ca_error EVBME_Connect(const char *aAppName, struct ca821x_dev *pDeviceRef)
{
	ca_error status;
	u64_t    device_id = BSP_GetUniqueId();

	EVBME_HasReset = 1; // initialise/reset PIBs on higher layers
	printf("EVBME connected");
	if (aAppName != NULLP)
	{
		printf(", %s", aAppName);
	}
	printf(", %s\n", ca821x_get_version());

	if (device_id)
	{
		printf("Device ID: %08x%08x\n", (uint32_t)(device_id >> 32ULL), (uint32_t)device_id);
	}

	status = EVBME_ResetRF(50, pDeviceRef); // reset RF for 50 ms

	return status;
} // End of EVBME_Connect()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Reset Test Modes and RF when GUI is disconnected
 *******************************************************************************
 ******************************************************************************/
void EVBME_Disconnect(void)
{
	EVBME_HasReset = 1;
	BSP_ResetRF(50); // reset RF for 50 ms
} // End of EVBME_Disconnect()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Processes messages received over available interfaces. This function
 *        should be called regularly from application context eg the main loop.
 *******************************************************************************
 ******************************************************************************/
void cascoda_io_handler(struct ca821x_dev *pDeviceRef)
{
	DISPATCH_FromCA821x(pDeviceRef);

#if defined(USE_USB) || defined(USE_UART)
	SerialGetCommand();
	if (SerialRxPending && !SPI_IsFifoFull())
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

/******************************************************************************/
/***************************************************************************/ /**
* \brief Put the system into a state of power-down for a given time
*******************************************************************************
* \param mode - Power-Down Mode
* \param sleeptime_sec - Seconds to sleep for
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
void EVBME_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef)
{
	u8_t CLKExternal_saved;

	// save external clock status
	CLKExternal_saved = CLKExternal;

	// switch off external clock
	EVBME_SwitchClock(pDeviceRef, 0);

	// power down CAX
	if ((mode == PDM_POWEROFF) || (mode == PDM_DPD)) // mode has to use CAX sleep timer
		EVBME_CAX_PowerDown(mode, sleeptime_ms, pDeviceRef);
	else
		EVBME_CAX_PowerDown(mode, 0, pDeviceRef);

	// power down
	if (mode == PDM_DPD) // mode has to use CAX sleep timer
		BSP_PowerDown(sleeptime_ms, 0, 1);
	else if (mode == PDM_POWEROFF) // mode has to use CAX sleep timer
		BSP_PowerDown(sleeptime_ms, 0, 0);
	else if (mode == PDM_ALLON)
		TIME_WaitTicks(sleeptime_ms);
	else
		BSP_PowerDown(sleeptime_ms, 1, 0);

	// Fastforward time for missed ticks
	if (mode != PDM_ALLON)
	{
		TIME_FastForward(sleeptime_ms);
	}

	// wake up CAX
	if ((mode == PDM_POWEROFF) && BSP_IsWatchdogTriggered())
	{
		EVBME_CAX_Restart(pDeviceRef);
	}
	else
	{
		if ((mode != PDM_ACTIVE) && (mode != PDM_ALLON))
			EVBME_CAX_Wakeup(mode, sleeptime_ms, pDeviceRef);
	}

	// restore external clock status
	EVBME_SwitchClock(pDeviceRef, CLKExternal_saved);

} // End of EVBME_PowerDown()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Switch External Clock from CAX on or off
 *******************************************************************************
 * \param on_offb - 0: Off 1: On
 * \retval CA_ERROR_SUCCESS for success, CA_ERROR_FAIL for failure
 *******************************************************************************
 ******************************************************************************/
ca_error EVBME_CAX_ExternalClock(u8_t on_offb, struct ca821x_dev *pDeviceRef)
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
 * \brief System Clock Switch
 ********************************************************************************
 * \param pDeviceRef - a pointer to the ca821x_dev struct
 * \param useExternalClock - boolean (1: use external clock from ca821x, 0: use internal clock)
 *******************************************************************************
 ******************************************************************************/
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

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Program CAX Low-Power Mode
 *******************************************************************************
 * \param mode - the required power down mode
 * \param sleeptime_ms - milliseconds to sleep for
 * \param pDeviceRef - a pointer to the ca821x_dev struct
 *******************************************************************************
 ******************************************************************************/
void EVBME_CAX_PowerDown(enum powerdown_mode mode, u32_t sleeptime_ms, struct ca821x_dev *pDeviceRef)
{
	u8_t          pdparam[5];
	ca_mac_status status;

	if ((mode == PDM_POWEROFF) || (mode == PDM_DPD))
		pdparam[0] = 0x1C; // power-off mode 1, wake-up by sleep timer
	else if (mode == PDM_POWERDOWN)
		pdparam[0] = 0x2A; // power-off mode 0, wake-up by gpio (ssb)
	else if (mode == PDM_STANDBY)
		pdparam[0] = 0x24; // standby mode,     wake-up by gpio (ssb)
	else
		return; // nothing to do, remain active

	pdparam[1] = LS0_BYTE(sleeptime_ms);
	pdparam[2] = LS1_BYTE(sleeptime_ms);
	pdparam[3] = LS2_BYTE(sleeptime_ms);
	pdparam[4] = LS3_BYTE(sleeptime_ms);

	if ((status = HWME_SET_request_sync(HWME_POWERCON, 5, pdparam, pDeviceRef)))
	{
		printf("CA-821X PowerDown: %02X\n", status);
	}
} // End of EVBME_CAX_PowerDown()

int EVBME_CAX_Wakeup_callback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
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
		printf("CA-821X wakeup: mode: %02X; condition: %02X\n", mode, condition);
	}

	if ((mode == PDM_POWEROFF) || (mode == PDM_POWERDOWN))
	{
		TDME_ChipInit(pDeviceRef);
	}

	return 0;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wait for CAX wakeup after power-down
 *******************************************************************************
 * CAX GPIO wake-up by sending dummy SPI packet, wait for wakeup message.
 *******************************************************************************
 * \param mode - Power-Down Mode
 * \param timeout_ms - Timeout for the wakeup indication (ms)
 *******************************************************************************
 ******************************************************************************/
void EVBME_CAX_Wakeup(enum powerdown_mode mode, int timeout_ms, struct ca821x_dev *pDeviceRef)
{
	ca_error status;

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
		printf("CA-821X wakeup: status: %02X\n", status);
	}

} // End of EVBME_CAX_Wakeup()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Restarts Air Interface
 *******************************************************************************
 * Resets and re-initialises CAX, calls app_reinitialise callback.
 *******************************************************************************
 ******************************************************************************/
void EVBME_CAX_Restart(struct ca821x_dev *pDeviceRef)
{
	ca_error status = CA_ERROR_SUCCESS;

	EVBME_SwitchClock(pDeviceRef, 0);

	BSP_ResetRF(5);

	status = WAIT_Callback(SPI_HWME_WAKEUP_INDICATION, 20, NULL, pDeviceRef);

	if (status)
	{
		printf("CA-821X restart: status: %02X\n", status);
		return;
	}

	if (TDME_ChipInit(pDeviceRef))
	{
		printf("CA-821X restart: ChipInit fail\n");
		return;
	}

	// call application specific re-initialisation
	if (app_reinitialise)
		app_reinitialise(pDeviceRef);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Wake Up CAX RF Chip by toggling GPIO
 *******************************************************************************
 * requires several 32kHz cycles on SPI CSB
 *******************************************************************************
 ******************************************************************************/
void EVBME_WakeUpRF(void)
{
	BSP_SetRFSSBLow();
	BSP_WaitUs(100);
	BSP_SetRFSSBHigh();
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Notifies the higher layer that an error has occurred in the EVBME
 *******************************************************************************
 * \param error_code - error number
 * \param has_restarted - True if the device was restarted because of the error
 * \param pDeviceRef - Reference to device that has caused the error
 *******************************************************************************
 ******************************************************************************/
void EVBME_ERROR_Indication(ca_error error_code, u8_t has_restarted, struct ca821x_dev *pDeviceRef)
{
	uint8_t payload[2] = {(uint8_t)error_code, has_restarted};
	(void)pDeviceRef;

	if (MAC_Message)
		MAC_Message(EVBME_ERROR_INDICATION, 2, payload);
}
