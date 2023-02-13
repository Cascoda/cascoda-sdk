/*
 * Copyright (c) 2019, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *
 * This is the windows variant of the serial adapter, that has a reduced
 * functionality in comparison to the posix one. This is only intended
 * for use with human-readable data, whereas the posix variant works with
 * binary data also.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>

#include "ca821x-posix/ca821x-posix-evbme.h"
#include "ca821x-posix/ca821x-posix.h"
#include "evbme_messages.h"

static struct ca821x_dev sDeviceRef;

static uint8_t s_enabled = false;

/**
 * Configure the IO to be pretty raw (no io processing from the os)
 * @return ca_error
 * @retval CA_ERROR_SUCCESS Successful
 * @retval CA_ERROR_INVALID_STATE Already initialised
 */
static ca_error configure_io(void)
{
	ca_error error   = CA_ERROR_SUCCESS;
	HANDLE   hStdin  = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD    mode    = 0;

	if (s_enabled)
		error = CA_ERROR_INVALID_STATE;

	//Disable echoing of input & line-by-line processing
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode & (~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)));

	setmode(STDIN_FILENO, O_BINARY);
	setmode(STDOUT_FILENO, O_BINARY);

	if (error == CA_ERROR_SUCCESS)
		s_enabled = true;

	return error;
}

/**
 * Handle writing some data to the terminal from the embedded device.
 *
 * @param length Length of the data to print
 * @param buf The buffer containing the data to print
 * @return ca_error
 * @retval CA_ERROR_SUCCESS if successful
 * @retval CA_ERROR_FAIL if irrecoverable failure
 */
static ca_error handle_write(size_t length, const uint8_t *buf)
{
	ssize_t rval   = 0;
	ssize_t offset = 0;

	while (length)
	{
		rval = write(STDOUT_FILENO, buf + offset, length);

		if (rval <= 0)
		{
			return CA_ERROR_FAIL;
		}

		offset += rval;
		length -= rval;
	}

	return CA_ERROR_SUCCESS;
}

/**
 * Handle evbme message indication
 * @param params evbme message params
 * @param pDeviceRef ca821x_dev struct representing device this data was received from
 * @return CA_ERROR_SUCCESS
 */
static ca_error handle_evbme_message(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef)
{
	fprintf(stderr, "Rx: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	fflush(stderr);
	return CA_ERROR_SUCCESS;
}

/**
 * Handle a buffer received from the Cascoda device that is not a CA-821x command
 * @param buf Data buffer, [0] byte is command, [1] is len, [2] is start of payload
 * @param len length of entire data buffer
 * @param pDeviceRef ca821x_dev struct representing device this data was received from
 * @return ca_error
 * @retval CA_ERROR_SUCCESS Successfully handled
 * @retval CA_ERROR_NOT_HANDLED Command was not handled in this function
 */
static ca_error handle_user_command(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	ca_error error = CA_ERROR_NOT_HANDLED;

	if (buf[0] == 0xB3)
	{
		error = handle_write(len - 2, buf + 2);

		if (error != CA_ERROR_SUCCESS)
			abort();
	}

	return error;
}

/**
 * Function to receive and process input until s_enabled is false.
 *
 * @param pDeviceRef ca821x_dev device reference
 */
static void process_input(struct ca821x_dev *pDeviceRef)
{
	uint8_t receive_buffer[128]; // Size must be < 255

	while (s_enabled)
	{
		ssize_t rval = 0;

		rval = read(STDIN_FILENO, receive_buffer, sizeof(receive_buffer));

		if (rval == -EAGAIN)
		{
			continue;
		}
		else if (rval < 0)
		{
			abort();
		}
		else if (rval > 0)
		{
			exchange_user_command(0xB2, (uint8_t)rval, receive_buffer, pDeviceRef);
		}
	}
}

int main(int argc, char *argv[])
{
	char *serial_num = NULL;
	if (argc > 1)
	{
		serial_num = argv[1];
	}

	struct ca821x_dev *pDeviceRef = &sDeviceRef;
	while (ca821x_util_init(pDeviceRef, NULL, serial_num))
	{
		return -1;
		sleep(1);
	}
	configure_io();

	if (EVBME_CheckVersion(NULL, pDeviceRef) != CA_ERROR_SUCCESS)
		exit(1);

	exchange_register_user_callback(handle_user_command, pDeviceRef);
	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_MESSAGE_indication = &handle_evbme_message;
	ca821x_util_start_upstream_dispatch_worker();

	process_input(pDeviceRef);
}
