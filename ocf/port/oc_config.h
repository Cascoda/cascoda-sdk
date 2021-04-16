/*
 // Copyright (c) 2016 Intel Corporation
// Modifications copyright (c) 2020, Cascoda Ltd.
 //
 // Licensed under the Apache License, Version 2.0 (the "License");
 // you may not use this file except in compliance with the License.
 // You may obtain a copy of the License at
 //
 //      http://www.apache.org/licenses/LICENSE-2.0
 //
 // Unless required by applicable law or agreed to in writing, software
 // distributed under the License is distributed on an "AS IS" BASIS,
 // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 // See the License for the specific language governing permissions and
 // limitations under the License.
 */

#ifndef OC_CONFIG_H
#define OC_CONFIG_H

/* Time resolution */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* enable flash storage API */
#define OC_STORAGE

typedef uint64_t oc_clock_time_t;
/* 1 clock tick = 1ms */
#define OC_CLOCK_CONF_TICKS_PER_SECOND (1000)

#define OC_BYTES_POOL_SIZE (16 * 1024)
#define OC_INTS_POOL_SIZE (16)
#define OC_DOUBLES_POOL_SIZE (16)

/* Server-side parameters */
/* Maximum number of server resources */
#define OC_MAX_APP_RESOURCES (1)

/* Common paramters */
/* Maximum size of request/response PDUs */
#ifndef OC_DYNAMIC_ALLOCATION
#define OC_MAX_APP_DATA_SIZE (8192)
#endif // !OC_DYNAMIC_ALLOCATION

#define OC_INOUT_BUFFER_POOL (2)
#define OC_INOUT_BUFFER_SIZE (1232)

#define OC_APP_DATA_BUFFER_POOL (1)
#define OC_APP_DATA_BUFFER_SIZE (8192)

// The maximum size of a response to an OBSERVE request, in bytes.
#define OC_MAX_OBSERVE_SIZE 512

// Deduplicate CoAP messages
#define OC_REQUEST_HISTORY

//#define OC_APP_DATA_STORAGE_BUFFER

#define OC_BLOCK_WISE
#define OC_BLOCK_WISE_SET_MTU (1200)

/* Maximum number of concurrent requests */
#define OC_MAX_NUM_CONCURRENT_REQUESTS (2)

/* Maximum number of nodes in a payload tree structure */
#define OC_MAX_NUM_REP_OBJECTS (70)

/* Number of devices on the OCF platform */
#define OC_MAX_NUM_DEVICES (1)

#define OC_MAX_NUM_ENDPOINTS (2)

/* Security layer */
/* Maximum number of authorized clients */
#define OC_MAX_NUM_SUBJECTS (1)

/* Maximum number of concurrent DTLS sessions */
#define OC_MAX_DTLS_PEERS (1)

/* Max inactivity timeout before tearing down DTLS connection */
#define OC_DTLS_INACTIVITY_TIMEOUT (604800)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* OC_CONFIG_H */
