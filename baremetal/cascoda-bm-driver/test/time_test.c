/**
 * @file
 * @brief  Unit tests for TIME module
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
//cmocka must be after system headers
#include <cmocka.h>

#include "cascoda-bm/cascoda_time.h"

//Dummy CHILI_FastForward declaration
void CHILI_FastForward(u32_t ticks);

void __wrap_BSP_Waiting()
{
	CHILI_FastForward((u32_t)mock());
}

static void ff_test(void **state)
{
	u32_t newTime, origTime;
	(void)state;

	//100ms Fastforward
	origTime = TIME_ReadAbsoluteTime();
	CHILI_FastForward(100);
	newTime = TIME_ReadAbsoluteTime();

	assert_int_equal(origTime + 100, newTime);

	//Test 0 Fastforward
	origTime = TIME_ReadAbsoluteTime();
	CHILI_FastForward(0);
	newTime = TIME_ReadAbsoluteTime();

	assert_int_equal(origTime, newTime);
}

static void wait_test(void **state)
{
	//Zero Wait
	TIME_WaitTicks(0);

	//Wait 10ms
	will_return_count(__wrap_BSP_Waiting, 1, 10);
	TIME_WaitTicks(10);

	//Wait 10ms, but overshoot
	will_return(__wrap_BSP_Waiting, 100);
	TIME_WaitTicks(10);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
	    cmocka_unit_test(ff_test),
	    cmocka_unit_test(wait_test),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
