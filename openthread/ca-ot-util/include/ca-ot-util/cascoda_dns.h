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
 * @brief  Hostname resolution, DNS & DNS64 helper functions
 */
/**
 * @ingroup cascoda-ot-util
 * @defgroup ca-ot-dns Hostname resolution
 * @brief  Hostname resolution, DNS & DNS64 helper functions
 *
 * @{
 */

#ifndef CASCODA_DNS_H
#define CASCODA_DNS_H

#include <stddef.h>
#include <stdint.h>

#include "openthread/ip6.h"

#include "ca821x_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque dns_index type which holds the DNS server that was used for a request.
 */
typedef struct dnsServer *dns_index;

/**
 * The callback used to report results from the host resolution to the requesting layer.
 *
 * @warning The values of aAddress and aIndex are only valid if aError equals CA_ERROR_SUCCESS
 *
 * @param aError   The status of the host resolution
 *                   - CA_ERROR_SUCCESS: Successful host resolution, result in aAddress
 *                   - CA_ERROR_TIMEOUT: Host resolution failed due to no response from any server (Do you have network connectivity?)
 *                   - CA_ERROR_NOT_FOUND: Received an empty DNS response, hostname could not be resolved
 *                   - CA_ERROR_FAIL: Miscellaneous failure
 * @param aAddress The IPv6 address resolved, only valid if aError is CA_ERROR_SUCCESS
 * @param aIndex   The DNS server that was used to obtain the response, only valid if aError is CA_ERROR_SUCCESS
 * @param aContext The context pointer that was provided to the request function
 */
typedef void (*dns_callback)(ca_error aError, const otIp6Address *aAddress, dns_index aIndex, void *aContext);

/**
 * Initialise the DNS utilities
 * @param aInstance Initialised openthread instance.
 */
void DNS_Init(otInstance *aInstance);

/**
 * Resolve a hostname to an IPv6 address, running DNS queries if necessary.
 * This request is non-blocking, and results will be provided via callback.
 *
 * @param aInstance  The initialised OpenThread instance to use.
 * @param host       Host string - can be an IPv6 address, an IPv4 address, or a hostname to be resolved by DNS.
 * @param aCallback  Callback to be called when the resolution is complete (if this function returns CA_ERROR_SUCCESS)
 * @param aContext   Context to provide to the callback
 *
 * @return ca-error value for request status
 * @retval CA_ERROR_SUCCESS       Request successful (Callback will be called with results/future error)
 * @retval CA_ERROR_NO_BUFFER     No buffer could be allocated for the internal state.
 * @retval CA_ERROR_NOT_FOUND     No configured DNS server could be found
 * @retval CA_ERROR_INVALID_ARGS  Invalid hostname
 * @retval CA_ERROR_FAIL          DNS request failed in OpenThread stack
 */
ca_error DNS_HostToIpv6(otInstance *aInstance, char *host, dns_callback aCallback, void *aContext);

/**
 * Add a DNS server with the given preference, to be used for DNS requests. Will overwrite servers with
 * lower preference if all slots are full.
 *
 * @remark If aAddress is a NAT64 address, then aUseDns64 should probably be used. If aDns64 is used, then
 * a NAT64 address should probably be used. The result of this is that if the server can be reached, then
 * there is probably valid connectivity.
 *
 * @param aAddress DNS server address
 * @param aPreference Preference value, MUST be greater than 1. Higher is more preferred. Default value is 6, but dynamically changes.
 * @param aUseDns64   Boolean parameter to force an IPv4 request which will be mapped with DNS64.
 * @retval CA_ERROR_SUCCESS DNS server successfully added
 * @retval CA_ERROR_NO_BUFFER No slots for DNS server are available with a lower preference
 */
ca_error DNS_AddServer(otIp6Address *aAddress, uint8_t aPreference, bool aUseDns64);

/**
 * Register the fact that a DNS server returned an address that could not be contacted.
 * @param aIndex The dns server index provided to the dns callback
 */
void DNS_RegisterServiceFail(dns_index aIndex);

/**
 * Register a successful use of a service resolved with DNS and increase the preference of the corresponding DNS server.
 * @param aIndex The dns server index provided to the dns callback
 */
void DNS_RegisterSuccess(dns_index aIndex);

#ifdef __cplusplus
}
#endif

#endif // CASCODA_DNS_H

/**
 * @}
 */
