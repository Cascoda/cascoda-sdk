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
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
//cmocka must be after system
#include <cmocka.h>

#include "ca821x_api.h"
#include "ca821x_blacklist.h"
#include "mac_messages.h"

int received_data_indication;

// Stubs, since tests are built without a platform layer that handles the actual IO
ca_error ca821x_api_downstream(const uint8_t *buf, uint8_t *response, struct ca821x_dev *pDeviceRef)
{
	return CA_ERROR_SUCCESS;
}
void ca_log(ca_loglevel loglevel, const char *format, va_list argp)
{
	(void)loglevel;
	(void)format;
	(void)argp;
}

ca_error data_indication_handler(struct MCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	(void)params;
	(void)pDeviceRef;
	received_data_indication = 1;
	return CA_ERROR_SUCCESS;
}

static void blacklist_test(void **state)
{
#if CASCODA_MAC_BLACKLIST != 0
	struct ca821x_dev  device;
	struct MacAddr     blacklisted_address = {};
	struct MAC_Message data_indication     = {};

	blacklisted_address.AddressMode = MAC_MODE_SHORT_ADDR;
	received_data_indication        = 0;

	ca821x_api_init(&device);
	device.callbacks.MCPS_DATA_indication = data_indication_handler;
	BLACKLIST_Add(&blacklisted_address, &device);

	// Send a message from a blacklisted device - only the command byte
	// and the source address matter for this test
	data_indication.CommandId                     = SPI_MCPS_DATA_INDICATION;
	data_indication.PData.DataInd.Src.AddressMode = blacklisted_address.AddressMode;
	memcpy(data_indication.PData.DataInd.Src.Address, blacklisted_address.Address, 2);
	ca821x_upstream_dispatch(&data_indication, &device);
	assert_true(received_data_indication == 0);

	// Send a message from a non-blacklisted device and make sure it makes
	// it through and calls the indication handler
	data_indication.PData.DataInd.Src.Address[0] = 0xFF;
	ca821x_upstream_dispatch(&data_indication, &device);
	assert_true(received_data_indication == 1);

	// Clear the blacklist and make sure you can receive the indication
	BLACKLIST_Clear(&device);
	received_data_indication                      = 0;
	data_indication.PData.DataInd.Src.AddressMode = blacklisted_address.AddressMode;
	memcpy(data_indication.PData.DataInd.Src.Address, blacklisted_address.Address, 2);
	ca821x_upstream_dispatch(&data_indication, &device);
	assert_true(received_data_indication == 1);
#endif
}

int main(void)
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test(&blacklist_test),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
