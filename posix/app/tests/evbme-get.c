/*
 * Copyright (c) 2020, Cascoda
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix-evbme.h"
#include "ca821x-posix/ca821x-posix.h"
#include "evbme_messages.h"

struct ca821x_dev sDeviceRef;

enum printmethod
{
	PRINT_STRING,
	PRINT_U64BE,
	PRINT_U64LE,
	PRINT_HEXDUMP,
};

#define test_evbme_param(attr, pm, optional) test_evbme_param_fn(attr, #attr, pm, optional)

/**
 * Run a test to get an evbme parameter and print it. Use test_evbme_param macro to make life easier.
 *
 * @param attr The attribute to get
 * @param attr_name The string name for that attribute
 * @param pm The method to use to print the attribute
 * @param is_optional Is the attribute allowed to be excluded from a device?
 *
 * @return Test pass/fail status
 * @retval CA_ERROR_SUCCESS Test passed
 * @retval CA_ERROR_FAIL Test failed
 */
static ca_error test_evbme_param_fn(enum evbme_attribute attr,
                                    const char *         attr_name,
                                    enum printmethod     pm,
                                    bool                 is_optional)
{
	uint8_t  buffer[50] = {};
	uint8_t  attrlen    = 0;
	ca_error error      = EVBME_GET_request_sync(attr, sizeof(buffer), buffer, &attrlen, &sDeviceRef);

	if (error == CA_ERROR_UNKNOWN && is_optional)
	{
		printf("%s Not available on this device\n", attr_name);
		error = CA_ERROR_SUCCESS;
	}
	else if (error)
	{
		printf("Failed to get %s\n", attr_name);
		error = CA_ERROR_FAIL;
	}
	else
	{
		printf("%s: ", attr_name);
		switch (pm)
		{
		case PRINT_STRING:
			printf("%s", buffer);
			break;
		case PRINT_U64BE:
			printf("%016llx", GETBE64(buffer));
			break;
		case PRINT_U64LE:
			printf("%016llx", GETLE64(buffer));
			break;
		case PRINT_HEXDUMP:
		default:
			for (uint8_t i = 0; i < attrlen; i++) printf("%02x ", buffer[i]);
			break;
		}
		printf("\n");
		error = CA_ERROR_SUCCESS;
	}
	return error;
}

static ca_error handle_evbme_message(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef)
{
	fprintf(stderr, "IN: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

static void initialise_ca821x(struct ca821x_dev *pDeviceRef)
{
	printf("Initialising.");
	while (ca821x_util_init(pDeviceRef, NULL, NULL))
	{
		sleep(1); //Wait while there isn't a device available to connect
		printf(".");
	}
	//Register callbacks for async messages
	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_MESSAGE_indication = &handle_evbme_message;
	ca821x_util_start_downstream_dispatch_worker();

	printf("\r\nInitialised.\r\n\n");
}

int main(int argc, char *argv[])
{
	struct ca821x_dev *pDeviceRef = &(sDeviceRef);
	ca_error           error      = CA_ERROR_SUCCESS;
	int                retval     = 0;
	uint8_t            buffer[20] = {};
	uint8_t            len        = 0;

	initialise_ca821x(pDeviceRef);

	//Run through EVBME Parameters
	error |= test_evbme_param(EVBME_VERSTRING, PRINT_STRING, false);
	error |= test_evbme_param(EVBME_PLATSTRING, PRINT_STRING, false);
	error |= test_evbme_param(EVBME_APPSTRING, PRINT_STRING, false);
	error |= test_evbme_param(EVBME_SERIALNO, PRINT_U64LE, false);
	error |= test_evbme_param(EVBME_OT_EUI64, PRINT_U64BE, true);
	error |= test_evbme_param(EVBME_OT_JOINCRED, PRINT_STRING, true);

	if (error)
	{
		retval = -1;
		printf("Test Failed\n");
	}
	else
	{
		printf("Test Success\n");
	}

	return retval;
}
