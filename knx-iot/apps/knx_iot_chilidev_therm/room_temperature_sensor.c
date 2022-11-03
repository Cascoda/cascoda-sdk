/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright (c) 2022 Cascoda Ltd
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

/**
 * @file
 * 
 * Room Temperature
 * 2022-08-22 12:13:46.041049
 * ## Application Design
 *
 * support functions:
 *
 * - app_init
 *   initializes the stack values.
 * - register_resources
 *   function that registers all endpoints,
 *   e.g. sets the GET/PUT/POST/DELETE
 *      handlers for each end point
 *
 * - main
 *   starts the stack, with the registered resources.
 *   can be compiled out with NO_MAIN
 *
 *  handlers for the implemented methods (get/post):
 *   - get_[path]
 *     function that is being called when a GET is called on [path]
 *     set the global variables in the output
 *   - post_[path]
 *     function that is being called when a POST is called on [path]
 *     if input data is of the correct type
 *       updates the global variables
 *
 * ## stack specific defines
 * - __linux__
 *   build for Linux
 * - WIN32
 *   build for Windows
 * - OC_OSCORE
 *   oscore is enabled as compile flag
 * ## File specific defines
 * - NO_MAIN
 *   compile out the function main()
 * - INCLUDE_EXTERNAL
 *   includes header file "external_header.h", so that other tools/dependencies
 *   can be included without changing this code
 * - KNX_GUI
 *   build the GUI with console option, so that all 
 *   logging can be seen in the command window
 */
#include <signal.h>
#include "port/oc_clock.h"
#include "oc_api.h"
#include "oc_core_res.h"
/* test purpose only; commandline reset */
#include "api/oc_knx_dev.h"
#ifdef OC_SPAKE
#include "security/oc_spake2plus.h"
#endif
#ifdef INCLUDE_EXTERNAL
/* import external definitions from header file*/
/* this file should be externally supplied */
#include "external_header.h"
#endif
#include "room_temperature_sensor.h"

#include <stdlib.h>

#ifdef __linux__
/** linux specific code */
#include <pthread.h>
#ifndef NO_MAIN
static pthread_mutex_t mutex;
static pthread_cond_t  cv;
static struct timespec ts;
#endif /* NO_MAIN */
#endif

#include <stdio.h> /* defines FILENAME_MAX */

#define MY_NAME "Room Temperature" /**< The name of the application */
#define APP_MAX_STRING 30

#ifdef WIN32
/** windows specific code */
#include <windows.h>
static CONDITION_VARIABLE cv; /**< event loop variable */
static CRITICAL_SECTION   cs; /**< event loop variable */
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#define btoa(x) ((x) ? "true" : "false")
volatile int quit                = 0;     /**< stop variable, used by handle_signal */
bool         g_reset             = false; /**< reset variable, set by commandline arguments */
char         g_serial_number[20] = "0005000";

/* list all object urls as defines */
#define CH1_URL_THENAME "/p/o_1_1" /**< define for url "/p/o_1_1" of "thename" */

/* list all parameter urls as defines */

volatile double g_thename; /**< global variable for thename */

// BOOLEAN code

/**
 * @brief function to check if the url is represented by a boolean
 *
 * @param true = url value is a boolean
 * @param false = url is not a boolean
 */
bool app_is_bool_url(char *url)
{
	return false;
}

/**
 * @brief sets the global boolean variable at the url
 *
 * @param url the url indicating the global variable
 * @param value the value to be set
 */
void app_set_bool_variable(char *url, bool value)
{
}

/**
 * @brief retrieve the global boolean variable at the url
 *
 * @param url the url indicating the global variable
 * @return the value of the variable
 */
bool app_retrieve_bool_variable(char *url)
{
	return false;
}

// INTEGER code

/**
 * @brief function to check if the url is represented by a integer
 *
 * @param true = url value is a integer
 * @param false = url is not a integer
 */
bool app_is_int_url(char *url)
{
	return false;
}
/**
 * @brief sets the global int variable at the url
 *
 * @param url the url indicating the global variable
 * @param value the value to be set
 */
void app_set_integer_variable(char *url, int value)
{
}
/**
 * @brief retrieve the global integer variable at the url
 *
 * @param url the url indicating the global variable
 * @return the value of the variable
 */
int app_retrieve_int_variable(char *url)
{
	return -1;
}

// DOUBLE code

/**
 * @brief function to check if the url is represented by a double
 *
 * @param true = url value is a double
 * @param false = url is not a double
 */
bool app_is_double_url(char *url)
{
	if (strcmp(url, CH1_URL_THENAME) == 0)
	{
		return true; /**< thename is an double */
	}
	return false;
}
/**
 * @brief sets the global double variable at the url
 *
 * @param url the url indicating the global variable
 * @param value the value to be set
 */
void app_set_double_variable(char *url, double value)
{
	if (strcmp(url, CH1_URL_THENAME) == 0)
	{
		g_thename = value; /**< global variable for thename */
		return;
	}
}
/**
 * @brief retrieve the global double variable at the url
 *
 * @param url the url indicating the global variable
 * @return the value of the variable
 */
double app_retrieve_double_variable(char *url)
{
	if (strcmp(url, CH1_URL_THENAME) == 0)
	{
		return g_thename; /**< global variable for thename */
	}
	return -1;
}

// STRING code

/**
 * @brief function to check if the url is represented by a string
 *
 * @param true = url value is a string
 * @param false = url is not a string
 */
bool app_is_string_url(char *url)
{
	return false;
}

/**
 * @brief sets the global string variable at the url
 *
 * @param url the url indicating the global variable
 * @param value the value to be set
 */
void app_set_string_variable(char *url, char *value)
{
}

/**
 * @brief retrieve the global string variable at the url
 *
 * @param url the url indicating the global variable
 * @return the value of the variable
 */
char *app_retrieve_string_variable(char *url)
{
	return NULL;
}

// FAULT code

/**
 * @brief set the fault (boolean) variable at the url
 *
 * @param url the url indicating the fault variable
 * @param value the value of the fault variable
 */
void app_set_fault_variable(char *url, bool value)
{
}

/**
 * @brief retrieve the fault (boolean) variable at the url
 *
 * @param url the url indicating the fault variable
 * @return the value of the fault variable
 */
bool app_retrieve_fault_variable(char *url)
{
	return false;
}

// PARAMETER code

bool app_is_url_parameter(char *url)
{
	return false;
}

char *app_get_parameter_url(int index)
{
	return NULL;
}

char *app_get_parameter_name(int index)
{
	return NULL;
}

bool app_is_secure()
{
#ifdef OC_OSCORE
	return true;
#else
	return false;
#endif /* OC_OSCORE */
}

static oc_post_struct_t app_post = {NULL};

void app_set_post_cb(oc_post_cb_t cb)
{
	app_post.cb = cb;
}

oc_post_struct_t *oc_get_post_cb(void)
{
	return &app_post;
}

void do_post_cb(char *url)
{
	oc_post_struct_t *my_cb = oc_get_post_cb();
	if (my_cb && my_cb->cb)
	{
		my_cb->cb(url);
	}
}

#ifdef __cplusplus
extern "C" {
#endif
int  app_initialize_stack();
void signal_event_loop(void);
void register_resources(void);
int  app_init(void);
#ifdef __cplusplus
}
#endif

/**
 * @brief s-mode response callback
 * will be called when a response is received on an s-mode read request
 *
 * @param url the url
 * @param rep the full response
 * @param rep_value the parsed value of the response
 */
void oc_add_s_mode_response_cb(char *url, oc_rep_t *rep, oc_rep_t *rep_value)
{
	(void)rep;
	(void)rep_value;

	PRINT("oc_add_s_mode_response_cb %s\n", url);
}

/**
 * @brief function to set up the device.
 *
 * sets the:
 * - manufacturer     : cascoda
 * - serial number    : 0005000
 * - base path
 * - knx spec version 
 * - hardware version : [0, 1, 2]
 * - firmware version : [3, 4, 5]
 * - hardware type    : my_type
 * - device model     : Sensor
 *
 */
int app_init(void)
{
	int ret = oc_init_platform("cascoda", NULL, NULL);

	/* set the application name, version, base url, device serial number */
	ret |= oc_add_device(MY_NAME, "1.0.0", "//", g_serial_number, NULL, NULL);

	oc_device_info_t *device = oc_core_get_device_info(0);
	PRINT("Serial Number: %s\n", oc_string(device->serialnumber));

	/* set the hardware version 0.1.2 */
	oc_core_set_device_hwv(0, 0, 1, 2);

	/* set the firmware version 3.4.5 */
	oc_core_set_device_fwv(0, 3, 4, 5);

	/* set the hardware type*/
	oc_core_set_device_hwt(0, "my_type");

	/* set the model */
	oc_core_set_device_model(0, "Sensor");

	oc_set_s_mode_response_cb(oc_add_s_mode_response_cb);

#ifdef OC_SPAKE
#define PASSWORD "LETTUCE"
	oc_spake_set_password(PASSWORD);
	PRINT(" SPAKE password %s\n", PASSWORD);
#endif

	return ret;
}

// data point (objects) handling

/**
 * @brief CoAP GET method for data point "thename" resource at url CH1_URL_THENAME ("/p/o_1_1").
 * resource types: ['urn:knx:dpa.321.51', 'DPT_Switch']
 * function is called to initialize the return values of the GET method.
 * initialization of the returned values are done from the global property
 * values. 
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param user_data the user data.
 */
void get_thename(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
	(void)user_data; /* variable not used */

	/* MANUFACTORER: SENSOR add here the code to talk to the HW if one implements a
     sensor. the call to the HW needs to fill in the global variable before it
     returns to this function here. alternative is to have a callback from the
     hardware that sets the global variables.
  */
	bool error_state = false; /* the error state, the generated code */

	PRINT("-- Begin get_thename %s \n", CH1_URL_THENAME);
	/* check if the accept header is CBOR */
	if (request->accept != APPLICATION_CBOR)
	{
		oc_send_response(request, OC_STATUS_BAD_OPTION);
		return;
	}
	char *m;
	int   m_len = oc_get_query_value(request, "m", &m);
	if (m_len != -1)
	{
		oc_rep_begin_root_object();
		if (strncmp(m, "unit", m_len) == 0)
		{
			oc_rep_set_text_string(root, unit, "DEG_C");
		}
		if (strncmp(m, "datatype", m_len) == 0)
		{
			oc_rep_set_text_string(root, datatype, "number");
		}
		oc_rep_end_root_object();
		oc_send_cbor_response(request, OC_STATUS_OK);
		return;
	}

	oc_rep_begin_root_object();
#ifdef OC_SIMULATION
	/* simulation code */
	g_thename += 0.2;
	if (g_thename > 5.0)
	{
		g_thename = 0.0;
	}
#endif /* OC_SIMULATION */
	oc_rep_i_set_double(root, 1, g_thename);
	oc_rep_end_root_object();
	if (g_err)
	{
		error_state = true;
	}
	PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());
	if (error_state == false)
	{
		oc_send_cbor_response(request, OC_STATUS_OK);
	}
	else
	{
		oc_send_response(request, OC_STATUS_BAD_OPTION);
	}
	PRINT("-- End get_thename\n");
}

// parameters handling

/**
 * @brief register all the data point resources to the stack
 * this function registers all data point level resources:
 * - each resource path is bind to a specific function for the supported methods
 *  (GET, POST)
 * - each resource is
 *   - secure
 *   - observable
 *   - discoverable through well-known/core
 *   - used interfaces as: dpa.xxx.yyy 
 *      - xxx : function block number
 *      - yyy : data point function number
 */
void register_resources(void)
{
	PRINT("Register Resource 'thename' with local path \"%s\"\n", CH1_URL_THENAME);
	oc_resource_t *res_thename = oc_new_resource("thename", CH1_URL_THENAME, 2, 0);
	oc_resource_bind_resource_type(res_thename, "urn:knx:dpa.321.51");
	oc_resource_bind_resource_type(res_thename, "DPT_Switch");
	oc_resource_bind_content_type(res_thename, APPLICATION_CBOR);
	oc_resource_bind_resource_interface(res_thename, OC_IF_S); /* if.s */
	oc_resource_set_function_block_instance(res_thename, 1);   /* instance 1 */
	oc_resource_set_discoverable(res_thename, true);
	/* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
	/* oc_resource_set_periodic_observable(res_thename, 1); */
	/* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
	oc_resource_set_observable(res_thename, true);
	oc_resource_set_request_handler(res_thename, OC_GET, get_thename, NULL);
	oc_add_resource(res_thename);
}

/**
 * @brief initiate preset for device
 * current implementation: device reset as command line argument
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void factory_presets_cb(size_t device_index, void *data)
{
	(void)device_index;
	(void)data;

	if (g_reset)
	{
		PRINT("factory_presets_cb: resetting device\n");
		oc_knx_device_storage_reset(device_index, 2);
	}
}

/**
 * @brief application reset
 *
 * @param device_index the device identifier of the list of devices
 * @param reset_value the knx reset value
 * @param data the supplied data.
 */
void reset_cb(size_t device_index, int reset_value, void *data)
{
	(void)device_index;

	PRINT("reset_cb %d\n", reset_value);
}

/**
 * @brief restart the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void restart_cb(size_t device_index, void *data)
{
	(void)device_index;
	(void)data;

	PRINT("-----restart_cb -------\n");
}

/**
 * @brief set the host name on the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param host_name the host name to be set on the device
 * @param data the supplied data.
 */
void hostname_cb(size_t device_index, oc_string_t host_name, void *data)
{
	(void)device_index;
	(void)data;

	PRINT("-----host name ------- %s\n", oc_string(host_name));
}

/**
 * @brief software update callback
 *
 * @param device_index the device index
 * @param offset the offset of the image
 * @param payload the image data
 * @param len the length of the image data
 * @param data the user data
 */
void swu_cb(size_t device_index, size_t offset, uint8_t *payload, size_t len, void *data)
{
	(void)device_index;
	char *fname = (char *)data;
	PRINT(" swu_cb %s block=%d size=%d \n", fname, (int)offset, (int)len);

	FILE *fp = fopen(fname, "rw");
	fseek(fp, offset, SEEK_SET);
	size_t written = fwrite(payload, len, 1, fp);
	if (written != len)
	{
		PRINT(" swu_cb returned %d != %d (expected)\n", (int)written, (int)len);
	}
	fclose(fp);
}

/**
 * @brief initializes the global variables
 * for the resources 
 * for the parameters
 */
void initialize_variables(void)
{
	/* initialize global variables for resources */
	/* if wanted read them from persistent storage */
	g_thename = 0; /**< global variable for thename */
	               /* parameter variables */
}

int app_set_serial_number(char *serial_number)
{
	strncpy(g_serial_number, serial_number, 20);
	return 0;
}

int app_initialize_stack()
{
	int   init;
	char *fname = "my_software_image";

	PRINT("KNX-IOT Server name : \"%s\"\n", MY_NAME);

	/* show the current working folder */
	char  buff[FILENAME_MAX];
	char *retbuf = NULL;
	retbuf       = GetCurrentDir(buff, FILENAME_MAX);
	if (retbuf != NULL)
	{
		PRINT("Current working dir: %s\n", buff);
	}

	/*
   The storage folder depends on the build system
   the folder is created in the makefile, with $target as name with _cred as
   post fix.
  */
#ifdef WIN32
	char storage[40];
	sprintf(storage, "./room_temperature_sensor_%s", g_serial_number);
	PRINT("\tstorage at '%s' \n", storage);
	oc_storage_config(storage);
#else
	PRINT("\tstorage at 'room_temperature_sensor_creds' \n");
	oc_storage_config("./room_temperature_sensor_creds");
#endif

	/*initialize the variables */
	initialize_variables();

	/* initializes the handlers structure */
	static oc_handler_t handler = {.init               = app_init,
	                               .signal_event_loop  = signal_event_loop,
	                               .register_resources = register_resources,
	                               .requests_entry     = NULL};

	/* set the application callbacks */
	oc_set_hostname_cb(hostname_cb, NULL);
	oc_set_reset_cb(reset_cb, NULL);
	oc_set_restart_cb(restart_cb, NULL);
	oc_set_factory_presets_cb(factory_presets_cb, NULL);
	oc_set_swu_cb(swu_cb, (void *)fname);

	/* start the stack */
	init = oc_main_init(&handler);

	if (init < 0)
	{
		PRINT("oc_main_init failed %d, exiting.\n", init);
		return init;
	}

#ifdef OC_OSCORE
	PRINT("OSCORE - Enabled\n");
#else
	PRINT("OSCORE - Disabled\n");
#endif /* OC_OSCORE */

	oc_device_info_t *device = oc_core_get_device_info(0);
	PRINT("serial number: %s\n", oc_string(device->serialnumber));
	oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(0);
	if (my_ep != NULL)
	{
		PRINTipaddr(*my_ep);
		PRINT("\n");
	}
	PRINT("Server \"%s\" running, waiting on incoming "
	      "connections.\n",
	      MY_NAME);
	return 0;
}

#ifdef WIN32
/**
 * @brief signal the event loop (windows version)
 * wakes up the main function to handle the next callback
 */
void signal_event_loop(void)
{
#ifndef NO_MAIN
	WakeConditionVariable(&cv);
#endif /* NO_MAIN */
}
#endif /* WIN32 */

#ifdef __linux__
/**
 * @brief signal the event loop (Linux)
 * wakes up the main function to handle the next callback
 */
void signal_event_loop(void)
{
#ifndef NO_MAIN
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cv);
	pthread_mutex_unlock(&mutex);
#endif /* NO_MAIN */
}
#endif /* __linux__ */

#ifndef NO_MAIN

/**
 * @brief handle Ctrl-C
 * @param signal the captured signal
 */
static void handle_signal(int signal)
{
	(void)signal;
	signal_event_loop();
	quit = 1;
}

/**
 * @brief print usage and quits
 *
 */
static void print_usage()
{
	PRINT("Usage:\n");
	PRINT("no arguments : starts the server\n");
	PRINT("-help  : this message\n");
	PRINT("reset  : does an full reset of the device\n");
	PRINT("-s <serial number> : sets the serial number of the device\n");
	exit(0);
}
/**
 * @brief main application.
 * initializes the global variables
 * registers and starts the handler
 * handles (in a loop) the next event.
 * shuts down the stack
 */
int main(int argc, char *argv[])
{
	oc_clock_time_t next_event;
	bool            do_send_s_mode = false;

#ifdef KNX_GUI
	WinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOWNORMAL);
#endif

#ifdef WIN32
	/* windows specific */
	InitializeCriticalSection(&cs);
	InitializeConditionVariable(&cv);
	/* install Ctrl-C */
	signal(SIGINT, handle_signal);
#endif
#ifdef __linux__
	/* Linux specific */
	struct sigaction sa;
	sigfillset(&sa.sa_mask);
	sa.sa_flags   = 0;
	sa.sa_handler = handle_signal;
	/* install Ctrl-C */
	sigaction(SIGINT, &sa, NULL);
#endif

	for (int i = 0; i < argc; i++)
	{
		printf("argv[%d] = %s\n", i, argv[i]);
	}
	if (argc > 1)
	{
		if (strcmp(argv[1], "reset") == 0)
		{
			PRINT(" internal reset\n");
			g_reset = true;
		}
		if (strcmp(argv[1], "-help") == 0)
		{
			print_usage();
		}
	}
	if (argc > 2)
	{
		if (strcmp(argv[1], "-s") == 0)
		{
			// serial number
			PRINT("serial number %s\n", argv[2]);
			app_set_serial_number(argv[2]);
		}
	}

	/* do all initialization */
	app_initialize_stack();

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
	/* Linux specific loop */
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
	oc_main_shutdown();
	return 0;
}
#endif /* NO_MAIN */
