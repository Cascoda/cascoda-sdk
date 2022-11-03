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

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_types.h"

/**
 * @brief define what happens when receiving a post mesage on a URL
 * 
 * @param url the URL of the message as a string
 */
void post_callback(char *url);

/**
 * @brief any hardware specific setup e.g. registering board LEDs and buttons
 * 
 */
void hardware_init();

/**
 * @brief any hardware specific actions to be continually run e.g. checking buttons for input
 * 
 */
void hardware_poll();