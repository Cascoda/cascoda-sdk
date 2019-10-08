/*
 *  Copyright (c) 2019, Cascoda Ltd.
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

#include "ca821x_endian.h"

/** Test all endian functions to check they exhibit required behaviour
 */
static void endian_test(void **state)
{
	const uint8_t rawin[4] = {0x11, 0x22, 0x33, 0x44};
	uint8_t       rawout[4];

	//First assert that reading works correctly
	assert_int_equal(GETLE16(rawin), 0x2211);
	assert_int_equal(GETLE32(rawin), 0x44332211);
	assert_int_equal(GETBE16(rawin), 0x1122);
	assert_int_equal(GETBE32(rawin), 0x11223344);

	//Then use the already-proven reads to verify the writes
	memset(rawout, 0, sizeof(rawout));
	PUTLE16(0x2211, rawout);
	assert_int_equal(GETLE16(rawout), 0x2211);
	memset(rawout, 0, sizeof(rawout));
	PUTLE32(0x44332211, rawout);
	assert_int_equal(GETLE32(rawout), 0x44332211);
	memset(rawout, 0, sizeof(rawout));
	PUTBE16(0x1122, rawout);
	assert_int_equal(GETBE16(rawout), 0x1122);
	memset(rawout, 0, sizeof(rawout));
	PUTBE32(0x11223344, rawout);
	assert_int_equal(GETBE32(rawout), 0x11223344);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test(&endian_test),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
