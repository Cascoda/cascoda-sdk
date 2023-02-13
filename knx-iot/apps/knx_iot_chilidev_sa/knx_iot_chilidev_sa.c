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

#include "knx_iot_virtual_sa.h"
#include "knx_iot_wakeful_main_extern.h"

#include "devboard_btn.h"

void set_led(int led_nr, bool value)
{
	// Set the LED to the required state
	// LEDs are active low, so set the inverse
	DVBD_SetLED(led_nr, !value);

	printf("Set LED %d to %d\n", led_nr, value);
}

void put_callback(char *url)
{
	bool my_bool = app_retrieve_bool_variable(url);
	if (strcmp(url, "/p/o_1_1") == 0)
		set_led(LED_BTN_0, my_bool);
	if (strcmp(url, "/p/o_3_3") == 0)
		set_led(LED_BTN_1, my_bool);
	if (strcmp(url, "/p/o_5_5") == 0)
		set_led(LED_BTN_2, my_bool);
	if (strcmp(url, "/p/o_7_7") == 0)
		set_led(LED_BTN_3, my_bool);
}

void hardware_init()
{
	// Register all the board LEDs as an output
	DVBD_RegisterLEDOutput(LED_BTN_0, JUMPER_POS_2);
	DVBD_RegisterLEDOutput(LED_BTN_1, JUMPER_POS_2);
	DVBD_RegisterLEDOutput(LED_BTN_2, JUMPER_POS_2);
	DVBD_RegisterLEDOutput(LED_BTN_3, JUMPER_POS_2);

	// Setup callbacks for the info datapoints on pb
	app_set_put_cb(put_callback);
}

void hardware_poll()
{
}