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

#ifndef POSIX_APP_CHILICTL_COMMON_ARGS_H_
#define POSIX_APP_CHILICTL_COMMON_ARGS_H_

#include <functional>
#include <limits>
#include <stdio.h>
#include <vector>

#include "ca821x_error.h"

namespace ca {

/**
 * Structure for holding the definition of potential options to pass on the command line
 */
class ArgOpt
{
	friend class Args;
	using argCallback                          = std::function<ca_error(const char *)>;
	template <class T> using argCallbackMemRaw = ca_error (T::*)(const char *argument);

public:
	/**
	 * Enumeration for whether or not an option permits/requires arguments
	 */
	enum ArgPerm
	{
		NO_ARG = 0,    //!< No argument allowed to option
		OPTIONAL_ARG,  //!< Option has optional argument
		MANDATORY_ARG, //!< Option requires an argument to be provided
	};

	/**
	 * Construct an ArgOpt data structure
	 * @param aShortArg Short form option (or '\0' if none)
	 * @param aLongArg Long form option (or NULL if none)
	 * @param aArgPerm @see ArgPerm
	 */
	ArgOpt(char aShortArg, const char *aLongArg, ArgPerm aArgPerm = ArgPerm::NO_ARG);

	/**
	 * Set the help string of the option
	 * @param aHelpString Pointer to a help string with static lifetime (e.g. a string constant)
	 */
	void SetHelpString(const char *aHelpString) { helpstr = aHelpString; }

	/**
	 * Set the short argument hint string, describing the argument (eg "yes|no", "path", "value)
	 * @param aArgHint Pointer to a arg hint string with static lifetime (e.g. a string constant)
	 */
	void SetArgHint(const char *aArgHint) { arghint = aArgHint; }

	/**
	 * Set the callback to be called when the option is detected.
	 * @param aCallback The callback to be called, or NULL for none.
	 */
	void SetCallback(argCallback aCallback) { callback = aCallback; }

	/**
	 * Set the callback to be called when the option is detected.
	 * @param aCallback Function pointer to member function to call
	 * @param aInstance Instance to call the member function on
	 */
	template <class T> void SetCallback(argCallbackMemRaw<T> aCallback, T &aInstance)
	{
		callback = std::bind(aCallback, &aInstance, std::placeholders::_1);
	}

	/**
	 * Get the number of times this option was detected during parsing. This is incremented just before calling the callback.
	 * @return Count of the number of times this option has been detected so far during parsing.
	 */
	int GetCallCount(void) { return count; }

private:
	char        shortarg;      //!< Short form option (or '\0' if none)
	const char *longarg;       //!< Long form option (or NULL if none)
	const char *helpstr;       //!< Help string
	const char *arghint;       //!< Argument hint string, eg 'value', 'yes|no', 'path'
	bool        optional_arg;  //!< True if the option takes an optional argument
	bool        mandatory_arg; //!< True if the option takes a mandatory argument
	argCallback callback;      //!< Callback to be called whenever option is detected

	int count; //!< Count of the number of times this option appears in command line.

	/**
	 * Notify the higher layer of an argument detection.
	 * @param aArg Argument if included, or ""
	 * @return status of callback (no callback is still success)
	 */
	ca_error call_callback(const char *aArg)
	{
		count++;
		if (callback)
			return callback(aArg);
		return CA_ERROR_SUCCESS;
	}
};

/**
 * Class for parsing and handling options passed as command line arguments.
 * Conforms to POSIX, also supports GNU style long options.
 */
class Args
{
public:
	/**
	 * Default constructor, initialises empty arg parser.
	 */
	Args()
	    : mFinalOpt(std::numeric_limits<int>::max())
	    , mOptions(){};

	/**
	 * Process a single option from argv using the provided ArgOpt structures.
	 *
	 * This function increments argi internally, and in the case where an option
	 * takes an argument, it may increment it more than once. If the end of options
	 * has been reached, or a non-option argument in encountered, CA_ERROR_NOT_HANDLED
	 * will be returned.
	 *
	 * argi should begin at 1 to avoid processing the arg0, which is the executable name.
	 *
	 * @param argi Current index to process in argv. argi will be increased if processing is successful.
	 * @param argc Argument count (number of args in argv)
	 * @param argv Argument vector (Array of c-strings, representing the arguments)
	 *
	 * @return Status of the processing
	 * @retval CA_ERROR_SUCCESS      Successfully processed options
	 * @retval CA_ERROR_NOT_HANDLED  Positional argument encountered or end of arguments reached
	 * @retval CA_ERROR_INVALID_ARGS Option not recognised or incorrect usage
	 */
	ca_error ProcessOption(int &argi, int argc, const char *argv[]);

	/**
	 * Print all of the option help strings to the 'out' file pointer in manpage format.
	 *
	 * @param aOutFile FILE to print to.
	 */
	void PrintOptionHelpStrings(FILE *aOutFile);

	/**
	 * Add an option definition to this parser.
	 *
	 * The ArgOpt instance added should have a lifetime at least as long as the Args
	 * instance.
	 *
	 * @param aOption The option definition to add.
	 */
	void AddOption(ArgOpt &aOption) { mOptions.push_back(&aOption); }

private:
	int                   mFinalOpt; //!< Final option index
	std::vector<ArgOpt *> mOptions;  //!< Vector of the option definitions

	/**
	 * Check if the string is a shortopt of the form "-a" or "-abc"
	 * @param str The string to check
	 * @return True if str is a shortopt, False otherwise
	 */
	static bool is_shortopt(const char *aStr);

	/**
	 * Check if the string is a longopt of the form "--long-opt"
	 * @param str The string to check
	 * @return True if str is a longopt, False otherwise
	 */
	static bool is_longopt(const char *aStr);

	/**
	 * Check if the string is the "--" end of options delimiter
	 * @param str The string to check
	 * @return True if str is equal to "--", False otherwise
	 */
	static bool is_end_of_opts(const char *aStr);

	/**
	 * Check if the string is any kind of option (-o, --long or --)
	 * @param str The string to check
	 * @return True if str is an option, False otherwise
	 */
	static bool is_opt(const char *aStr);

	/**
	 * Process short options in the current argument.
	 *
	 * @param argi Current argument index, will be incremented by this function as required
	 * @param argc Argument count (number of args in argv)
	 * @param argv Argument vector (Array of c-strings, representing the arguments)
	 * @return ca_error value
	 * @retval CA_ERROR_SUCCESS      Successfully processed short arg
	 * @retval CA_ERROR_NOTFOUND     One of the arguments in the string was not found
	 * @retval CA_ERROR_INVALID_ARGS Invalid argument to one of the options (or argument missing)
	 */
	ca_error process_short_opts(int &argi, int argc, const char *argv[]);

	/**
	 * Process long options in the current argument.
	 *
	 * @param argi Current argument index, will be incremented by this function as required
	 * @param argc Argument count (number of args in argv)
	 * @param argv Argument vector (Array of c-strings, representing the arguments)
	 * @return ca_error value
	 * @retval CA_ERROR_SUCCESS      Successfully processed short arg
	 * @retval CA_ERROR_NOTFOUND     One of the arguments in the string was not found
	 * @retval CA_ERROR_INVALID_ARGS Invalid argument to one of the options (or argument missing)
	 */
	ca_error process_long_opt(int &argi, int argc, const char *argv[]);

	/**
	 * Print the error message for ambiguous long options.
	 * @param optptr Pointer to option string (without '--')
	 * @param optlen Length of the option string part (up until the '=' or '\0')
	 */
	void print_longopt_error(const char *optptr, size_t optlen);

	/**
	 * Print the 'opt' help string to file 'out'
	 * @param out Output file
	 * @param opt Pointer to ArgOpt to print
	 */
	void print_opt_help_string(FILE *out, ArgOpt *opt);
};

} /* namespace ca */

#endif /* POSIX_APP_CHILICTL_COMMON_ARGS_H_ */
