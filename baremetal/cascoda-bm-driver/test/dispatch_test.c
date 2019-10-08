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
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_wait.h"
#include "ca821x_api.h"

//Dummy CHILI_FastForward declaration
void CHILI_FastForward(u32_t ticks);

static struct ca821x_dev sdev;

enum TestMsduHandle
{
	TestHandle1 = 0x33,
	TestHandle2 = 0x44
};

void __wrap_BSP_Waiting()
{
	CHILI_FastForward(1);
}

static ca_error handleDataConfirm(struct MCPS_DATA_confirm_pset *params, struct ca821x_dev *pDeviceRef)
{
	assert_ptr_equal(pDeviceRef, &sdev);
	check_expected(params->MsduHandle);
}

static void timeout_mcps_test(void **state)
{
	(void)state;

	LargestIntegralType handleSet[] = {TestHandle1, TestHandle2};
	expect_in_set_count(handleDataConfirm, params->MsduHandle, handleSet, 2);

	struct FullAddr fa;
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

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test(timeout_mcps_test)};

	ca821x_api_init(&sdev);
	sdev.ca821x_api_downstream       = DISPATCH_ToCA821x;
	sdev.callbacks.MCPS_DATA_confirm = handleDataConfirm;

	return cmocka_run_group_tests(tests, NULL, NULL);
}
