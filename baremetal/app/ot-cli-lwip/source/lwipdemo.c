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

/******************************************************************************/
/****** Openthread cli demo for LWIP                                         **/
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"

#include "openthread/cli.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/thread.h"
#include "platform.h"

#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/tcp.h"

#include "lwip-port.h"

enum
{
	DEMO_PORT = 51700,
};

static struct tcp_pcb *demo_isocket = NULL;
static struct tcp_pcb *demo_osocket = NULL;

/**
 * Callback triggered when TCP socket is connected.
 * @param arg unused
 * @param tpcb a pointer to the relevant socket
 * @param err the status of the connection
 * @return ERR_OK
 */
err_t demo_tcpconnected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	if (err != ERR_OK || !tpcb || tpcb != demo_osocket)
		return ERR_OK;

	otCliOutputFormat("Outgoing TCP Connection opened\r\n");
	return ERR_OK;
}

/**
 * Callback triggered when the TCP socket is disconnected unexpectedly
 * @param arg
 * @param err
 */
void demo_tcperr(void *arg, err_t err)
{
	otCliOutputFormat("Outgoing TCP Connection terminated\r\n");
	demo_osocket = NULL;
}

/**
 * Callback triggered when data is received on a TCP socket
 * @param arg unused
 * @param tpcb A pointer to the relevant socket
 * @param p A buffer containing the received data
 * @param err The status of the TCP connection
 * @return ERR_OK
 */
static err_t demo_tcpreceive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	if (err == ERR_OK && p)
	{
		struct pbuf *curp = p;
		ca_log_note("Received TCP Data");
		tcp_recved(tpcb, p->tot_len);

		//The buffer may be split into chunks, so iterate through them and print.
		while (curp)
		{
			otCliOutputFormat("TCP In: %.*s\r\n", curp->len, (char *)curp->payload);
			curp = curp->next;
		}
	}
	else
	{
		otCliOutputFormat("TCP Connection Closed by client\r\n");
	}

	if (p)
		pbuf_free(p);

	return ERR_OK;
}

/**
 * A callback triggered when a new incoming TCP connection is accepted
 * @param arg unused
 * @param newpcb a pointer to the relevant socket
 * @param err The status of the the TCP accept
 * @return ERR_OK if successful, ERR_VAL if invalid
 */
static err_t demo_tcpaccept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	struct tcpecho_raw_state *es;

	if (err != ERR_OK || !newpcb)
		return ERR_VAL;

	otCliOutputFormat("Incoming TCP Connection opened\r\n");
	tcp_setprio(newpcb, TCP_PRIO_MIN);
	tcp_recv(newpcb, &demo_tcpreceive);

	return ERR_OK;
}

/**
 * A callback triggered when a dns query is completed
 * @param name pointer to the name that was looked up.
 * @param ipaddr pointer to an ip_addr_t containing the IP address of the hostname,
 *        or NULL if the name could not be found (or on any other error).
 * @param callback_arg a user-specified callback argument passed to dns_gethostbyname
 */
void demo_dnsfound(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
	(void)callback_arg;

	if (ipaddr)
	{
		char buf[IP6ADDR_STRLEN_MAX + 1] = {};

		ipaddr_ntoa_r(ipaddr, buf, sizeof(buf));
		otCliOutputFormat("DNS success, result: %s\r\n", buf);
	}
	else
	{
		otCliOutputFormat("Failed to resolve dns query\r\n");
	}
}

/**
 * Initialise the listening TCP socket.
 * @return The state of the initialisation
 */
static ca_error init_demosocket()
{
	ca_error error = CA_ERROR_SUCCESS;

	if ((demo_isocket = tcp_new_ip_type(IPADDR_TYPE_V6)))
	{
		err_t lerror = ERR_OK;

		lerror = tcp_bind(demo_isocket, IP6_ADDR_ANY, DEMO_PORT);

		if (lerror)
		{
			error = CA_ERROR_INVALID_STATE;
			goto exit;
		}

		demo_isocket = tcp_listen(demo_isocket);
		tcp_accept(demo_isocket, demo_tcpaccept);
	}
	else
	{
		error = CA_ERROR_NO_BUFFER;
	}
exit:
	return error;
}

/**
 * Handle the lwip cli command from the openthread CLI using the lwip callback api.
 * @param argc arg count
 * @param argv arg vector
 */
void handle_cli_lwipdemo(int argc, char *argv[])
{
	if (argc <= 0)
	{
		otCliOutputFormat("lwip - print help\r\n");
		otCliOutputFormat("lwip tcp - print status\r\n");
		otCliOutputFormat("lwip tcp con <ip> - open tcp connection to port %d\r\n", 51700);
		otCliOutputFormat("lwip tcp sen <msg> - send text string over tcp connection\r\n");
		otCliOutputFormat("lwip tcp clo - close tcp connection\r\n");
		otCliOutputFormat("lwip dns <hostname> - Resolve IP address of hostname using DNS\r\n");
		otCliOutputFormat("lwip dns ser <ipv6 addr> - Set the IPv6 address of the DNS server\r\n");
		return;
	}

	if (strcmp(argv[0], "tcp") == 0)
	{
		if (argc <= 1)
		{
			otCliOutputFormat("incoming socket ");
			if (demo_isocket)
				otCliOutputFormat("connected\r\n");
			else
				otCliOutputFormat("listening\r\n");

			otCliOutputFormat("outgoing connection socket ");
			if (!demo_osocket)
				otCliOutputFormat("dis");
			otCliOutputFormat("connected\r\n");

			return;
		}
		//strncmp is used so 'con', 'connect' etc are all accepted
		if (strncmp(argv[1], "con", 3) == 0)
		{
			ip_addr_t daddr;
			if (argc < 3)
			{
				otCliOutputFormat("'tcp con' expects IP address argument\r\n");
				return;
			}
			if (demo_osocket)
			{
				otCliOutputFormat("CLI demo program only supports single outgoing TCP connection\r\n");
				return;
			}
			if (!ipaddr_aton(argv[2], &daddr))
			{
				otCliOutputFormat("Invalid IP address\r\n");
				return;
			}
			if (!(demo_osocket = tcp_new_ip_type(IPADDR_TYPE_V6)))
			{
				otCliOutputFormat("Failed to allocate TCP Socket\r\n");
				return;
			}
			tcp_connect(demo_osocket, &daddr, DEMO_PORT, &demo_tcpconnected);
			tcp_err(demo_osocket, &demo_tcperr);
		}
		//strncmp is used so 'sen', 'send' etc are all accepted
		else if (strncmp(argv[1], "sen", 3) == 0)
		{
			for (int i = 2; i < argc; i++)
			{
				err_t err    = ERR_OK;
				u8_t  flags  = TCP_WRITE_FLAG_COPY;
				bool  isLast = (i == argc - 1);

				err = tcp_write(demo_osocket, argv[i], strlen(argv[i]), TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);

				if (isLast)
					err = tcp_write(demo_osocket, "\0", 1, TCP_WRITE_FLAG_COPY);
				else
					err = tcp_write(demo_osocket, " ", 1, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
			}
			tcp_output(demo_osocket);
		}
		//strncmp is used so 'clo', 'close' etc are all accepted
		else if (strncmp(argv[1], "clo", 3) == 0)
		{
			err_t err    = tcp_close(demo_osocket);
			demo_osocket = NULL;

			if (err)
				otCliOutputFormat("Failed to close TCP Socket\r\n");
			else
				otCliOutputFormat("Closed outgoing TCP Socket\r\n");
		}
	}
	else if (strcmp(argv[0], "dns") == 0)
	{
		if (argc == 2)
		{
			err_t     rval = ERR_OK;
			ip_addr_t ip_in;

			rval = dns_gethostbyname(argv[1], &ip_in, &demo_dnsfound, NULL);

			if (rval == ERR_OK)
				demo_dnsfound(argv[1], &ip_in, NULL);
			else if (rval == ERR_INPROGRESS)
				otCliOutputFormat("Resolving...\r\n");
			else
				demo_dnsfound(argv[1], NULL, NULL);
		}
		//strncmp is used so 'ser', 'server' etc are all accepted
		else if (argc == 3 && strncmp(argv[1], "ser", 3) == 0)
		{
			ip_addr_t dnsServer = {};

			if (otIp6AddressFromString(argv[2], (otIp6Address *)(&dnsServer)) == OT_ERROR_NONE)
			{
#if LWIP_IPV4
				dnsServer.type = IPADDR_TYPE_V6;
#endif
				dns_setserver(0, &dnsServer);
				otCliOutputFormat("Done\r\n");
			}
			else
			{
				otCliOutputFormat("Invalid ip\r\n");
			}
		}
		else
		{
			otCliOutputFormat("'lwip dns' parse error\r\n");
		}
	}
}

ca_error init_lwipdemo(otInstance *aInstance, struct ca821x_dev *pDeviceRef)
{
	LWIP_NetifInit(aInstance);
	lwip_init();

	init_demosocket();

	return CA_ERROR_SUCCESS;
}
