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
 * @brief  Unit tests for WAIT module
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

//cmocka must be after system headers
#include <cmocka.h>
#include "cascoda-util/cascoda_tasklet.h"

#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

static uint32_t sTestTime = 0;

void FastForward(uint32_t ticks)
{
	sTestTime += ticks;
}

uint32_t TIME_ReadAbsoluteTime(void)
{
	return sTestTime;
}

static ca_error verify_callback(void *aContext)
{
	uint32_t *checkval = aContext;
	check_expected(*checkval);
	return CA_ERROR_SUCCESS;
}

static ca_error verify_callback2(void *aContext)
{
	uint32_t *checkval = aContext;
	check_expected(*checkval);
	return CA_ERROR_SUCCESS;
}

static ca_error reschedule_callback(void *aContext)
{
	ca_tasklet *task = aContext;
	ca_error    status;

	function_called();
	status = TASKLET_ScheduleDelta(task, 10, task);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(task));
	return CA_ERROR_SUCCESS;
}

static int testSetup(void **state)
{
	//Reset time
	sTestTime = 0;
	return 0;
}

/**Test basic functionality of TASKLET_ScheduleDelta */
static void delta_test(void **state)
{
	ca_error   status;
	uint32_t   contextPtr = 1234;
	ca_tasklet testTasklet;

	//Schedule the tasklet for 10ms in the future
	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	status = TASKLET_ScheduleDelta(&testTasklet, 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));

	//Pass 9 ms, make sure it isn't triggered
	FastForward(9);
	assert_int_equal(TASKLET_Process(), CA_ERROR_NOT_FOUND);

	//Now pass the final millisecond
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(1);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Now try again but tick past the scheduled time, check it still fires
	status = TASKLET_ScheduleDelta(&testTasklet, 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	//Pass 9 ms, make sure it isn't triggered
	FastForward(9);
	assert_int_equal(TASKLET_Process(), CA_ERROR_NOT_FOUND);

	//Now pass the scheduled time
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(10);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Now check we can schedule a tasklet for right now
	status = TASKLET_ScheduleDelta(&testTasklet, 0, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	expect_value(verify_callback, *checkval, contextPtr);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
}

/**Test basic functionality of TASKLET_ScheduleAbs */
static void abs_test(void **state)
{
	ca_error   status;
	uint32_t   contextPtr = 5678;
	ca_tasklet testTasklet;
	uint32_t   absTime = TIME_ReadAbsoluteTime();

	//Schedule the tasklet for 10ms in the future
	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	status = TASKLET_ScheduleAbs(&testTasklet, absTime, absTime + 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));

	//Pass 9 ms, make sure it isn't triggered
	FastForward(9);
	assert_int_equal(TASKLET_Process(), CA_ERROR_NOT_FOUND);

	//Now pass the final millisecond
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(1);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//update time
	absTime = TIME_ReadAbsoluteTime();

	//Now try again but tick past the scheduled time, check it still fires
	status = TASKLET_ScheduleAbs(&testTasklet, absTime, absTime + 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	//Pass 9 ms, make sure it isn't triggered
	FastForward(9);
	assert_int_equal(TASKLET_Process(), CA_ERROR_NOT_FOUND);

	//Now pass the scheduled time
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(10);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Now check we can schedule a tasklet for right now
	absTime = TIME_ReadAbsoluteTime();
	status  = TASKLET_ScheduleAbs(&testTasklet, absTime, absTime, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	expect_value(verify_callback, *checkval, contextPtr);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
}

/**Test double registering failing */
static void double_test(void **state)
{
	ca_error   status;
	uint32_t   contextPtr = 0xABCD;
	uint32_t   absTime    = 0;
	ca_tasklet testTasklet;

	//Schedule the tasklet for 10ms in the future
	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	status = TASKLET_ScheduleDelta(&testTasklet, 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	//Try and schedule again, check it fails
	status = TASKLET_ScheduleDelta(&testTasklet, 20, &contextPtr);
	assert_int_equal(status, CA_ERROR_ALREADY);

	//Now make sure the original still works
	assert_true(TASKLET_IsQueued(&testTasklet));
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(10);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//And check the second one doesn't somehow work
	FastForward(10);
	absTime = TIME_ReadAbsoluteTime();

	//Repeat for absolute
	status = TASKLET_ScheduleAbs(&testTasklet, absTime, 30, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	status = TASKLET_ScheduleAbs(&testTasklet, absTime, 40, &contextPtr);
	assert_int_equal(status, CA_ERROR_ALREADY);
	assert_true(TASKLET_IsQueued(&testTasklet));
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(10);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
	FastForward(10);
}

/** Test that tasklets can be cancelled */
static void cancel_test(void **state)
{
	ca_error   status;
	uint32_t   contextPtr = 0xCA5C;
	ca_tasklet testTasklet;

	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	//Assert not queued
	assert_false(TASKLET_IsQueued(&testTasklet));
	status = TASKLET_Cancel(&testTasklet);
	assert_int_equal(status, CA_ERROR_ALREADY);

	//Schedule the tasklet
	status = TASKLET_ScheduleDelta(&testTasklet, 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));

	//Cancel it, assert removed
	status = TASKLET_Cancel(&testTasklet);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_false(TASKLET_IsQueued(&testTasklet));

	//Check that cancelling again results in the correct error
	status = TASKLET_Cancel(&testTasklet);
	assert_int_equal(status, CA_ERROR_ALREADY);

	//Verify it doesn't get triggered (check_expected() call will fail)
	FastForward(11);

	//Verify it can be correctly scheduled again following a cancellation
	status = TASKLET_ScheduleDelta(&testTasklet, 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));

	//Now pass the scheduled time
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(10);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
}

/** Test that non-scheduled tasklets behave correctly */
static void nonschedule_test(void **state)
{
	ca_error   status;
	uint32_t   timeTest = 0x12345678;
	ca_tasklet testTasklet;

	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	//Assert not queued
	assert_false(TASKLET_IsQueued(&testTasklet));

	//Assert cannot be cancelled
	status = TASKLET_Cancel(&testTasklet);
	assert_int_equal(status, CA_ERROR_ALREADY);

	//Assert time cannot be extracted, and does not change value
	status = TASKLET_GetScheduledTime(&testTasklet, &timeTest);
	assert_int_equal(status, CA_ERROR_INVALID_STATE);
	assert_int_equal(timeTest, 0x12345678);
	//Be paranoid
	timeTest = 0x87654321;
	status   = TASKLET_GetScheduledTime(&testTasklet, &timeTest);
	assert_int_equal(status, CA_ERROR_INVALID_STATE);
	assert_int_equal(timeTest, 0x87654321);

	//Assert global time cannot be extracted, and does not change value
	status = TASKLET_GetTimeToNext(&timeTest);
	assert_int_equal(status, CA_ERROR_NOT_FOUND);
	assert_int_equal(timeTest, 0x87654321);
	//Be paranoid
	timeTest = 0x12345678;
	status   = TASKLET_GetTimeToNext(&timeTest);
	assert_int_equal(status, CA_ERROR_NOT_FOUND);
	assert_int_equal(timeTest, 0x12345678);

	//Assert processing with no tasklets returns NOT_FOUND
	status = TASKLET_Process();
	assert_int_equal(status, CA_ERROR_NOT_FOUND);
}

static void multi_init(ca_tasklet *testTasklet,
                       ca_tasklet *testTasklet2,
                       ca_tasklet *testTasklet3,
                       void *      contextPtr,
                       void *      contextPtr2,
                       void *      contextPtr3)
{
	uint32_t timeAbs, timeDelta;
	ca_error status;

	testSetup(NULL);

	status = TASKLET_Init(testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	status = TASKLET_Init(testTasklet2, &verify_callback2);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	status = TASKLET_Init(testTasklet3, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	//Assert not queued
	assert_false(TASKLET_IsQueued(testTasklet));
	assert_false(TASKLET_IsQueued(testTasklet2));
	assert_false(TASKLET_IsQueued(testTasklet3));

	//Schedule the tasklet
	status = TASKLET_ScheduleAbs(testTasklet2, 0, 10, contextPtr2);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(testTasklet2));

	//Check it is scheduled for the correct time.
	timeAbs   = 0;
	timeDelta = 0;
	status    = TASKLET_GetScheduledTime(testTasklet2, &timeAbs);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeAbs, 10);
	status = TASKLET_GetTimeToNext(&timeDelta);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeDelta, 10);
	status = TASKLET_GetScheduledTimeDelta(testTasklet2, &timeDelta);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeDelta, 10);

	//Schedule another tasklet for beforehand
	status = TASKLET_ScheduleAbs(testTasklet, 0, 5, contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(testTasklet));

	//Check scheduled time is correct
	timeAbs   = 0;
	timeDelta = 0;
	status    = TASKLET_GetScheduledTime(testTasklet, &timeAbs);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeAbs, 5);
	status = TASKLET_GetTimeToNext(&timeDelta);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeDelta, 5);

	//Schedule final tasklet for the end
	status = TASKLET_ScheduleAbs(testTasklet3, 0, 15, contextPtr3);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(testTasklet3));

	//Check scheduled time is correct
	timeAbs   = 0;
	timeDelta = 0;
	status    = TASKLET_GetScheduledTime(testTasklet3, &timeAbs);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeAbs, 15);
	status = TASKLET_GetTimeToNext(&timeDelta);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeDelta, 5);
}

/** Test that multiple tasklets can be scheduled */
static void multi_test(void **state)
{
	ca_error   status;
	uint32_t   contextPtr  = 123;
	uint32_t   contextPtr2 = 456;
	uint32_t   contextPtr3 = 789;
	ca_tasklet testTasklet, testTasklet2, testTasklet3;

	multi_init(&testTasklet, &testTasklet2, &testTasklet3, &contextPtr, &contextPtr2, &contextPtr3);

	//Check the first is triggered
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(5);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Fastforward past both of the others, and make sure they are called in order
	expect_value(verify_callback2, *checkval, contextPtr2);
	expect_value(verify_callback, *checkval, contextPtr3);
	FastForward(20);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
}

/** Test that a single tasklet can be cancelled in a multi tasklet environment */
static void multicancel_test(void **state)
{
	ca_error   status;
	uint32_t   contextPtr  = 123;
	uint32_t   contextPtr2 = 456;
	uint32_t   contextPtr3 = 789;
	ca_tasklet testTasklet, testTasklet2, testTasklet3;

	multi_init(&testTasklet, &testTasklet2, &testTasklet3, &contextPtr, &contextPtr2, &contextPtr3);

	//Check the first is triggered
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(5);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Cancel the middle one
	status = TASKLET_Cancel(&testTasklet2);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_false(TASKLET_IsQueued(&testTasklet2));

	//FastForward past the final, make sure cancelled one isn't called
	expect_value(verify_callback, *checkval, contextPtr3);
	FastForward(20);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Repeat, but cancel the last ------------
	multi_init(&testTasklet, &testTasklet2, &testTasklet3, &contextPtr, &contextPtr2, &contextPtr3);

	//Check the first is triggered
	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(5);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Cancel the last one
	status = TASKLET_Cancel(&testTasklet3);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_false(TASKLET_IsQueued(&testTasklet3));

	//FastForward past the final, make sure cancelled one isn't called
	expect_value(verify_callback2, *checkval, contextPtr2);
	FastForward(20);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Repeat, but cancel the first ------------
	multi_init(&testTasklet, &testTasklet2, &testTasklet3, &contextPtr, &contextPtr2, &contextPtr3);
	status = TASKLET_Cancel(&testTasklet);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_false(TASKLET_IsQueued(&testTasklet));
	expect_value(verify_callback2, *checkval, contextPtr2);
	expect_value(verify_callback, *checkval, contextPtr3);
	FastForward(30);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
}

/** Test that tasklets can not be scheduled too far into future */
static void future_test(void **state)
{
	ca_error   status;
	uint32_t   contextPtr = 4545;
	ca_tasklet testTasklet;

	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	//Test absolute
	status = TASKLET_ScheduleAbs(&testTasklet, 0, 0x80000000, &contextPtr);
	assert_int_equal(status, CA_ERROR_INVALID_ARGS);
	assert_false(TASKLET_IsQueued(&testTasklet));

	//Test delta
	status = TASKLET_ScheduleDelta(&testTasklet, 0x80000000, &contextPtr);
	assert_int_equal(status, CA_ERROR_INVALID_ARGS);
	assert_false(TASKLET_IsQueued(&testTasklet));

	//Pass time
	FastForward(20);

	//Check that the delta still works when it should
	status = TASKLET_ScheduleDelta(&testTasklet, 0x7FFFFFFF, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));

	expect_value(verify_callback, *checkval, contextPtr);
	FastForward(0x7FFFFFFF);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
}

static void reschedule_test(void **state)
{
	ca_error   status;
	ca_tasklet testTasklet;

	status = TASKLET_Init(&testTasklet, &reschedule_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	status = TASKLET_ScheduleDelta(&testTasklet, 10, &testTasklet);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));

	//Check 3 times that it successfully reschedules
	for (int i = 0; i < 3; i++)
	{
		expect_function_call(reschedule_callback);
		FastForward(10);
		assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
		assert_true(TASKLET_IsQueued(&testTasklet));
	}

	//Clean up for next test
	status = TASKLET_Cancel(&testTasklet);
	assert_int_equal(status, CA_ERROR_SUCCESS);
}

static void past_test(void **state)
{
	ca_error   status;
	ca_tasklet testTasklet;
	uint32_t   timeToNext = 0;
	uint32_t   contextPtr = 7878;

	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	status = TASKLET_ScheduleDelta(&testTasklet, 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));

	FastForward(30);

	//Assert that GetTimeToNext notices that an event in the past is due.
	status = TASKLET_GetTimeToNext(&timeToNext);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(timeToNext, 0);

	expect_value(verify_callback, *checkval, contextPtr);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);
}

static void get_scheduled_delta_test(void **state)
{
	ca_error   status;
	ca_tasklet testTasklet;
	uint32_t   contextPtr = 54321;
	uint32_t   timeDelta  = 0;

	status = TASKLET_Init(&testTasklet, &verify_callback);
	assert_int_equal(status, CA_ERROR_SUCCESS);

	status = TASKLET_ScheduleDelta(&testTasklet, 10, &contextPtr);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_true(TASKLET_IsQueued(&testTasklet));
	status = TASKLET_GetScheduledTimeDelta(&testTasklet, &timeDelta);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(10, timeDelta);

	FastForward(5);

	//Assert that delta time is still correct after time passes
	status = TASKLET_GetScheduledTimeDelta(&testTasklet, &timeDelta);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(5, timeDelta);

	FastForward(10);

	//Assert that delta time is zero when tasklet should have happened in the past
	status = TASKLET_GetScheduledTimeDelta(&testTasklet, &timeDelta);
	assert_int_equal(status, CA_ERROR_SUCCESS);
	assert_int_equal(0, timeDelta);

	//Cause tasklet to be triggered
	expect_value(verify_callback, *checkval, contextPtr);
	assert_int_equal(TASKLET_Process(), CA_ERROR_SUCCESS);

	//Assert that delta time is not modified when tasklet is not scheduled, and returns error
	timeDelta = 12345;
	status    = TASKLET_GetScheduledTimeDelta(&testTasklet, &timeDelta);
	assert_int_equal(status, CA_ERROR_INVALID_STATE);
	assert_int_equal(12345, timeDelta);
}

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test_setup(delta_test, testSetup),
	                                   cmocka_unit_test_setup(abs_test, testSetup),
	                                   cmocka_unit_test_setup(double_test, testSetup),
	                                   cmocka_unit_test_setup(cancel_test, testSetup),
	                                   cmocka_unit_test_setup(nonschedule_test, testSetup),
	                                   cmocka_unit_test_setup(multi_test, testSetup),
	                                   cmocka_unit_test_setup(multicancel_test, testSetup),
	                                   cmocka_unit_test_setup(future_test, testSetup),
	                                   cmocka_unit_test_setup(reschedule_test, testSetup),
	                                   cmocka_unit_test_setup(past_test, testSetup),
	                                   cmocka_unit_test_setup(get_scheduled_delta_test, testSetup)};

	//Any global init here

	return cmocka_run_group_tests(tests, NULL, NULL);
}
