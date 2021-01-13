/*
 *  Copyright (c) 2020, Cascoda Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file
 * @brief  Doxygen high-level grouping for easy Doxygen browsing
 */

/**
 * @defgroup baremetal  Baremetal
 * @brief The API functions specific to the Cascoda baremetal platforms
 * such as the Chili 1 and Chili 2 modules.
 *
 * @{
 *
 * @defgroup bm-core  Baremetal Core
 * @brief The core baremetal functionality for interfacing with the CA-821x
 *
 * @defgroup bm-driver  Baremetal Drivers
 * @brief The extended baremetal functionality for using Cascoda baremetal platforms.
 *
 * @defgroup bm-thread  Baremetal Thread
 * @brief Functionality for running OpenThread on Cascoda Baremetal platforms
 *
 * @defgroup bm-sensorif Baremetal Sensor/Actuator interface
 * @brief Functionality to interact with supported sensors and actuators.
 *
 * @}
 *
 */

/**
 * @defgroup posix Posix
 * @brief The API functions specific to posix (and Windows where possible), to allow for Chili platforms to be controlled from an OS host.
 */

/**
 * @defgroup ca821x-api CA-821x API
 * @brief The core, cross-platform API for interaction with a CA-821x device. It provides functions for interaction with a CA-821x transceiver or Chili platform, and a collection of support functions.
 *
 * @{
 *
 * @defgroup ca821x-api-support CA-821x Support functionality
 * @brief The support functionality used by the Cascoda API and SDK
 *
 * @defgroup ca821x-api-struct CA-821x Definitions
 * @brief The data structures, constants, and definitions used by the Cascoda API and SDK
 *
 * @}
 */

/**
 * @defgroup cascoda-util Cascoda Utilities
 * @brief Cross-Platform utilities not directly related to the CA-821x
 */

/**
 * @defgroup cascoda-ot-util Cascoda OpenThread utilities
 * @brief Cross-Platform utilities built on top of OpenThread
 */

