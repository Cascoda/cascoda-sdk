/*
 * Copyright (c) 2020, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Host resolution:
// 1. IPv6 string -> ipv6 addr -> SuccessOrFail
// 2. IPv4 string -> nat64 ipv6 addr -> SuccessOrFail
// 3.a Select DNS server based on preference val. (using forced DNS64 if configured)
// 3.b If use fails, try a new DNS server

// There are 3 possible states of connectivity to consider.
// 1. No connectivity to internet
// 2. Full IPv6 connectivity to the internet (and probably/possibly NAT64)
// 3. IPv4 connectivity to the internet via NAT64

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "openthread/dns.h"

#include "ca-ot-util/cascoda_dns.h"
#include "ca821x_log.h"

#define NAT64_PREFIX "64:ff9b::"
#define MAX_IPV4 "255.255.255.255"
#define DNS_INDEX_INIT ((dns_index)NULL)

enum
{
	DNS_SERVER_COUNT = 2,   //!< Number of DNS servers to remember at a time
	DNS_PREF_MIN     = 1,   //!< Minimum value for DNS preference (also set for timeout)
	DNS_PREF_MAX     = 255, //!< Maximum value for DNS preference
	DNS_PREF_BASE    = 6,   //!< Base value for DNS preference
	DNS_PREF_BONUS   = 1,   //!< Value to add to DNS preference upon success
	DNS_PREF_PENALTY = 4,   //!< Value to remove from DNS preference upon service fail (if it is above base)
};

/**
 * DNS server entry for managed list of DNS servers
 */
struct dnsServer
{
	otIp6Address addr;       //!< IPv6 address of DNS server
	uint8_t      preference; //!< Preference value, 0 is never used, higher is higher priority
	bool         useDns64;   //!< True if an A request should be made instead of AAAA, and converted to IPv6 with DNS64
};

/**
 * Dynamically allocated context structure to pass to the DNS subsystem as a context.
 */
struct dns_context
{
	dns_index    server;  //!< Index of the DNS server used
	void *       context; //!< User context to be provided to the callback
	dns_callback callback;
	char         hostname[]; //!< Variable length hostname
};

static struct dnsServer dns_servers[DNS_SERVER_COUNT];

static ca_error dns_query_next_server(otInstance *aInstance, struct dns_context *aContext);

/**
 * Get a DNS server, iteratively (internal func)
 * @param[out] aAddrOut Pointer to store IPv6 address of DNS server if return value is CA_ERROR_SUCCESS
 * @param[out] aIndexOut Pointer to store index of DNS server, or previous (failed) server attempted.
 *                         Pointer destination should have value DNS_INDEX_INIT on first call.
 * @retval CA_ERROR_SUCCESS DNS server found and aAddrOut is valid
 * @retval CA_ERROR_NOT_FOUND good DNS server could not be found
 */
static ca_error dns_get_server(otIp6Address *aAddrOut, dns_index *aIndexOut)
{
	ca_error error     = CA_ERROR_NOT_FOUND;
	uint8_t  best_pref = 0;
	int      best      = 0;

	for (int i = 0; i < DNS_SERVER_COUNT; i++)
	{
		if (dns_servers[i].preference > best_pref)
		{
			best      = i;
			best_pref = dns_servers[i].preference;
			error     = CA_ERROR_SUCCESS;
		}
	}

	if (error == CA_ERROR_SUCCESS)
	{
		memcpy(aAddrOut, &dns_servers[best].addr, sizeof(*aAddrOut));
		*aIndexOut = &dns_servers[best];
	}
	return error;
}

/**
 * If every destination has timed out, we may not have internet connectivity, this function restores preference values so priority works in future
 */
static void dns_full_timeout_recovery()
{
	for (int i = 0; i < DNS_SERVER_COUNT; i++)
	{
		if (dns_servers[i].preference > DNS_PREF_MIN)
			return;
	}

	//All servers are timed out, restore preference values
	for (int i = 0; i < DNS_SERVER_COUNT; i++)
	{
		if (dns_servers[i].preference == DNS_PREF_MIN)
			dns_servers[i].preference = DNS_PREF_BASE;
	}
}

/**
 * Register the fact that a DNS server timed out and reduce its priority
 * @param aAddr The IPv6 address of the DNS server
 */
static void dns_register_timeout(dns_index aIndex)
{
	if (aIndex->preference >= DNS_PREF_MIN)
	{
		aIndex->preference = DNS_PREF_MIN;
		dns_full_timeout_recovery();
	}
}

/**
 * Handle the DNS callback from the openthread stack
 * @param aContext  dns_context pointer, must be freed in here
 * @param aHostname host name string
 * @param aAddress  address that has been resolved
 * @param aTtl      DNS ttl for caching
 * @param aResult   openthread error code
 */
static void dns_handle_response(void *              aContext,
                                const char *        aHostname,
                                const otIp6Address *aAddress,
                                uint32_t            aTtl,
                                otError             aResult)
{
	struct dns_context *context = aContext;

	(void)aHostname;
	(void)aTtl;

	ca_log_debg("DNS Response error %s", otThreadErrorToString(aResult));

	if (aResult == OT_ERROR_NONE)
	{
		DNS_RegisterSuccess(context->server);
		context->callback(CA_ERROR_SUCCESS, aAddress, context->server, context->context);
	}
	else if (aResult == OT_ERROR_RESPONSE_TIMEOUT)
	{
		dns_register_timeout(context->server);
		//TODO: Consider auto-retrying with a new server upon timeout
		context->callback(CA_ERROR_TIMEOUT, NULL, context->server, context->context);
	}
	else
	{
		DNS_RegisterServiceFail(context->server);
		context->callback(CA_ERROR_NOT_FOUND, NULL, context->server, context->context);
	}

	free(context);
}

/**
 * Send a DNS query to the next highest priority DNS server in the list
 * @param aInstance Initialised OpenThread instance
 * @param aContext  The dns_context for the query
 *
 * @return ca_error value
 * @retval CA_ERROR_SUCCESS Query successfully transmitted
 * @retval CA_ERROR_FAIL    Internal error
 */
static ca_error dns_query_next_server(otInstance *aInstance, struct dns_context *aContext)
{
	ca_error      error = CA_ERROR_SUCCESS;
	otDnsQuery    dnsQuery;
	otMessageInfo messageInfo;
	otError       oterr;
	bool          dns64;

	memset(&dnsQuery, 0, sizeof(dnsQuery));
	memset(&messageInfo, 0, sizeof(messageInfo));

	dnsQuery.mMessageInfo = &messageInfo;
	messageInfo.mPeerPort = 53;
	error                 = dns_get_server(&(messageInfo.mPeerAddr), &(aContext->server));
	if (error)
		goto exit;

	dns64              = aContext->server->useDns64;
	dnsQuery.mHostname = aContext->hostname;
	ca_log_debg("Sending %s request to DNS server %d", dns64 ? "DNS64" : "DNS", aContext->server - dns_servers);
	if (aContext->server->useDns64)
		oterr = otDnsClientQueryDNS64(aInstance, &dnsQuery, &dns_handle_response, aContext);
	else
		oterr = otDnsClientQuery(aInstance, &dnsQuery, &dns_handle_response, aContext);

	if (oterr)
	{
		error = CA_ERROR_FAIL;
		goto exit;
	}

exit:
	return error;
}

/**
 * Determine whether a hostname is valid.
 *
 * @note This is just a basic sanity check, and not a full hostname correctness check.
 *
 * @param host The hostname string to check
 *
 * @return true if hostname looks valid, false if not.
 */
static bool dns_is_valid_hostname(const char *host)
{
	bool pass = true;

	for (const char *curPtr = host; *curPtr && pass; curPtr++)
	{
		char curChar = *curPtr;

		if (curChar >= 'a' && curChar <= 'z')
			continue;

		if (curChar >= 'A' && curChar <= 'Z')
			continue;

		if (host == curPtr)
		{
			//First character must be alphabetical
			pass = false;
			break;
		}

		if (curChar >= '0' && curChar <= '9')
			continue;

		if (curChar == '.' || curChar == '-')
			continue;
	}
	return pass;
}

void DNS_RegisterServiceFail(dns_index aIndex)
{
	if (aIndex->preference >= DNS_PREF_BASE)
		aIndex->preference -= DNS_PREF_PENALTY;
}

void DNS_RegisterSuccess(dns_index aIndex)
{
	if (aIndex->preference < (DNS_PREF_MAX - DNS_PREF_BONUS))
		aIndex->preference += DNS_PREF_BONUS;
}

ca_error DNS_AddServer(otIp6Address *aAddress, uint8_t aPreference, bool aUseDns64)
{
	ca_error error    = CA_ERROR_NO_BUFFER;
	int      found    = 0;
	uint8_t  min_pref = aPreference;

	// Find the lowest preference server, and replace it if new server is higher preference
	for (int i = 0; i < DNS_SERVER_COUNT; i++)
	{
		uint8_t pref = dns_servers[i].preference;
		if (pref < min_pref)
		{
			found    = i;
			min_pref = pref;
			error    = CA_ERROR_SUCCESS;
		}
	}

	// If we found a slot, fill it.
	if (!error)
	{
		dns_servers[found].preference = aPreference;
		dns_servers[found].addr       = *aAddress;
		dns_servers[found].useDns64   = aUseDns64;
	}

	return error;
}

ca_error DNS_HostToIpv6(otInstance *aInstance, char *host, dns_callback aCallback, void *aContext)
{
	ca_error     error   = CA_ERROR_SUCCESS;
	size_t       hostlen = strlen(host);
	otIp6Address addr;

	// Host
	// Is the hostname an IPv6 address?
	if (otIp6AddressFromString(host, &addr) == OT_ERROR_NONE)
	{
		aCallback(CA_ERROR_SUCCESS, &addr, DNS_INDEX_INIT, aContext);
		goto exit;
	}

	// Is the hostname an IPv4 address? (we append it to the well-known nat64 prefix and attempt to parse it in mixed-notation)
	if (hostlen <= strlen(MAX_IPV4))
	{
		size_t nat64len = strlen(NAT64_PREFIX) + strlen(MAX_IPV4) + 1;
		char   nat64[nat64len];

		memcpy(nat64, NAT64_PREFIX, strlen(NAT64_PREFIX) + 1);
		memcpy(nat64 + strlen(NAT64_PREFIX), host, hostlen + 1);
		if (otIp6AddressFromString(nat64, &addr) == OT_ERROR_NONE)
		{
			aCallback(CA_ERROR_SUCCESS, &addr, DNS_INDEX_INIT, aContext);
			goto exit;
		}
	}

	// Is the hostname a host name? If so, we do a DNS request and wait for the response.
	if (dns_is_valid_hostname(host))
	{
		struct dns_context *context;

		context = calloc(1, sizeof(struct dns_context) + hostlen + 1);
		if (!context)
		{
			error = CA_ERROR_NO_BUFFER;
			goto exit;
		}
		memcpy(context->hostname, host, hostlen);
		context->context  = aContext;
		context->callback = aCallback;

		error = dns_query_next_server(aInstance, context);

		if (error)
			free(context);
	}
	else
	{
		error = CA_ERROR_INVALID_ARGS;
	}

exit:
	return error;
}

void DNS_Init(otInstance *aInstance)
{
	otIp6Address addr;
	(void)aInstance;

	//Cloudflare DNS64 IPv6 (true IPv6)
	otIp6AddressFromString("2606:4700:4700::64", &addr);
	DNS_AddServer(&addr, 7, false);

	//Cloudflare DNS IPv4 (with NAT64 well known prefix)
	otIp6AddressFromString(NAT64_PREFIX "1.1.1.1", &addr);
	DNS_AddServer(&addr, 6, true);

	/*
	 * TODO: Go through the network data, send DHCP requests to the P_configure BRs and ND requests to P_nd_dns BRs.
	 * However, it isn't possible to extract the ND anycast addresses from the network data on a device which only holds
	 * the stable network data.
	 * }
 */
}
