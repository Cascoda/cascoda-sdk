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

#ifndef POSIX_APP_CHILICTL_FLASH_FLASHER_HPP_
#define POSIX_APP_CHILICTL_FLASH_FLASHER_HPP_

#include <fstream>
#include <mutex>

#include "common/DeviceInfo.hpp"

namespace ca {

class Flasher
{
public:
	/**
	 * Enumeration of different areas of flash to rewrite
	 */
	enum FlashType
	{
		APROM = 0, //!< Rewrite the APROM (application)
		DFU   = 1, //!< Rewrite the DFU area (boot loader)
	};

	/**
	 * State enumeration for flasher state machine.
	 *
	 * @startuml
	 * hide empty description
	 *
	 * [*] --> INVALID
	 * [*] -> INIT
     * INIT -[#blue]> OTA_ERASE: If OTA enabled
	 * INIT -> REBOOT
	 * OTA_ERASE -[#blue]> REBOOT: If OTA enabled
	 * OTA_ERASE -[#blue]> FAIL: If OTA enabled
	 * REBOOT -> ERASE
	 * REBOOT --> FAIL
	 * ERASE -> FLASH
	 * ERASE --> FAIL
	 * FLASH -> VERIFY
	 * FLASH --> FAIL
	 * VERIFY -> VALIDATE
	 * VERIFY --> FAIL
	 * VALIDATE -> COMPLETE
	 * VALIDATE --> FAIL
	 * COMPLETE -> [*]
	 * @enduml
	 */
	enum State
	{
		INIT,      /**< Initial state */
		OTA_ERASE, /**< Erase the metadata region of the external flash (only happens if OTA upgrade is enabled) */
		REBOOT,    /**< Rebooted into DFU mode */
		ERASE,     /**< Erasing flash */
		FLASH,     /**< Flashing program */
		VERIFY,    /**< Verifying correct flashing */
		VALIDATE,  /**< Validating new program functionality */
		COMPLETE,  /**< Flashing completed successfully */
		FAIL,      /**< Flashing failed */
		INVALID,   /**< Class not correctly instantiated */
	};

	/**
	 * Construct a flasher instance
	 * @param aAppFilePath     Path to the binary file of the application to be flashed
	 * @param aOtaBootFilePath Path to the binary file of the ota bootloader to be flashed
	 * @param aDeviceInfo      Reference to a deviceInfo struct of the device to be reflashed
	 * @param aFlashType       Type of reflashing to execute (Update application or bootloader?)
	 */
	Flasher(const char *      aAppFilePath,
	        const char *      aOtaBootFilePath,
	        const DeviceInfo &aDeviceInfo,
	        FlashType         aFlashType);

	~Flasher();

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

	/**
	 * Enable/Disable the version check for the connected device.
	 *
	 * This is useful if the connected device's version number is broken for some reason, but the actual firmware
	 * is up to date. Defaults to false (Version check is enabled)
	 *
	 * @param aIgnoreVersion Set to true to ignore the version check, Set to false to enable the version check
	 */
	void SetIgnoreVersion(bool aIgnoreVersion) { mIgnoreVersion = aIgnoreVersion; }

private:
	enum
	{
		kMaxRebootDiscoverAttempts = 10,
		kMsgSendTimeout            = 5,
		kWriteLen                  = 244,
	};

	std::mutex    mMutex;
	std::ifstream mAppFile;
	std::ifstream mOtaBootFile;
	size_t        mAppFileSize;
	size_t        mOtaBootFileSize;
	size_t        mAppMaxFileSize;
	size_t        mCombinedFileSize;
	size_t        mOtaBootMaxFileSize;
	size_t        mPageSize;
	uint32_t      mAppStartAddr;
	uint32_t      mOtaBootStartAddr;
	ca821x_dev    mDeviceRef;
	DeviceInfo    mDeviceInfo;
	uint32_t      mCounter;
	State         mState;
	FlashType     mFlashType;
	bool          mIgnoreVersion;
	bool          mOtaBootFilePresent;

	void     set_state(State aNextState);
	ca_error init();
	ca_error ota_erase();
	ca_error reboot();
	ca_error erase();
	ca_error flash();
	ca_error verify();
	ca_error validate();

	ca_error ota_erase_done(ca_error status);
	ca_error reboot_done(ca_error status);
	ca_error erase_done(ca_error status);
	ca_error flash_done(ca_error status);
	ca_error verify_done(ca_error status);
	ca_error validate_done(ca_error status);

	ca_error        dfu_callback(EVBME_Message *params);
	static ca_error dfu_callback(EVBME_Message *params, ca821x_dev *pDeviceRef);

	ca_error        handle_evbme_message(EVBME_Message *params);
	static ca_error handle_evbme_message(EVBME_Message *params, ca821x_dev *pDeviceRef);

	ca_error send_reboot_request();
	void     configure_max_binsize();
	size_t   get_page_count() { return (mCombinedFileSize + (mPageSize - 1)) / mPageSize; }
	uint32_t get_start_address() { return mOtaBootFilePresent ? mOtaBootStartAddr : mAppStartAddr; }

	static const char *state_string(State aState);
};

} /* namespace ca */

#endif /* POSIX_APP_CHILICTL_FLASH_FLASHER_HPP_ */
