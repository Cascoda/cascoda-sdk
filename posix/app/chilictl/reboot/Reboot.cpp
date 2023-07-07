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

#include "Reboot.hpp"

#include "ca821x-posix/ca821x-posix.h"
#include "cascoda-util/cascoda_flash.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

#if defined(_WIN32)
#include <synchapi.h> // for Windows Sleep() function
#else
#include <unistd.h> // for posix usleep() function
#endif

namespace ca {

Reboot::Reboot()
    : Command("reboot", "Utility for rebooting connected chili devices\n  type 'reboot -h' for more info\n")
    , mState(INIT)
    , mArgParser()
    , mHelpArg('h', "help")
    , mSerialArg('s', "serialno", ArgOpt::MANDATORY_ARG)
    , mBatchArg('b', "batch")
    , mFactoryReset('r', "factory-reset")
    , mEnumerateUartDevicesArg('u', "enumerate-uart")
{
	mHelpArg.SetHelpString("Print this message to stdout.");
	mHelpArg.SetCallback(&Reboot::print_help_string, *this);
	mArgParser.AddOption(mHelpArg);

	mSerialArg.SetArgHint("serialno");
	mSerialArg.SetHelpString("Reboot the device with the given serial number.");
	mSerialArg.SetCallback(&Reboot::set_serialno_filter, *this);
	mArgParser.AddOption(mSerialArg);

	mBatchArg.SetHelpString("Reboot all connected devices.");
	mArgParser.AddOption(mBatchArg);

	mFactoryReset.SetHelpString("Resets the Thread and KNX credentials to their factory settings before rebooting.");
	mArgParser.AddOption(mFactoryReset);

	mEnumerateUartDevicesArg.SetHelpString("Will also enumerate devices connected to COM ports on your PC.");
	mArgParser.AddOption(mEnumerateUartDevicesArg);
}

Reboot::~Reboot()
{
	ca821x_util_deinit(&mDeviceRef);
}

ca_error Reboot::Process(int argc, const char *argv[])
{
	ca_error error    = CA_ERROR_SUCCESS;
	int      argi     = 1;
	size_t   devcount = 0;

	while (argi < argc)
	{
		error = mArgParser.ProcessOption(argi, argc, argv);

		if (error)
		{
			//Something went wrong, abort.
			fprintf(stderr, "Error: %s\n", ca_error_str(error));
			exit(-1);
		}
	}

	if (mHelpArg.GetCallCount())
		goto exit;

	mDeviceListFilter.SetAvailable(true);
	mDeviceList.Refresh(mDeviceListFilter, mEnumerateUartDevicesArg.GetCallCount());

	devcount = mDeviceList.Get().size();

	fprintf(stderr, "%d devices found.\n", devcount);
	if (devcount == 0)
	{
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	if (devcount > 1 && !mBatchArg.GetCallCount())
	{
		fprintf(stderr, "Error: Multiple devices found, but '--batch' not specified.\n");
		error = CA_ERROR_INVALID_STATE;
		goto exit;
	}

	for (const DeviceInfo &di : mDeviceList.Get())
	{
		mDeviceInfo = di;

		do
		{
			error = reboot_process();
			if (!error)
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
		} while (!error);

		if (mState == COMPLETE)
		{
			error = CA_ERROR_SUCCESS;
		}
		else
		{
			fprintf(stderr, "Error: Early termination - %s\n", ca_error_str(error));
			error = CA_ERROR_FAIL;
		}
	}

exit:
	return error;
}

ca_error Reboot::reboot_process()
{
	std::lock_guard<std::mutex> guard(mMutex);

	switch (mState)
	{
	case INIT:
		return init();
	case FACTORY_RESET:
		return factory_reset();
	case REBOOT:
		return reboot();
	case COMPLETE:
		return CA_ERROR_ALREADY;
	case FAIL:
		return CA_ERROR_INVALID_STATE;
		// Purposefully no default state, so compiler checks for exhaustive enum matching
	}

	return CA_ERROR_INVALID_STATE;
}

ca_error Reboot::init()
{
	ca_error status = CA_ERROR_SUCCESS;

	if (mDeviceInfo.GetExchangeType() == ca821x_exchange_kernel)
	{
		fprintf(stderr, "Error: Only USB and UART are currently supported for chilictl\n");
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

	if (mFactoryReset.GetCallCount())
		set_state(FACTORY_RESET);
	else
		set_state(REBOOT);

exit:
	return status;
}

ca_error Reboot::factory_reset()
{
	static bool function_called = false;
	if (function_called)
		return CA_ERROR_SUCCESS;

	function_called = true;

	ca_error status = CA_ERROR_SUCCESS;

	struct ca_flash_info flash_info;
	status =
	    EVBME_GET_request_sync(EVBME_ONBOARD_FLASH_INFO, sizeof(flash_info), (uint8_t *)&flash_info, NULL, &mDeviceRef);

	if (status)
	{
		fprintf(stderr, "Error, failed to get onboard flash info of [%s]\n", mDeviceInfo.GetSerialNo());
		set_state(FAIL);
		return CA_ERROR_FAIL;
	}

	status =
	    EVBME_DFU_ERASE_request(flash_info.dataFlashBaseAddr, flash_info.pageSize * flash_info.numPages, &mDeviceRef);

	return status;
}

ca_error Reboot::factory_reset_done(ca_error status)
{
	if (status)
	{
		fprintf(stderr, "Failed to factory reset the device\n");
		set_state(FAIL);
	}
	else
	{
		set_state(REBOOT);
	}

	return status;
}

ca_error Reboot::reboot()
{
	ca_error status = CA_ERROR_SUCCESS;
	// Reboot device into APROM
	EVBME_DFU_REBOOT_request(EVBME_DFU_REBOOT_APROM, &mDeviceRef);

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
		set_state(COMPLETE);
	}
	else
	{
		fprintf(stderr, "Error: Failed to send reboot command\n");
		set_state(FAIL);
		status = CA_ERROR_FAIL;
	}

	return status;
}

ca_error Reboot::print_help_string(const char *aArg)
{
	(void)aArg;

	fprintf(stdout, "--- Chili Control: Rebooting Sub-Application ---\n");

	fprintf(stdout, "\nEXAMPLE - To reboot the connected Chili device:\n");
	fprintf(stdout, "\t$ ./chilictl reboot -s FBC647CDB300A0DA\n");

	fprintf(stdout, "\nSYNOPSIS\n");
	fprintf(stdout, "\tchilictl reboot [command options]\n");

	fprintf(stdout, "\nCOMMAND OPTIONS\n");
	mArgParser.PrintOptionHelpStrings(stdout);

	return CA_ERROR_SUCCESS;
}

ca_error Reboot::set_serialno_filter(const char *aArg)
{
	(void)aArg;

	mDeviceListFilter.AddSerialNo(aArg);

	return CA_ERROR_SUCCESS;
}

ca_error Reboot::dfu_callback(EVBME_Message *params)
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
	case FACTORY_RESET:
		factory_reset_done(status);
		break;
	case REBOOT:
	case COMPLETE:
	case FAIL:
		break;
	}

	return CA_ERROR_SUCCESS;
}

ca_error Reboot::handle_evbme_message(EVBME_Message *params)
{
	fprintf(stderr, "Rx: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

ca_error Reboot::dfu_callback(EVBME_Message *params, ca821x_dev *pDeviceRef)
{
	return static_cast<Reboot *>(pDeviceRef->context)->dfu_callback(params);
}

ca_error Reboot::handle_evbme_message(EVBME_Message *params, ca821x_dev *pDeviceRef)
{
	return static_cast<Reboot *>(pDeviceRef->context)->handle_evbme_message(params);
}

const char *Reboot::state_string(State aState)
{
	switch (aState)
	{
	case INIT:
		return "INIT";
	case FACTORY_RESET:
		return "FACTORY_RESET";
	case REBOOT:
		return "REBOOT";
	case COMPLETE:
		return "COMPLETE";
	case FAIL:
		return "FAIL";
	}
	return "???";
}

void Reboot::set_state(State aNextState)
{
	printf("REBOOT [%s]: %s -> %s", mDeviceInfo.GetSerialNo(), state_string(mState), state_string(aNextState));
	std::cout << std::endl; // Added to flush output on git bash
	mState = aNextState;
}

} /* namespace ca */
