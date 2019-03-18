/*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
