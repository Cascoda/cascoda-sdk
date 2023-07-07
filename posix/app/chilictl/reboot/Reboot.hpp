/*
 *  Copyright (c) 2023, Cascoda Ltd.
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

#ifndef POSIX_APP_CHILICTL_REBOOT_REBOOT_HPP_
#define POSIX_APP_CHILICTL_REBOOT_REBOOT_HPP_

#include <mutex>

#include "common/Args.hpp"
#include "common/Command.hpp"
#include "common/DeviceList.hpp"

#include "ca821x_error.h"

namespace ca {

class Reboot : public Command
{
public:
	enum State
	{
		INIT,          /**< Initial state */
		FACTORY_RESET, /**< Erasing Data Flash region */
		REBOOT,        /**< Rebooting the device */
		COMPLETE,      /**< Flashing completed successfully */
		FAIL,          /**< Flashing failed */
	};
	Reboot();
	~Reboot();

	/**
		 * @copydoc Command::Process
		 */
	ca_error Process(int argc, const char *argv[]);

private:
	enum
	{
		kMsgSendTimeout = 5,
	};

	std::mutex       mMutex;
	State            mState;
	Args             mArgParser;
	ArgOpt           mHelpArg;
	ArgOpt           mSerialArg;
	ArgOpt           mBatchArg;
	ArgOpt           mFactoryReset;
	ArgOpt           mEnumerateUartDevicesArg;
	DeviceList       mDeviceList;
	DeviceListFilter mDeviceListFilter;
	ca821x_dev       mDeviceRef;
	DeviceInfo       mDeviceInfo;

	// State machine
	ca_error reboot_process();

	// init state
	ca_error init();

	// Requests factory reset from target device
	ca_error factory_reset();

	// Finalizes factory reset, this function is called from callback
	// which is triggered when host receives EVBME DFU_STATUS from target device
	ca_error factory_reset_done(ca_error status);

	// Makes the target device reboot (normal reboot, APROM to APROM)
	ca_error reboot();

	// Callbacks for handling received EVBME_DFU from target device
	ca_error        dfu_callback(EVBME_Message *params);
	static ca_error dfu_callback(EVBME_Message *params, ca821x_dev *pDeviceRef);

	// Callbacks for handling received EVBME_MESSAGES from target device
	ca_error        handle_evbme_message(EVBME_Message *params);
	static ca_error handle_evbme_message(EVBME_Message *params, ca821x_dev *pDeviceRef);

	ca_error print_help_string(const char *aArg);
	ca_error set_serialno_filter(const char *aArg);

	void               set_state(State aNextState);
	static const char *state_string(State aState);
};

} /* namespace ca */

#endif /* POSIX_APP_CHILICTL_REBOOT_REBOOT_HPP_ */
