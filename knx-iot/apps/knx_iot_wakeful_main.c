#include <unistd.h>

#include <openthread/diag.h>
#include <openthread/platform/settings.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <platform.h>
#include "cascoda-util/cascoda_tasklet.h"

#include "ca-ot-util/cascoda_dns.h"
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

#include "cascoda-bm/cascoda_wait.h"

#include "knx_iot_wakeful_main_extern.h"

#include "port/oc_assert.h"
#include "port/oc_clock.h"
#include "oc_api.h"
#include "oc_buffer_settings.h"
#include "oc_core_res.h"
#include "oc_uuid.h"
#include "sntp_helper.h"

// Used for knxctl (not yet implemented)
#define KNX_COMMAND (0x00)
#define KNX_COMMAND_RFOTM (0x01)
#define KNX_COMMAND_POWER (0x02)
#define KNX_COMMAND_FACTORY (0x03)

void exit(int e)
{
	(void)e;
	while (1)
	{
		;
	}
}

otInstance *OT_INSTANCE;

// To be implemented by the application
void register_resources(void);
void factory_presets_cb(size_t device_index, void *data);
void reset_cb(size_t device_index, int reset_value, void *data);
void restart_cb(size_t device_index, void *data);
void hostname_cb(size_t device_index, oc_string_t host_name, void *data);
void swu_cb(size_t device_index, size_t offset, uint8_t *payload, size_t len, void *data);
int  app_init(void);

/**
 * @file
 *  Example of wakeful main
 */
/**
 * @ingroup ocf
 * @defgroup ocf-wakeful-main example ocf wakeful main
 * @brief  Example of wakeful main
 *
 * @{
 * 
 * ## Application Main
 * This file contains the functionality and main function of the OCF application.
 * main is using the bare metal module to run on the chili.
 *
 * \include wakeful_main.c
 */

/**
 * Handle application specific commands.
 */
static int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret = 0;

	// KNX Commands, to be used with the "knxctl" POSIX application
	if (buf[0] == KNX_COMMAND)
	{
		switch (buf[2])
		{
		case KNX_COMMAND_RFOTM:
			// TODO how reset in knx???
			// oc_reset();
			break;
		case KNX_COMMAND_POWER:
			BSP_SystemReset(SYSRESET_APROM);
			break;
		case KNX_COMMAND_FACTORY:
			otInstanceFactoryReset(OT_INSTANCE);
			break;
		default:
			break;
		}
	}

	// switch clock otherwise chip is locking up as it loses external clock
	if (((buf[0] == EVBME_SET_REQUEST) && (buf[2] == EVBME_RESETRF)) || (buf[0] == EVBME_HOST_CONNECTED))
	{
		EVBME_SwitchClock(pDeviceRef, 0);
	}
	return ret;
}

static void ot_state_changed(uint32_t flags, void *context)
{
	(void)context;

	if (flags & OT_CHANGED_THREAD_ROLE)
	{
		otDeviceRole role = otThreadGetDeviceRole(OT_INSTANCE);
		PRINT("Role: %d\n", role);

		bool must_update_rtc = (SNTP_GetState() == NO_TIME);
		if ((role != OT_DEVICE_ROLE_DISABLED && role != OT_DEVICE_ROLE_DETACHED) && must_update_rtc)
			SNTP_Update();
	}
}

static void signal_event_loop(void)
{
}

/**
* main application.
* intializes the global variables
* registers and starts the handler
* handles (in a loop) the next event.
* shuts down the stack
*/
int main(void)
{
	int               init;
	oc_clock_time_t   next_event;
	u8_t              StartupStatus;
	struct ca821x_dev dev;
	cascoda_serial_dispatch = ot_serial_dispatch;
	otError otErr           = OT_ERROR_NONE;

	ca821x_api_init(&dev);

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(CA_TARGET_NAME, &dev);
	BSP_RTCInitialise();

	PlatformRadioInitWithDev(&dev);

	// OpenThread Configuration
	OT_INSTANCE = otInstanceInitSingle();

	otIp6SetEnabled(OT_INSTANCE, true);

	oc_assert(OT_INSTANCE);

	// Hardware specific setup
	hardware_init();

	// A backoff mechanism for joining the network
	u32_t joinCooldownTimer = 0;

	// Try to join network
	do
	{
		cascoda_io_handler(&dev);

		// If the timer has expired, try to join the network
		if (joinCooldownTimer == 10)
		{
			printf("Trying to join Thread network...\n");

			// Print the joiner credentials, delaying for up to 1 second
			PlatformPrintJoinerCredentials(&dev, OT_INSTANCE, 0);

			otErr = PlatformTryJoin(&dev, OT_INSTANCE);
			if (otErr == OT_ERROR_NONE || otErr == OT_ERROR_ALREADY)
				break;
			joinCooldownTimer = 0;
		}

		joinCooldownTimer += 1;

		WAIT_ms(50);
	} while (1);

	otThreadSetEnabled(OT_INSTANCE, true);

	DNS_Init(OT_INSTANCE);
	SNTP_Init();

#ifdef OC_RETARGET
	oc_assert(otPlatUartEnable() == OT_ERROR_NONE);
#endif

	otSetStateChangedCallback(OT_INSTANCE, ot_state_changed, NULL);

	/* initializes the handlers structure */
	static const oc_handler_t handler = {.init               = app_init,
	                                     .signal_event_loop  = signal_event_loop,
	                                     .register_resources = register_resources
#ifdef OC_CLIENT
	                                     ,
	                                     .requests_entry = 0
#endif
	};

	oc_storage_config("./knx_iot_creds");

	/* set the application callbacks */
	oc_set_hostname_cb(hostname_cb, NULL);
	oc_set_reset_cb(reset_cb, NULL);
	oc_set_restart_cb(restart_cb, NULL);
	oc_set_factory_presets_cb(factory_presets_cb, NULL);
	oc_set_swu_cb(swu_cb, (void *)"image_name");

	/* start the stack */
	init = oc_main_init(&handler);

	oc_set_max_app_data_size(1024);
	oc_set_mtu_size(1232);

	if (init < 0)
	{
		PRINT("oc_main_init failed %d.\n", init);
	}

	PRINT("KNX IoT device, waiting on incoming connections.\n");

	while (1)
	{
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
		hardware_poll();
		oc_main_poll();
	}

	/* shut down the stack, should not get here */
	oc_main_shutdown();
	return 0;
}

/**
 * @}
 */
