#include <unistd.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <platform.h>
#include "cascoda-util/cascoda_tasklet.h"

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

#include "ocf_application.h"

otInstance *OT_INSTANCE;

/**
 * @file
 *  Example of sleepy main
 */
/**
 * @ingroup ocf
 * @defgroup ocf-sleepy-main example ocf sleepy main
 * @brief  Example of sleepy main
 *
 * @{
 *
 * ## Application Main
 * This file contains the functionality and main function of the OCF application.
 * main is using the bare metal module to run on the chili.
 *
 * \include sleepy_main.c
 */

/**
 * the signal event loop, stub implemenation for IoTivity
 */
static void signal_event_loop(void)
{
}

/**
 * the signal event loop, stub implemenation for openthread
 */
void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
	(void)aBuf;
	(void)aBufLength;
}

/**
 * the signal event loop, stub implemenation for openthread
 */
void otPlatUartSendDone(void)
{
}

/**
 * Handle application specific commands.
 */
static int ot_serial_dispatch(uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	int ret = 0;

	if (buf[0] == OT_SERIAL_DOWNLINK)
	{
		PlatformUartReceive(buf + 2, buf[1]);
		ret = 1;
	}

	return ret;
}

static void ot_state_changed(uint32_t flags, void *context)
{
	(void)context;

	if (flags & OT_CHANGED_THREAD_ROLE)
	{
		PRINT("Role: %d\n", otThreadGetDeviceRole(OT_INSTANCE));
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Checks current device status and goes to sleep if nothing is happening
 *******************************************************************************
 ******************************************************************************/
static void sleep_if_possible(oc_clock_time_t timeToNextOcfEvent)
{
	uint32_t nextOcf = (uint32_t)timeToNextOcfEvent;

	if (timeToNextOcfEvent > 0x7FFFFFFF || !timeToNextOcfEvent)
		nextOcf = 0x7FFFFFFF;

	if (PlatformCanSleep(OT_INSTANCE))
	{
		uint32_t taskletTimeLeft = nextOcf;

		TASKLET_GetTimeToNext(&taskletTimeLeft);
		if (taskletTimeLeft > nextOcf)
			taskletTimeLeft = nextOcf;

		if (taskletTimeLeft > 100)
		{
			PlatformSleep(taskletTimeLeft);
		}
	}
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
 * the main application.
 * uses functions from the examples.
 */
int main(void)
{
	int               init;
	struct ca821x_dev dev;
	otLinkModeConfig  linkMode = {0};

	cascoda_serial_dispatch = ot_serial_dispatch;

	ca821x_api_init(&dev);

	// Initialisation of Chip and EVBME
	EVBMEInitialise(CA_TARGET_NAME, &dev);
	BSP_RTCInitialise();

	PlatformRadioInitWithDev(&dev);

	// OpenThread Configuration
	OT_INSTANCE = otInstanceInitSingle();

	// Print the joiner credentials, delaying for up to 5 seconds
	PlatformPrintJoinerCredentials(&dev, OT_INSTANCE, 5000);
	// Try to join network
	do
	{
		otError otErr = PlatformTryJoin(&dev, OT_INSTANCE);
		if (otErr == OT_ERROR_NONE || otErr == OT_ERROR_ALREADY)
			break;

		PlatformSleep(30000);
	} while (1);

	otLinkSetPollPeriod(OT_INSTANCE, 500);
	otThreadSetLinkMode(OT_INSTANCE, linkMode);

	otThreadSetEnabled(OT_INSTANCE, true);

	oc_assert(OT_INSTANCE);

#ifdef OC_RETARGET
	oc_assert(otPlatUartEnable() == OT_ERROR_NONE);
#endif

	otSetStateChangedCallback(OT_INSTANCE, ot_state_changed, NULL);

	PRINT("Used input file : \"../iotivity-lite-lightdevice/out_codegeneration_merged.swagger.json\"\n");
	PRINT("OCF Server name : \"server_lite_53868\"\n");

	/*intialize the variables */
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
    - have no display capabilities to display the PIN value
    - server does not require to implement RANDOM PIN (oic.sec.doxm.rdp) onboarding mechanism
  */
	//oc_set_random_pin_callback(random_pin_cb, NULL);
#endif /* OC_SECURITY */

	oc_set_factory_presets_cb(factory_presets_cb, NULL);

	/* start the stack */
	init = oc_main_init(&handler);

	if (init < 0)
	{
		PRINT("oc_main_init failed %d, exiting.\n", init);
		return init;
	}

	oc_set_max_app_data_size(OC_MAX_APP_DATA_SIZE);
	oc_set_mtu_size(800);

	PRINT("OCF server \"server_lite_53868\" running, waiting on incoming connections.\n");

	while (1)
	{
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
		sleep_if_possible(oc_main_poll());
	}

	/* shut down the stack */
	oc_main_shutdown();
	return 0;
}

/**
 * @}
 */
