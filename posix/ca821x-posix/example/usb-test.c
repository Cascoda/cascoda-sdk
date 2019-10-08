
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix.h"

struct ca821x_dev sDeviceRef;

ca_error handleUserCallback(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	if (buf[0] == 0xA0)
	{
		if (strstr((char *)(buf + 2), "dispatching on SPI") != NULL)
		{
			return CA_ERROR_SUCCESS;
		}

		fprintf(stderr, "IN: %.*s\r\n", (int)(len - 2), buf + 2);
		return CA_ERROR_SUCCESS;
	}
	else if (buf[0] == 0xA2)
	{
		printf("Got Handle [%x]\r\n", buf[2]);
		return CA_ERROR_SUCCESS;
	}
	return CA_ERROR_SUCCESS;
}

struct EVBME_COMM_CHECK_request
{
	uint8_t mHandle;   //!< Handle identifying this comm check
	uint8_t mDelay;    //!< Delay before sending responses
	uint8_t mIndCount; //!< Number of indications to send up
};

int main(int argc, char *argv[])
{
	struct ca821x_dev *             pDeviceRef   = &(sDeviceRef);
	struct EVBME_COMM_CHECK_request CommCheckReq = {};

	printf("Initialising.");
	while (ca821x_util_init(pDeviceRef, NULL))
	{
		sleep(1); //Wait while there isn't a device available to connect
		printf(".");
	}
	//Register callbacks for async messages
	exchange_register_user_callback(&handleUserCallback, pDeviceRef);
	ca821x_util_start_downstream_dispatch_worker();

	MLME_RESET_request_sync(1, pDeviceRef);

	printf("\r\nInitialised.\r\n");

	//Send a comm check
	CommCheckReq.mDelay    = 10;
	CommCheckReq.mIndCount = 10;

	for (uint64_t i = 0; i < atoi(argv[1]); i++)
	{
		CommCheckReq.mHandle = (uint8_t)i;
		exchange_user_command(0xA1, sizeof(CommCheckReq), (uint8_t *)(&CommCheckReq), pDeviceRef);
	}
	while (1)
	{
		sleep(1);
	}

	return 0;
}
