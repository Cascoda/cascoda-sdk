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
 * @brief  Unit tests for EVBME version comparisons
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

//cmocka must be after system headers
#include <cmocka.h>

#include "ca821x-posix/ca821x-posix-evbme.h"

static void version_compare_test(void **state)
{
	// Simple case
	assert_int_equal(-1, EVBME_CompareVersions("v0.18", "v0.19", NULL, NULL));
	assert_int_equal(1, EVBME_CompareVersions("v0.19", "v0.18", NULL, NULL));
	assert_int_equal(0, EVBME_CompareVersions("v0.18", "v0.18", NULL, NULL));
	assert_int_equal(-1, EVBME_CompareVersions("v0.18", "v1.18", NULL, NULL));
	assert_int_equal(1, EVBME_CompareVersions("v4.18", "v0.18", NULL, NULL));
	// Ignore patch number
	assert_int_equal(0, EVBME_CompareVersions("v0.15-38", "v0.15-68", NULL, NULL));
	// Accept different formats
	assert_int_equal(0, EVBME_CompareVersions("0.15-38", "v0.15-68", NULL, NULL));
	// TODO: Expand these
}

static void version_out_test(void **state)
{
	struct ca_version_number version1, version2;
	assert_int_equal(-1, EVBME_CompareVersions("v0.18-75", "v1.19-87", &version1, &version2));
	assert_int_equal(0, version1.major_version);
	assert_int_equal(18, version1.minor_version);
	assert_int_equal(1, version2.major_version);
	assert_int_equal(19, version2.minor_version);
	//Missing patch number
	assert_int_equal(-1, EVBME_CompareVersions("0.18", "v1.19", &version1, &version2));
	assert_int_equal(0, version1.major_version);
	assert_int_equal(18, version1.minor_version);
	assert_int_equal(1, version2.major_version);
	assert_int_equal(19, version2.minor_version);
}

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test_setup(version_compare_test, NULL),
	                                   cmocka_unit_test_setup(version_out_test, NULL)};

	//Any global init here

	return cmocka_run_group_tests(tests, NULL, NULL);
}
