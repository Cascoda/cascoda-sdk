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

#include "knx_iot_virtual_pb.h"
#include "knx_iot_wakeful_main_extern.h"

#include "api/oc_knx_dev.h"
#include "api/oc_knx_fp.h"

#include "cascoda_devboard_btn.h"

void set_led(int led_nr, bool value)
{
	printf("Would set %d to %d\n", led_nr, value);
}

void post_callback(char *url)
{
	bool my_bool = app_retrieve_bool_variable(url);
	if (strcmp(url, "/p/o_2_2") == 0)
		set_led(LED_BTN_0, my_bool);
	if (strcmp(url, "/p/o_4_4") == 0)
		set_led(LED_BTN_1, my_bool);
	if (strcmp(url, "/p/o_6_6") == 0)
		set_led(LED_BTN_2, my_bool);
	if (strcmp(url, "/p/o_8_8") == 0)
		set_led(LED_BTN_3, my_bool);
}

void generic_button_cb(char url[])
{
	printf("Handling %s\n", url);
	bool val = app_retrieve_bool_variable(url);
	if (val == true)
	{
		val = false;
	}
	else
	{
		val = true;
	}
	app_set_bool_variable(url, val);
	oc_do_s_mode_with_scope(2, url, "w");
	oc_do_s_mode_with_scope(5, url, "w");
}

void button0_cb()
{
	generic_button_cb("/p/o_1_1");
}

void button1_cb()
{
	generic_button_cb("/p/o_3_3");
}

void button2_cb()
{
	generic_button_cb("/p/o_5_5");
}

void button3_cb()
{
	generic_button_cb("/p/o_7_7");
}

void hardware_init()
{
	// Register all the board LEDs as an input
	DVBD_RegisterButtonInput(LED_BTN_0, JUMPER_POS_2);
	DVBD_RegisterButtonInput(LED_BTN_1, JUMPER_POS_2);
	DVBD_RegisterButtonInput(LED_BTN_2, JUMPER_POS_2);
	DVBD_RegisterButtonInput(LED_BTN_3, JUMPER_POS_2);

	DVBD_SetButtonShortPressCallback(LED_BTN_0, &button0_cb);
	DVBD_SetButtonShortPressCallback(LED_BTN_1, &button1_cb);
	DVBD_SetButtonShortPressCallback(LED_BTN_2, &button2_cb);
	DVBD_SetButtonShortPressCallback(LED_BTN_3, &button3_cb);

	// Setup callbacks for the info datapoints on pb
	app_set_post_cb(post_callback);
}

void hardware_poll()
{
	DVBD_PollButtons();
}