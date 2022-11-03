#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/chili_test.h"
#include "cascoda-bm/test15_4_evbme.h"

int TEST15_4_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	(void)SerialRxBuffer;
	return 0;
}

void TEST15_4_Initialise(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
}

void TEST15_4_Handler(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
}

int TEST15_4_SerialDispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	(void)buf;
	(void)len;
	(void)pDeviceRef;
	return 0;
}

int CHILI_TEST_UpStreamDispatch(struct SerialBuffer *SerialRxBuffer, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	(void)SerialRxBuffer;
	return 0;
}

void CHILI_TEST_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
	(void)status;
}

uint8_t CHILI_TEST_IsInTestMode(void)
{
	return false;
}

void CHILI_TEST_Handler(struct ca821x_dev *pDeviceRef)
{
	(void)pDeviceRef;
}
