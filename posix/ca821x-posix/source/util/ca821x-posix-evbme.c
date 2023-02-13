/*
 * Copyright (c) 2020, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ca821x-generic-exchange.h"
#include "ca821x-posix-evbme-internal.h"

static struct ca_version_number string_to_ca_version_number(const char *aVersionString)
{
	char temp[50];
	temp[49]                                = '\0';
	char                    *strtokrContext = NULL;
	struct ca_version_number result;

	//Parse the string into 'ca_version_number' format
	strncpy(temp, aVersionString, sizeof(temp) - 1);
	char *first_digit = temp;

	//Get to the first digit in the string
	while (!isdigit(*first_digit) && *first_digit) first_digit++;

	//Extract the pure version part of the string
	char *version_token = strtok_r(first_digit, "-", &strtokrContext);

	//Extract integer part: before the '.'
	char *integer_token  = strtok_r(version_token, ".", &strtokrContext);
	result.major_version = atoi(integer_token);

	//Extract fractional part: after '.'
	char *fractional_token = strtok_r(NULL, ".", &strtokrContext);
	result.minor_version   = atoi(fractional_token);

	return result;
}

struct EVBME_callbacks *EVBME_GetCallbackStruct(struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *base = pDeviceRef->exchange_context;

	return &base->evbme_callbacks;
}

ca_error EVBME_COMM_CHECK_request(uint8_t            aHandle,
                                  uint8_t            aDelay,
                                  uint8_t            aIndCount,
                                  uint8_t            aIndSize,
                                  uint8_t            aPayloadLen,
                                  struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;
	size_t               maxPayload    = sizeof(txMsg.EVBME) - sizeof(txMsg.EVBME.COMM_CHECK_request);
	size_t               maxIndication = sizeof(txMsg.EVBME) - sizeof(txMsg.EVBME.COMM_indication);

	if ((aPayloadLen > maxPayload) || (aIndSize > maxIndication))
		return CA_ERROR_INVALID_ARGS;

	txMsg.mCmdId = EVBME_COMM_CHECK;
	txMsg.mLen   = sizeof(txMsg.EVBME.COMM_CHECK_request) + aPayloadLen;

	txMsg.EVBME.COMM_CHECK_request.mHandle   = aHandle;
	txMsg.EVBME.COMM_CHECK_request.mDelay    = aDelay;
	txMsg.EVBME.COMM_CHECK_request.mIndCount = aIndCount;
	txMsg.EVBME.COMM_CHECK_request.mIndSize  = aIndSize;
	memset(txMsg.EVBME.COMM_CHECK_request.mPayload, 0, aPayloadLen);

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_HOST_CONNECTED_notify(struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;

	txMsg.mCmdId = EVBME_HOST_CONNECTED;
	txMsg.mLen   = 0;

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_HOST_DISCONNECTED_notify(struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;

	txMsg.mCmdId = EVBME_HOST_DISCONNECTED;
	txMsg.mLen   = 0;

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_DFU_REBOOT_request(enum evbme_dfu_rebootmode aRebootMode, struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;

	txMsg.mCmdId = EVBME_DFU_CMD;
	txMsg.mLen   = sizeof(txMsg.EVBME.DFU_cmd.mDfuSubCmdId) + sizeof(txMsg.EVBME.DFU_cmd.mSubCmd.reboot_cmd);

	txMsg.EVBME.DFU_cmd.mDfuSubCmdId                  = DFU_REBOOT;
	txMsg.EVBME.DFU_cmd.mSubCmd.reboot_cmd.rebootMode = (uint8_t)aRebootMode;

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_DFU_ERASE_request(uint32_t aStartAddr, uint32_t aEraseLen, struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;

	txMsg.mCmdId = EVBME_DFU_CMD;
	txMsg.mLen   = sizeof(txMsg.EVBME.DFU_cmd.mDfuSubCmdId) + sizeof(txMsg.EVBME.DFU_cmd.mSubCmd.erase_cmd);

	txMsg.EVBME.DFU_cmd.mDfuSubCmdId = DFU_ERASE;
	PUTLE32(aStartAddr, txMsg.EVBME.DFU_cmd.mSubCmd.erase_cmd.startAddr);
	PUTLE32(aEraseLen, txMsg.EVBME.DFU_cmd.mSubCmd.erase_cmd.eraseLen);

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_DFU_WRITE_request(uint32_t aStartAddr, size_t aWriteLen, void *aWriteData, struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;
	size_t len = sizeof(txMsg.EVBME.DFU_cmd.mDfuSubCmdId) + sizeof(txMsg.EVBME.DFU_cmd.mSubCmd.write_cmd) + aWriteLen;

	if (len > sizeof(txMsg.EVBME))
		return CA_ERROR_INVALID_ARGS;

	txMsg.mCmdId = EVBME_DFU_CMD;
	txMsg.mLen   = len;

	txMsg.EVBME.DFU_cmd.mDfuSubCmdId = DFU_WRITE;
	PUTLE32(aStartAddr, txMsg.EVBME.DFU_cmd.mSubCmd.write_cmd.startAddr);
	memcpy(txMsg.EVBME.DFU_cmd.mSubCmd.write_cmd.data, aWriteData, aWriteLen);

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_DFU_CHECK_request(uint32_t           aStartAddr,
                                 uint32_t           aCheckLen,
                                 uint32_t           aChecksum,
                                 struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;

	txMsg.mCmdId = EVBME_DFU_CMD;
	txMsg.mLen   = sizeof(txMsg.EVBME.DFU_cmd.mDfuSubCmdId) + sizeof(txMsg.EVBME.DFU_cmd.mSubCmd.check_cmd);

	txMsg.EVBME.DFU_cmd.mDfuSubCmdId = DFU_CHECK;
	PUTLE32(aStartAddr, txMsg.EVBME.DFU_cmd.mSubCmd.check_cmd.startAddr);
	PUTLE32(aCheckLen, txMsg.EVBME.DFU_cmd.mSubCmd.check_cmd.checkLen);
	PUTLE32(aChecksum, txMsg.EVBME.DFU_cmd.mSubCmd.check_cmd.checksum);

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_DFU_BOOTMODE_request(enum evbme_dfu_rebootmode aBootMode, struct ca821x_dev *pDeviceRef)
{
	struct EVBME_Message txMsg;

	txMsg.mCmdId = EVBME_DFU_CMD;
	txMsg.mLen   = sizeof(txMsg.EVBME.DFU_cmd.mDfuSubCmdId) + sizeof(txMsg.EVBME.DFU_cmd.mSubCmd.bootmode_cmd);

	txMsg.EVBME.DFU_cmd.mDfuSubCmdId                  = DFU_BOOTMODE;
	txMsg.EVBME.DFU_cmd.mSubCmd.bootmode_cmd.bootMode = (uint8_t)aBootMode;

	return ca821x_api_downstream((uint8_t *)&txMsg, NULL, pDeviceRef);
}

ca_error EVBME_SET_request_sync(enum evbme_attribute aAttrId,
                                size_t               aAttrLen,
                                uint8_t             *aAttrData,
                                struct ca821x_dev   *pDeviceRef)
{
	ca_error             status = CA_ERROR_SUCCESS;
	struct EVBME_Message msg;
	size_t               maxPayload = sizeof(msg.EVBME) - sizeof(msg.EVBME.SET_request);

	if (aAttrLen > maxPayload)
		return CA_ERROR_INVALID_ARGS;

	msg.mCmdId = EVBME_SET_REQUEST;
	msg.mLen   = sizeof(msg.EVBME.SET_request) + aAttrLen;

	msg.EVBME.SET_request.mAttributeId  = aAttrId;
	msg.EVBME.SET_request.mAttributeLen = aAttrLen;
	memcpy(msg.EVBME.SET_request.mAttribute, aAttrData, aAttrLen);

	status = ca821x_api_downstream((uint8_t *)&msg, (uint8_t *)&msg, pDeviceRef);

	//If the command didn't fail, replace the status with the value of the SET Confirm.
	if (!status)
	{
		status = (ca_error)msg.EVBME.SET_confirm.mStatus;
	}

	return status;
}

ca_error EVBME_GET_request_sync(enum evbme_attribute aAttrId,
                                size_t               aMaxAttrLen,
                                uint8_t             *aAttrData,
                                uint8_t             *aAttrLen,
                                struct ca821x_dev   *pDeviceRef)
{
	ca_error             status = CA_ERROR_SUCCESS;
	struct EVBME_Message msg;

	if (aMaxAttrLen > 255)
		aMaxAttrLen = 255;

	msg.mCmdId = EVBME_GET_REQUEST;
	msg.mLen   = sizeof(msg.EVBME.GET_request);

	msg.EVBME.GET_request.mAttributeId = (uint8_t)aAttrId;

	status = ca821x_api_downstream((uint8_t *)&msg, (uint8_t *)&msg, pDeviceRef);

	if (status)
		goto exit;

	status = msg.EVBME.GET_confirm.mStatus;

	if (status)
		goto exit;

	if (msg.EVBME.GET_confirm.mAttributeId != (uint8_t)aAttrId)
	{
		status = CA_ERROR_FAIL;
		goto exit;
	}

	if (msg.EVBME.GET_confirm.mAttributeLen > aMaxAttrLen)
	{
		status = CA_ERROR_NO_BUFFER;
		goto exit;
	}

	memcpy(aAttrData, msg.EVBME.GET_confirm.mAttribute, msg.EVBME.GET_confirm.mAttributeLen);
	if (aAttrLen)
		*aAttrLen = msg.EVBME.GET_confirm.mAttributeLen;

exit:
	return status;
}

int EVBME_CompareVersions(const char               *aVersion1,
                          const char               *aVersion2,
                          struct ca_version_number *aVersion1Number,
                          struct ca_version_number *aVersion2Number)
{
	struct ca_version_number version1Number = string_to_ca_version_number(aVersion1);
	struct ca_version_number version2Number = string_to_ca_version_number(aVersion2);

	if (aVersion1Number)
		*aVersion1Number = version1Number;
	if (aVersion2Number)
		*aVersion2Number = version2Number;

	//Compare the version numbers and return the result
	if (version1Number.major_version == version2Number.major_version)
	{
		if (version1Number.minor_version == version2Number.minor_version)
			return 0;
		else if (version1Number.minor_version > version2Number.minor_version)
			return 1;
		else
			return -1;
	}
	else if (version1Number.major_version > version2Number.major_version)
		return 1;
	else
		return -1;
}

ca_error EVBME_CheckVersion(const char *aMinVerString, struct ca821x_dev *pDeviceRef)
{
	ca_error                 status = CA_ERROR_SUCCESS;
	const char              *hostVerString;
	uint8_t                  evbmeVerString[50]      = {};
	bool                     evbmeVerNumLessThan0_11 = false;
	struct ca_version_number hostVerNumber, evbmeVerNumber;

	//Get the host version number
	hostVerString = ca821x_get_version_nodate();

	//Get the EVBME version number (if known)
	ca_error error = EVBME_GET_request_sync(EVBME_VERSTRING, sizeof(evbmeVerString), evbmeVerString, NULL, pDeviceRef);
	if (error)
	{
		ca_log_warn("Failed to get firmware version number. This probably means that the version number is less than "
		            "0.11. Please update firmware to at least version 0.11.\n");
		evbmeVerNumLessThan0_11 = true;
	}

	// There is a minimum firmware version requirement
	if (aMinVerString != NULL)
	{
		// No way of knowing if the minimum requirement is met
		if (evbmeVerNumLessThan0_11)
		{
			ca_log_crit(
			    "FATAL: The connected module's firmware (version less than 0.11) is out of date and not supported "
			    "by this "
			    "application. The minimum firmware version required by this application is %s.\n Please update "
			    "the firmware.",
			    aMinVerString);
			status = CA_ERROR_FAIL;
		}
		// Minimum requirement is not met
		else if (EVBME_CompareVersions((char *)evbmeVerString, aMinVerString, &evbmeVerNumber, NULL) == -1)
		{
			ca_log_crit(
			    "FATAL: The connected module's firmware (version v%d.%d) is out of date and not supported by this "
			    "application. The minimum firmware version required by this application is %s.\n Please update "
			    "the firmware.",
			    evbmeVerNumber.major_version,
			    evbmeVerNumber.minor_version,
			    aMinVerString);
			status = CA_ERROR_FAIL;
		}
	}
	// No minimum requirement
	else
	{
		if (!evbmeVerNumLessThan0_11)
		{
			int result = EVBME_CompareVersions(hostVerString, (char *)evbmeVerString, &hostVerNumber, &evbmeVerNumber);
			if (result != 0) // Host and EVBME versions are not equal
			{
				ca_log_warn("VERSION MISMATCH: host version is v%d.%d and Chili module firmware version is v%d.%d.\n",
				            hostVerNumber.major_version,
				            hostVerNumber.minor_version,
				            evbmeVerNumber.major_version,
				            evbmeVerNumber.minor_version);
				if (result == -1) // Host version older than connected module's EVBME version
					ca_log_warn("It is recommended that you get the latest host version of this application.\n");
				else // Connected module's EVBME version is older than the host version.
					ca_log_warn(
					    "It is recommended that you get the latest firmware version for the connected module.\n");
			}
		}
	}

	return status;
}

ca_error ca821x_evbme_dispatch(uint8_t *aBuf, size_t aBufLen, struct ca821x_dev *pDeviceRef)
{
	struct ca821x_exchange_base *base     = pDeviceRef->exchange_context;
	EVBME_Message_callback       callback = NULL;
	struct EVBME_Message        *rxMsg    = (struct EVBME_Message *)aBuf;
	ca_error                     error    = CA_ERROR_NOT_HANDLED;

	if (aBufLen != (size_t)(rxMsg->mLen) + 2)
		return CA_ERROR_INVALID_ARGS;

	switch (rxMsg->mCmdId)
	{
	case EVBME_MESSAGE_INDICATION:
		callback = base->evbme_callbacks.EVBME_MESSAGE_indication;
		break;
	case EVBME_COMM_INDICATION:
		callback = base->evbme_callbacks.EVBME_COMM_indication;
		break;
	case EVBME_DFU_CMD:
		callback = base->evbme_callbacks.EVBME_DFU_cmd;
		break;
	}

	if (callback)
		error = callback(rxMsg, pDeviceRef);

	return error;
}
