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
 input_file            : ../sensorif-generated-files//out_codegeneration_merged.swagger.json
 version of input_file : 20190808
 title of input_file   : server_lite_61314
*/

#include <signal.h>
#include "cascoda-bm/cascoda_sensorif.h"
#include "port/oc_clock.h"
#include "oc_api.h"

#include "sif_ltr303als.h"
#include "sif_si7021.h"

#ifdef __linux__
/* linux specific code */
#include <pthread.h>
static pthread_mutex_t mutex;
static pthread_cond_t  cv;
static struct timespec ts;
#endif

#ifdef WIN32
/* windows specific code */
#include <windows.h>
static CONDITION_VARIABLE cv; /* event loop variable */
static CRITICAL_SECTION   cs; /* event loop variable */
#endif

#define btoa(x) ((x) ? "true" : "false")

#define MAX_STRING 30         /* max size of the strings. */
#define MAX_PAYLOAD_STRING 65 /* max size strings in the payload */
#define MAX_ARRAY 10          /* max size of the array */
/* Note: Magic numbers are derived from the resource definition, either from the example or the definition.*/

volatile int quit = 0; /* stop variable, used by handle_signal */

/* global property variables for path: "/humidity" */
static char g_humidity_RESOURCE_PROPERTY_NAME_humidity[] = "humidity"; /* the name for the attribute */
int         g_humidity_humidity = 40; /* current value of property "humidity" The current sensed value for humidity. */
/* global property variables for path: "/illuminance" */
static char g_illuminance_RESOURCE_PROPERTY_NAME_illuminance[] = "illuminance"; /* the name for the attribute */
double      g_illuminance_illuminance                          = 450.0;
/* current value of property "illuminance"  The sensed luminous flux per unit area in lux. */ /* registration data variables for the resources */
/* global property variables for path: "/temperature" */
static char g_temperature_RESOURCE_PROPERTY_NAME_temperature[] = "temperature"; /* the name for the attribute */
double      g_temperature_temperature =
    20.0; /* current value of property "temperature"  The current temperature setting or measurement. */
static char g_temperature_RESOURCE_PROPERTY_NAME_units[] = "units"; /* the name for the attribute */
char        g_temperature_units[MAX_PAYLOAD_STRING]      = "C";
/* current value of property "units" The unit for the conveyed temperature value, Note that when doing an UPDATE, the unit on the device does NOT change, it only indicates the unit of the conveyed value during the UPDATE operation. */ /* registration data variables for the resources */

/* global resource variables for path: /humidity */
static char g_humidity_RESOURCE_ENDPOINT[]              = "/humidity";        /* used path for this resource */
static char g_humidity_RESOURCE_TYPE[][MAX_STRING]      = {"oic.r.humidity"}; /* rt value (as an array) */
int         g_humidity_nr_resource_types                = 1;
static char g_humidity_RESOURCE_INTERFACE[][MAX_STRING] = {"oic.if.s",
                                                           "oic.if.baseline"}; /* interface if (as an array) */
int         g_humidity_nr_resource_interfaces           = 2;

/* global resource variables for path: /illuminance */
static char g_illuminance_RESOURCE_ENDPOINT[]              = "/illuminance"; /* used path for this resource */
static char g_illuminance_RESOURCE_TYPE[][MAX_STRING]      = {"oic.r.sensor.illuminance"}; /* rt value (as an array) */
int         g_illuminance_nr_resource_types                = 1;
static char g_illuminance_RESOURCE_INTERFACE[][MAX_STRING] = {"oic.if.s",
                                                              "oic.if.baseline"}; /* interface if (as an array) */
int         g_illuminance_nr_resource_interfaces           = 2;

/* global resource variables for path: /temperature */
static char g_temperature_RESOURCE_ENDPOINT[]              = "/temperature";        /* used path for this resource */
static char g_temperature_RESOURCE_TYPE[][MAX_STRING]      = {"oic.r.temperature"}; /* rt value (as an array) */
int         g_temperature_nr_resource_types                = 1;
static char g_temperature_RESOURCE_INTERFACE[][MAX_STRING] = {"oic.if.s",
                                                              "oic.if.baseline"}; /* interface if (as an array) */
int         g_temperature_nr_resource_interfaces           = 2;
/**
* function to set up the device.
*
*/
int app_init(void)
{
	int ret = oc_init_platform("ocf", NULL, NULL);
	/* the settings determine the appearance of the device on the network
     can be OCF1.3.1 or OCF2.0.0 (or even higher)
     supplied values are for OCF1.3.1 */
	ret |= oc_add_device("/oic/d",
	                     "x.com.cascoda.sensorif",
	                     "sensorif OCF Server",
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

/**
* helper function to check if the POST input document contains
* the common readOnly properties or the resouce readOnly properties
* @param name the name of the property
* @param error_state whether there was previously an error
* @return the error_status, e.g. if error_status is true, then the input document contains something illegal
*/
static bool check_on_readonly_common_resource_properties(oc_string_t name, bool error_state)
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
* get method for "/humidity" resource.
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This Resource describes a sensed or desired humidity.
* The Property "humidity" is an integer describing the percentage measured relative humidity.
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_humidity(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	uint32_t humidity;
	SENSORIF_I2C_Init();
	humidity = SIF_SI7021_ReadHumidity();
	SENSORIF_I2C_Deinit();

	g_humidity_humidity = humidity;
	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
	bool error_state = false;

	PRINT("-- Begin get_humidity: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_S:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (integer) 'humidity' */
		oc_rep_set_int(root, humidity, g_humidity_humidity);
		PRINT("   %s : %d\n", g_humidity_RESOURCE_PROPERTY_NAME_humidity, g_humidity_humidity);
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
	PRINT("-- End get_humidity\n");
}

/**
* get method for "/illuminance" resource.
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This Resource describes an illuminance sensor.
* The Property "illuminance" is a float and represents the sensed luminous flux per unit area in lux.
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_illuminance(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	uint16_t lightLevel0, lightLevel1;

	SENSORIF_I2C_Init();
	SIF_LTR303ALS_ReadLight(&lightLevel0, &lightLevel1);
	SENSORIF_I2C_Deinit();
	g_illuminance_illuminance = lightLevel0;

	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
	bool error_state = false;

	PRINT("-- Begin get_illuminance: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_S:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (number) 'illuminance' */
		oc_rep_set_double(root, illuminance, g_illuminance_illuminance);
		PRINT("   %s : %f\n", g_illuminance_RESOURCE_PROPERTY_NAME_illuminance, g_illuminance_illuminance);
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
	PRINT("-- End get_illuminance\n");
}

/**
* get method for "/temperature" resource.
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
	SENSORIF_I2C_Init();
	g_temperature_temperature = SIF_SI7021_ReadTemperature();
	SENSORIF_I2C_Deinit();

	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
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
	case OC_IF_S:
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
	PRINT("Register Resource with local path \"/humidity\"\n");
	oc_resource_t *res_humidity = oc_new_resource(NULL, g_humidity_RESOURCE_ENDPOINT, g_humidity_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_humidity_nr_resource_types);
	for (int a = 0; a < g_humidity_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_humidity_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_humidity, g_humidity_RESOURCE_TYPE[a]);
	}
	for (int a = 0; a < g_humidity_nr_resource_interfaces; a++)
	{
		oc_resource_bind_resource_interface(res_humidity, convert_if_string(g_humidity_RESOURCE_INTERFACE[a]));
	}
	oc_resource_set_default_interface(res_humidity, convert_if_string(g_humidity_RESOURCE_INTERFACE[0]));
	PRINT("     Default OCF Interface: \"%s\"\n", g_humidity_RESOURCE_INTERFACE[0]);
	oc_resource_set_discoverable(res_humidity, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	oc_resource_set_periodic_observable(res_humidity, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, perferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_humidity, true); */

	oc_resource_set_request_handler(res_humidity, OC_GET, get_humidity, NULL);

	oc_add_resource(res_humidity);

	PRINT("Register Resource with local path \"/illuminance\"\n");
	oc_resource_t *res_illuminance =
	    oc_new_resource(NULL, g_illuminance_RESOURCE_ENDPOINT, g_illuminance_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_illuminance_nr_resource_types);
	for (int a = 0; a < g_illuminance_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_illuminance_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_illuminance, g_illuminance_RESOURCE_TYPE[a]);
	}
	for (int a = 0; a < g_illuminance_nr_resource_interfaces; a++)
	{
		oc_resource_bind_resource_interface(res_illuminance, convert_if_string(g_illuminance_RESOURCE_INTERFACE[a]));
	}
	oc_resource_set_default_interface(res_illuminance, convert_if_string(g_illuminance_RESOURCE_INTERFACE[0]));
	PRINT("     Default OCF Interface: \"%s\"\n", g_illuminance_RESOURCE_INTERFACE[0]);
	oc_resource_set_discoverable(res_illuminance, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	oc_resource_set_periodic_observable(res_illuminance, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, perferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_illuminance, true); */

	oc_resource_set_request_handler(res_illuminance, OC_GET, get_illuminance, NULL);
	oc_add_resource(res_illuminance);

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
		PRINT("ERROR installing manufacturer certificate\n");
	}
	else
	{
		PRINT("Successfully installed manufacturer certificate\n");
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
#endif /* OC_SECURITY && OC_PKI */
}

/**
* intializes the global variables
* registers and starts the handler

*/
void initialize_variables(void)
{
	/* initialize global variables for resource "/humidity" */
	g_humidity_humidity = 40; /* current value of property "humidity" The current sensed value for humidity. */
	/* initialize global variables for resource "/illuminance" */
	g_illuminance_illuminance =
	    450.0; /* current value of property "illuminance"  The sensed luminous flux per unit area in lux. */
	/* initialize global variables for resource "/temperature" */
	g_temperature_temperature =
	    20.0; /* current value of property "temperature"  The current temperature setting or measurement. */
	strcpy(
	    g_temperature_units,
	    "C"); /* current value of property "units" The unit for the conveyed temperature value, Note that when doing an UPDATE, the unit on the device does NOT change, it only indicates the unit of the conveyed value during the UPDATE operation. */

	/* set the flag for NO oic/con resource. */
	oc_set_con_res_announced(false);
}
