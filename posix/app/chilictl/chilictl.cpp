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
/**
 * @file
 * Main file of the chilictl utility - directs commands to relevant submodules
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "ca821x-posix/ca821x-posix.h"
#include "ca821x_api.h"
#include "ca821x_error.h"
#include "ca821x_log.h"

#include "common/Args.hpp"
#include "flash/Flash.hpp"
#include "list/List.hpp"
#include "pipe/Pipe.hpp"

static ca::Args                   sArgParser;
static std::vector<ca::Command *> sCommands;
static const char *               arg0;

static void print_command_help_strings(FILE *out)
{
	for (ca::Command *pcommand : sCommands)
	{
		ca::Command &cmd = *pcommand;
		fprintf(out, "\t%s\n\t\t%s\n", cmd.GetCommandName(), cmd.GetHelpString());
	}
}

static void print_help_string(FILE *out)
{
	fprintf(out, "--- Chili Control Application ---\n");
	fprintf(out, "SYNOPSIS\n");
	fprintf(out, "\t%s [options] <command> [command options]\n", arg0);
	fprintf(out, "OPTIONS\n");
	sArgParser.PrintOptionHelpStrings(out);
	fprintf(out, "COMMANDS\n");
	print_command_help_strings(out);
}

static ca_error opt_print_help(const char *arg)
{
	(void)arg;

	print_help_string(stdout);
	exit(0);

	return CA_ERROR_SUCCESS;
}

static ca_error opt_print_version(const char *arg)
{
	(void)arg;

	printf("chilictl %s\n", ca821x_get_version());
	printf("Copyright (c) 2020, Cascoda Ltd\n");
	exit(0);

	return CA_ERROR_SUCCESS;
}

static ca_error process_command(int &argi, int argc, const char *argv[])
{
	ca_error    error   = CA_ERROR_NOT_HANDLED;
	const char *cmdname = argv[argi];
	bool        found   = false;

	for (ca::Command *pcommand : sCommands)
	{
		ca::Command &command = *pcommand;
		if (strcmp(cmdname, command.GetCommandName()) == 0)
		{
			error = command.Process(argc - argi, argv + argi);
			found = true;
			if (!error)
				exit(0);
			break;
		}
	}
	if (!found)
	{
		fprintf(stderr, "Error, Unrecognised command %s\n", cmdname);
	}
	return error;
}

int main(int argc, const char *argv[])
{
	int      argi  = 1;
	ca_error error = CA_ERROR_SUCCESS;

	ca::ArgOpt helpOpt{'h', "help"};
	helpOpt.SetHelpString("Print this message to stdout and exit.");
	helpOpt.SetCallback(&opt_print_help);
	sArgParser.AddOption(helpOpt);

	ca::ArgOpt versOpt{'v', "version"};
	versOpt.SetHelpString("Print version information to stdout and exit.");
	versOpt.SetCallback(&opt_print_version);
	sArgParser.AddOption(versOpt);

	ca::Flash flashCmd{};
	ca::List  listCmd{};
	ca::Pipe  pipeCmd{};
	sCommands.push_back(&listCmd);
	sCommands.push_back(&flashCmd);
	sCommands.push_back(&pipeCmd);

	ca821x_util_start_downstream_dispatch_worker();

	arg0 = argv[0];
	while (argi < argc)
	{
		error = sArgParser.ProcessOption(argi, argc, argv);

		if (error == CA_ERROR_NOT_HANDLED)
			error = process_command(argi, argc, argv);

		if (error)
		{
			fprintf(stderr, "Error: %s\n", ca_error_str(error));
			exit(-1);
		}
	}

	fprintf(stderr, "Error: Reached end of arguments without command.\n");
	print_help_string(stderr);
}
