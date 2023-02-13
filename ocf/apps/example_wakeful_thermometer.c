/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright 2017-2021 Open Connectivity Foundation
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
#ifndef DOXYGEN
// Force doxygen to document static inline
#define STATIC static
#endif

/**
 * @file
 *  example of OCF light
 */
/**
 * @ingroup ocf
 * @defgroup ocf-examples-light example ocf light 
 * @brief  example of OCF light
 *
 * @{
 *
*
* ## Application Design
*
* support functions:
* - app_init
*   initializes the oic/p and oic/d values.
* - register_resources
*   function that registers all endpoints, e.g. sets the RETRIEVE/UPDATE handlers for each end point
*
* ## main
*
*  starts the stack, with the registered resources.
*  can use either:
*  - sleepy_main
*  - wakeful_main 
*
* as main application.
*
* Each resource has:
* - global property variables (per resource path) for:
*   - the property name
*       naming convention: g_[path]_RESOURCE_PROPERTY_NAME_[propertyname]
*   - the actual value of the property, which is typed from the json data type
*      naming convention: g_[path]_<propertyname>
* - global resource variables (per path) for:
*    the path in a variable:
*      naming convention: g_[path]_RESOURCE_ENDPOINT
*
*  handlers for the implemented methods (get/post):
*  - get_[path]
*     function that is being called when a RETRIEVE is called on [path]
*     set the global variables in the output
*  - post_[path]
*     function that is being called when a UPDATE is called on [path]
*     checks the input data
*     if input data is correct
*       updates the global variables
*
* \include example_light.c
*/
/*
 tool_version          : 20200103
 input_file            : ../device_output/out_codegeneration_merged.swagger.json
 version of input_file : 2019-02-22
 title of input_file   : server_lite_54881
*/

#include <signal.h>
#include "port/oc_clock.h"
#include "oc_api.h"

// ~CascodaPrivStart (This is for autoremoval of this section in the release script)
#if CASCODA_OTA_UPGRADE_ENABLED
#include "ota_swu.h"
#endif
// ~CascodaPrivEnd (This ends the autoremoval section)

#ifdef OC_CLOUD
#include "oc_cloud.h"
#endif
#if defined(OC_IDD_API)
#include "oc_introspection.h"
#endif

#ifdef __linux__
/* linux specific code */
#include <pthread.h>
#ifndef NO_MAIN
static pthread_mutex_t mutex;
static pthread_cond_t  cv;
static struct timespec ts;
#endif /* NO_MAIN */
#endif

#ifdef WIN32
/* windows specific code */
#include <windows.h>
STATIC CONDITION_VARIABLE cv; /**< event loop variable */
STATIC CRITICAL_SECTION   cs; /**< event loop variable */
#endif

#define btoa(x) ((x) ? "true" : "false")

#define MAX_STRING 30         /**< max size of the strings. */
#define MAX_PAYLOAD_STRING 65 /**< max size strings in the payload */
#define MAX_ARRAY 10          /**< max size of the array */
/* Note: Magic numbers are derived from the resource definition, either from the example or the definition.*/

volatile int quit = 0; /**< stop variable, used by handle_signal */

/** Cascoda Additions */
#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <platform.h>

#include "cascoda-bm/cascoda_interface.h"

#include "oc_core_res.h"
#include "oc_ri.h"

otInstance *OT_INSTANCE;
// Pin used by the relay, for the lamp demo
#if CASCODA_OTA_UPGRADE_ENABLED // For_OTA
#define RELAY_OUT_PIN 31        // For_OTA
#else                           // For_OTA
#define RELAY_OUT_PIN 15
#endif // For_OTA

/** End of Cascoda Additions */

/** global property variables for path: "/temperature" */
STATIC char g_temperature_RESOURCE_PROPERTY_NAME_temperature[] = "temperature"; /**< the name for the attribute */
double      g_temperature_temperature =
    20.0; /**< current value of property "temperature"  The current temperature setting or measurement. */
STATIC char g_temperature_RESOURCE_PROPERTY_NAME_units[] = "units"; /**< the name for the attribute */
char        g_temperature_units[MAX_PAYLOAD_STRING]      = "C";
/*<* current value of property "units" The unit for the conveyed temperature value, Note that when doing an UPDATE, the unit on the device does NOT change, it only indicates the unit of the conveyed value during the UPDATE operation. */ /* registration data variables for the resources */

/** global resource variables for path: /temperature */
STATIC char        g_temperature_RESOURCE_ENDPOINT[]    = "/temperature";        /**< used path for this resource */
STATIC const char *g_temperature_RESOURCE_TYPE[]        = {"oic.r.temperature"}; /**< rt value (as an array) */
int                g_temperature_nr_resource_types      = 1;                     /**< amount of resource type entries */
STATIC const char *g_temperature_RESOURCE_INTERFACE[]   = {"oic.if.s",
                                                         "oic.if.baseline"}; /**< interface if (as an array) */
int                g_temperature_nr_resource_interfaces = 2; /**< amount of resource interface entries */

void set_additional_info(void *data);

/**
* function to set up the device.
* 
* sets the:
* - OCF device_type
* - friendly device name
* - OCF version
* - introspection device data
*/
int app_init(void)
{
	int ret = oc_init_platform("Cascoda", set_additional_info, NULL);
	/* the settings determine the appearance of the device on the network
     can be ocf.2.2.3 (or even higher)
     supplied values are for OCF1.3.1 */
	ret |= oc_add_device("/oic/d",
	                     "oic.d.sensor",
	                     "Cascoda Thermometer",
	                     "ocf.2.2.3",                   /* icv value */
	                     "ocf.res.1.3.0, ocf.sh.1.3.0", /* dmv value */
	                     NULL,
	                     NULL);

#if defined(OC_IDD_API)
	FILE      *fp;
	uint8_t   *buffer;
	size_t     buffer_size;
	const char introspection_error[] = "\tERROR Could not read 'server_introspection.cbor'\n"
	                                   "\tIntrospection data not set.\n";
	fp                               = fopen("./server_introspection.cbor", "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		buffer_size = ftell(fp);
		rewind(fp);

		buffer           = (uint8_t *)malloc(buffer_size * sizeof(uint8_t));
		size_t fread_ret = fread(buffer, buffer_size, 1, fp);
		fclose(fp);

		if (fread_ret == 1)
		{
			oc_set_introspection_data(0, buffer, buffer_size);
			PRINT("\tIntrospection data set 'server_introspection.cbor': %d [bytes]\n", (int)buffer_size);
		}
		else
		{
			PRINT("%s", introspection_error);
		}
		free(buffer);
	}
	else
	{
		PRINT("%s", introspection_error);
	}
#else
	PRINT("\t introspection via header file\n");
#endif
	return ret;
}

/**
* helper function to convert the interface string definition to the constant defintion used by the stack.
* @param interface_name the interface string e.g. "oic.if.a"
* @return the stack constant for the interface
*/
STATIC int convert_if_string(char *interface_name)
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

/**
* helper function to check if the POST input document contains
* the common readOnly properties or the resouce readOnly properties
* @param name the name of the property
* @param error_state the current (input) error state
* @return the error_status, e.g. if error_status is true, then the input document contains something illegal
*/
STATIC bool check_on_readonly_common_resource_properties(oc_string_t name, bool error_state)
{
	if (strcmp(oc_string(name), "n") == 0)
	{
		error_state = true;
		PRINT("   property \"n\" is ReadOnly \n");
	}
	else if (strcmp(oc_string(name), "if") == 0)
	{
		error_state = true;
		PRINT("   property \"if\" is ReadOnly \n");
	}
	else if (strcmp(oc_string(name), "rt") == 0)
	{
		error_state = true;
		PRINT("   property \"rt\" is ReadOnly \n");
	}
	else if (strcmp(oc_string(name), "id") == 0)
	{
		error_state = true;
		PRINT("   property \"id\" is ReadOnly \n");
	}
	else if (strcmp(oc_string(name), "id") == 0)
	{
		error_state = true;
		PRINT("   property \"id\" is ReadOnly \n");
	}
	return error_state;
}

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
STATIC void get_temperature(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
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
	case OC_IF_S:
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
void register_resources(void)
{
	PRINT("Register Resource with local path \"/temperature\"\n");
	oc_resource_t *res_temperature = oc_new_resource(
	    "M2351 internal thermometer data", g_temperature_RESOURCE_ENDPOINT, g_temperature_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_temperature_nr_resource_types);
	for (int a = 0; a < g_temperature_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_temperature_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_temperature, g_temperature_RESOURCE_TYPE[a]);
	}
	for (int a = 0; a < g_temperature_nr_resource_interfaces; a++)
	{
		oc_resource_bind_resource_interface(res_temperature,
		                                    convert_if_string((char *)g_temperature_RESOURCE_INTERFACE[a]));
	}
	oc_resource_set_default_interface(res_temperature, convert_if_string((char *)g_temperature_RESOURCE_INTERFACE[0]));
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

#ifdef OC_SECURITY
void random_pin_cb(const unsigned char *pin, size_t pin_len, void *data)
{
	(void)data;
	PRINT("\n====================\n");
	PRINT("Random PIN: %.*s\n", (int)pin_len, pin);
	PRINT("====================\n");
}
#endif /* OC_SECURITY */

void factory_presets_cb(size_t device, void *data)
{
	(void)device;
	(void)data;
#if defined(OC_SECURITY) && defined(OC_PKI)
/* code to include an pki certificate and root trust anchor */
#include "oc_pki.h"
#include "pki_certs.h"
	int credid = oc_pki_add_mfg_cert(
	    0, (const unsigned char *)my_cert, strlen(my_cert), (const unsigned char *)my_key, strlen(my_key));
	if (credid < 0)
	{
		PRINT("ERROR installing PKI certificate\n");
	}
	else
	{
		PRINT("Successfully installed PKI certificate\n");
	}

	if (oc_pki_add_mfg_intermediate_cert(0, credid, (const unsigned char *)int_ca, strlen(int_ca)) < 0)
	{
		PRINT("ERROR installing intermediate CA certificate\n");
	}
	else
	{
		PRINT("Successfully installed intermediate CA certificate\n");
	}

	if (oc_pki_add_mfg_trust_anchor(0, (const unsigned char *)root_ca, strlen(root_ca)) < 0)
	{
		PRINT("ERROR installing root certificate\n");
	}
	else
	{
		PRINT("Successfully installed root certificate\n");
	}

	oc_pki_set_security_profile(0, OC_SP_BLACK, OC_SP_BLACK, credid);
#else
	PRINT("No PKI certificates installed\n");
#endif /* OC_SECURITY && OC_PKI */
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

	/* set the flag for oic/con resource. */
	oc_set_con_res_announced(true);
}

#ifndef NO_MAIN

#ifdef WIN32
/**
* signal the event loop (windows version)
* wakes up the main function to handle the next callback
*/
STATIC void signal_event_loop(void)
{
	WakeConditionVariable(&cv);
}
#endif /* WIN32 */

#ifdef __linux__
/**
* signal the event loop (Linux)
* wakes up the main function to handle the next callback
*/
STATIC void signal_event_loop(void)
{
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cv);
	pthread_mutex_unlock(&mutex);
}
#endif /* __linux__ */

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

#ifdef OC_CLOUD
/**
* cloud status handler.
* handler to print out the status of the cloud connection
*/
STATIC void cloud_status_handler(oc_cloud_context_t *ctx, oc_cloud_status_t status, void *data)
{
	(void)data;
	PRINT("\nCloud Manager Status:\n");
	if (status & OC_CLOUD_REGISTERED)
	{
		PRINT("\t\t-Registered\n");
	}
	if (status & OC_CLOUD_TOKEN_EXPIRY)
	{
		PRINT("\t\t-Token Expiry: ");
		if (ctx)
		{
			PRINT("%d\n", oc_cloud_get_token_expiry(ctx));
		}
		else
		{
			PRINT("\n");
		}
	}
	if (status & OC_CLOUD_FAILURE)
	{
		PRINT("\t\t-Failure\n");
	}
	if (status & OC_CLOUD_LOGGED_IN)
	{
		PRINT("\t\t-Logged In\n");
	}
	if (status & OC_CLOUD_LOGGED_OUT)
	{
		PRINT("\t\t-Logged Out\n");
	}
	if (status & OC_CLOUD_DEREGISTERED)
	{
		PRINT("\t\t-DeRegistered\n");
	}
	if (status & OC_CLOUD_REFRESHED_TOKEN)
	{
		PRINT("\t\t-Refreshed Token\n");
	}
}
#endif // OC_CLOUD

/**
* main application.
* intializes the global variables
* registers and starts the handler
* handles (in a loop) the next event.
* shuts down the stack
*/
int main(void)
{
	int             init;
	oc_clock_time_t next_event;

#ifdef WIN32
	/* windows specific */
	InitializeCriticalSection(&cs);
	InitializeConditionVariable(&cv);
	/* install Ctrl-C */
	signal(SIGINT, handle_signal);
#endif
#ifdef __linux__
	/* linux specific */
	struct sigaction sa;
	sigfillset(&sa.sa_mask);
	sa.sa_flags   = 0;
	sa.sa_handler = handle_signal;
	/* install Ctrl-C */
	sigaction(SIGINT, &sa, NULL);
#endif

	PRINT("Used input file : \"../device_output/out_codegeneration_merged.swagger.json\"\n");
	PRINT("OCF Server name : \"server_lite_54881\"\n");

	/*intialize the variables */
	initialize_variables();

/*
 The storage folder depends on the build system
 for Windows the projects simpleserver and cloud_server are overwritten, hence the folders should be the same as those targets.
 for Linux (as default) the folder is created in the makefile, with $target as name with _cred as post fix.
*/
#ifdef OC_SECURITY
	PRINT("Intialize Secure Resources\n");
#ifdef WIN32
#ifdef OC_CLOUD
	PRINT("\t storage at './cloudserver_creds' \n");
	oc_storage_config("./cloudserver_creds");
#else
	PRINT("\t storage at './simpleserver_creds' \n");
	oc_storage_config("./simpleserver_creds/");
#endif
#else
	PRINT("\t storage at './device_builder_server_creds' \n");
	oc_storage_config("./device_builder_server_creds");
#endif

#endif /* OC_SECURITY */

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
	/* please comment out if the server:
    - have no display capabilities to display the PIN value
    - server does not require to implement RANDOM PIN (oic.sec.doxm.rdp) onboarding mechanism
  */
	oc_set_random_pin_callback(random_pin_cb, NULL);
#endif /* OC_SECURITY */

	oc_set_factory_presets_cb(factory_presets_cb, NULL);

	/* start the stack */
	init = oc_main_init(&handler);

	if (init < 0)
	{
		PRINT("oc_main_init failed %d, exiting.\n", init);
		return init;
	}

#ifdef OC_CLOUD
	/* get the cloud context and start the cloud */
	PRINT("Start Cloud Manager\n");
	oc_cloud_context_t *ctx = oc_cloud_get_context(0);
	if (ctx)
	{
		oc_cloud_manager_start(ctx, cloud_status_handler, NULL);
	}
#endif

	PRINT("OCF server \"server_lite_54881\" running, waiting on incoming connections.\n");

#ifdef WIN32
	/* windows specific loop */
	while (quit != 1)
	{
		next_event = oc_main_poll();
		if (next_event == 0)
		{
			SleepConditionVariableCS(&cv, &cs, INFINITE);
		}
		else
		{
			oc_clock_time_t now = oc_clock_time();
			if (now < next_event)
			{
				SleepConditionVariableCS(&cv, &cs, (DWORD)((next_event - now) * 1000 / OC_CLOCK_SECOND));
			}
		}
	}
#endif

#ifdef __linux__
	/* linux specific loop */
	while (quit != 1)
	{
		next_event = oc_main_poll();
		pthread_mutex_lock(&mutex);
		if (next_event == 0)
		{
			pthread_cond_wait(&cv, &mutex);
		}
		else
		{
			ts.tv_sec  = (next_event / OC_CLOCK_SECOND);
			ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
			pthread_cond_timedwait(&cv, &mutex, &ts);
		}
		pthread_mutex_unlock(&mutex);
	}
#endif

	/* shut down the stack */
#ifdef OC_CLOUD
	PRINT("Stop Cloud Manager\n");
	oc_cloud_manager_stop(ctx);
#endif
	oc_main_shutdown();
	return 0;
}
#endif /* NO_MAIN */

/**
 * @}
 */
