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
 input_file            : ../iotivity-lite-lightdevice/out_codegeneration_merged.swagger.json
 version of input_file : 20190222
 title of input_file   : server_lite_56903
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

otInstance *OT_INSTANCE;

// Pin used by the relay, for the lamp demo
#define RELAY_OUT_PIN 15

/* global property variables for path: "/binaryswitch" */
static char g_binaryswitch_RESOURCE_PROPERTY_NAME_value[] = "value"; /* the name for the attribute */
bool        g_binaryswitch_value = false; /* current value of property "value" The status of the switch. */
/* global property variables for path: "/dimming" */
static char g_dimming_RESOURCE_PROPERTY_NAME_dimmingSetting[] = "dimmingSetting"; /* the name for the attribute */
int         g_dimming_dimmingSetting                          = 30;
/* current value of property "dimmingSetting" The current dimming value. */ /* registration data variables for the resources */

/* global resource variables for path: /binaryswitch */
static char g_binaryswitch_RESOURCE_ENDPOINT[]              = "/binaryswitch";         /* used path for this resource */
static char g_binaryswitch_RESOURCE_TYPE[][MAX_STRING]      = {"oic.r.switch.binary"}; /* rt value (as an array) */
int         g_binaryswitch_nr_resource_types                = 1;
static char g_binaryswitch_RESOURCE_INTERFACE[][MAX_STRING] = {"oic.if.baseline",
                                                               "oic.if.a"}; /* interface if (as an array) */
int         g_binaryswitch_nr_resource_interfaces           = 2;

/* global resource variables for path: /dimming */
static char g_dimming_RESOURCE_ENDPOINT[]              = "/dimming";              /* used path for this resource */
static char g_dimming_RESOURCE_TYPE[][MAX_STRING]      = {"oic.r.light.dimming"}; /* rt value (as an array) */
int         g_dimming_nr_resource_types                = 1;
static char g_dimming_RESOURCE_INTERFACE[][MAX_STRING] = {"oic.if.baseline",
                                                          "oic.if.a"}; /* interface if (as an array) */
int         g_dimming_nr_resource_interfaces           = 2;

// Command for locally controlling the server
static otCliCommand ocfCommand;
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
	                     "oic.d.light",
	                     "server_lite_56903",
	                     "ocf.2.0.5",                   /* icv value */
	                     "ocf.res.1.3.0, ocf.sh.1.3.0", /* dmv value */
	                     NULL,
	                     NULL);
	BSP_ModuleRegisterGPIOOutput(RELAY_OUT_PIN, MODULE_PIN_TYPE_GENERIC);

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
* @param error_state the initial value of the error state
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
* get method for "/binaryswitch" resource.
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This Resource describes a binary switch (on/off).
* The Property "value" is a boolean.
* A value of 'true' means that the switch is on.
* A value of 'false' means that the switch is off.
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_binaryswitch(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
	bool error_state = false;

	PRINT("-- Begin get_binaryswitch: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_A:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (boolean) 'value' */
		oc_rep_set_boolean(root, value, g_binaryswitch_value);
		PRINT("   %s : %s\n", g_binaryswitch_RESOURCE_PROPERTY_NAME_value, btoa(g_binaryswitch_value));
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
	PRINT("-- End get_binaryswitch\n");
}

/**
* get method for "/dimming" resource.
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This Resource describes a dimming function.
* The Property "dimmingSetting" is an integer showing the current dimming level.
* If Property "step" is present then it represents the increment between dimmer values.
* When the Property "range" is omitted, then the range is [0,100].
* A value of 0 means total dimming; a value of 100 means no dimming.
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_dimming(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
	bool error_state = false;

	PRINT("-- Begin get_dimming: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_A:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (integer) 'dimmingSetting' */
		oc_rep_set_int(root, dimmingSetting, g_dimming_dimmingSetting);
		PRINT("   %s : %d\n", g_dimming_RESOURCE_PROPERTY_NAME_dimmingSetting, g_dimming_dimmingSetting);
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
	PRINT("-- End get_dimming\n");
}

/**
* post method for "/binaryswitch" resource.
* The function has as input the request body, which are the input values of the POST method.
* The input values (as a set) are checked if all supplied values are correct.
* If the input values are correct, they will be assigned to the global  property values.
* Resource Description:

*
* @param request the request representation.
* @param interfaces the used interfaces during the request.
* @param user_data the supplied user data.
*/
static void post_binaryswitch(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)interfaces;
	(void)user_data;
	bool error_state = false;
	PRINT("-- Begin post_binaryswitch:\n");
	oc_rep_t *rep = request->request_payload;

	/* loop over the request document for each required input field to check if all required input fields are present */
	bool var_in_request = false;
	rep                 = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_binaryswitch_RESOURCE_PROPERTY_NAME_value) == 0)
		{
			var_in_request = true;
		}
		rep = rep->next;
	}
	if (var_in_request == false)
	{
		error_state = true;
		PRINT(" required property: 'value' not in request\n");
	}
	/* loop over the request document to check if all inputs are ok */
	rep = request->request_payload;
	while (rep != NULL)
	{
		PRINT("key: (check) %s \n", oc_string(rep->name));

		error_state = check_on_readonly_common_resource_properties(rep->name, error_state);
		if (strcmp(oc_string(rep->name), g_binaryswitch_RESOURCE_PROPERTY_NAME_value) == 0)
		{
			/* property "value" of type boolean exist in payload */
			if (rep->type != OC_REP_BOOL)
			{
				error_state = true;
				PRINT("   property 'value' is not of type bool %d \n", rep->type);
			}
		}
		rep = rep->next;
	}
	/* if the input is ok, then process the input document and assign the global variables */
	if (error_state == false)
	{
		/* loop over all the properties in the input document */
		oc_rep_t *rep = request->request_payload;
		while (rep != NULL)
		{
			PRINT("key: (assign) %s \n", oc_string(rep->name));
			/* no error: assign the variables */

			if (strcmp(oc_string(rep->name), g_binaryswitch_RESOURCE_PROPERTY_NAME_value) == 0)
			{
				/* assign "value" */
				PRINT("  property 'value' : %s\n", btoa(rep->value.boolean));
				g_binaryswitch_value = rep->value.boolean;
				BSP_ModuleSetGPIOPin(RELAY_OUT_PIN, g_binaryswitch_value);
			}
			rep = rep->next;
		}
		/* set the response */
		PRINT("Set response \n");
		oc_rep_start_root_object();
		/*oc_process_baseline_interface(request->resource); */
		oc_rep_set_boolean(root, value, g_binaryswitch_value);

		oc_rep_end_root_object();
		/* TODO: ACTUATOR add here the code to talk to the HW if one implements an actuator.
       one can use the global variables as input to those calls
       the global values have been updated already with the data from the request */
		oc_send_response(request, OC_STATUS_CHANGED);
	}
	else
	{
		PRINT("  Returning Error \n");
		/* TODO: add error response, if any */
		//oc_send_response(request, OC_STATUS_NOT_MODIFIED);
		oc_send_response(request, OC_STATUS_BAD_REQUEST);
	}
	PRINT("-- End post_binaryswitch\n");
}

/**
* post method for "/dimming" resource.
* The function has as input the request body, which are the input values of the POST method.
* The input values (as a set) are checked if all supplied values are correct.
* If the input values are correct, they will be assigned to the global  property values.
* Resource Description:
* Sets the desired dimming level.
*
* @param request the request representation.
* @param interfaces the used interfaces during the request.
* @param user_data the supplied user data.
*/
static void post_dimming(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)interfaces;
	(void)user_data;
	bool error_state = false;
	PRINT("-- Begin post_dimming:\n");
	oc_rep_t *rep = request->request_payload;

	/* loop over the request document for each required input field to check if all required input fields are present */
	bool var_in_request = false;
	rep                 = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_dimming_RESOURCE_PROPERTY_NAME_dimmingSetting) == 0)
		{
			var_in_request = true;
		}
		rep = rep->next;
	}
	if (var_in_request == false)
	{
		error_state = true;
		PRINT(" required property: 'dimmingSetting' not in request\n");
	}
	/* loop over the request document to check if all inputs are ok */
	rep = request->request_payload;
	while (rep != NULL)
	{
		PRINT("key: (check) %s \n", oc_string(rep->name));

		error_state = check_on_readonly_common_resource_properties(rep->name, error_state);
		if (strcmp(oc_string(rep->name), g_dimming_RESOURCE_PROPERTY_NAME_dimmingSetting) == 0)
		{
			/* property "dimmingSetting" of type integer exist in payload */
			if (rep->type != OC_REP_INT)
			{
				error_state = true;
				PRINT("   property 'dimmingSetting' is not of type int %d \n", rep->type);
			}
		}
		rep = rep->next;
	}
	/* if the input is ok, then process the input document and assign the global variables */
	if (error_state == false)
	{
		/* loop over all the properties in the input document */
		oc_rep_t *rep = request->request_payload;
		while (rep != NULL)
		{
			PRINT("key: (assign) %s \n", oc_string(rep->name));
			/* no error: assign the variables */

			if (strcmp(oc_string(rep->name), g_dimming_RESOURCE_PROPERTY_NAME_dimmingSetting) == 0)
			{
				/* assign "dimmingSetting" */
				PRINT("  property 'dimmingSetting' : %d\n", rep->value.integer);
				g_dimming_dimmingSetting = rep->value.integer;
			}
			rep = rep->next;
		}
		/* set the response */
		PRINT("Set response \n");
		oc_rep_start_root_object();
		/*oc_process_baseline_interface(request->resource); */
		oc_rep_set_int(root, dimmingSetting, g_dimming_dimmingSetting);

		oc_rep_end_root_object();
		/* TODO: ACTUATOR add here the code to talk to the HW if one implements an actuator.
       one can use the global variables as input to those calls
       the global values have been updated already with the data from the request */
		oc_send_response(request, OC_STATUS_CHANGED);
	}
	else
	{
		PRINT("  Returning Error \n");
		/* TODO: add error response, if any */
		//oc_send_response(request, OC_STATUS_NOT_MODIFIED);
		oc_send_response(request, OC_STATUS_BAD_REQUEST);
	}
	PRINT("-- End post_dimming\n");
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
	PRINT("Register Resource with local path \"/binaryswitch\"\n");
	oc_resource_t *res_binaryswitch =
	    oc_new_resource(NULL, g_binaryswitch_RESOURCE_ENDPOINT, g_binaryswitch_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_binaryswitch_nr_resource_types);
	for (int a = 0; a < g_binaryswitch_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_binaryswitch_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_binaryswitch, g_binaryswitch_RESOURCE_TYPE[a]);
	}
	for (int a = 0; a < g_binaryswitch_nr_resource_interfaces; a++)
	{
		oc_resource_bind_resource_interface(res_binaryswitch, convert_if_string(g_binaryswitch_RESOURCE_INTERFACE[a]));
	}
	oc_resource_set_default_interface(res_binaryswitch, convert_if_string(g_binaryswitch_RESOURCE_INTERFACE[0]));
	PRINT("     Default OCF Interface: \"%s\"\n", g_binaryswitch_RESOURCE_INTERFACE[0]);
	oc_resource_set_discoverable(res_binaryswitch, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	oc_resource_set_periodic_observable(res_binaryswitch, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, perferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_binaryswitch, true); */

	oc_resource_set_request_handler(res_binaryswitch, OC_GET, get_binaryswitch, NULL);

	oc_resource_set_request_handler(res_binaryswitch, OC_POST, post_binaryswitch, NULL);
	oc_add_resource(res_binaryswitch);

	PRINT("Register Resource with local path \"/dimming\"\n");
	oc_resource_t *res_dimming = oc_new_resource(NULL, g_dimming_RESOURCE_ENDPOINT, g_dimming_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_dimming_nr_resource_types);
	for (int a = 0; a < g_dimming_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_dimming_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_dimming, g_dimming_RESOURCE_TYPE[a]);
	}
	for (int a = 0; a < g_dimming_nr_resource_interfaces; a++)
	{
		oc_resource_bind_resource_interface(res_dimming, convert_if_string(g_dimming_RESOURCE_INTERFACE[a]));
	}
	oc_resource_set_default_interface(res_dimming, convert_if_string(g_dimming_RESOURCE_INTERFACE[0]));
	PRINT("     Default OCF Interface: \"%s\"\n", g_dimming_RESOURCE_INTERFACE[0]);
	oc_resource_set_discoverable(res_dimming, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	oc_resource_set_periodic_observable(res_dimming, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, perferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_dimming, true); */

	oc_resource_set_request_handler(res_dimming, OC_GET, get_dimming, NULL);

	oc_resource_set_request_handler(res_dimming, OC_POST, post_dimming, NULL);
	oc_add_resource(res_dimming);
}

static void signal_event_loop(void)
{
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
	/*
#if defined(OC_SECURITY) && defined(OC_PKI)
// code to include an pki certificate and root trust anchor
#include "oc_pki.h"
#include "pki_certs.h"
  int credid =
    oc_pki_add_mfg_cert(0, (const unsigned char *)my_cert, strlen(my_cert), (const unsigned char *)my_key, strlen(my_key));
  if (credid < 0) {
    PRINT("ERROR installing manufacturer certificate\n");
  } else {
    PRINT("Successfully installed manufacturer certificate\n");
  }

  if (oc_pki_add_mfg_intermediate_cert(0, credid, (const unsigned char *)int_ca, strlen(int_ca)) < 0) {
    PRINT("ERROR installing intermediate CA certificate\n");
  } else {
    PRINT("Successfully installed intermediate CA certificate\n");
  }

  if (oc_pki_add_mfg_trust_anchor(0, (const unsigned char *)root_ca, strlen(root_ca)) < 0) {
    PRINT("ERROR installing root certificate\n");
  } else {
    PRINT("Successfully installed root certificate\n");
  }

  oc_pki_set_security_profile(0, OC_SP_BLACK, OC_SP_BLACK, credid);
#endif // OC_SECURITY && OC_PKI
*/
}

/**
* intializes the global variables
* registers and starts the handler

*/
void initialize_variables(void)
{
	/* initialize global variables for resource "/binaryswitch" */ g_binaryswitch_value =
	    false; /* current value of property "value" The status of the switch. */
	/* initialize global variables for resource "/dimming" */
	g_dimming_dimmingSetting = 30; /* current value of property "dimmingSetting" The current dimming value. */

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

void handle_ocf_light_server(int argc, char *argv[])
{
	if (strcmp(argv[0], "set-switch") == 0)
	{
		if (strcmp(argv[1], "0") == 0)
		{
			g_binaryswitch_value = 0;
			BSP_ModuleSetGPIOPin(RELAY_OUT_PIN, g_binaryswitch_value);
		}
		else if (strcmp(argv[1], "1") == 0)
		{
			g_binaryswitch_value = 1;
			BSP_ModuleSetGPIOPin(RELAY_OUT_PIN, g_binaryswitch_value);
		}
		else
		{
			otCliOutputFormat("Invalid argument!\n\r");
			return;
		}
	}
	else if (strcmp(argv[0], "set-dimming") == 0)
	{
		g_dimming_dimmingSetting = atoi(argv[1]);
	}
	else if (strcmp(argv[0], "reset") == 0)
	{
		oc_reset();
	}
	else
	{
		otCliOutputFormat("Invalid argument!\n\r");
	}
	return;
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

	ca821x_api_init(&dev);

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(CA_TARGET_NAME, &dev);
	PlatformRadioInitWithDev(&dev);

	// OpenThread Configuration
	OT_INSTANCE = otInstanceInitSingle();

	otIp6SetEnabled(OT_INSTANCE, true);

	otIp6Address address;
	otIp6AddressFromString("fd00:db8::", &address);
	otIp6SubscribeMulticastAddress(OT_INSTANCE, &address);

	// Enable the OpenThread CLI and add a custom command.
	otCliUartInit(OT_INSTANCE);
	ocfCommand.mCommand = handle_ocf_light_server;
	ocfCommand.mName    = "ocflight";
	otCliSetUserCommands(&ocfCommand, 1);

	if (otDatasetIsCommissioned(OT_INSTANCE))
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
    - has no display capabilities to display the PIN value
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

	oc_set_max_app_data_size(4096);
	oc_set_mtu_size(1210);

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
