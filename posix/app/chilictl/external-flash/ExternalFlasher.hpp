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

#ifndef POSIX_APP_CHILICTL_EXTERNAL_FLASH_EXTERNALFLASHER_HPP_
#define POSIX_APP_CHILICTL_EXTERNAL_FLASH_EXTERNALFLASHER_HPP_

#include <fstream>
#include <mutex>

#include "common/DeviceInfo.hpp"

namespace ca {

class ExternalFlasher
{
public:
	enum State
	{
		INIT,  /**< Initial state */
		ERASE, /**< Erasing flash */
		PROGRAM,
		VERIFY,
		COMPLETE, /**< Flashing completed successfully */
		FAIL,     /**< Flashing failed */
		INVALID,  /**< Class not correctly instantiated */
	};

	/**
	 * Construct an ExternalFlasher instance
	 * @param aFilePath   Path to the binary file to be written to the external flash
	 * @param aDeviceInfo Reference to a deviceInfo struct of the device whose external flash will be erased/programmed/read.
	 */
	ExternalFlasher(const char *aFilePath, const DeviceInfo &aDeviceInfo);

	~ExternalFlasher();

	/**
	 * Process the internal state, returning an error if no more processing can be done
	 * @retval CA_ERROR_SUCCESS       Processing successful, not yet complete
	 * @retval CA_ERROR_ALREADY       Processing complete and successful
	 * @retval CA_ERROR_INVALID_STATE Processing complete but failed
	 */
	ca_error Process();

	/**
	 * Is the instance complete and successful?
	 * @return True if complete and successful
	 */
	bool IsComplete();

	/**
	 * Get the current state of the flasher
	 * @return The current state of the flasher instance
	 */
	State GetState() { return mState; }

private:
	enum
	{
		kMsgSendTimeout = 5,
		kWriteLen       = 244
	};

	std::mutex    mMutex;
	std::ifstream mFile;
	size_t        mFileSize;
	size_t        mMaxFileSize;
	size_t        mPageSize;
	uint32_t      mStartAddr;
	ca821x_dev    mDeviceRef;
	DeviceInfo    mDeviceInfo;
	uint32_t      mCounter;
	State         mState;

	void     set_state(State aNextState);
	ca_error init();
	ca_error erase();
	ca_error program();
	ca_error verify();

	ca_error erase_done(ca_error status);
	ca_error program_done(ca_error status);
	ca_error verify_done(ca_error status);

	ca_error        dfu_callback(EVBME_Message *params);
	static ca_error dfu_callback(EVBME_Message *params, ca821x_dev *pDeviceRef);

	void   configure_max_binsize();
	size_t get_page_count() { return (mFileSize + (mPageSize - 1)) / mPageSize; }

	static const char *state_string(State aState);
};

} /* namespace ca */

#endif /* POSIX_APP_CHILICTL_EXTERNAL_FLASH_EXTERNALFLASHER_HPP_ */
