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
#include <iostream>
#include <limits>

#if defined(_WIN32)
#include <synchapi.h> // for Windows Sleep() function
#else
#include <unistd.h> // for posix usleep() function
#endif

#include "ca821x-posix/ca821x-posix.h"
#include "cascoda-util/cascoda_hash.h"

#include "common/DeviceList.hpp"
#include "flash/Flasher.hpp"

namespace ca {

static constexpr size_t apromSize = 0x80000;

Flasher::Flasher(const char       *aAppFilePath,
                 const char       *aOtaBootFilePath,
                 const char       *aManufacturerDataFilePath,
                 const DeviceInfo &aDeviceInfo,
                 FlashType         aFlashType)
    : mAppFile(aAppFilePath, std::ios::in | std::ios::binary | std::ios::ate)
    , mOtaBootFile(aOtaBootFilePath, std::ios::in | std::ios::binary | std::ios::ate)
    , mManuDataFile(aManufacturerDataFilePath, std::ios::in | std::ios::binary | std::ios::ate)
    , mOtaBootFileSize(0)
    , mManuDataFileSize(0)
    , mPageSize()
    , mDeviceInfo(aDeviceInfo)
    , mCounter(0)
    , mState(INIT)
    , mFlashType(aFlashType)
    , mIgnoreVersion(false)
    , mOtaBootFilePresent(strcmp(aOtaBootFilePath, "") != 0)
{
	if ((mFlashType != APROM_CLEAR) && (mFlashType != MANUFACTURER))
	{
		if (!mAppFile.is_open())
		{
			fprintf(stderr, "Error: File \"%s\" could not be opened\n", aAppFilePath);
			return;
		}

		if (mOtaBootFilePresent && !mOtaBootFile.is_open())
		{
			fprintf(stderr, "Error: File \"%s\" could not be opened\n", aOtaBootFilePath);
			return;
		}

		mAppFileSize = mAppFile.tellg();
		mAppFile.seekg(0, std::ios::beg);

		if (mOtaBootFilePresent)
		{
			mOtaBootFileSize = mOtaBootFile.tellg();
			mOtaBootFile.seekg(0, std::ios::beg);
		}

		if (mAppFileSize == 0)
		{
			fprintf(stderr, "Error: File \"%s\" appears to be empty\n", aAppFilePath);
			mAppFile.close();
			return;
		}

		if (mOtaBootFilePresent && mOtaBootFileSize == 0)
		{
			fprintf(stderr, "Error: File \"%s\" appears to be empty\n", aOtaBootFilePath);
			mOtaBootFile.close();
			return;
		}
	}
	else if (mFlashType == MANUFACTURER)
	{
		if (!mManuDataFile.is_open())
		{
			fprintf(stderr, "Error: File \"%s\" could not be opened\n", aManufacturerDataFilePath);
			return;
		}

		mManuDataFileSize = mManuDataFile.tellg();
		mManuDataFile.seekg(0, std::ios::beg);
	}

	configure_size_and_addresses();

	if (mPageSize == 0)
	{
		fprintf(stderr, "Error: Device page size unknown\n");
		mAppFile.close();
		return;
	}
}

Flasher::~Flasher()
{
	ca821x_util_deinit(&mDeviceRef);
}

ca_error Flasher::Process()
{
	std::lock_guard<std::mutex> guard(mMutex);

	switch (mState)
	{
	case INIT:
		return init();
	case OTA_ERASE:
		return ota_erase();
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

	if (mDeviceInfo.GetExchangeType() == ca821x_exchange_kernel)
	{
		fprintf(stderr, "Error: Only USB and UART are currently supported for chilictl reflash\n");
		status = CA_ERROR_INVALID_ARGS;
		set_state(FAIL);
		goto exit;
	}

	if (mFlashType != APROM_CLEAR)
	{
		//Verify file size is less than device max size, and greater than zero
		if (mAppFileSize == 0 || (mOtaBootFilePresent && (mOtaBootFileSize == 0)) ||
		    (mFlashType == MANUFACTURER && mManuDataFileSize == 0))
		{
			fprintf(stderr, "Error: No file loaded\n");
			status = CA_ERROR_INVALID_STATE;
			set_state(FAIL);
			goto exit;
		}

		if ((mFlashType != DFU && mAppFileSize > mAppMaxFileSize) ||
		    (mOtaBootFilePresent && mOtaBootFileSize > mOtaBootMaxFileSize))
		{
			fprintf(stderr, "Error: File too large\n");
			status = CA_ERROR_INVALID_ARGS;
			set_state(FAIL);
			goto exit;
		}
	}

	//Open device
	status = ca821x_util_init_path(&mDeviceRef, nullptr, mDeviceInfo.GetExchangeType(), mDeviceInfo.GetPath());
	if (status)
		goto exit;

	mDeviceRef.context                                             = this;
	EVBME_GetCallbackStruct(&mDeviceRef)->EVBME_DFU_cmd            = &dfu_callback;
	EVBME_GetCallbackStruct(&mDeviceRef)->EVBME_MESSAGE_indication = &handle_evbme_message;

	//Can't flash Chilis with a firmware version of less than 0.14 (except if in DFU mode)
	if (!mIgnoreVersion && strcmp(mDeviceInfo.GetAppName(), "DFU") != 0)
	{
		if (EVBME_CheckVersion("0.14", &mDeviceRef) != CA_ERROR_SUCCESS)
		{
			fprintf(stderr,
			        "Error: Chili version too old - must be v0.14 or above for USB reflash. Please use an external "
			        "programmer. See --ignore-version flag if you want to attempt to override this check.\n");
			status = CA_ERROR_INVALID;
			set_state(FAIL);
			goto exit;
		}
	}

	if (mOtaBootFilePresent)
		set_state(OTA_ERASE);
	else
		status = send_reboot_request();

exit:
	return status;
}

ca_error Flasher::ota_erase()
{
	ca_error status;

	if (mCounter)
		return CA_ERROR_SUCCESS;

	//Erase metadata region of external flash
	mDeviceRef.context                                  = this;
	EVBME_GetCallbackStruct(&mDeviceRef)->EVBME_DFU_cmd = &dfu_callback;

	status = EVBME_DFU_ERASE_request(0xB007f000, mPageSize * 2, &mDeviceRef);

	if (status)
		set_state(FAIL);

	mCounter++;

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
	if (mFlashType <= APROM_CLEAR_AND_PROGRAM)
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
		if (mFlashType <= APROM_CLEAR_AND_PROGRAM)
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

	//TODO: No need to erase unused memory between ota bootloader and app, only erase what's necessary
	size_t   reqPageErase = get_page_count_for_erase();
	uint32_t startAddr    = get_start_address();

	status = EVBME_DFU_ERASE_request(startAddr, mPageSize * reqPageErase, &mDeviceRef);
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
	static int       retries    = 0;

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
		if (retries++ >= kMaxRebootDiscoverAttempts)
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
	else if (mFlashType != DFU)
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

// Give a chance for the device to wake-up post reboot, before proceeding
// NOTE: This was observed to be necessary for UART, but isn't needed for USB.
#if defined(_WIN32)
		Sleep(1); //ms
#else
		usleep(1000);     //us
#endif

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
	printf("Flasher [%s]: %s -> %s", mDeviceInfo.GetSerialNo(), state_string(mState), state_string(aNextState));
	std::cout << std::endl; // Added to flush output on git bash
	mCounter = 0;
	mState   = aNextState;
}

ca_error Flasher::ota_erase_done(ca_error status)
{
	if (status == CA_ERROR_SUCCESS)
	{
		status = send_reboot_request();
		if (status)
			goto exit;
	}
	else
	{
		fprintf(stderr, "Error: External flash metadata region erase failed - %s\n", ca_error_str(status));
		set_state(FAIL);
		status = CA_ERROR_FAIL;
	}

exit:
	return status;
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
		if (mFlashType == APROM_CLEAR)
			set_state(VERIFY);
		else
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
	char  *buffer = nullptr;

	if (status)
		goto exit;

	if (mCounter >= mCombinedFileSize)
	{
		set_state(VERIFY);
		goto exit;
	}

	if (mFlashType == MANUFACTURER)
	{
		writeLen = std::min<size_t>(mManuDataFileSize - mCounter, DFU_WRITE_MAX_LEN);
		writeLen = (writeLen + 3) & (~0x3); //Align it
		buffer   = new char[writeLen];
		memset(buffer, 0xff, writeLen);
		mManuDataFile.read(buffer, writeLen);
	}
	else if (mCounter >= mOtaBootFileSize)
	{
		//Advance mCounter, this is to account for the gap between ota bootloader region and app region.
		if (mOtaBootFilePresent && mCounter < mAppStartAddr)
			mCounter = mAppStartAddr;

		writeLen = std::min<size_t>(mCombinedFileSize - mCounter, DFU_WRITE_MAX_LEN);
		writeLen = (writeLen + 3) & (~0x3);
		buffer   = new char[writeLen];
		memset(buffer, 0xff, writeLen);
		mAppFile.read(buffer, writeLen);
	}
	else
	{
		writeLen = std::min<size_t>(mOtaBootFileSize - mCounter, DFU_WRITE_MAX_LEN);
		writeLen = (writeLen + 3) & (~0x3);
		buffer   = new char[writeLen];
		memset(buffer, 0xff, writeLen);
		mOtaBootFile.read(buffer, writeLen);
	}

	status = EVBME_DFU_WRITE_request(get_start_address() + mCounter, writeLen, buffer, &mDeviceRef);
	if (status)
		goto exit;
	mCounter += writeLen;

exit:
	if (status)
	{
		fprintf(
		    stderr, "Error: Flash failed at address 0x%x - %s\n", get_start_address() + mCounter, ca_error_str(status));
		set_state(FAIL);
	}
	delete[] buffer;
	return status;
}

ca_error Flasher::verify_done(ca_error status)
{
	uint32_t crc = 0xFFFFFFFF;

	if (status)
		goto exit;

	if (mFlashType == APROM_CLEAR)
	{
		if (mCounter >= apromSize)
		{
			set_state(COMPLETE);
			goto exit;
		}

		for (size_t i = 0; i < mPageSize; ++i)
		{
			uint8_t data[1];
			// 0xFF is the value of flash when it is erased
			data[0] = 0xFF;
			HASH_CRC32_stream(data, 1, &crc);
		}
		crc = ~crc;

		status = EVBME_DFU_CHECK_request(mCounter + get_start_address(), mPageSize, crc, &mDeviceRef);
		if (status)
			goto exit;
		mCounter += mPageSize;
	}
	else
	{
		size_t dataLen;
		if (mCounter == 0)
		{
			if (mFlashType == MANUFACTURER)
			{
				mManuDataFile.clear();
				mManuDataFile.seekg(0, std::ios::beg);
			}
			else
			{
				mAppFile.clear();
				mAppFile.seekg(0, std::ios::beg);
			}

			if (mOtaBootFilePresent)
			{
				mOtaBootFile.clear();
				mOtaBootFile.seekg(0, std::ios::beg);
			}
		}

		if (mCounter >= mCombinedFileSize)
		{
			//Only the App file will be validated, even if the ota bootloader was flashed as well.
			mAppFile.clear();
			mAppFile.seekg(0, std::ios::beg);

			//Reboot device into newly flashed area
			if (mFlashType != DFU)
				EVBME_DFU_REBOOT_request(evbme_dfu_rebootmode::EVBME_DFU_REBOOT_APROM, &mDeviceRef);
			else //DFU
				EVBME_DFU_REBOOT_request(evbme_dfu_rebootmode::EVBME_DFU_REBOOT_DFU, &mDeviceRef);

// Give a chance for the device to wake-up post reboot, before proceeding
// NOTE: This was observed to be necessary for UART, but isn't needed for USB.
#if defined(_WIN32)
			Sleep(1); //ms
#else
			usleep(1000); //us
#endif

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

		if (mFlashType == MANUFACTURER)
		{
			dataLen = std::min<size_t>(mManuDataFileSize - mCounter, mPageSize);
		}
		else if (mCounter >= mOtaBootFileSize)
		{
			//Advance mCounter, this is to account for the gap between ota bootloader and app regions.
			if (mOtaBootFilePresent && mCounter < mAppStartAddr)
			{
				mCounter = mAppStartAddr;
			}

			dataLen = std::min<size_t>(mCombinedFileSize - mCounter, mPageSize);
		}
		else
		{
			dataLen = std::min<size_t>(mOtaBootFileSize - mCounter, mPageSize);
		}
		for (size_t i = 0; i < mPageSize; ++i)
		{
			uint8_t data[1];

			if (mFlashType == MANUFACTURER)
			{
				//data[0] = (i < dataLen) ? mManuDataFile.get() : 0xFF;
				if (i < dataLen)
					data[0] = mManuDataFile.get();
				else
					data[0] = 0xff;
			}
			else if (mCounter >= mOtaBootFileSize)
				data[0] = (i < dataLen) ? mAppFile.get() : 0xFF;
			else
				data[0] = (i < dataLen) ? mOtaBootFile.get() : 0xFF;

			HASH_CRC32_stream(data, 1, &crc);
		}
		crc = ~crc;

		status = EVBME_DFU_CHECK_request(mCounter + get_start_address(), mPageSize, crc, &mDeviceRef);
		if (status)
			goto exit;
		mCounter += mPageSize;
	}

exit:
	if (status)
	{
		fprintf(stderr,
		        "Error: verify failed at address 0x%x - %s\n",
		        mCounter + get_start_address(),
		        ca_error_str(status));
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

ca_error Flasher::send_reboot_request()
{
	ca_error status = CA_ERROR_SUCCESS;

	if (mFlashType <= APROM_CLEAR_AND_PROGRAM)
	{
		// Reboot device into DFU
		EVBME_DFU_REBOOT_request(EVBME_DFU_REBOOT_DFU, &mDeviceRef);
	}
	else //Flashing DFU
	{
		//Reboot device into APROM
		EVBME_DFU_REBOOT_request(EVBME_DFU_REBOOT_APROM, &mDeviceRef);
	}

// Give a chance for the device to wake-up post reboot, before proceeding
// NOTE: This was observed to be necessary for UART, but isn't needed for USB.
#if defined(_WIN32)
	Sleep(1); //ms
#else
	usleep(1000);         //us
#endif

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

	return status;
}

void Flasher::configure_size_and_addresses()
{
	mAppMaxFileSize     = std::numeric_limits<size_t>::max();
	mOtaBootMaxFileSize = std::numeric_limits<size_t>::max();

	if (strcmp(mDeviceInfo.GetDeviceName(), "Chili2") == 0)
	{
		mPageSize           = 0x800; //2KiB
		mOtaBootMaxFileSize = 0;
		mOtaBootStartAddr   = 0;

		if (mFlashType <= APROM_CLEAR_AND_PROGRAM)
		{
			if (mOtaBootFilePresent)
			{
				mAppMaxFileSize = 0x70000; //448KiB
				mAppStartAddr   = 0x10000;

				mOtaBootMaxFileSize = 0x10000; //64kiB
				mOtaBootStartAddr   = 0;
			}
			else
			{
				mAppMaxFileSize = 0x80000; //512KiB
				mAppStartAddr   = 0;
			}
		}
		else if (mFlashType == DFU)
		{
			mAppMaxFileSize = 0x1000; //4KiB
			mAppStartAddr   = 0x100000;
		}
		else if (mFlashType == MANUFACTURER)
		{
			mManuDataMaxFileSize = mPageSize;
			mManuDataStartAddr   = 0x80000 - mPageSize;
		}
	}

	if (mOtaBootFilePresent)
		mCombinedFileSize = mAppStartAddr + mAppFileSize;
	else if (mFlashType == MANUFACTURER)
		mCombinedFileSize = mManuDataFileSize;
	else
		mCombinedFileSize = mAppFileSize;
}

size_t Flasher::get_page_count_for_erase()
{
	if (mFlashType == APROM_CLEAR || mFlashType == APROM_CLEAR_AND_PROGRAM)
		return (apromSize + (mPageSize - 1)) / mPageSize;
	else
		return (mCombinedFileSize + (mPageSize - 1)) / mPageSize;
}

uint32_t Flasher::get_start_address()
{
	if (mFlashType == MANUFACTURER)
		return mManuDataStartAddr;
	else
		return mOtaBootFilePresent ? mOtaBootStartAddr : mAppStartAddr;
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
	case OTA_ERASE:
		ota_erase_done(status);
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

ca_error Flasher::handle_evbme_message(EVBME_Message *params)
{
	fprintf(stderr, "Rx: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

ca_error Flasher::dfu_callback(EVBME_Message *params, ca821x_dev *pDeviceRef)
{
	return static_cast<Flasher *>(pDeviceRef->context)->dfu_callback(params);
}

ca_error Flasher::handle_evbme_message(EVBME_Message *params, ca821x_dev *pDeviceRef)
{
	return static_cast<Flasher *>(pDeviceRef->context)->handle_evbme_message(params);
}

const char *Flasher::state_string(State aState)
{
	switch (aState)
	{
	case INIT:
		return "INIT";
	case OTA_ERASE:
		return "OTA_ERASE";
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
