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

#include "port/oc_assert.h"
#include "port/oc_clock.h"
#include "oc_api.h"
#include "oc_buffer_settings.h"
#include "oc_core_res.h"
#include "oc_uuid.h"
#include "sntp_helper.h"

#include "ocf_application.h"


#define OCF_COMMAND (0xB0)
#define OCF_COMMAND_RFOTM (0x00)
#define OCF_COMMAND_POWER (0x01)
#define OCF_COMMAND_FACTORY (0x02)
#define OCF_COMMAND_GET_UUID (0x03)

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

	// OCF Commands, to be used with the "ocfctl" POSIX application
	if (buf[0] == OCF_COMMAND)
	{
		ret = 1;
		switch (buf[2])
		{
		case OCF_COMMAND_RFOTM:
			oc_reset();
			break;
		case OCF_COMMAND_POWER:
			BSP_SystemReset(SYSRESET_APROM);
			break;
		case OCF_COMMAND_FACTORY:
			otInstanceFactoryReset(OT_INSTANCE);
			break;
		case OCF_COMMAND_GET_UUID:;
			char uuid[38];
			uuid[0] = OCF_COMMAND_GET_UUID;
			oc_uuid_to_str(oc_core_get_device_id(0), uuid + 1, sizeof(uuid) - 1);
			MAC_Message(OCF_COMMAND, sizeof(uuid), uuid);
			break;
		default:
			break;
		}
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
* Add additional device information to oic.wk.p
*/
void set_additional_info(void *data)
{
	(void)data;

	// Get device serial number and convert it to string
	uint64_t serial_no   = BSP_GetUniqueId();
	uint32_t serial_no_1 = (uint32_t)(serial_no >> 32ULL);
	uint32_t serial_no_2 = (uint32_t)serial_no;
	char     serial_no_str[9];
	char     serial_no_str_2[9];
	sprintf(serial_no_str, "%x", serial_no_1);
	sprintf(serial_no_str_2, "%x", serial_no_2);
	strcat(serial_no_str, serial_no_str_2);

	// Get binary version
	const char *binary_version = ca821x_get_version_nodate();

	// Add custom properties to oic.wk.p
	oc_set_custom_platform_property(mnsel, serial_no_str); // Serial number
	oc_set_custom_platform_property(mnfv, binary_version); // Binary version
	oc_set_custom_platform_property(mnpv, CA_TARGET_NAME); // Binary name
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

	// Print the joiner credentials, delaying for up to 5 seconds
	PlatformPrintJoinerCredentials(&dev, OT_INSTANCE, 5000);


	// Try to join network
	do
	{
		otErr = PlatformTryJoin(&dev, OT_INSTANCE);
		if (otErr == OT_ERROR_NONE || otErr == OT_ERROR_ALREADY)
			break;

		BSP_WaitTicks(30000);
	} while (1);

	otThreadSetEnabled(OT_INSTANCE, true);

	DNS_Init(OT_INSTANCE);
	SNTP_Init();

#ifdef OC_RETARGET
	oc_assert(otPlatUartEnable() == OT_ERROR_NONE);
#endif

	otSetStateChangedCallback(OT_INSTANCE, ot_state_changed, NULL);

	PRINT("Used input file : \"../iotivity-lite-lightdevice/out_codegeneration_merged.swagger.json\"\n");
	PRINT("OCF Server name : \"server_lite_53868\"\n");

	initialize_variables();

	/* initializes the handlers structure */
	static const oc_handler_t handler = {.init               = app_init,
	                                     .signal_event_loop  = signal_event_loop,
	                                     .register_resources = register_resources
#ifdef OC_CLIENT
	                                     ,
	                                     .requests_entry = 0
#endif
	};

#ifdef OC_SECURITY
	PRINT("Intialize Secure Resources\n");
	oc_storage_config("./devicebuilderserver_creds");
#endif /* OC_SECURITY */

#ifdef OC_SECURITY
	/* please comment out if the server:
    - has no display capabilities to display the PIN value
    - server does not require to implement RANDOM PIN (oic.sec.doxm.rdp) onboarding mechanism
  */
	//oc_set_random_pin_callback(random_pin_cb, NULL);
#endif /* OC_SECURITY */

	oc_set_factory_presets_cb(factory_presets_cb, NULL);

	/* start the stack */
	init = oc_main_init(&handler);

	oc_set_max_app_data_size(OC_MAX_APP_DATA_SIZE);
	oc_set_mtu_size(1232);

	if (init < 0)
	{
		PRINT("oc_main_init failed %d.\n", init);
	}

	PRINT("OCF server \"server_lite_53868\" running, waiting on incoming connections.\n");

	while (1)
	{
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
		oc_main_poll();
	}

	/* shut down the stack, should not get here */
	oc_main_shutdown();
	return 0;
}

/**
 * @}
 */
