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

/* jitter added to response to some multicast requests */
#define OC_MULTICAST_RESPONSE_JITTER_MS (2000)

/* Server-side parameters */
/* Maximum number of server resources */
#define OC_MAX_APP_RESOURCES (1)

/* Common paramters */

#define OC_INOUT_BUFFER_POOL (2)
#define OC_INOUT_BUFFER_SIZE (1232)

#define OC_APP_DATA_BUFFER_POOL (1)
#define OC_APP_DATA_BUFFER_SIZE (7168)

// The maximum size of a response to an OBSERVE request, in bytes.
#define OC_MAX_OBSERVE_SIZE 512

// Deduplicate CoAP messages
#define OC_REQUEST_HISTORY

//#define OC_APP_DATA_STORAGE_BUFFER

#define OC_BLOCK_WISE
#define OC_BLOCK_WISE_SET_MTU (1200)

#define OC_MAX_NUM_DEVICES (1)

/* Security layer */
/* Maximum number of authorized clients */
#define OC_MAX_NUM_SUBJECTS (1)

/* Maximum number of concurrent (D)TLS sessions */
#define OC_MAX_TLS_PEERS (1)

/* Max inactivity timeout before tearing down DTLS connection */
#define OC_DTLS_INACTIVITY_TIMEOUT (604800)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* OC_CONFIG_H */
