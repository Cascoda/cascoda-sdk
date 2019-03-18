/**
 * @file   wait_test.c
 * @brief  Unit tests for WAIT module
 * @author Ciaran Woodward
 * @date   25/02/19
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
//cmocka must be after system headers
#include <cmocka.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_wait.h"
#include "ca821x_api.h"

static struct ca821x_dev sdev;
void *const              magicNumber = (void *)0x12345;

void __wrap_BSP_Waiting()
{
	TIME_FastForward((u32_t)mock());
}

u8_t __wrap_BSP_SPIPopByte(u8_t *InByte)
{
	*InByte = (u8_t)mock();
	return 1;
}

u8_t __wrap_BSP_SPIExchangeByte(u8_t OutByte)
{
	u8_t InByte;
	(void)OutByte;

	__wrap_BSP_SPIPopByte(&InByte);

	return InByte;
}

static void timeout_test(void **state)
{
	int  status;
	u8_t dummy[1];
	will_return_always(__wrap_BSP_Waiting, 1);

	status = WAIT_Callback(SPI_HWME_WAKEUP_INDICATION, 10, NULL, &sdev);
	assert_int_equal(status, CA_ERROR_SPI_WAIT_TIMEOUT);

	status = WAIT_CallbackSwap(SPI_HWME_WAKEUP_INDICATION, NULL, 10, NULL, &sdev);
	assert_int_equal(status, CA_ERROR_SPI_WAIT_TIMEOUT);

	status = WAIT_Legacy(SPI_HWME_WAKEUP_INDICATION, 10, dummy, &sdev);
	assert_int_equal(status, CA_ERROR_SPI_WAIT_TIMEOUT);
}

static void nullcontext_test(void **state)
{
	assert_null(WAIT_GetContext());
}

static void legacywait_test(void **state)
{
	uint8_t bufin[3] = {0x35, 0x1, HWME_WAKEUP_POWERUP}; //HWME_Wakeup
	uint8_t bufout[3];

	will_return(__wrap_BSP_SPIPopByte, bufin[0]);
	will_return(__wrap_BSP_SPIPopByte, bufin[1]);
	will_return(__wrap_BSP_SPIPopByte, bufin[2]);

	SPI_Exchange(NULL, &sdev);

	WAIT_Legacy(SPI_HWME_WAKEUP_INDICATION, 10, bufout, &sdev);

	assert_memory_equal(bufin, bufout, 3);
}

ca_error wakeup_callback(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	function_called();
	assert_int_equal(params->WakeUpCondition, HWME_WAKEUP_POWERUP);
	assert_ptr_equal(pDeviceRef, &sdev);
	assert_ptr_equal(WAIT_GetContext(), magicNumber);

	//Check that wait functions fail from an awaited callback context
	assert_int_equal(WAIT_Callback(SPI_MCPS_DATA_INDICATION, 10, magicNumber, &sdev), CA_ERROR_INVALID_STATE);
	assert_int_equal(WAIT_CallbackSwap(SPI_MCPS_DATA_INDICATION, NULL, 10, magicNumber, &sdev), CA_ERROR_INVALID_STATE);
	assert_int_equal(WAIT_Legacy(SPI_MCPS_DATA_INDICATION, 10, NULL, &sdev), CA_ERROR_INVALID_STATE);

	return CA_ERROR_SUCCESS;
}

static void waitcallback_test(void **state)
{
	uint8_t bufin[3] = {0x35, 0x1, HWME_WAKEUP_POWERUP}; //HWME_Wakeup
	int     status;

	will_return(__wrap_BSP_SPIPopByte, bufin[0]);
	will_return(__wrap_BSP_SPIPopByte, bufin[1]);
	will_return(__wrap_BSP_SPIPopByte, bufin[2]);
	expect_function_call(wakeup_callback);

	SPI_Exchange(NULL, &sdev);

	sdev.callbacks.HWME_WAKEUP_indication = wakeup_callback;
	status                                = WAIT_Callback(SPI_HWME_WAKEUP_INDICATION, 10, magicNumber, &sdev);
	assert_int_equal(status, MAC_SUCCESS);

	//Check fails for invalid command IDs
	status = WAIT_Callback(SPI_MCPS_DATA_REQUEST, 10, magicNumber, &sdev);
	assert_int_not_equal(status, MAC_SUCCESS);
}

static void waitswap_test(void **state)
{
	uint8_t bufin[3] = {0x35, 0x1, HWME_WAKEUP_POWERUP}; //HWME_Wakeup
	int     status;

	will_return(__wrap_BSP_SPIPopByte, bufin[0]);
	will_return(__wrap_BSP_SPIPopByte, bufin[1]);
	will_return(__wrap_BSP_SPIPopByte, bufin[2]);
	expect_function_call(wakeup_callback);

	SPI_Exchange(NULL, &sdev);

	status = WAIT_CallbackSwap(
	    SPI_HWME_WAKEUP_INDICATION, (ca821x_generic_callback)&wakeup_callback, 10, magicNumber, &sdev);
	assert_int_equal(status, MAC_SUCCESS);

	//Check fails for invalid command IDs
	status =
	    WAIT_CallbackSwap(SPI_MCPS_DATA_REQUEST, (ca821x_generic_callback)&wakeup_callback, 10, magicNumber, &sdev);
	assert_int_not_equal(status, MAC_SUCCESS);
}

ca_error wakeup_callback_nonwaited(struct HWME_WAKEUP_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	//Check that wait functions fail sensibly from a normal callback context
	assert_int_equal(WAIT_Callback(SPI_MCPS_DATA_INDICATION, 10, magicNumber, &sdev), CA_ERROR_INVALID_STATE);
	assert_int_equal(WAIT_CallbackSwap(SPI_MCPS_DATA_INDICATION, NULL, 10, magicNumber, &sdev), CA_ERROR_INVALID_STATE);
	assert_int_equal(WAIT_Legacy(SPI_MCPS_DATA_INDICATION, 10, NULL, &sdev), CA_ERROR_INVALID_STATE);

	return CA_ERROR_SUCCESS;
}

static void waitincallback_fail_test(void **state)
{
	uint8_t bufin[3] = {0x35, 0x1, HWME_WAKEUP_POWERUP}; //HWME_Wakeup

	will_return(__wrap_BSP_SPIPopByte, bufin[0]);
	will_return(__wrap_BSP_SPIPopByte, bufin[1]);
	will_return(__wrap_BSP_SPIPopByte, bufin[2]);

	sdev.callbacks.HWME_WAKEUP_indication = &wakeup_callback_nonwaited;

	assert_int_equal(SPI_Exchange(NULL, &sdev), CA_ERROR_SUCCESS);
	assert_int_equal(EVBME_Dispatch(&sdev), CA_ERROR_SUCCESS);
}

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test(timeout_test),
	                                   cmocka_unit_test(nullcontext_test),
	                                   cmocka_unit_test(legacywait_test),
	                                   cmocka_unit_test(waitcallback_test),
	                                   cmocka_unit_test(waitswap_test),
	                                   cmocka_unit_test(waitincallback_fail_test)};

	ca821x_api_init(&sdev);

	return cmocka_run_group_tests(tests, NULL, NULL);
}
