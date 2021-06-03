#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ca821x-posix/ca821x-posix-evbme.h"
#include "ca821x-posix/ca821x-posix.h"
#include "evbme_messages.h"

#define OCF_COMMAND (0xB0)
#define OCF_COMMAND_RFOTM (0x00)
#define OCF_COMMAND_POWER (0x01)
#define OCF_COMMAND_FACTORY (0x02)

static struct ca821x_dev  sDeviceRef;
static struct ca821x_dev *pDeviceRef;

static ca_error handle_evbme_message(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef)
{
	fprintf(stderr, "Rx: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

static void handle_user_commands()
{
	char    in_buf[255];
	uint8_t cmd[1];

	if (fgets(in_buf, sizeof(in_buf), stdin) == NULL)
		exit(0);
	in_buf[strcspn(in_buf, "\n")] = '\0'; // remove trailing newline

	if (strcmp(in_buf, "rfotm") == 0)
	{
		*cmd = OCF_COMMAND_RFOTM;
	}
	else if (strcmp(in_buf, "power") == 0)
	{
		*cmd = OCF_COMMAND_POWER;
	}
	else if (strcmp(in_buf, "factory") == 0)
	{
		*cmd = OCF_COMMAND_FACTORY;
	}
	else if (strcmp(in_buf, "help") == 0)
	{
		printf("Available commands:\n");
		printf("help - Display this message.\n");
		printf("rfotm - Clear ownership data & put device in Ready for Ownership Transfer Method state.\n");
		printf("power - Power cycle the attached device.\n");
		printf("factory - Reset the device to its factory default state.\n");
	}
	else
	{
		printf("Unrecognized command!\n");
		return;
	}

	exchange_user_command(OCF_COMMAND, 1, cmd, pDeviceRef);

	if (*cmd == OCF_COMMAND_POWER || *cmd == OCF_COMMAND_FACTORY)
	{
		sleep(5);
		// innocent EVBME command, to make sure debug messages are sent
		// after a reboot
		uint64_t serialno;
		EVBME_GET_request_sync(EVBME_SERIALNO, sizeof(serialno), (uint8_t *)&serialno, NULL, pDeviceRef);
		(void)serialno;
	}
}

int main()
{
	// Connect to the Chili
	pDeviceRef = &sDeviceRef;

	while (ca821x_util_init(pDeviceRef, NULL))
	{
		return -1;
	}

	// Print debug messages coming from the Chili
	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_MESSAGE_indication = &handle_evbme_message;
	ca821x_util_start_downstream_dispatch_worker();

	// innocent EVBME command, to make sure debug messages are being sent upstream
	uint64_t serialno;
	EVBME_GET_request_sync(EVBME_SERIALNO, sizeof(serialno), (uint8_t *)&serialno, NULL, pDeviceRef);
	(void)serialno;

	// Send commands downstream, to the embedded device
	while (1)
	{
		handle_user_commands();
	}
	return 0;
}
