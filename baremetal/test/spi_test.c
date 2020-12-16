/**
 * @file
 * @brief  Unit tests for SPI module
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
//cmocka must be after system headers
#include <cmocka.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

//Dummy CHILI_FastForward declaration
void CHILI_FastForward(u32_t ticks);

static struct ca821x_dev sdev;

void __wrap_BSP_Waiting()
{
	u32_t ffTime = (u32_t)mock();

	if (ffTime == 0)
	{
		SPI_Exchange(NULL, &sdev);
	}
	else
	{
		CHILI_FastForward(ffTime);
	}
}

u8_t __wrap_BSP_SPIPopByte(u8_t *InByte)
{
	*InByte = (u8_t)mock();
	return 1;
}

static void empty_test(void **state)
{
	(void)state;

	assert_true(SPI_IsFifoEmpty());
	assert_false(SPI_IsFifoFull());
	assert_false(SPI_IsFifoAlmostFull()); /* Fifo should be long enough that it isn't almost full when empty */
	assert_null(SPI_PeekFullBuf());

	//Check dequeueing from an empty fifo doesn't break anything
	SPI_DequeueFullBuf();

	assert_true(SPI_IsFifoEmpty());
	assert_false(SPI_IsFifoFull());
	assert_false(SPI_IsFifoAlmostFull()); /* Fifo should be long enough that it isn't almost full when empty */
	assert_null(SPI_PeekFullBuf());
}

static void sync_fail_test(void **state)
{
	uint8_t len;
	uint8_t leArr[2];
	(void)state;

	//Check that an SPI timeout causes the CAx to be reset and the app reinitialised
	will_return_always(__wrap_BSP_Waiting, 1);
	will_return_always(__wrap_BSP_SPIPopByte, 0xFF);
	assert_int_not_equal(MLME_GET_request_sync(macShortAddress, 0, &len, leArr, &sdev), MAC_SUCCESS);
}

static void sync_success_test(void **state)
{
	uint8_t len;
	uint8_t leArr[2];
	(void)state;

	will_return(__wrap_BSP_Waiting, 0);
	will_return_always(__wrap_BSP_Waiting, 1);
	will_return_count(__wrap_BSP_SPIPopByte, 0xFF, 4); //Idle bytes for request
	will_return(__wrap_BSP_SPIPopByte, 0x68);          //MLME_GET_Confirm for reply
	will_return(__wrap_BSP_SPIPopByte, 6);
	will_return(__wrap_BSP_SPIPopByte, MAC_SUCCESS);
	will_return(__wrap_BSP_SPIPopByte, macShortAddress);
	will_return(__wrap_BSP_SPIPopByte, 0);
	will_return(__wrap_BSP_SPIPopByte, 2);
	will_return(__wrap_BSP_SPIPopByte, 0x12);
	will_return(__wrap_BSP_SPIPopByte, 0x34);

	assert_int_equal(MLME_GET_request_sync(macShortAddress, 0, &len, leArr, &sdev), MAC_SUCCESS);
	assert_int_equal(len, 2);
	assert_int_equal(leArr[0], 0x12);
	assert_int_equal(leArr[1], 0x34);
}

static void async_send_test(void **state)
{
	will_return_always(__wrap_BSP_SPIPopByte, 0xFF);

	struct FullAddr dest = {0};
	uint8_t         msdu = 0;

	assert_int_equal(MCPS_DATA_request(0, dest, 1, &msdu, 0, 0x05, NULL, &sdev), MAC_SUCCESS);
}

static int setup(void **state)
{
	(void)state;

	will_return_maybe(__wrap_BSP_Waiting, 1);

	ca821x_api_init(&sdev);
	SPI_Initialise();

	return 0;
}

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test(empty_test),
	                                   cmocka_unit_test(sync_fail_test),
	                                   cmocka_unit_test(sync_success_test),
	                                   cmocka_unit_test(async_send_test)};

	return cmocka_run_group_tests(tests, setup, NULL);
}
