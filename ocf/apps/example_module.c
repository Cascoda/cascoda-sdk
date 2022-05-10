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
 tool_version          : 20200103
 input_file            : ../device_output/out_codegeneration_merged.swagger.json
 version of input_file :
 title of input_file   : server_lite_41154
*/

#include <signal.h>
#include "port/oc_clock.h"
#include "oc_api.h"
#include "oc_core_res.h"

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
static CONDITION_VARIABLE cv; /* event loop variable */
static CRITICAL_SECTION   cs; /* event loop variable */
#endif

#define btoa(x) ((x) ? "true" : "false")

#define MAX_STRING 30         /* max size of the strings. */
#define MAX_PAYLOAD_STRING 65 /* max size strings in the payload */
#define MAX_ARRAY 10          /* max size of the array */
/* Note: Magic numbers are derived from the resource definition, either from the example or the definition.*/

volatile int        quit   = 0; /* stop variable, used by handle_signal */
static const size_t DEVICE = 0; /* default device index */

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
/** End of Cascoda Additions */

/* global property variables for path: "/queryurl" */
static char *g_queryurl_RESOURCE_PROPERTY_NAME_queryvalue = "queryvalue"; /* the name for the attribute */
char         g_queryurl_queryvalue[MAX_PAYLOAD_STRING]    = ""; /* current value of property "queryvalue" query param */
static char *g_queryurl_RESOURCE_PROPERTY_NAME_value      = "value"; /* the name for the attribute */
double       g_queryurl_value = 0; /* current value of property "value"  Current setting or measurement */
/* global property variables for path: "/rangeurl" */
static char *g_rangeurl_RESOURCE_PROPERTY_NAME_range = "range"; /* the name for the attribute */

/* array range  The valid range for the Property in the Resource as a number. The first value in the array is the minimum value, the second value in the array is the maximum value. */
double g_rangeurl_range[2];
size_t g_rangeurl_range_array_size;

static char *g_rangeurl_RESOURCE_PROPERTY_NAME_step = "step"; /* the name for the attribute */
double       g_rangeurl_step =
    0; /* current value of property "step"  Step value across the defined range an integer when the range is a number.  This is the increment for valid values across the range; so if range is 0.0..10.0 and step is 2.5 then valid values are 0.0,2.5,5.0,7.5,10.0. */
static char *g_rangeurl_RESOURCE_PROPERTY_NAME_value = "value"; /* the name for the attribute */
double       g_rangeurl_value = 0; /* current value of property "value"  Current setting or measurement */
/* global property variables for path: "/varianturl" */
static char *g_varianturl_RESOURCE_PROPERTY_NAME_valueboolean = "valueboolean"; /* the name for the attribute */
bool         g_varianturl_valueboolean = false; /* current value of property "valueboolean" on/off */
static char *g_varianturl_RESOURCE_PROPERTY_NAME_valueinteger = "valueinteger"; /* the name for the attribute */
int          g_varianturl_valueinteger = 75; /* current value of property "valueinteger" value integer. */
static char *g_varianturl_RESOURCE_PROPERTY_NAME_valuenumber = "valuenumber"; /* the name for the attribute */
double       g_varianturl_valuenumber = 0; /* current value of property "valuenumber"  value number. */
static char *g_varianturl_RESOURCE_PROPERTY_NAME_valuestring = "valuestring"; /* the name for the attribute */
char         g_varianturl_valuestring[100]                   = ""
                                     "";
/* current value of property "valuestring" value string, using a different length than the default lenght. */ /* registration data variables for the resources */

/* global resource variables for path: /queryurl */
static char *g_queryurl_RESOURCE_ENDPOINT         = "/queryurl";          /* used path for this resource */
static char *g_queryurl_RESOURCE_TYPE[MAX_STRING] = {"oic.r.test.query"}; /* rt value (as an array) */
int          g_queryurl_nr_resource_types         = 1;
/* global resource variables for path: /rangeurl */
static char *g_rangeurl_RESOURCE_ENDPOINT         = "/rangeurl";          /* used path for this resource */
static char *g_rangeurl_RESOURCE_TYPE[MAX_STRING] = {"oic.r.test.range"}; /* rt value (as an array) */
int          g_rangeurl_nr_resource_types         = 1;
/* global resource variables for path: /varianturl */
static char *g_varianturl_RESOURCE_ENDPOINT         = "/varianturl";          /* used path for this resource */
static char *g_varianturl_RESOURCE_TYPE[MAX_STRING] = {"oic.r.test.variant"}; /* rt value (as an array) */
int          g_varianturl_nr_resource_types         = 1;

void set_additional_info(void *data);

/**
* function to set up the device.
*
*/
int app_init(void)
{
	int ret = oc_init_platform("Cascoda", set_additional_info, NULL);
	/* the settings determine the appearance of the device on the network
     can be ocf.2.2.0 (or even higher)
     supplied values are for ocf.2.2.2 */
	ret |= oc_add_device("/oic/d",
	                     "oic.d.test.module.actuator",
	                     "Cascoda Actuator Module",
	                     "ocf.2.2.3",                   /* icv value */
	                     "ocf.res.1.3.0, ocf.sh.1.3.0", /* dmv value */
	                     NULL,
	                     NULL);

#if defined(OC_IDD_API)
	FILE *     fp;
	uint8_t *  buffer;
	size_t     buffer_size;
	const char introspection_error[] = "\tERROR Could not read 'server_introspection.cbor'\n"
	                                   "\tIntrospection data not set.\n";
	fp = fopen("./server_introspection.cbor", "rb");
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
* helper function to check if the POST input document contains
* the common readOnly properties or the resouce readOnly properties
* @param name the name of the property
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
* get method for "/queryurl" resource.
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This resource describes a test resource with a optional query parameter. using the queryvalue set the value of the queryvalue property in the resource
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_queryurl(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
	bool error_state    = false;
	int  oc_status_code = OC_STATUS_OK;

	/* query name 'queryvalue' type: 'string', enum: ['A', 'B', 'C']*/
	char *_queryvalue     = NULL; /* not null terminated queryvalue, only using A, B and C */
	int   _queryvalue_len = oc_get_query_value(request, "queryvalue", &_queryvalue);
	char  owned_queryvalue[2];
	if (_queryvalue_len != -1)
	{
		PRINT(" query value 'queryvalue': %.*s\n", _queryvalue_len, _queryvalue);
		bool query_ok = false;
		/* input check ['A', 'B', 'C']  */

		if (strncmp(_queryvalue, "A", _queryvalue_len) == 0)
			query_ok = true;
		if (strncmp(_queryvalue, "B", _queryvalue_len) == 0)
			query_ok = true;
		if (strncmp(_queryvalue, "C", _queryvalue_len) == 0)
			query_ok = true;
		if (query_ok == false)
			error_state = true;
		/* TODO: use the query value to tailer the response*/
		owned_queryvalue[0] = *_queryvalue;
		owned_queryvalue[1] = '\0';
	}
	else
	{
		strcpy(owned_queryvalue, g_queryurl_queryvalue);
	}

	PRINT("-- Begin get_queryurl: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_A:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (string) 'queryvalue' */
		oc_rep_set_text_string(root, queryvalue, owned_queryvalue);
		PRINT("   %s : %s\n", g_queryurl_RESOURCE_PROPERTY_NAME_queryvalue, owned_queryvalue);
		/* property (number) 'value' */
		oc_rep_set_double(root, value, g_queryurl_value);
		PRINT("   %s : %f\n", g_queryurl_RESOURCE_PROPERTY_NAME_value, g_queryurl_value);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	if (error_state == false)
	{
		oc_send_response(request, oc_status_code);
	}
	else
	{
		oc_send_response(request, OC_STATUS_BAD_OPTION);
	}
	PRINT("-- End get_queryurl\n");
}

/**
* get method for "/rangeurl" resource.
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This resource describes a test resource with a required range parameter.
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_rangeurl(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
	bool error_state    = false;
	int  oc_status_code = OC_STATUS_OK;

	PRINT("-- Begin get_rangeurl: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_A:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (array of numbers) 'range' */
		oc_rep_set_array(root, range);
		PRINT("   %s double = [ ", g_rangeurl_RESOURCE_PROPERTY_NAME_range);
		for (int i = 0; i < (int)g_rangeurl_range_array_size; i++)
		{
			oc_rep_add_double(range, g_rangeurl_range[i]);
			PRINT("   %f ", g_rangeurl_range[i]);
		}
		PRINT("   ]\n");
		oc_rep_close_array(root, range);
		/* property (number) 'step' */
		oc_rep_set_double(root, step, g_rangeurl_step);
		PRINT("   %s : %f\n", g_rangeurl_RESOURCE_PROPERTY_NAME_step, g_rangeurl_step);
		/* property (number) 'value' */
		oc_rep_set_double(root, value, g_rangeurl_value);
		PRINT("   %s : %f\n", g_rangeurl_RESOURCE_PROPERTY_NAME_value, g_rangeurl_value);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	if (error_state == false)
	{
		oc_send_response(request, oc_status_code);
	}
	else
	{
		oc_send_response(request, OC_STATUS_BAD_OPTION);
	}
	PRINT("-- End get_rangeurl\n");
}

/**
* get method for "/varianturl" resource.
* function is called to intialize the return values of the GET method.
* initialisation of the returned values are done from the global property values.
* Resource Description:
* This resource defines setting of number, integer boolean and string.
*
* @param request the request representation.
* @param interfaces the interface used for this call
* @param user_data the user data.
*/
static void get_varianturl(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */
	/* TODO: SENSOR add here the code to talk to the HW if one implements a sensor.
     the call to the HW needs to fill in the global variable before it returns to this function here.
     alternative is to have a callback from the hardware that sets the global variables.

     The implementation always return everything that belongs to the resource.
     this implementation is not optimal, but is functionally correct and will pass CTT1.2.2 */
	bool error_state    = false;
	int  oc_status_code = OC_STATUS_OK;

	PRINT("-- Begin get_varianturl: interface %d\n", interfaces);
	oc_rep_start_root_object();
	switch (interfaces)
	{
	case OC_IF_BASELINE:
		/* fall through */
	case OC_IF_A:
		PRINT("   Adding Baseline info\n");
		oc_process_baseline_interface(request->resource);

		/* property (boolean) 'valueboolean' */
		oc_rep_set_boolean(root, valueboolean, g_varianturl_valueboolean);
		PRINT(
		    "   %s : %s\n", g_varianturl_RESOURCE_PROPERTY_NAME_valueboolean, (char *)btoa(g_varianturl_valueboolean));
		/* property (integer) 'valueinteger' */
		oc_rep_set_int(root, valueinteger, g_varianturl_valueinteger);
		PRINT("   %s : %d\n", g_varianturl_RESOURCE_PROPERTY_NAME_valueinteger, g_varianturl_valueinteger);
		/* property (number) 'valuenumber' */
		oc_rep_set_double(root, valuenumber, g_varianturl_valuenumber);
		PRINT("   %s : %f\n", g_varianturl_RESOURCE_PROPERTY_NAME_valuenumber, g_varianturl_valuenumber);
		/* property (string) 'valuestring' */
		oc_rep_set_text_string(root, valuestring, g_varianturl_valuestring);
		PRINT("   %s : %s\n", g_varianturl_RESOURCE_PROPERTY_NAME_valuestring, g_varianturl_valuestring);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	if (error_state == false)
	{
		oc_send_response(request, oc_status_code);
	}
	else
	{
		oc_send_response(request, OC_STATUS_BAD_OPTION);
	}
	PRINT("-- End get_varianturl\n");
}

/**
* post method for "/queryurl" resource.
* The function has as input the request body, which are the input values of the POST method.
* The input values (as a set) are checked if all supplied values are correct.
* If the input values are correct, they will be assigned to the global  property values.
* Resource Description:
* Sets the desired value.
* If a queryvalue is included and the server does not support the queryvalue indicated the request will fail.
* If the queryvalue are omitted the value is taken to be 'C'.
*
* @param request the request representation.
* @param interfaces the used interfaces during the request.
* @param user_data the supplied user data.
*/
static void post_queryurl(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)interfaces;
	(void)user_data;
	bool error_state = false;
	PRINT("-- Begin post_queryurl:\n");
	oc_rep_t *rep = request->request_payload;

	/* query name 'queryvalue' type: 'string', enum: ['A', 'B', 'C']*/
	char *_queryvalue     = NULL; /* not null terminated queryvalue, only using A, B and C */
	int   _queryvalue_len = oc_get_query_value(request, "queryvalue", &_queryvalue);
	if (_queryvalue_len != -1)
	{
		bool query_ok = false;

		if (strncmp(_queryvalue, "A", _queryvalue_len) == 0)
			query_ok = true;
		if (strncmp(_queryvalue, "B", _queryvalue_len) == 0)
			query_ok = true;
		if (strncmp(_queryvalue, "C", _queryvalue_len) == 0)
			query_ok = true;
		if (query_ok == false)
			error_state = true;
		PRINT(" query value 'queryvalue': %.*s\n", _queryvalue_len, _queryvalue);
		/* TODO: use the query value to tailer the response*/
		if (!error_state)
		{
			strcpy(g_queryurl_queryvalue, _queryvalue);
		}
	}
	/* loop over the request document for each required input field to check if all required input fields are present */
	bool var_in_request = false;
	rep                 = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_queryurl_RESOURCE_PROPERTY_NAME_value) == 0)
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
		if (strcmp(oc_string(rep->name), g_queryurl_RESOURCE_PROPERTY_NAME_queryvalue) == 0)
		{
			/* property "queryvalue" of type string exist in payload */
			if (rep->type != OC_REP_STRING)
			{
				error_state = true;
				PRINT("   property 'queryvalue' is not of type string %d \n", rep->type);
			}
			if (strlen(oc_string(rep->value.string)) >= (MAX_PAYLOAD_STRING - 1))
			{
				error_state = true;
				PRINT("   property 'queryvalue' is too long %d expected: MAX_PAYLOAD_STRING-1 \n",
				      (int)strlen(oc_string(rep->value.string)));
			}
		}
		if (strcmp(oc_string(rep->name), g_queryurl_RESOURCE_PROPERTY_NAME_value) == 0)
		{
			/* property "value" of type double exist in payload */
			if ((rep->type != OC_REP_DOUBLE) & (rep->type != OC_REP_INT))
			{
				error_state = true;
				PRINT("   property 'value' is not of type double or int %d \n", rep->type);
			}
		}
		rep = rep->next;
	}
	/* if the input is ok, then process the input document and assign the global variables */
	if (error_state == false)
	{
		switch (interfaces)
		{
		default:
		{
			/* loop over all the properties in the input document */
			oc_rep_t *rep = request->request_payload;
			while (rep != NULL)
			{
				PRINT("key: (assign) %s \n", oc_string(rep->name));
				/* no error: assign the variables */

				if (strcmp(oc_string(rep->name), g_queryurl_RESOURCE_PROPERTY_NAME_queryvalue) == 0)
				{
					/* assign "queryvalue" */
					PRINT("  property 'queryvalue' : %s\n", oc_string(rep->value.string));
					strncpy(g_queryurl_queryvalue, oc_string(rep->value.string), MAX_PAYLOAD_STRING - 1);
				}
				if (strcmp(oc_string(rep->name), g_queryurl_RESOURCE_PROPERTY_NAME_value) == 0)
				{
					/* assign "value" */
					PRINT("  property 'value' : %f\n", rep->value.double_p);
					g_queryurl_value = rep->value.double_p;
				}
				rep = rep->next;
			}
			/* set the response */
			PRINT("Set response \n");
			oc_rep_start_root_object();
			/*oc_process_baseline_interface(request->resource); */
			PRINT("   %s : %s\n", g_queryurl_RESOURCE_PROPERTY_NAME_queryvalue, g_queryurl_queryvalue);
			oc_rep_set_text_string(root, queryvalue, g_queryurl_queryvalue);
			PRINT("   %s : %f\n", g_queryurl_RESOURCE_PROPERTY_NAME_value, g_queryurl_value);
			oc_rep_set_double(root, value, g_queryurl_value);

			oc_rep_end_root_object();
			/* TODO: ACTUATOR add here the code to talk to the HW if one implements an actuator.
       one can use the global variables as input to those calls
       the global values have been updated already with the data from the request */
			oc_send_response(request, OC_STATUS_CHANGED);
		}
		}
	}
	else
	{
		PRINT("  Returning Error \n");
		/* TODO: add error response, if any */
		//oc_send_response(request, OC_STATUS_NOT_MODIFIED);
		oc_send_response(request, OC_STATUS_BAD_REQUEST);
	}
	PRINT("-- End post_queryurl\n");
}

/**
* post method for "/rangeurl" resource.
* The function has as input the request body, which are the input values of the POST method.
* The input values (as a set) are checked if all supplied values are correct.
* If the input values are correct, they will be assigned to the global  property values.
* Resource Description:
* Sets the desired value.
* If a unit is included and the server does not support the unit indicated the request will fail.
* If the units are omitted value is taken to be in C.
*
* @param request the request representation.
* @param interfaces the used interfaces during the request.
* @param user_data the supplied user data.
*/
static void post_rangeurl(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)interfaces;
	(void)user_data;
	bool error_state = false;
	PRINT("-- Begin post_rangeurl:\n");
	oc_rep_t *rep = request->request_payload;

	/* loop over the request document for each required input field to check if all required input fields are present */
	bool var_in_request = false;
	rep                 = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_rangeurl_RESOURCE_PROPERTY_NAME_value) == 0)
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
		if (strcmp(oc_string(rep->name), g_rangeurl_RESOURCE_PROPERTY_NAME_value) == 0)
		{
			/* property "value" of type double exist in payload */
			if ((rep->type != OC_REP_DOUBLE) & (rep->type != OC_REP_INT))
			{
				error_state = true;
				PRINT("   property 'value' is not of type double or int %d \n", rep->type);
			}
		}
		rep = rep->next;
	}
	/* if the input is ok, then process the input document and assign the global variables */
	if (error_state == false)
	{
		switch (interfaces)
		{
		default:
		{
			/* loop over all the properties in the input document */
			oc_rep_t *rep = request->request_payload;
			while (rep != NULL)
			{
				PRINT("key: (assign) %s \n", oc_string(rep->name));
				/* no error: assign the variables */

				if (strcmp(oc_string(rep->name), g_rangeurl_RESOURCE_PROPERTY_NAME_value) == 0)
				{
					/* assign "value" */
					PRINT("  property 'value' : %f\n", rep->value.double_p);
					g_rangeurl_value = rep->value.double_p;
				}
				rep = rep->next;
			}
			/* set the response */
			PRINT("Set response \n");
			oc_rep_start_root_object();
			/*oc_process_baseline_interface(request->resource); */

			oc_rep_set_array(root, range);
			for (int i = 0; i < (int)g_rangeurl_range_array_size; i++)
			{
				oc_rep_add_double(range, g_rangeurl_range[i]);
			}
			oc_rep_close_array(root, range);

			PRINT("   %s : %f\n", g_rangeurl_RESOURCE_PROPERTY_NAME_step, g_rangeurl_step);
			oc_rep_set_double(root, step, g_rangeurl_step);
			PRINT("   %s : %f\n", g_rangeurl_RESOURCE_PROPERTY_NAME_value, g_rangeurl_value);
			oc_rep_set_double(root, value, g_rangeurl_value);

			oc_rep_end_root_object();
			/* TODO: ACTUATOR add here the code to talk to the HW if one implements an actuator.
       one can use the global variables as input to those calls
       the global values have been updated already with the data from the request */
			oc_send_response(request, OC_STATUS_CHANGED);
		}
		}
	}
	else
	{
		PRINT("  Returning Error \n");
		/* TODO: add error response, if any */
		//oc_send_response(request, OC_STATUS_NOT_MODIFIED);
		oc_send_response(request, OC_STATUS_BAD_REQUEST);
	}
	PRINT("-- End post_rangeurl\n");
}

/**
* post method for "/varianturl" resource.
* The function has as input the request body, which are the input values of the POST method.
* The input values (as a set) are checked if all supplied values are correct.
* If the input values are correct, they will be assigned to the global  property values.
* Resource Description:

*
* @param request the request representation.
* @param interfaces the used interfaces during the request.
* @param user_data the supplied user data.
*/
static void post_varianturl(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)interfaces;
	(void)user_data;
	bool error_state = false;
	PRINT("-- Begin post_varianturl:\n");
	oc_rep_t *rep = request->request_payload;

	/* loop over the request document for each required input field to check if all required input fields are present */
	bool var_in_request = false;
	rep                 = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valuestring) == 0)
		{
			var_in_request = true;
		}
		rep = rep->next;
	}
	if (var_in_request == false)
	{
		error_state = true;
		PRINT(" required property: 'valuestring' not in request\n");
	}
	var_in_request = false;
	rep            = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valueboolean) == 0)
		{
			var_in_request = true;
		}
		rep = rep->next;
	}
	if (var_in_request == false)
	{
		error_state = true;
		PRINT(" required property: 'valueboolean' not in request\n");
	}
	var_in_request = false;
	rep            = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valueinteger) == 0)
		{
			var_in_request = true;
		}
		rep = rep->next;
	}
	if (var_in_request == false)
	{
		error_state = true;
		PRINT(" required property: 'valueinteger' not in request\n");
	}
	var_in_request = false;
	rep            = request->request_payload;
	while (rep != NULL)
	{
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valuenumber) == 0)
		{
			var_in_request = true;
		}
		rep = rep->next;
	}
	if (var_in_request == false)
	{
		error_state = true;
		PRINT(" required property: 'valuenumber' not in request\n");
	}
	/* loop over the request document to check if all inputs are ok */
	rep = request->request_payload;
	while (rep != NULL)
	{
		PRINT("key: (check) %s \n", oc_string(rep->name));

		error_state = check_on_readonly_common_resource_properties(rep->name, error_state);
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valueboolean) == 0)
		{
			/* property "valueboolean" of type boolean exist in payload */
			if (rep->type != OC_REP_BOOL)
			{
				error_state = true;
				PRINT("   property 'valueboolean' is not of type bool %d \n", rep->type);
			}
		}
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valueinteger) == 0)
		{
			/* property "valueinteger" of type integer exist in payload */
			if (rep->type != OC_REP_INT)
			{
				error_state = true;
				PRINT("   property 'valueinteger' is not of type int %d \n", rep->type);
			}

			int value_max = (int)rep->value.integer;
			if (value_max > 100)
			{
				/* check the maximum range */
				PRINT("   property 'valueinteger' value exceed max : 0 >  value: %d \n", value_max);
				error_state = true;
			}
		}
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valuenumber) == 0)
		{
			/* property "valuenumber" of type double exist in payload */
			if ((rep->type != OC_REP_DOUBLE) & (rep->type != OC_REP_INT))
			{
				error_state = true;
				PRINT("   property 'valuenumber' is not of type double or int %d \n", rep->type);
			}
			double value_max_check = 0;
			if (rep->type == OC_REP_DOUBLE)
				value_max_check = rep->value.double_p;
			if (rep->type == OC_REP_INT)
				value_max_check = (double)rep->value.integer;
			if (value_max_check > 100.0)
			{
				/* check the maximum range */
				PRINT("   property 'valuenumber' value exceed max : 0.0 >  value: %f \n", value_max_check);
				error_state = true;
			}
		}
		if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valuestring) == 0)
		{
			/* property "valuestring" of type string exist in payload */
			if (rep->type != OC_REP_STRING)
			{
				error_state = true;
				PRINT("   property 'valuestring' is not of type string %d \n", rep->type);
			}
			if (strlen(oc_string(rep->value.string)) >= (100 - 1))
			{
				error_state = true;
				PRINT("   property 'valuestring' is too long %d expected: 100-1 \n",
				      (int)strlen(oc_string(rep->value.string)));
			}
		}
		rep = rep->next;
	}
	/* if the input is ok, then process the input document and assign the global variables */
	if (error_state == false)
	{
		switch (interfaces)
		{
		default:
		{
			/* loop over all the properties in the input document */
			oc_rep_t *rep = request->request_payload;
			while (rep != NULL)
			{
				PRINT("key: (assign) %s \n", oc_string(rep->name));
				/* no error: assign the variables */

				if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valueboolean) == 0)
				{
					/* assign "valueboolean" */
					PRINT("  property 'valueboolean' : %s\n", (char *)btoa(rep->value.boolean));
					g_varianturl_valueboolean = rep->value.boolean;
				}
				if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valueinteger) == 0)
				{
					/* assign "valueinteger" */
					PRINT("  property 'valueinteger' : %d\n", (int)rep->value.integer);
					g_varianturl_valueinteger = (int)rep->value.integer;
				}
				if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valuenumber) == 0)
				{
					/* assign "valuenumber" */
					PRINT("  property 'valuenumber' : %f\n", rep->value.double_p);
					g_varianturl_valuenumber = rep->value.double_p;
				}
				if (strcmp(oc_string(rep->name), g_varianturl_RESOURCE_PROPERTY_NAME_valuestring) == 0)
				{
					/* assign "valuestring" */
					PRINT("  property 'valuestring' : %s\n", oc_string(rep->value.string));
					strncpy(g_varianturl_valuestring, oc_string(rep->value.string), 100 - 1);
				}
				rep = rep->next;
			}
			/* set the response */
			PRINT("Set response \n");
			oc_rep_start_root_object();
			/*oc_process_baseline_interface(request->resource); */
			PRINT("   %s : %s",
			      g_varianturl_RESOURCE_PROPERTY_NAME_valueboolean,
			      (char *)btoa(g_varianturl_valueboolean));
			oc_rep_set_boolean(root, valueboolean, g_varianturl_valueboolean);
			PRINT("   %s : %d\n", g_varianturl_RESOURCE_PROPERTY_NAME_valueinteger, g_varianturl_valueinteger);
			oc_rep_set_int(root, valueinteger, g_varianturl_valueinteger);
			PRINT("   %s : %f\n", g_varianturl_RESOURCE_PROPERTY_NAME_valuenumber, g_varianturl_valuenumber);
			oc_rep_set_double(root, valuenumber, g_varianturl_valuenumber);
			PRINT("   %s : %s\n", g_varianturl_RESOURCE_PROPERTY_NAME_valuestring, g_varianturl_valuestring);
			oc_rep_set_text_string(root, valuestring, g_varianturl_valuestring);

			oc_rep_end_root_object();
			/* TODO: ACTUATOR add here the code to talk to the HW if one implements an actuator.
       one can use the global variables as input to those calls
       the global values have been updated already with the data from the request */
			oc_send_response(request, OC_STATUS_CHANGED);
		}
		}
	}
	else
	{
		PRINT("  Returning Error \n");
		/* TODO: add error response, if any */
		//oc_send_response(request, OC_STATUS_NOT_MODIFIED);
		oc_send_response(request, OC_STATUS_BAD_REQUEST);
	}
	PRINT("-- End post_varianturl\n");
}

/**
* register all the resources to the stack
* this function registers all application level resources:
* - each resource path is bind to a specific function for the supported methods (GET, POST, PUT)
* - each resource is
*   - secure
*   - observable
*   - discoverable
*   - used interfaces, including the default interface.
*     default interface is the first of the list of interfaces as specified in the input file
*/
void register_resources(void)
{
	PRINT("Register Resource with local path \"/queryurl\"\n");
	oc_resource_t *res_queryurl =
	    oc_new_resource("Query resource", g_queryurl_RESOURCE_ENDPOINT, g_queryurl_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_queryurl_nr_resource_types);
	for (int a = 0; a < g_queryurl_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_queryurl_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_queryurl, g_queryurl_RESOURCE_TYPE[a]);
	}

	oc_resource_bind_resource_interface(res_queryurl, OC_IF_A);        /* oic.if.a */
	oc_resource_bind_resource_interface(res_queryurl, OC_IF_BASELINE); /* oic.if.baseline */
	oc_resource_set_default_interface(res_queryurl, OC_IF_A);
	PRINT("     Default OCF Interface: 'oic.if.a'\n");
	oc_resource_set_discoverable(res_queryurl, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	oc_resource_set_periodic_observable(res_queryurl, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, preferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_queryurl, true); */

	oc_resource_set_request_handler(res_queryurl, OC_GET, get_queryurl, NULL);

#ifdef OC_CLOUD
	oc_cloud_add_resource(res_queryurl);
#endif
	oc_resource_set_request_handler(res_queryurl, OC_POST, post_queryurl, NULL);

#ifdef OC_CLOUD
	oc_cloud_add_resource(res_queryurl);
#endif
	oc_add_resource(res_queryurl);
	PRINT("Register Resource with local path \"/rangeurl\"\n");
	oc_resource_t *res_rangeurl =
	    oc_new_resource("Range resource", g_rangeurl_RESOURCE_ENDPOINT, g_rangeurl_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_rangeurl_nr_resource_types);
	for (int a = 0; a < g_rangeurl_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_rangeurl_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_rangeurl, g_rangeurl_RESOURCE_TYPE[a]);
	}

	oc_resource_bind_resource_interface(res_rangeurl, OC_IF_A);        /* oic.if.a */
	oc_resource_bind_resource_interface(res_rangeurl, OC_IF_BASELINE); /* oic.if.baseline */
	oc_resource_set_default_interface(res_rangeurl, OC_IF_A);
	PRINT("     Default OCF Interface: 'oic.if.a'\n");
	oc_resource_set_discoverable(res_rangeurl, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	// oc_resource_set_periodic_observable(res_rangeurl, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, preferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_rangeurl, true); */

	oc_resource_set_request_handler(res_rangeurl, OC_GET, get_rangeurl, NULL);

#ifdef OC_CLOUD
	oc_cloud_add_resource(res_rangeurl);
#endif
	oc_resource_set_request_handler(res_rangeurl, OC_POST, post_rangeurl, NULL);

#ifdef OC_CLOUD
	oc_cloud_add_resource(res_rangeurl);
#endif
	oc_add_resource(res_rangeurl);
	PRINT("Register Resource with local path \"/varianturl\"\n");
	oc_resource_t *res_varianturl =
	    oc_new_resource("Variant resource", g_varianturl_RESOURCE_ENDPOINT, g_varianturl_nr_resource_types, 0);
	PRINT("     number of Resource Types: %d\n", g_varianturl_nr_resource_types);
	for (int a = 0; a < g_varianturl_nr_resource_types; a++)
	{
		PRINT("     Resource Type: \"%s\"\n", g_varianturl_RESOURCE_TYPE[a]);
		oc_resource_bind_resource_type(res_varianturl, g_varianturl_RESOURCE_TYPE[a]);
	}

	oc_resource_bind_resource_interface(res_varianturl, OC_IF_A);        /* oic.if.a */
	oc_resource_bind_resource_interface(res_varianturl, OC_IF_BASELINE); /* oic.if.baseline */
	oc_resource_set_default_interface(res_varianturl, OC_IF_A);
	PRINT("     Default OCF Interface: 'oic.if.a'\n");
	oc_resource_set_discoverable(res_varianturl, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	// oc_resource_set_periodic_observable(res_varianturl, 1);
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is called.
    this function must be called when the value changes, preferable on an interrupt when something is read from the hardware. */
	/*oc_resource_set_observable(res_varianturl, true); */

	oc_resource_set_request_handler(res_varianturl, OC_GET, get_varianturl, NULL);

#ifdef OC_CLOUD
	oc_cloud_add_resource(res_varianturl);
#endif
	oc_resource_set_request_handler(res_varianturl, OC_POST, post_varianturl, NULL);

#ifdef OC_CLOUD
	oc_cloud_add_resource(res_varianturl);
#endif
	oc_add_resource(res_varianturl);
	/* disable observe for oic/d */
	oc_resource_t *device_resource = oc_core_get_resource_by_index(OCF_D, DEVICE);
	oc_resource_set_observable(device_resource, false);
	/* disable observe for oic/p */
	oc_resource_t *platform_resource = oc_core_get_resource_by_index(OCF_P, DEVICE);
	oc_resource_set_observable(platform_resource, false);

	// Cascoda addition: disable observe on PSTAT resurce to save on peak
	// memory consumption
	oc_resource_t *pstat_resource = oc_core_get_resource_by_index(OCF_SEC_PSTAT, 0);
	oc_resource_set_observable(pstat_resource, false);
}

#ifdef OC_SECURITY
#ifdef OC_SECURITY_PIN
void random_pin_cb(const unsigned char *pin, size_t pin_len, void *data)
{
	(void)data;
	PRINT("\n====================\n");
	PRINT("Random PIN: %.*s\n", (int)pin_len, pin);
	PRINT("====================\n");
}
#endif /* OC_SECURITY_PIN */
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
	/* initialize global variables for resource "/queryurl" */
	strcpy(g_queryurl_queryvalue, "C"); /* current value of property "queryvalue" query param */
	g_queryurl_value = 0;               /* current value of property "value"  Current setting or measurement */
	/* initialize global variables for resource "/rangeurl" */
	/* initialize array "range" : The valid range for the Property in the Resource as a number. The first value in the array is the minimum value, the second value in the array is the maximum value. */
	g_rangeurl_range[0]         = 0.0;
	g_rangeurl_range[1]         = 100.0;
	g_rangeurl_range_array_size = 2;

	g_rangeurl_step =
	    0; /* current value of property "step"  Step value across the defined range an integer when the range is a number.  This is the increment for valid values across the range; so if range is 0.0..10.0 and step is 2.5 then valid values are 0.0,2.5,5.0,7.5,10.0. */
	g_rangeurl_value = 0; /* current value of property "value"  Current setting or measurement */
	/* initialize global variables for resource "/varianturl" */
	g_varianturl_valueboolean = false; /* current value of property "valueboolean" on/off */
	g_varianturl_valueinteger = 75;    /* current value of property "valueinteger" value integer. */
	g_varianturl_valuenumber  = 0;     /* current value of property "valuenumber"  value number. */
	strcpy(
	    g_varianturl_valuestring,
	    ""
	    ""); /* current value of property "valuestring" value string, using a different length than the default lenght. */

	/* set the flag for oic/con resource. */
	oc_set_con_res_announced(true);
}

#ifndef NO_MAIN

#ifdef WIN32
/**
* signal the event loop (windows version)
* wakes up the main function to handle the next callback
*/
static void signal_event_loop(void)
{
	WakeConditionVariable(&cv);
}
#endif /* WIN32 */

#ifdef __linux__
/**
* signal the event loop (Linux)
* wakes up the main function to handle the next callback
*/
static void signal_event_loop(void)
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
static void cloud_status_handler(oc_cloud_context_t *ctx, oc_cloud_status_t status, void *data)
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
	PRINT("OCF Server name : \"server_lite_42088\"\n");

/*
 The storage folder depends on the build system
 for Windows the projects simpleserver and cloud_server are overwritten, hence the folders should be the same as those targets.
 for Linux (as default) the folder is created in the makefile, with $target as name with _cred as post fix.
*/
#ifdef OC_SECURITY
	PRINT("Intialize Secure Resources\n");
#ifdef WIN32
#ifdef OC_CLOUD
	PRINT("\tstorage at './cloudserver_creds' \n");
	oc_storage_config("./cloudserver_creds");
#else
	PRINT("\tstorage at './simpleserver_creds' \n");
	oc_storage_config("./simpleserver_creds/");
#endif
#else
	PRINT("\tstorage at './device_builder_server_creds' \n");
	oc_storage_config("./device_builder_server_creds");
#endif

	/*intialize the variables */
	initialize_variables();

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
#ifdef OC_SECURITY_PIN
	/* please enable OC_SECURITY_PIN
    - have display capabilities to display the PIN value
    - server require to implement RANDOM PIN (oic.sec.doxm.rdp) onboarding mechanism
  */
	oc_set_random_pin_callback(random_pin_cb, NULL);
#endif /* OC_SECURITY_PIN */
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

	PRINT("OCF server \"server_lite_42088\" running, waiting on incoming connections.\n");

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
