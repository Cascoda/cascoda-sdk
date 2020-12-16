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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <thread>

#include "Flash.hpp"
#include "Flasher.hpp"

namespace ca {

Flash::Flash()
    : Command("flash", "Utility for flashing new binaries to connected chili devices")
    , mArgParser()
    , mHelpArg('h', "help")
    , mSerialArg('s', "serialno", ArgOpt::MANDATORY)
    , mBatchArg('b', "batch")
    , mFileArg('f', "file", ArgOpt::MANDATORY)
    , mDfuUpdateArg('d', "dfu-update")

{
	mHelpArg.SetHelpString("Print this message to stdout");
	mHelpArg.SetCallback(&Flash::print_help_string, *this);
	mArgParser.AddOption(mHelpArg);

	mSerialArg.SetArgHint("serialno");
	mSerialArg.SetHelpString("Flash the device with the given serial number.");
	mSerialArg.SetCallback(&Flash::set_serialno_filter, *this);
	mArgParser.AddOption(mSerialArg);

	mBatchArg.SetHelpString("Permit the flashing of multiple devices at a time.");
	mArgParser.AddOption(mBatchArg);

	mFileArg.SetArgHint("filepath");
	mFileArg.SetHelpString("Set the .bin file to flash to the device(s)");
	mFileArg.SetCallback(&Flash::set_file, *this);
	mArgParser.AddOption(mFileArg);

	mDfuUpdateArg.SetHelpString("Update the DFU region itself, rather than the application.");
	mArgParser.AddOption(mDfuUpdateArg);
}

ca_error Flash::Process(int argc, const char *argv[])
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
	mDeviceList.Refresh(mDeviceListFilter);

	devcount = mDeviceList.Get().size();

	if (devcount == 0)
	{
		fprintf(stderr, "No devices found.");
		error = CA_ERROR_NOT_FOUND;
		goto exit;
	}

	if (devcount > 1 && !mBatchArg.GetCallCount())
	{
		fprintf(stderr, "Error: Multiple devices found, but '--batch' not specified.");
		error = CA_ERROR_INVALID_STATE;
		goto exit;
	}

	//TODO: Expand this to be parallel, and have a non-polling mechanism to call 'process'
	for (const DeviceInfo &di : mDeviceList.Get())
	{
		Flasher::FlashType flashType =
		    mDfuUpdateArg.GetCallCount() ? Flasher::FlashType::DFU : Flasher::FlashType::APROM;
		Flasher f{mFilePath.c_str(), di, flashType};
		do
		{
			error = f.Process();
			if (!error)
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
		} while (!error);

		if (f.IsComplete())
		{
			error = CA_ERROR_SUCCESS;
		}
		else
		{
			//Exit early if fail
			goto exit;
		}
	}

exit:
	return error;
}

ca_error Flash::print_help_string(const char *aArg)
{
	(void)aArg;

	fprintf(stdout, "--- Chili Control: Flashing Sub-Application ---\n");
	fprintf(stdout, "SYNOPSIS\n");
	fprintf(stdout, "\tchilictl [options] flash [command options]\n");
	fprintf(stdout, "COMMAND OPTIONS\n");
	mArgParser.PrintOptionHelpStrings(stdout);

	return CA_ERROR_SUCCESS;
}

ca_error Flash::set_serialno_filter(const char *aArg)
{
	if (mSerialArg.GetCallCount() > 1)
	{
		fprintf(stderr, "Error: Multiple serialno args not currently supported");
		return CA_ERROR_INVALID_ARGS;
	}
	mDeviceListFilter.SetSerialNo(aArg);
	return CA_ERROR_SUCCESS;
}

ca_error Flash::set_file(const char *aArg)
{
	if (mFileArg.GetCallCount() > 1)
	{
		fprintf(stderr, "Error: Multiple flash file arguments detected");
		return CA_ERROR_INVALID_ARGS;
	}

	mFilePath = aArg;
	return CA_ERROR_SUCCESS;
}

} /* namespace ca */
