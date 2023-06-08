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

#include <cstdio>
#include <cstring>
#include <iostream>

#if defined(_WIN32)
#include <synchapi.h> // for Windows Sleep() function
#else
#include <unistd.h> // for posix usleep() function
#endif

namespace ca {

Reboot::Reboot()
    : Command("reboot", "Utility for rebooting connected chili devices\n  type 'reboot -h' for more info\n")
    , mArgParser()
    , mHelpArg('h', "help")
    , mSerialArg('s', "serialno", ArgOpt::MANDATORY_ARG)
    , mBatchArg('b', "batch")
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
		error       = reboot_process();
		if (error)
			goto exit;
	}

exit:
	return error;
}

ca_error Reboot::reboot_process()
{
	ca_error status            = CA_ERROR_SUCCESS;
	bool     reboot_successful = false;

	status = init();
	if (status)
		goto exit;

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
		reboot_successful = true;
	}
	else
	{
		fprintf(stderr, "Error: Failed to send reboot command\n");
		reboot_successful = false;
		status            = CA_ERROR_FAIL;
	}

	reboot_done(reboot_successful);

exit:
	return status;
}

ca_error Reboot::init()
{
	ca_error status = CA_ERROR_SUCCESS;

	if (mDeviceInfo.GetExchangeType() == ca821x_exchange_kernel)
	{
		fprintf(stderr, "Error: Only USB and UART are currently supported for chilictl\n");
		status = CA_ERROR_INVALID_ARGS;
		goto exit;
	}

	//Open device
	status = ca821x_util_init_path(&mDeviceRef, nullptr, mDeviceInfo.GetExchangeType(), mDeviceInfo.GetPath());
	if (status)
		goto exit;

exit:
	return status;
}

ca_error Reboot::reboot_done(bool success)
{
	printf("[%s]: REBOOT -> %s", mDeviceInfo.GetSerialNo(), success ? "DONE" : "FAIL");
	std::cout << std::endl;
	return CA_ERROR_SUCCESS;
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
} /* namespace ca */
