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
/**
 * @file
 * @brief  Unit tests for SETTINGS module
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

//cmocka must be after system headers
#include <cmocka.h>

#include "ca821x-posix/ca821x-types.h"
#include "cascoda-util/cascoda_settings.h"

struct ca821x_dev           device1, device2     = {};
struct ca821x_exchange_base exchange1, exchange2 = {};

// ensure that the multiple storage regions work on POSIX
static void multiple_regions(void **state)
{
	char test_string_1[] =
	    "this is the first test string, which is very long in order to make sure devices don't get mixed up";
	char     test_string_2[] = "second test string";
	char     buffer[128];
	uint16_t len;
	caUtilSettingsInit(&device1, "test_storage", 1);
	caUtilSettingsInit(&device2, "test_storage", 2);

	// add an item to the first storage and read it back
	caUtilSettingsSet(&device1, 0, test_string_1, sizeof(test_string_1));
	len = sizeof(buffer);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_memory_equal(buffer, test_string_1, sizeof(test_string_1));

	// add a different item at the same key in the second storage
	caUtilSettingsSet(&device2, 0, test_string_2, sizeof(test_string_2));
	len = sizeof(buffer);
	caUtilSettingsGet(&device2, 0, 0, buffer, &len);
	assert_string_equal(buffer, test_string_2);

	// make sure the first storage still has the first item
	len = sizeof(buffer);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_memory_equal(buffer, test_string_1, sizeof(test_string_1));

	// make sure the second storage still has the item
	len = sizeof(buffer);
	caUtilSettingsGet(&device2, 0, 0, buffer, &len);
	assert_memory_equal(buffer, test_string_2, sizeof(test_string_2));

	// delete the item in the first storage and ensure you can read
	// the item in the second storage
	caUtilSettingsDelete(&device1, 0, -1);

	assert_int_equal(caUtilSettingsGet(&device1, 0, 0, NULL, NULL), CA_ERROR_NOT_FOUND);

	len = sizeof(buffer);
	caUtilSettingsGet(&device2, 0, 0, buffer, &len);
	assert_string_equal(buffer, test_string_2);

	// clear the storage
	caUtilSettingsWipe(&device1, "test_storage", 1);
	caUtilSettingsWipe(&device2, "test_storage", 2);

	caUtilSettingsDeinit(&device1);
	caUtilSettingsDeinit(&device2);

	// ensure you cannot read the items anymore
	assert_int_equal(caUtilSettingsGet(&device1, 0, 0, NULL, NULL), CA_ERROR_NOT_FOUND);
	assert_int_equal(caUtilSettingsGet(&device2, 0, 0, NULL, NULL), CA_ERROR_NOT_FOUND);
}

// ensure that operations on one byte of data work
static void one_byte(void **state)
{
	char     test_char = 'g';
	char     buffer[1];
	uint16_t len = 1;

	caUtilSettingsInit(&device1, "test_storage", 1);
	caUtilSettingsSet(&device1, 0, &test_char, len);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_int_equal(buffer[0], test_char);

	caUtilSettingsWipe(&device1, "test_storage", 1);
	assert_int_equal(caUtilSettingsGet(&device1, 0, 0, NULL, NULL), CA_ERROR_NOT_FOUND);
	caUtilSettingsDeinit(&device1);
}

// ensure that you get the same data after deinitializing and reinitializing
static void on_off(void **state)
{
	char     test_char = 'g';
	char     buffer[1];
	uint16_t len = 1;

	// add some data, make sure it works
	caUtilSettingsInit(&device1, "test_storage", 1);
	caUtilSettingsSet(&device1, 0, &test_char, len);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_int_equal(buffer[0], test_char);

	// turn it off and on again, check the data's still there
	buffer[0] = '\0';
	caUtilSettingsDeinit(&device1);
	caUtilSettingsInit(&device1, "test_storage", 1);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_int_equal(buffer[0], test_char);

	caUtilSettingsWipe(&device1, "test_storage", 1);
	caUtilSettingsDeinit(&device1);
}

static void freshness(void **state)
{
	const char *test_string = "abcd";
	char        buffer[5];
	uint16_t    len = strlen(test_string);

	caUtilSettingsInit(&device1, "test_storage", 1);

	// store a string
	caUtilSettingsSet(&device1, 0, test_string, len);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_memory_equal(buffer, test_string, len);

	// store a string at the same key, make sure it is overwritten
	test_string = "efgh";
	caUtilSettingsSet(&device1, 0, test_string, len);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_memory_equal(buffer, test_string, len);

	// and again, just in case
	test_string = "ijkl";
	caUtilSettingsSet(&device1, 0, test_string, len);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_memory_equal(buffer, test_string, len);

	// do two different sets, check the second one overwrites the first
	test_string = "mnop";
	caUtilSettingsSet(&device1, 0, test_string, len);
	test_string = "qrst";
	caUtilSettingsSet(&device1, 0, test_string, len);
	caUtilSettingsGet(&device1, 0, 0, buffer, &len);
	assert_memory_equal(buffer, test_string, len);

	caUtilSettingsWipe(&device1, "test_storage", 1);
	caUtilSettingsDeinit(&device1);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test_setup(multiple_regions, NULL),
	    cmocka_unit_test_setup(one_byte, NULL),
	    cmocka_unit_test_setup(on_off, NULL),
	    cmocka_unit_test_setup(freshness, NULL),
	};

	// initialize the device structs
	// we do not use the usual initialization function because
	// we do not want to claim physical chilis in this
	// unit test
	device1.exchange_context = &exchange1;
	device2.exchange_context = &exchange2;

	return cmocka_run_group_tests(tests, NULL, NULL);
}
