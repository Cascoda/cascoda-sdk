/**
 * @file
 * @brief  Unit tests for WAIT module
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
//cmocka must be after system headers
#include <cmocka.h>

#include "cascoda-bm/cascoda_dispatch.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_wait.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

//Dummy CHILI_FastForward declaration
void CHILI_FastForward(u32_t ticks);

static struct ca821x_dev sdev;

enum TestMsduHandle
{
	TestHandle1 = 0x11,
	TestHandle2 = 0x22,
	TestHandle3 = 0x33
};

void __wrap_BSP_Waiting()
{
	CHILI_FastForward(1);
}

ca_error __wrap_SPI_Send(const uint8_t *buf, size_t len, u8_t *response, struct ca821x_dev *pDeviceRef)
{
	if (buf[0] == SPI_MCPS_PURGE_REQUEST)
	{
		response[0] = SPI_MCPS_PURGE_CONFIRM; //cmdid
		response[1] = 2;                      //Len
		response[2] = buf[2];                 //MsduHandle
		response[3] = MAC_SUCCESS;            //status
	}
	return CA_ERROR_SUCCESS;
}

static ca_error handleDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	assert_ptr_equal(pDeviceRef, &sdev);
	check_expected(params->MsduHandle);
	return CA_ERROR_SUCCESS;
}

/** Test that direct data confirms actually time out */
static void timeout_mcps_test(void **state)
{
	struct FullAddr fa;
	(void)state;

	expect_value(handleDataConfirm, params->MsduHandle, TestHandle1);
	expect_value(handleDataConfirm, params->MsduHandle, TestHandle2);

	fa.AddressMode = MAC_MODE_SHORT_ADDR;
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, TestHandle1, 0, NULL, &sdev);

	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);

	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, TestHandle2, 0, NULL, &sdev);

	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);
	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);
}

/** Test that indirect data requests are also 'confirmed' when a direct times out */
static void timeout_mcps_indirect_test(void **state)
{
	struct FullAddr fa;
	(void)state;

	expect_value(handleDataConfirm, params->MsduHandle, TestHandle1);
	expect_value(handleDataConfirm, params->MsduHandle, TestHandle2);
	expect_value(handleDataConfirm, params->MsduHandle, TestHandle3);

	fa.AddressMode = MAC_MODE_SHORT_ADDR;
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, TestHandle1, 0, NULL, &sdev);
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, TestHandle2, TXOPT_INDIRECT, NULL, &sdev);

	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);

	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, TestHandle3, 0, NULL, &sdev);

	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);
	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);
}

/** Test that 'purging' an indirect stops it from being 'confirmed' */
static void timeout_mcps_purge_test(void **state)
{
	struct FullAddr fa;
	uint8_t         purgeHandle = TestHandle2;
	(void)state;

	expect_value(handleDataConfirm, params->MsduHandle, TestHandle1);
	expect_value(handleDataConfirm, params->MsduHandle, TestHandle3);

	fa.AddressMode = MAC_MODE_SHORT_ADDR;
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, TestHandle1, 0, NULL, &sdev);
	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, purgeHandle, TXOPT_INDIRECT, NULL, &sdev);
	MCPS_PURGE_request_sync(&purgeHandle, &sdev);

	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);

	MCPS_DATA_request(MAC_MODE_SHORT_ADDR, fa, 8, fa.Address, TestHandle3, 0, NULL, &sdev);

	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);
	CHILI_FastForward(250);
	DISPATCH_FromCA821x(&sdev);
}

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test(timeout_mcps_test),
	                                   cmocka_unit_test(timeout_mcps_indirect_test),
	                                   cmocka_unit_test(timeout_mcps_purge_test)};

	ca821x_api_init(&sdev);
	sdev.callbacks.MCPS_DATA_confirm = handleDataConfirm;

	return cmocka_run_group_tests(tests, NULL, NULL);
}
