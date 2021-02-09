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
 * @brief  Unit tests for WAIT module
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

//cmocka must be after system headers
#include <cmocka.h>

#include "cascoda-util/cascoda_time.h"

static void time_cmp_test(void **state)
{
	//Simple
	assert_int_equal(TIME_Cmp(1, 1), 0);
	assert_int_equal(TIME_Cmp(1, 0), 1);
	assert_int_equal(TIME_Cmp(0, 1), -1);
	//Wraparound
	assert_int_equal(TIME_Cmp(0xFFFFFFFF, 1), -1);
	assert_int_equal(TIME_Cmp(1, 0xFFFFFFFF), 1);
	//Max range
	assert_int_equal(TIME_Cmp(0x7FFFFFFF, 0), 1);
	assert_int_equal(TIME_Cmp(0, 0x7FFFFFFF), -1);
}

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test_setup(time_cmp_test, NULL)};

	//Any global init here

	return cmocka_run_group_tests(tests, NULL, NULL);
}
