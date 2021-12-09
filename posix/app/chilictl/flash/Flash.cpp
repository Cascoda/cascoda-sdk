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

#include "ExternalFlasher.hpp"
#include "Flash.hpp"
#include "Flasher.hpp"

namespace ca {

Flash::Flash()
    : Command("flash", "Utility for flashing new binaries to connected chili devices\n  type 'flash -h' for more info")
    , mArgParser()
    , mHelpArg('h', "help")
    , mSerialArg('s', "serialno", ArgOpt::MANDATORY)
    , mBatchArg('b', "batch")
    , mAppFileArg('f', "app-file", ArgOpt::MANDATORY)
    , mOtaBootFileArg('o', "ota-boot-file", ArgOpt::MANDATORY)
    , mDfuUpdateArg('d', "dfu-update")
    , mExtFlashUpdateArg('e', "external-flash-update")
    , mIgnoreVersionArg('\0', "ignore-version")
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

	mAppFileArg.SetArgHint("filepath");
	mAppFileArg.SetHelpString("Set the .bin application file to flash to the device(s)");
	mAppFileArg.SetCallback(&Flash::set_application_file, *this);
	mArgParser.AddOption(mAppFileArg);

	mOtaBootFileArg.SetArgHint("filepath");
	mOtaBootFileArg.SetHelpString("Set the .bin ota bootloader file to flash to the device(s)");
	mOtaBootFileArg.SetCallback(&Flash::set_ota_bootloader_file, *this);
	mArgParser.AddOption(mOtaBootFileArg);

	mDfuUpdateArg.SetHelpString("Update the DFU region itself, rather than the application.");
	mArgParser.AddOption(mDfuUpdateArg);

	mExtFlashUpdateArg.SetHelpString(
	    "Use the external flash update procedure (or OTA update) to flash the new binary. This is only supported by devices with a 2nd level bootloader (ota-bootloader.bin) flashed alongside an application binary which supports ota upgrade.");
	mArgParser.AddOption(mExtFlashUpdateArg);

	mIgnoreVersionArg.SetHelpString(
	    "Ignore the version check on the device to be flashed. Warning: Flashing will not work if device firmware is older than v0.14.");
	mArgParser.AddOption(mIgnoreVersionArg);
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

	fprintf(stderr, "%d devices found.\n", devcount);
	if (devcount == 0)
	{
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
		Flasher::FlashType flashType;

		if (mOtaBootFileArg.GetCallCount())
		{
			flashType = Flasher::FlashType::APROM;
		}
		else
		{
			flashType = mDfuUpdateArg.GetCallCount() ? Flasher::FlashType::DFU : Flasher::FlashType::APROM;
			mOtaBootFilePath.clear();
		}

		//TODO: Use polymorphism to get rid of those 2 functions (external_flash_process and flash_process)
		//which have very similar code.
		if (mExtFlashUpdateArg.GetCallCount())
		{
			error = external_flash_process(di);
			if (error)
				goto exit;
		}
		else
		{
			error = flash_process(di, flashType);
			if (error)
				goto exit;
		}
	}

exit:
	return error;
}

ca_error Flash::flash_process(const DeviceInfo &aDeviceInfo, Flasher::FlashType flashType)
{
	ca_error error = CA_ERROR_SUCCESS;
	Flasher  f{mAppFilePath.c_str(), mOtaBootFilePath.c_str(), aDeviceInfo, flashType};

	if (mIgnoreVersionArg.GetCallCount())
		f.SetIgnoreVersion(true);

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
		fprintf(stderr, "Error: Early termination - %s\n", ca_error_str(error));
		error = CA_ERROR_FAIL;
	}

	return error;
}

ca_error Flash::external_flash_process(const DeviceInfo &aDeviceInfo)
{
	ca_error        error = CA_ERROR_SUCCESS;
	ExternalFlasher ext_f{mAppFilePath.c_str(), aDeviceInfo};

	do
	{
		error = ext_f.Process();
		if (!error)
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
	} while (!error);

	if (ext_f.IsComplete())
	{
		error = CA_ERROR_SUCCESS;
	}
	else
	{
		//Exit early if fail
		fprintf(stderr, "Error: Early termination - %s\n", ca_error_str(error));
		error = CA_ERROR_FAIL;
	}

	return error;
}

ca_error Flash::print_help_string(const char *aArg)
{
	(void)aArg;

	fprintf(stdout, "--- Chili Control: Flashing Sub-Application ---\n");
	fprintf(stdout,
	        "EXAMLPE 1 - DFU region update (this only has to be done once before being able to flash applications)\n");
	fprintf(stdout, "\t$ ./chilictl flash -s FBC647CDB300A0DA -df \"~/sdk-chili2/bin/ldrom-hid.bin\"\n");
	fprintf(stdout, "EXAMLPE 2 - Application flashing\n");
	fprintf(stdout, "\t$ ./chilictl flash -s FBC647CDB300A0DA -f \"~/sdk-chili2/bin/mac-dongle.bin\"\n");
	fprintf(stdout, "SYNOPSIS\n");
	fprintf(stdout, "\tchilictl [options] flash [command options]\n");
	fprintf(stdout, "COMMAND OPTIONS\n");
	mArgParser.PrintOptionHelpStrings(stdout);

	return CA_ERROR_SUCCESS;
}

ca_error Flash::set_serialno_filter(const char *aArg)
{
	mDeviceListFilter.AddSerialNo(aArg);
	return CA_ERROR_SUCCESS;
}

ca_error Flash::set_application_file(const char *aArg)
{
	if (mAppFileArg.GetCallCount() > 1)
	{
		fprintf(stderr, "Error: Multiple flash file arguments detected");
		return CA_ERROR_INVALID_ARGS;
	}

	mAppFilePath = aArg;
	return CA_ERROR_SUCCESS;
}
ca_error Flash::set_ota_bootloader_file(const char *aArg)
{
	if (mOtaBootFileArg.GetCallCount() > 1)
	{
		fprintf(stderr, "Error: Multiple flash file arguments detected");
		return CA_ERROR_INVALID_ARGS;
	}

	mOtaBootFilePath = aArg;
	return CA_ERROR_SUCCESS;
}

} /* namespace ca */
