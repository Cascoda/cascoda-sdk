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

#ifndef POSIX_APP_CHILICTL_COMMON_COMMAND_HPP_
#define POSIX_APP_CHILICTL_COMMON_COMMAND_HPP_

#include "ca821x_error.h"

namespace ca {

/**
 *  Abstract class for handling command positional arguments, to be implemented by the command classes.
 */
class Command
{
public:
	/**
	 * Run the command, processing the arguments passed in argv.
	 *
	 * Argv contains the arguments that followed the command name, including the command name as arg0.
	 *
	 * @param argc Argument Count
	 * @param argv Argument Vector (of length argc)
	 * @return ca_error value
	 * @retval CA_ERROR_SUCCESS Successful processing
	 * @retval CA_ERROR_INVALID_ARGS Invalid arguments passed to function
	 */
	virtual ca_error Process(int argc, const char *argv[]) { return CA_ERROR_INVALID; }

	/** Get the help string for this command */
	const char *GetHelpString(void) { return mHelpString; }

	/** Get the command name of this command */
	const char *GetCommandName(void) { return mCommandName; }

protected:
	const char *mCommandName;
	const char *mHelpString;

	Command(const char *aCommandName, const char *aHelpString)
	    : mCommandName(aCommandName)
	    , mHelpString(aHelpString)
	{
	}

	~Command(){};
};

} // namespace ca

#endif /* POSIX_APP_CHILICTL_COMMON_COMMAND_HPP_ */
