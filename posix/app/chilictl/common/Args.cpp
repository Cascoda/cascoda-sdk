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
#include <cstring>

#include "Args.hpp"

namespace ca {

ArgOpt::ArgOpt(char aShortArg, const char *aLongArg, ArgPerm aArgPerm)
    : shortarg(aShortArg)
    , longarg(aLongArg)
    , helpstr("")
    , arghint("value")
    , optional_arg(false)
    , mandatory_arg(false)
    , callback()
    , count(0)
{
	if (aArgPerm == ArgPerm::OPTIONAL)
		optional_arg = true;
	if (aArgPerm == ArgPerm::MANDATORY)
		mandatory_arg = true;
}

ca_error Args::ProcessOption(int &argi, int argc, const char *argv[])
{
	ca_error    error  = CA_ERROR_NOT_HANDLED;
	const char *curarg = argv[argi];

	if (argi < argc && argi < mFinalOpt)
	{
		if (is_shortopt(curarg))
		{
			//We have a '-abc' style argument, pass details to process_short_args
			error = process_short_opts(argi, argc, argv);
		}
		else if (is_longopt(curarg))
		{
			//We have a '--long-opt' style argument, pass details to process_long_args
			error = process_long_opt(argi, argc, argv);
		}
		else if (is_end_of_opts(curarg))
		{
			//The '--' argument terminates option processing
			mFinalOpt = argi;
			argi += 1;
			error = CA_ERROR_SUCCESS;
		}
	}
	return error;
}

void Args::PrintOptionHelpStrings(FILE *aOutFile)
{
	for (ArgOpt *opt : mOptions)
	{
		print_opt_help_string(aOutFile, opt);
	}
}

bool Args::is_shortopt(const char *str)
{
	if (str[0] == '-' && str[1] && str[1] != '-')
		return true;
	return false;
}

bool Args::is_longopt(const char *str)
{
	if (str[0] == '-' && str[1] == '-' && str[2] && str[2] != '-')
		return true;
	return false;
}

bool Args::is_end_of_opts(const char *str)
{
	if (strcmp(str, "--") == 0)
		return true;
	return false;
}

bool Args::is_opt(const char *str)
{
	return is_shortopt(str) || is_longopt(str) || is_end_of_opts(str);
}

ca_error Args::process_short_opts(int &argi, int argc, const char *argv[])
{
	ca_error    status     = CA_ERROR_SUCCESS;
	const char *nextarg    = (argi + 1) < argc ? argv[argi + 1] : NULL;
	const char *curcharptr = argv[argi];
	char        curchar;

	while ((curchar = *(++curcharptr)))
	{
		bool found = false;

		for (ArgOpt *optp : mOptions)
		{
			ArgOpt &opt = *optp;
			if (opt.shortarg == curchar)
			{
				//match
				found = true;
				if (!opt.optional_arg && !opt.mandatory_arg)
				{
					// If this opt doesn't take an arg, call the callback
					status = opt.call_callback("");
				}
				else if (*(curcharptr + 1))
				{
					// If there is an argument in this shortopt (eg "-ofoo" form), use that as the arg
					status = opt.call_callback(curcharptr + 1);
					goto exit;
				}
				else if (nextarg && (opt.mandatory_arg || !is_opt(nextarg)))
				{
					// If there is an argument as a next-arg (eg "-o foo" form), use that as the arg
					status = opt.call_callback(nextarg);
					argi += 1;
					goto exit;
				}
				else if (opt.mandatory_arg)
				{
					// Error, no argument, but is required!
					fprintf(stderr, "Error: -%c requires an argument.\n", opt.shortarg);
					status = CA_ERROR_INVALID_ARGS;
					goto exit;
				}
				else
				{
					// No optional argument, that's ok!
					status = opt.call_callback("");
					goto exit;
				}
			}
		}

		if (!found)
		{
			fprintf(stderr, "Error: -%c option not recognised.\n", curchar);
			status = CA_ERROR_INVALID_ARGS;
			goto exit;
		}
	}

exit:
	argi += 1;
	return status;
}

void Args::print_longopt_error(const char *optptr, size_t optlen)
{
	bool first = true;
	fprintf(stderr, "Error: Ambiguous long option: --%.*s could be one of ", optlen, optptr);
	for (ArgOpt *optp : mOptions)
	{
		ArgOpt &opt = *optp;
		if (!opt.longarg)
			continue; // No longarg form

		if (strncmp(optptr, opt.longarg, optlen) != 0)
		{
			if (!first)
				fprintf(stderr, ", ");
			first = false;
			fprintf(stderr, "--%s", opt.longarg);
		}
	}
	fprintf(stderr, "\n");
}

ca_error Args::process_long_opt(int &argi, int argc, const char *argv[])
{
	ca_error    status    = CA_ERROR_SUCCESS;
	const char *optptr    = argv[argi] + 2; //Skip the leading '--'
	const char *argptr    = "";             // Argument string or "" if none
	const char *nextarg   = (argi + 1) < argc ? argv[argi + 1] : NULL;
	size_t      optlen    = 0;
	size_t      arglen    = 0;
	ArgOpt *    found_opt = NULL;
	(void)argc;

	argi += 1;

	// Break up the long opt into option and argument.
	while (true)
	{
		if (optptr[optlen] == '\0')
		{
			break;
		}
		if (optptr[optlen] == '=')
		{
			argptr = optptr + optlen + 1;
			arglen = strlen(argptr);
			break;
		}
		optlen++;
	}

	// Find a matching option struct
	for (ArgOpt *optp : mOptions)
	{
		ArgOpt &opt = *optp;

		if (!opt.longarg)
			continue; // No longarg form

		if (strncmp(optptr, opt.longarg, optlen) != 0)
			continue; // Does not match (abbreviations allowed)

		if (strlen(opt.longarg) == optlen)
		{
			// Exact match found
			status    = CA_ERROR_SUCCESS;
			found_opt = optp;
			break;
		}

		if (found_opt)
		{
			// Already found a partial match - ambiguous!
			// We don't exit now because we could still find exact match...
			status = CA_ERROR_INVALID_ARGS;
		}

		// Register the partial match
		found_opt = optp;
	}

	if (status == CA_ERROR_INVALID_ARGS)
	{
		print_longopt_error(optptr, optlen);
		goto exit;
	}

	if (!found_opt)
	{
		fprintf(stderr, "Error: --%.*s not recognised.\n", optlen, optptr);
		status = CA_ERROR_INVALID_ARGS;
		goto exit;
	}

	// If we have a mandatory arg but no 'arg=val' notation, check the next argument
	if (found_opt->mandatory_arg && !arglen && nextarg)
	{
		argptr = nextarg;
		arglen = strlen(argptr);
	}

	// Process the found option and call the callback.
	if ((found_opt->mandatory_arg || found_opt->optional_arg) && arglen)
	{
		// Argument expected and provided
		status = found_opt->call_callback(argptr);
	}
	else if (!(found_opt->mandatory_arg || found_opt->optional_arg) && arglen)
	{
		// Argument not expected, but provided
		fprintf(stderr, "Error: --%.*s does not take an argument.\n", optlen, optptr);
		status = CA_ERROR_INVALID_ARGS;
		goto exit;
	}
	else if (found_opt->mandatory_arg)
	{
		// Argument required but not provided
		fprintf(stderr, "Error: --%.*s requires an argument.\n", optlen, optptr);
		status = CA_ERROR_INVALID_ARGS;
		goto exit;
	}
	else
	{
		// Argument not provided and not required
		status = found_opt->call_callback("");
	}

exit:
	return status;
}

void Args::print_opt_help_string(FILE *out, ArgOpt *opt)
{
	fprintf(out, "\t");
	if (opt->shortarg)
	{
		fprintf(out, "-%c", opt->shortarg);

		if (opt->optional_arg)
			fprintf(out, " [<%s>]", opt->arghint);
		else if (opt->mandatory_arg)
			fprintf(out, " <%s>", opt->arghint);
	}
	if (opt->longarg && opt->longarg[0])
	{
		if (opt->shortarg)
			fprintf(out, ", ");
		fprintf(out, "--%s", opt->longarg);

		if (opt->optional_arg)
			fprintf(out, "[=<%s>]", opt->arghint);
		else if (opt->mandatory_arg)
			fprintf(out, "=<%s>", opt->arghint);
	}
	if (opt->helpstr)
	{
		fprintf(out, "\n\t\t%s", opt->helpstr);
	}
	fprintf(out, "\n");
}

} /* namespace ca */
