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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <thread>

#include "Pipe.hpp"

#include "ca821x-posix/ca821x-posix.h"
#include "ca821x_api.h"

namespace ca {

Pipe::Pipe()
    : Command("pipe",
              "Utility for piping binary commands to/from a connected chili device\n  type 'pipe -h' for more info")
    , mArgParser()
    , mHelpArg('h', "help")
    , mSerialArg('s', "serialno", ArgOpt::MANDATORY)
    , mAnyArg('a', "any")
    , mResetArg('r', "reset")

{
	mHelpArg.SetHelpString("Print this message to stdout");
	mHelpArg.SetCallback(&Pipe::print_help_string, *this);
	mArgParser.AddOption(mHelpArg);

	mSerialArg.SetArgHint("serialno");
	mSerialArg.SetHelpString("Pipe the device with the given serial number.");
	mSerialArg.SetCallback(&Pipe::set_serialno_filter, *this);
	mArgParser.AddOption(mSerialArg);

	mAnyArg.SetHelpString("Pick any matching device, rather than throwing error if more than one match.");
	mArgParser.AddOption(mAnyArg);

	mResetArg.SetHelpString("Reset the device with a software reset before piping.");
	mArgParser.AddOption(mResetArg);
}

ca_error Pipe::Process(int argc, const char *argv[])
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

	if (devcount > 1 && !mAnyArg.GetCallCount())
	{
		fprintf(stderr, "Error: Multiple devices found but any flag not specified.\n");
		error = CA_ERROR_INVALID_STATE;
		goto exit;
	}

	if (isatty(STDOUT_FILENO) || isatty(STDIN_FILENO))
	{
		fprintf(stderr, "Error: Cannot pipe to/from terminal - currently binary data only.\n");
		error = CA_ERROR_INVALID_STATE;
		goto exit;
	}

	error = run_pipe(mDeviceList.Get()[0]);

exit:
	return error;
}

ca_error Pipe::exchange_callback(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	Pipe *p = static_cast<Pipe *>(pDeviceRef->context);
	return p->exchange_callback(buf, len);
}

ca_error Pipe::exchange_callback(const uint8_t *buf, size_t len)
{
	std::lock_guard<std::mutex> lg(mIoMutex);
	int                         rval = write(STDOUT_FILENO, buf, len);
	if (rval < 0)
	{
		fprintf(stderr, "Error: Failed to write command to stdout.\n");
	}
	return CA_ERROR_SUCCESS;
}

ca_error Pipe::run_pipe(const DeviceInfo &di)
{
	struct ca821x_dev  dev;
	struct MAC_Message msg;
	struct MAC_Message rsp;
	ca_error           status;
	int                rval = 0;
	status                  = ca821x_util_init_path(&dev, nullptr, di.GetExchangeType(), di.GetPath());
	dev.context             = this;
	exchange_register_user_callback(&exchange_callback, &dev);

	if (mResetArg.GetCallCount() > 0)
	{
		status = EVBME_DFU_REBOOT_request(EVBME_DFU_REBOOT_APROM, &dev);
	}

#ifdef WIN32
	setmode(STDOUT_FILENO, O_BINARY);
	setmode(STDIN_FILENO, O_BINARY);
#endif
	while (status == CA_ERROR_SUCCESS)
	{
		// Read the command bytes
		rval = read(STDIN_FILENO, &msg.CommandId, 2);
		if (rval == 0)
		{
			// End of file
			break;
		}
		else if (rval < 0)
		{
			status = CA_ERROR_FAIL;
			break;
		}

		// Catch invalid length and command
		if (msg.CommandId == 0xFF || msg.Length == 0xFF)
		{
			status = CA_ERROR_INVALID;
			break;
		}

		// Read the remaining bytes
		rval = read(STDIN_FILENO, msg.PData.Payload, msg.Length);
		if (rval < 0)
		{
			status = CA_ERROR_FAIL;
			break;
		}
		rsp.CommandId = 0xFF;

		// Send the read command
		status = ca821x_api_downstream(&msg.CommandId, &rsp.CommandId, &dev);
		if (status)
		{
			fprintf(stderr, "Error: Downstream error %s.\n", ca_error_str(status));
			break;
		}

		// Indicate the response up if required
		if (rsp.CommandId != 0xFF)
		{
			std::lock_guard<std::mutex> lg(mIoMutex);
			rval = write(STDOUT_FILENO, &rsp.CommandId, rsp.Length + 2);
			if (rval < 0)
			{
				status = CA_ERROR_FAIL;
				break;
			}
		}
	}

	if (status == CA_ERROR_SUCCESS)
		status = exchange_wait_send_complete(1, &dev);

	ca821x_util_deinit(&dev);
	return status;
}

ca_error Pipe::print_help_string(const char *aArg)
{
	(void)aArg;

	fprintf(stdout, "--- Chili Control: Binary piping Sub-Application ---\n");
	fprintf(stdout, "SYNOPSIS\n");
	fprintf(stdout, "\tchilictl [options] pipe [command options]\n");
	fprintf(stdout, "COMMAND OPTIONS\n");
	mArgParser.PrintOptionHelpStrings(stdout);

	return CA_ERROR_SUCCESS;
}

ca_error Pipe::set_serialno_filter(const char *aArg)
{
	mDeviceListFilter.AddSerialNo(aArg);
	return CA_ERROR_SUCCESS;
}

} /* namespace ca */
