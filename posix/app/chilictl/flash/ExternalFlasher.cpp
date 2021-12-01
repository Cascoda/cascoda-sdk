/*
 *  Copyright (c) 2021, Cascoda Ltd.
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

#include <cstring>

#include "ca821x-posix/ca821x-posix.h"
#include "cascoda-util/cascoda_hash.h"
#include "ca821x_endian.h"

#include "common/DeviceList.hpp"
#include "ExternalFlasher.hpp"

namespace ca {

ExternalFlasher::ExternalFlasher(const char *aAppFilePath, const DeviceInfo &aDeviceInfo)
    : mFile(aAppFilePath, std::ios::in | std::ios::binary | std::ios::ate)
    , mPageSize()
    , mBinarySizeSent(false)
    , mBinaryHashSent(false)
    , mDeviceInfo(aDeviceInfo)
    , mState(INIT)
{
	if (!mFile.is_open())
	{
		fprintf(stderr, "Error: File \"%s\" could not be opened\n", aAppFilePath);
		return;
	}

	mFileSize = mFile.tellg();
	mFile.seekg(0, std::ios::beg);

	if (mFileSize == 0)
	{
		fprintf(stderr, "Error: File \"%s\" appears to be empty\n", aAppFilePath);
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

ExternalFlasher::~ExternalFlasher()
{
	ca821x_util_deinit(&mDeviceRef);
}

ca_error ExternalFlasher::Process()
{
	std::lock_guard<std::mutex> guard(mMutex);

	switch (mState)
	{
	case INIT:
		return init();
	case ERASE:
		return erase();
	case PROGRAM:
		return program();
	case VERIFY:
		return verify();
	case COMPLETE:
		return CA_ERROR_ALREADY;
	case FAIL:
	case INVALID:
		return CA_ERROR_INVALID_STATE;
		//Purposefully no default state, so compiler checks for exhaustive enum matching
	}

	return CA_ERROR_INVALID_STATE;
}

ca_error ExternalFlasher::init()
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
	if (mFileSize > mMaxFileSize)
	{
		fprintf(stderr, "Error: File too large\n");
		status = CA_ERROR_INVALID_ARGS;
		set_state(FAIL);
		goto exit;
	}
	if (mDeviceInfo.GetExchangeType() != ca821x_exchange_usb)
	{
		fprintf(stderr, "Error: Only USB currently supported for chilictl external flashing\n");
		status = CA_ERROR_INVALID_ARGS;
		set_state(FAIL);
		goto exit;
	}

	//Open device
	status = ca821x_util_init_path(&mDeviceRef, nullptr, mDeviceInfo.GetExchangeType(), mDeviceInfo.GetPath());
	if (status)
		goto exit;

	mDeviceRef.context                                             = this;
	EVBME_GetCallbackStruct(&mDeviceRef)->EVBME_DFU_cmd            = &dfu_callback;
	EVBME_GetCallbackStruct(&mDeviceRef)->EVBME_MESSAGE_indication = &handle_evbme_message;

	set_state(ERASE);

exit:
	return status;
}

ca_error ExternalFlasher::erase()
{
	if (mCounter)
		return CA_ERROR_SUCCESS;

	return erase_done(CA_ERROR_SUCCESS);
}

ca_error ExternalFlasher::program()
{
	if (mBinarySizeSent)
		return CA_ERROR_SUCCESS;

	return program_done(CA_ERROR_SUCCESS);
}

ca_error ExternalFlasher::verify()
{
	if (mCounter)
		return CA_ERROR_SUCCESS;

	return verify_done(CA_ERROR_SUCCESS);
}

void ExternalFlasher::set_state(State aNextState)
{
	printf(
	    "ExternalFlasher [%s]: %s -> %s\n", mDeviceInfo.GetSerialNo(), state_string(mState), state_string(aNextState));
	mCounter = 0;
	mState   = aNextState;
}

ca_error ExternalFlasher::erase_done(ca_error status)
{
	if (status)
		goto exit;

	if (mCounter == 0)
	{
		//Erase External flash where binary will be programmed
		size_t reqPageErase = get_page_count(mFileSize);
		status              = EVBME_DFU_ERASE_request(mStartAddr, mPageSize * reqPageErase, &mDeviceRef);
		if (status)
			goto exit;
	}
	else if (mCounter == 1)
	{
		//Erase metadata region of external flash
		status = EVBME_DFU_ERASE_request(mMetadataStartAddr, mPageSize * 16, &mDeviceRef);
		if (status)
			goto exit;
	}
	else if (mCounter == 2)
	{
		size_t reqPageErase = get_page_count(mMaxAddress - mCurrentAppStartAddr);

		//Erase External flash region dedicated for current binary
		status = EVBME_DFU_ERASE_request(mCurrentAppStartAddr, mPageSize * reqPageErase, &mDeviceRef);
	}
	else
	{
		set_state(PROGRAM);
		goto exit;
	}
	mCounter++;

exit:
	if (status)
	{
		fprintf(stderr, "Error: External flash erase failed - %s\n", ca_error_str(status));
		set_state(FAIL);
	}
	return status;
}

ca_error ExternalFlasher::program_done(ca_error status)
{
	size_t          writeLen;
	char *          buffer = nullptr;
	static uint64_t hash   = basis64;

	if (status)
		goto exit;

	// Send the size of the binary before sending the binary
	if (!mBinarySizeSent)
	{
		mBinarySizeSent = true;
		uint8_t binSizeBuf[4];
		PUTLE32((uint32_t)mFileSize, binSizeBuf);

		status = EVBME_DFU_WRITE_request(mBinarySizeMetadataPartitionAddr, sizeof(binSizeBuf), binSizeBuf, &mDeviceRef);
		goto exit;
	}

	if (mCounter >= mFileSize)
	{
		if (!mBinaryHashSent)
		{
			mBinaryHashSent = true;
			uint8_t hashBuf[8];
			PUTLE64(hash, hashBuf);

			//Send the hash of the binary after sending the binary
			status = EVBME_DFU_WRITE_request(mBinaryHashMetadataPartitionAddr, sizeof(hashBuf), hashBuf, &mDeviceRef);
			goto exit;
		}

		mFile.seekg(0, std::ios::beg);
		set_state(VERIFY);
		goto exit;
	}

	writeLen = std::min<size_t>(mFileSize - mCounter, DFU_WRITE_MAX_LEN);
	writeLen = (writeLen + 3) & (~0x3); //Align it (probably not necessary)
	buffer   = new char[writeLen];

	mFile.read(buffer, writeLen);

	HASH_fnv1a_64_stream(buffer, writeLen, &hash);

	status = EVBME_DFU_WRITE_request(mStartAddr + mCounter, writeLen, buffer, &mDeviceRef);
	if (status)
		goto exit;
	mCounter += writeLen;

exit:
	if (status)
	{
		fprintf(stderr,
		        "Error: External flash program failed at address 0x%x - %s\n",
		        mStartAddr + mCounter,
		        ca_error_str(status));
		set_state(FAIL);
	}
	delete[] buffer;
	return status;
}

ca_error ExternalFlasher::verify_done(ca_error status)
{
	size_t   dataLen;
	uint32_t crc = 0xFFFFFFFF;

	if (status)
		goto exit;

	if (mCounter >= mFileSize)
	{
		mFile.seekg(0, std::ios::beg);
		set_state(COMPLETE);
		goto exit;
	}

	//Verify one page at a time
	dataLen = std::min<size_t>(mFileSize - mCounter, mPageSize);

	for (size_t i = 0; i < mPageSize; ++i)
	{
		uint8_t data[1];
		data[0] = (i < dataLen) ? mFile.get() : 0xFF;
		HASH_CRC32_stream(data, 1, &crc);
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

bool ExternalFlasher::IsComplete()
{
	return mState == COMPLETE;
}

void ExternalFlasher::configure_max_binsize()
{
	mMaxFileSize = std::numeric_limits<size_t>::max();

	if (strcmp(mDeviceInfo.GetDeviceName(), "Chili2") == 0)
	{
		mPageSize            = 0x100;      //256 bytes
		mMaxFileSize         = 0x80000;    //512KiB
		mStartAddr           = 0xB0000000; //Address used to represent beginning of external flash address space
		mMaxAddress          = mStartAddr + 0x100000;
		mMetadataStartAddr   = mStartAddr + 0x07f000; //Address of the metadata region of the external flash
		mCurrentAppStartAddr = mStartAddr + 0x081000; //Address of the current application region of the external flash
		mBinarySizeMetadataPartitionAddr =
		    mStartAddr +
		    0x07f200; //Address of the sub-region in the metadata region where the size of the binary is stored
		mBinaryHashMetadataPartitionAddr =
		    mStartAddr +
		    0x07f100; //Address of the sub-region in the metadata region where the hash of the binary is stored
	}
}

ca_error ExternalFlasher::dfu_callback(EVBME_Message *params)
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
	case ERASE:
		erase_done(status);
		break;
	case PROGRAM:
		program_done(status);
		break;
	case VERIFY:
		verify_done(status);
		break;
	case COMPLETE:
		break;
	case FAIL:
	case INVALID:
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error ExternalFlasher::handle_evbme_message(EVBME_Message *params)
{
	fprintf(stderr, "Rx: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

ca_error ExternalFlasher::dfu_callback(EVBME_Message *params, ca821x_dev *pDeviceRef)
{
	return static_cast<ExternalFlasher *>(pDeviceRef->context)->dfu_callback(params);
}

ca_error ExternalFlasher::handle_evbme_message(EVBME_Message *params, ca821x_dev *pDeviceRef)
{
	return static_cast<ExternalFlasher *>(pDeviceRef->context)->handle_evbme_message(params);
}

const char *ExternalFlasher::state_string(State aState)
{
	switch (aState)
	{
	case INIT:
		return "INIT";
	case ERASE:
		return "ERASE";
	case PROGRAM:
		return "PROGRAM";
	case VERIFY:
		return "VERIFY";
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
