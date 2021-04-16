/*
 *  Copyright (c) 2020, Cascoda Ltd.
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

#include <algorithm>
#include <cstring>
#include <limits>

#include "ca821x-posix/ca821x-posix.h"

#include "common/DeviceList.hpp"
#include "flash/Flasher.hpp"

namespace ca {

/*
 * CRC32 algorithm taken from the zlib source, which is
 * Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
 */
static const unsigned int tinf_crc32tab[16] = {0x00000000,
                                               0x1db71064,
                                               0x3b6e20c8,
                                               0x26d930ac,
                                               0x76dc4190,
                                               0x6b6b51f4,
                                               0x4db26158,
                                               0x5005713c,
                                               0xedb88320,
                                               0xf00f9344,
                                               0xd6d6a3e8,
                                               0xcb61b38c,
                                               0x9b64c2b0,
                                               0x86d3d2d4,
                                               0xa00ae278,
                                               0xbdbdf21c};

Flasher::Flasher(const char *aFilePath, const DeviceInfo &aDeviceInfo, FlashType aFlashType)
    : mFile(aFilePath, std::ios::in | std::ios::binary | std::ios::ate)
    , mPageSize()
    , mDeviceInfo(aDeviceInfo)
    , mState(INIT)
    , mFlashType(aFlashType)
{
	if (!mFile.is_open())
	{
		fprintf(stderr, "Error: File \"%s\" could not be opened\n", aFilePath);
		return;
	}

	mFileSize = mFile.tellg();
	mFile.seekg(0, std::ios::beg);

	if (mFileSize == 0)
	{
		fprintf(stderr, "Error: File \"%s\" appears to be empty\n", aFilePath);
		mFile.close();
		return;
	}

	configure_max_binsize();

	if (mPageSize == 0)
	{
		fprintf(stderr, "Error: Device page size unknown\n");
		mFile.close();
		return;
	}
}

ca_error Flasher::Process()
{
	std::lock_guard<std::mutex> guard(mMutex);

	switch (mState)
	{
	case INIT:
		return init();
	case REBOOT:
		return reboot();
	case ERASE:
		return erase();
	case FLASH:
		return flash();
	case VERIFY:
		return verify();
	case VALIDATE:
		return validate();
	case COMPLETE:
		return CA_ERROR_ALREADY;
	case FAIL:
	case INVALID:
		return CA_ERROR_INVALID_STATE;
		//Purposefully no default state, so compiler checks for exhaustive enum matching
	}

	return CA_ERROR_INVALID_STATE;
}

ca_error Flasher::init()
{
	ca_error status = CA_ERROR_SUCCESS;

	//Verify file size is less than device max size, and greater than zero
	if (mFileSize == 0)
	{
		fprintf(stderr, "Error: No file loaded\n");
		status = CA_ERROR_INVALID_STATE;
		set_state(FAIL);
		goto exit;
	}
	if (mFlashType == APROM && mFileSize > mMaxFileSize)
	{
		fprintf(stderr, "Error: File too large\n");
		status = CA_ERROR_INVALID_ARGS;
		set_state(FAIL);
		goto exit;
	}
	if (mDeviceInfo.GetExchangeType() != ca821x_exchange_usb)
	{
		fprintf(stderr, "Error: Only USB currently supported for chilictl reflash\n");
		status = CA_ERROR_INVALID_ARGS;
		set_state(FAIL);
		goto exit;
	}

	//Open device
	status = ca821x_util_init_path(&mDeviceRef, nullptr, mDeviceInfo.GetExchangeType(), mDeviceInfo.GetPath());
	if (status)
		goto exit;
	mDeviceRef.context = this;

	if (mFlashType == APROM)
	{
		//Reboot device into DFU
		EVBME_DFU_REBOOT_request(EVBME_DFU_REBOOT_DFU, &mDeviceRef);
	}
	else //Flashing DFU
	{
		//Reboot device into APROM
		EVBME_DFU_REBOOT_request(EVBME_DFU_REBOOT_APROM, &mDeviceRef);
	}

	if (exchange_wait_send_complete(kMsgSendTimeout, &mDeviceRef) == CA_ERROR_SUCCESS)
	{
		ca821x_util_deinit(&mDeviceRef);
		memset(&mDeviceRef, 0, sizeof(mDeviceRef));
		set_state(REBOOT);
	}
	else
	{
		set_state(FAIL);
		fprintf(stderr, "Error: Failed to send reboot command\n");
		return CA_ERROR_FAIL;
	}

exit:
	return status;
}

ca_error Flasher::reboot()
{
	//Verify reboot successful & set boot mode to DFU
	DeviceListFilter dlf{};
	DeviceList       dl{};
	size_t           numresults = 0;
	ca_error         status     = CA_ERROR_SUCCESS;

	if (mDeviceRef.context == this)
		return CA_ERROR_SUCCESS;

	dlf.SetAvailable(true);
	dlf.AddSerialNo(mDeviceInfo.GetSerialNo());
	if (mFlashType == APROM)
		dlf.SetAppName("DFU");

	dl.Refresh(dlf);

	numresults = dl.Get().size();
	mCounter++;

	if (numresults == 0)
	{
		if (mCounter >= kMaxRebootDiscoverAttempts)
		{
			set_state(FAIL);
			fprintf(stderr, "Error: Failed to discover device [%s] after reboot to DFU\n", mDeviceInfo.GetSerialNo());
			return CA_ERROR_NOT_FOUND;
		}
		// We return success, but don't modify state, so that this is attempted again
		return CA_ERROR_SUCCESS;
	}
	if (numresults > 1)
	{
		set_state(FAIL);
		fprintf(stderr, "Error: Discovered multiple devices with serial number [%s]\n", mDeviceInfo.GetSerialNo());
		return CA_ERROR_FAIL;
	}
	if (mFlashType == DFU && mDeviceInfo.GetAppName() == std::string("DFU"))
	{
		set_state(FAIL);
		fprintf(stderr, "Error: Failed to reboot [%s] out of DFU mode for updating DFU\n", mDeviceInfo.GetSerialNo());
		return CA_ERROR_FAIL;
	}
	mDeviceInfo = dl.Get()[0];

	//Open device
	status = ca821x_util_init_path(&mDeviceRef, nullptr, mDeviceInfo.GetExchangeType(), mDeviceInfo.GetPath());
	if (status)
	{
		set_state(FAIL);
		fprintf(stderr, "Error: Failed to open DFU device\n");
	}
	else
	{
		mDeviceRef.context                                  = this;
		EVBME_GetCallbackStruct(&mDeviceRef)->EVBME_DFU_cmd = &dfu_callback;
		if (mFlashType == APROM)
			status = EVBME_DFU_BOOTMODE_request(evbme_dfu_rebootmode::EVBME_DFU_REBOOT_DFU, &mDeviceRef);
		else //DFU
			status = EVBME_DFU_BOOTMODE_request(evbme_dfu_rebootmode::EVBME_DFU_REBOOT_APROM, &mDeviceRef);
		if (status)
		{
			fprintf(stderr, "Error: Failed to set DFU boot mode\n");
			set_state(FAIL);
		}
	}

	return status;
}

ca_error Flasher::erase()
{
	ca_error status;

	if (mCounter)
		return CA_ERROR_SUCCESS;

	size_t reqPageErase = get_page_count();

	status = EVBME_DFU_ERASE_request(mStartAddr, mPageSize * reqPageErase, &mDeviceRef);
	mCounter++;

	return status;
}

ca_error Flasher::flash()
{
	if (mCounter)
		return CA_ERROR_SUCCESS;

	return flash_done(CA_ERROR_SUCCESS);
}

ca_error Flasher::verify()
{
	if (mCounter)
		return CA_ERROR_SUCCESS;

	return verify_done(CA_ERROR_SUCCESS);
}

ca_error Flasher::validate()
{
	// Verify reboot successful & set boot mode to Application
	DeviceListFilter dlf{};
	DeviceList       dl{};
	size_t           numresults = 0;
	ca_error         status     = CA_ERROR_SUCCESS;

	if (mCounter)
		return CA_ERROR_SUCCESS;

	dlf.SetAvailable(true);
	dlf.AddSerialNo(mDeviceInfo.GetSerialNo());
	if (mFlashType == DFU)
		dlf.SetAppName("DFU");

	dl.Refresh(dlf);

	numresults = dl.Get().size();

	if (numresults == 0)
	{
		if (mCounter >= kMaxRebootDiscoverAttempts)
		{
			set_state(FAIL);
			fprintf(
			    stderr, "Error: Failed to discover device [%s] after reboot to validate\n", mDeviceInfo.GetSerialNo());
			return CA_ERROR_NOT_FOUND;
		}
		// We return success, but don't modify state, so that this is attempted again
		return CA_ERROR_SUCCESS;
	}
	if (numresults > 1)
	{
		set_state(FAIL);
		fprintf(stderr, "Error: Discovered multiple devices with serial number [%s]\n", mDeviceInfo.GetSerialNo());
		return CA_ERROR_FAIL;
	}
	mDeviceInfo = dl.Get()[0];
	mCounter++;

	//Open device
	status = ca821x_util_init_path(&mDeviceRef, nullptr, mDeviceInfo.GetExchangeType(), mDeviceInfo.GetPath());
	if (status)
	{
		set_state(FAIL);
		fprintf(stderr, "Error: Failed to open flashed device [%s]\n", mDeviceInfo.GetSerialNo());
	}
	else if (mFlashType == APROM)
	{
		// Verify the EVBME
		uint64_t serialno;
		mDeviceRef.context                                  = this;
		EVBME_GetCallbackStruct(&mDeviceRef)->EVBME_DFU_cmd = &dfu_callback;
		status = EVBME_GET_request_sync(EVBME_SERIALNO, sizeof(serialno), (uint8_t *)&serialno, NULL, &mDeviceRef);

		if (status)
		{
			fprintf(stderr, "Error: Failed to verify EVBME of [%s]\n", mDeviceInfo.GetSerialNo());
			set_state(FAIL);
			return CA_ERROR_FAIL;
		}

		// Set boot mode to APROM
		status = EVBME_DFU_BOOTMODE_request(EVBME_DFU_REBOOT_APROM, &mDeviceRef);
		if (status)
		{
			fprintf(stderr, "Error: Failed to set APROM boot mode [%s]\n", mDeviceInfo.GetSerialNo());
			set_state(FAIL);
			return CA_ERROR_FAIL;
		}
	}
	else if (mFlashType == DFU)
	{
		//Found DFU appname, so we just reboot back into application
		EVBME_DFU_REBOOT_request(EVBME_DFU_REBOOT_APROM, &mDeviceRef);

		if (exchange_wait_send_complete(kMsgSendTimeout, &mDeviceRef) == CA_ERROR_SUCCESS)
		{
			ca821x_util_deinit(&mDeviceRef);
			memset(&mDeviceRef, 0, sizeof(mDeviceRef));
			set_state(COMPLETE);
		}
		else
		{
			fprintf(stderr, "Error: Final reboot timed out [%s]\n", mDeviceInfo.GetSerialNo());
			set_state(FAIL);
			status = CA_ERROR_FAIL;
		}
	}

	return status;
}

void Flasher::set_state(State aNextState)
{
	printf("Flasher [%s]: %s -> %s\n", mDeviceInfo.GetSerialNo(), state_string(mState), state_string(aNextState));
	mCounter = 0;
	mState   = aNextState;
}

ca_error Flasher::reboot_done(ca_error status)
{
	if (status == CA_ERROR_SUCCESS)
	{
		set_state(ERASE);
	}
	else
	{
		fprintf(stderr, "Error: Failed to set boot mode.\n");
		set_state(FAIL);
	}
	return status;
}

ca_error Flasher::erase_done(ca_error status)
{
	if (status == CA_ERROR_SUCCESS)
	{
		set_state(FLASH);
	}
	else
	{
		fprintf(stderr, "Error: Erase failed - %s\n", ca_error_str(status));
		set_state(FAIL);
	}
	return status;
}

ca_error Flasher::flash_done(ca_error status)
{
	size_t writeLen;
	char * buffer = nullptr;

	if (status)
		goto exit;

	if (mCounter >= mFileSize)
	{
		mFile.seekg(0, std::ios::beg);
		set_state(VERIFY);
		goto exit;
	}

	writeLen = std::min<size_t>(mFileSize - mCounter, DFU_WRITE_MAX_LEN);
	writeLen = (writeLen + 3) & (~0x3); //Align it (probably not necessary)
	buffer   = new char[writeLen];

	mFile.read(buffer, writeLen);

	status = EVBME_DFU_WRITE_request(mStartAddr + mCounter, writeLen, buffer, &mDeviceRef);
	if (status)
		goto exit;
	mCounter += writeLen;

exit:
	if (status)
	{
		fprintf(stderr, "Error: Flash failed at address 0x%x - %s\n", mStartAddr + mCounter, ca_error_str(status));
		set_state(FAIL);
	}
	delete[] buffer;
	return status;
}

ca_error Flasher::verify_done(ca_error status)
{
	size_t   dataLen;
	uint32_t crc = 0xFFFFFFFF;

	if (status)
		goto exit;

	if (mCounter >= mFileSize)
	{
		mFile.seekg(0, std::ios::beg);

		//Reboot device into newly flashed area
		if (mFlashType == APROM)
			EVBME_DFU_REBOOT_request(evbme_dfu_rebootmode::EVBME_DFU_REBOOT_APROM, &mDeviceRef);
		else //DFU
			EVBME_DFU_REBOOT_request(evbme_dfu_rebootmode::EVBME_DFU_REBOOT_DFU, &mDeviceRef);
		if (exchange_wait_send_complete(kMsgSendTimeout, &mDeviceRef) == CA_ERROR_SUCCESS)
		{
			ca821x_util_deinit(&mDeviceRef);
			memset(&mDeviceRef, 0, sizeof(mDeviceRef));
			set_state(VALIDATE);
		}
		else
		{
			set_state(FAIL);
			fprintf(stderr, "Error: Failed to send reboot to aprom command\n");
		}
		goto exit;
	}

	//Verify one page at a time
	dataLen = std::min<size_t>(mFileSize - mCounter, mPageSize);

	// CRC algorithm adapted from zlib source, which is
	// Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
	for (size_t i = 0; i < mPageSize; ++i)
	{
		uint8_t newByte = (i < dataLen) ? mFile.get() : 0xFF;
		crc ^= newByte;
		crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
		crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
	}
	crc = ~crc;

	status = EVBME_DFU_CHECK_request(mCounter + mStartAddr, mPageSize, crc, &mDeviceRef);
	if (status)
		goto exit;
	mCounter += mPageSize;

exit:
	if (status)
	{
		fprintf(stderr, "Error: verify failed at address 0x%x - %s\n", mCounter + mStartAddr, ca_error_str(status));
		set_state(FAIL);
	}
	return status;
}

ca_error Flasher::validate_done(ca_error status)
{
	if (status)
	{
		fprintf(stderr, "Error: DFU BOOTMODE request failed.\n");
		set_state(FAIL);
		return CA_ERROR_FAIL;
	}
	else
	{
		set_state(COMPLETE);
	}
	return CA_ERROR_SUCCESS;
}

bool Flasher::IsComplete()
{
	return mState == COMPLETE;
}

void Flasher::configure_max_binsize()
{
	mMaxFileSize = std::numeric_limits<size_t>::max();

	if (strcmp(mDeviceInfo.GetDeviceName(), "Chili2") == 0)
	{
		mPageSize = 0x800; //2KiB
		if (mFlashType == APROM)
		{
			mMaxFileSize = 0x80000; //512KiB
			mStartAddr   = 0;
		}
		else if (mFlashType == DFU)
		{
			mMaxFileSize = 0x1000; //4KiB
			mStartAddr   = 0x00100000;
		}
	}
}

ca_error Flasher::dfu_callback(EVBME_Message *params)
{
	ca_error                    status = static_cast<ca_error>(params->EVBME.DFU_cmd.mSubCmd.status_cmd.status);
	std::lock_guard<std::mutex> guard(mMutex);

	if (params->EVBME.DFU_cmd.mDfuSubCmdId != DFU_STATUS)
	{
		status = CA_ERROR_UNKNOWN;
	}

	switch (mState)
	{
	case INIT:
		break;
	case REBOOT:
		reboot_done(status);
		break;
	case ERASE:
		erase_done(status);
		break;
	case FLASH:
		flash_done(status);
		break;
	case VERIFY:
		verify_done(status);
		break;
	case VALIDATE:
		validate_done(status);
		break;
	case COMPLETE:
	case FAIL:
	case INVALID:
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error Flasher::dfu_callback(EVBME_Message *params, ca821x_dev *pDeviceRef)
{
	return static_cast<Flasher *>(pDeviceRef->context)->dfu_callback(params);
}

const char *Flasher::state_string(State aState)
{
	switch (aState)
	{
	case INIT:
		return "INIT";
	case REBOOT:
		return "REBOOT";
	case ERASE:
		return "ERASE";
	case FLASH:
		return "FLASH";
	case VERIFY:
		return "VERIFY";
	case VALIDATE:
		return "VALIDATE";
	case COMPLETE:
		return "COMPLETE";
	case FAIL:
		return "FAIL";
	case INVALID:
		return "INVALID";
	}
	return "???";
}

} /* namespace ca */
