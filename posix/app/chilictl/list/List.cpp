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

#include <cstdio>
#include <cstdlib>

#include "ca821x-posix/ca821x-posix.h"
#include "common/DeviceList.hpp"
#include "list/List.hpp"

namespace ca {

List::List()
    : Command("list", "Utility for listing connected chili devices")
    , mArgParser()
    , mHelpArg('h', "help")
    , mAvailableArg('\0', "available", ArgOpt::OPTIONAL)
    , mSerialArg('s', "serialno", ArgOpt::MANDATORY)
    , mDeviceList()
    , mDeviceListFilter()
    , mExitBeforeWork(false)
{
	mHelpArg.SetHelpString("Print this message to stdout and exit.");
	mHelpArg.SetCallback(&List::print_help_string, *this);
	mArgParser.AddOption(mHelpArg);

	mAvailableArg.SetArgHint("yes|no");
	mAvailableArg.SetHelpString("Filter the list to only include available devices, or unavailable devices if 'no'.");
	mAvailableArg.SetCallback(&List::set_available, *this);
	mArgParser.AddOption(mAvailableArg);

	mSerialArg.SetArgHint("serialno");
	mSerialArg.SetHelpString("Filter the list to only include the device with the given serial number.");
	mSerialArg.SetCallback(&List::set_serialno_filter, *this);
	mArgParser.AddOption(mSerialArg);
}

static const char *or_default(const char *val, const char *default_val)
{
	if (val[0])
		return val;
	return default_val;
}

ca_error List::Process(int argc, const char *argv[])
{
	ca_error error = CA_ERROR_SUCCESS;
	int      argi  = 1;

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

	if (mExitBeforeWork)
		return error;

	mDeviceList.Refresh(mDeviceListFilter);

	for (const DeviceInfo &di : mDeviceList.Get())
	{
		print_info(di);
	}

	if (mDeviceList.Get().size() == 0)
	{
		fprintf(stderr, "No devices found.");
	}

	return error;
}

void List::print_info(const DeviceInfo &aDeviceInfo)
{
	printf("Device Found:\n");
	printf("\tDevice: %s\n", or_default(aDeviceInfo.GetDeviceName(), "???"));
	printf("\tApp: %s\n", or_default(aDeviceInfo.GetAppName(), "???"));
	printf("\tVersion: %s\n", or_default(aDeviceInfo.GetVersion(), "???"));
	printf("\tSerial No: %s\n", or_default(aDeviceInfo.GetSerialNo(), "???"));
	printf("\tPath: %s\n", or_default(aDeviceInfo.GetPath(), "???"));
	printf("\tAvailable: %s\n", aDeviceInfo.IsAvailable() ? "Yes" : "No");
}

ca_error List::print_help_string(const char *aArg)
{
	(void)aArg;

	fprintf(stdout, "--- Chili Control: List Sub-Application ---\n");
	fprintf(stdout, "SYNOPSIS\n");
	fprintf(stdout, "\tchilictl [options] list [command options]\n");
	fprintf(stdout, "COMMAND OPTIONS\n");
	mArgParser.PrintOptionHelpStrings(stdout);

	mExitBeforeWork = true;

	return CA_ERROR_SUCCESS;
}

ca_error List::set_available(const char *aArg)
{
	if (!aArg[0] || aArg[0] == 'y')
	{
		mDeviceListFilter.SetAvailable(true);
	}
	else if (aArg[0] == 'n')
	{
		mDeviceListFilter.SetAvailable(false);
	}
	else
	{
		return CA_ERROR_INVALID_ARGS;
	}
	return CA_ERROR_SUCCESS;
}

ca_error List::set_serialno_filter(const char *aArg)
{
	mDeviceListFilter.SetSerialNo(aArg);
	return CA_ERROR_SUCCESS;
}

} /* namespace ca */
