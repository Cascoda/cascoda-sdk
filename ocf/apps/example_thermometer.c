/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright 2017-2019 Open Connectivity Foundation
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/

/* Application Design
*
* support functions:
* app_init
*  initializes the oic/p and oic/d values.
* register_resources
*  function that registers all endpoints, e.g. sets the RETRIEVE/UPDATE handlers for each end point
*
* main
*  starts the stack, with the registered resources.
*
* Each resource has:
*  global property variables (per resource path) for:
*    the property name
*       naming convention: g_<path>_RESOURCE_PROPERTY_NAME_<propertyname>
*    the actual value of the property, which is typed from the json data type
*      naming convention: g_<path>_<propertyname>
*  global resource variables (per path) for:
*    the path in a variable:
*      naming convention: g_<path>_RESOURCE_ENDPOINT
*    array of interfaces, where by the first will be set as default interface
*      naming convention g_<path>_RESOURCE_INTERFACE
*
*  handlers for the implemented methods (get/post)
*   get_<path>
*     function that is being called when a RETRIEVE is called on <path>
*     set the global variables in the output
*   post_<path>
*     function that is being called when a UPDATE is called on <path>
*     checks the input data
*     if input data is correct
*       updates the global variables
*
*/
/*
 tool_version          : 20171123
 input_file            : ../iotivity-thermometer-device/out_codegeneration_merged.swagger.json
 version of input_file : 20190215
 title of input_file   : cascoda_thermometer
*/

#include <unistd.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <platform.h>

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

#define btoa(x) ((x) ? "true" : "false")

#define MAX_STRING 30         /* max size of the strings. */
#define MAX_PAYLOAD_STRING 65 /* max size strings in the payload */
#define MAX_ARRAY 10          /* max size of the array */
/* Note: Magic numbers are derived from the resource definition, either from the example or the definition.*/

volatile int quit = 0; /* stop variable, used by handle_signal */

/* global property variables for path: "/temperature" */
static char g_temperature_RESOURCE_PROPERTY_NAME_temperature[] = "temperature"; /* the name for the attribute */
double      g_temperature_temperature =
    20.0; /* current value of property "temperature"  The current temperature setting or measurement. */
static char g_temperature_RESOURCE_PROPERTY_NAME_units[] = "units"; /* the name for the attribute */
char        g_temperature_units[MAX_PAYLOAD_STRING]      = "C";
/* current value of property "units" The unit for the conveyed temperature value, Note that when doing an UPDATE, the unit on the device does NOT change, it only indicates the unit of the conveyed value during the UPDATE operation. */ /* registration data variables for the resources */

/* global resource variables for path: /temperature */
static char g_temperature_RESOURCE_ENDPOINT[]              = "/temperature";        /* used path for this resource */
static char g_temperature_RESOURCE_TYPE[][MAX_STRING]      = {"oic.r.temperature"}; /* rt value (as an array) */
int         g_temperature_nr_resource_types                = 1;
static char g_temperature_RESOURCE_INTERFACE[][MAX_STRING] = {"oic.if.a",
                                                              "oic.if.baseline"}; /* interface if (as an array) */
int         g_temperature_nr_resource_interfaces           = 2;

const char *APP_NAME = "OT OCF Thermometer";
otInstance *OT_INSTANCE;
/**
* function to set up the device.
*
*/
static int app_init(void)
{
	int ret = oc_init_platform("ocf", NULL, NULL);
	/* the settings determine the appearance of the device on the network
     can be OCF1.3.1 or OCF2.0.0 (or even higher)
     supplied values are for OCF1.3.1 */
	ret |= oc_add_device("/oic/d",
	                     "x.com.cascoda.thermometer",
	                     "cascoda_thermometer",
	                     "ocf.2.0.5",                   /* icv value */
	                     "ocf.res.1.3.0, ocf.sh.1.3.0", /* dmv value */
	                     NULL,
	                     NULL);
	return ret;
}

/**
* helper function to convert the interface string definition to the constant defintion used by the stack.
* @param interface_name the interface string e.g. "oic.if.a"
* @return the stack constant for the interface
*/
static int convert_if_string(char *interface_name)
{
	if (strcmp(interface_name, "oic.if.baseline") == 0)
		return OC_IF_BASELINE; /* baseline interface */
	if (strcmp(interface_name, "oic.if.rw") == 0)
		return OC_IF_RW; /* read write interface */
	if (strcmp(interface_name, "oic.if.r") == 0)
		return OC_IF_R; /* read interface */
	if (strcmp(interface_name, "oic.if.s") == 0)
		return OC_IF_S; /* sensor interface */
	if (strcmp(interface_name, "oic.if.a") == 0)
		return OC_IF_A; /* actuator interface */
	if (strcmp(interface_name, "oic.if.b") == 0)
		return OC_IF_B; /* batch interface */
	if (strcmp(interface_name, "oic.if.ll") == 0)
		return OC_IF_LL; /* linked list interface */
	return OC_IF_A;
}

/*
* helper function to check if the POST input document contains
* the common readOnly properties or the resouce readOnly properties
* @param name the name of the property
* @return the error_status, e.g. if error_status is true, then the input document contains something illegal
*/

/*
static bool
check_on_readonly_common_resource_properties(oc_string_t name, bool error_state)
{
  if (strcmp ( oc_string(name), "n") == 0) {
    error_state = true;
    PRINT ("   property \"n\" is ReadOnly \n");
  }else if (strcmp ( oc_string(name), "if") == 0) {
    error_state = true;
    PRINT ("   property \"if\" is ReadOnly \n");
  } else if (strcmp ( oc_string(name), "rt") == 0) {
    error_state = true;
    PRINT ("   property \"rt\" is ReadOnly \n");
  } else if (strcmp ( oc_string(name), "id") == 0) {
    error_state = true;
    PRINT ("   property \"id\" is ReadOnly \n");
  } else if (strcmp ( oc_string(name), "id") == 0) {
    error_state = true;
    PRINT ("   property \"id\" is ReadOnly \n");
  }
  return error_state;
}
*/

/**
* get method for "/temperature" resource.
*
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This Resource describes a sensed or actuated Temperature value.
* The Property "temperature" describes the current value measured.
* The Property "units" is a single value that is one of "C", "F" or "K".
* It provides the unit of measurement for the "temperature" value.
* It is a read-only value that is provided by the server.
* If the "units" Property is missing the default is Celsius [C].
* When the Property "range" is omitted the default is +/- MAXINT.
* A client can specify the units for the requested temperature by use of a query parameter.
* If no query parameter is provided the server provides its default measure or set value.
* It is recommended to return always the units Property in the result.
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_temperature(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	g_temperature_temperature = BSP_GetTemperature() / 10.0;

	bool error_state = false;

	/* query name 'units' type: 'string', enum: ['C', 'F', 'K']*/
	char *_units     = NULL; /* not null terminated Units */
	int   _units_len = oc_get_query_value(request, "units", &_units);
	if (_units_len != -1)
	{
		PRINT(" query value 'units': %.*s\n", _units_len, _units);
		bool query_ok = false;

		if (strncmp(_units, "C", _units_len) == 0)
			query_ok = true;
		if (strncmp(_units, "F", _units_len) == 0)
			query_ok = true;
		if (strncmp(_units, "K", _units_len) == 0)
			query_ok = true;
		if (query_ok == false)
			error_state = true;

		/* TODO: use the query value to tailer the response*/
	}

	PRINT("-- Begin get_temperature: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_A:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (number) 'temperature' */
		oc_rep_set_double(root, temperature, g_temperature_temperature);
		PRINT("   %s : %f\n", g_temperature_RESOURCE_PROPERTY_NAME_temperature, g_temperature_temperature);
		/* property (string) 'units' */
		oc_rep_set_text_string(root, units, g_temperature_units);
		PRINT("   %s : %s\n", g_temperature_RESOURCE_PROPERTY_NAME_units, g_temperature_units);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	if (error_state == false)
	{
		oc_send_response(request, OC_STATUS_OK);
	}
	else
	{
		oc_send_response(request, OC_STATUS_BAD_OPTION);
	}
	PRINT("-- End get_temperature\n");
}
/**
* register all the resources to the stack
* this function registers all application level resources:
* - each resource path is bind to a specific function for the supported methods (GET, POST, PUT)
* - each resource is
*   - secure
*   - observable
*   - discoverable
*   - used interfaces (from the global variables).
*/
static void register_resources(void)
{
	PRINT("Register Resource with local path \"/temperature\"\n");
	oc_resource_t *res_temperature =
	    oc_new_resource(NULL, g_temperature_RESOURCE_ENDPOINT, g_temperature_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_temperature_nr_resource_types);
	for (int a = 0; a < g_temperature_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_temperature_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_temperature, g_temperature_RESOURCE_TYPE[a]);
	}
	for (int a = 0; a < g_temperature_nr_resource_interfaces; a++)
	{
		oc_resource_bind_resource_interface(res_temperature, convert_if_string(g_temperature_RESOURCE_INTERFACE[a]));
	}
	oc_resource_set_default_interface(res_temperature, convert_if_string(g_temperature_RESOURCE_INTERFACE[0]));
	PRINT("     Default OCF Interface: \"%s\"\n", g_temperature_RESOURCE_INTERFACE[0]);
	oc_resource_set_discoverable(res_temperature, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	oc_resource_set_periodic_observable(res_temperature, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, perferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_temperature, true); */

	oc_resource_set_request_handler(res_temperature, OC_GET, get_temperature, NULL);
#ifdef OC_SECURITY
	oc_resource_make_public(res_temperature);
#endif
	oc_add_resource(res_temperature);
}

static void signal_event_loop(void)
{
}

/**
* handle Ctrl-C
* @param signal the captured signal
*/
void handle_signal(int signal)
{
	(void)signal;
	signal_event_loop();
	quit = 1;
}

void factory_presets_cb(size_t device, void *data)
{
	(void)device;
	(void)data;
}

/**
* intializes the global variables
* registers and starts the handler

*/
void initialize_variables(void)
{
	/* initialize global variables for resource "/temperature" */
	g_temperature_temperature =
	    20.0; /* current value of property "temperature"  The current temperature setting or measurement. */
	strcpy(
	    g_temperature_units,
	    "C"); /* current value of property "units" The unit for the conveyed temperature value, Note that when doing an UPDATE, the unit on the device does NOT change, it only indicates the unit of the conveyed value during the UPDATE operation. */

	/* set the flag for NO oic/con resource. */
	oc_set_con_res_announced(false);
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
		PRINT("Role: %d\n", otThreadGetDeviceRole(OT_INSTANCE));
	}
}

int main(void)
{
	int               init;
	oc_clock_time_t   next_event;
	u8_t              StartupStatus;
	struct ca821x_dev dev;
	cascoda_serial_dispatch = ot_serial_dispatch;

	ca821x_api_init(&dev);

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(APP_NAME, &dev);
	PlatformRadioInitWithDev(&dev);

	// OpenThread Configuration
	OT_INSTANCE = otInstanceInitSingle();

	otIp6SetEnabled(OT_INSTANCE, true);

	otIp6Address address;
	otIp6AddressFromString("fd00:db8::", &address);
	otIp6SubscribeMulticastAddress(OT_INSTANCE, &address);

	otCliUartInit(OT_INSTANCE);
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

	oc_set_max_app_data_size(2048);
	oc_set_mtu_size(800);

	PRINT("OCF server \"server_lite_53868\" running, waiting on incoming connections.\n");

	while (1)
	{
		cascoda_io_handler(&dev);
		otTaskletsProcess(OT_INSTANCE);
		oc_main_poll();
	}

	/* shut down the stack */
	oc_main_shutdown();
	return 0;
}
