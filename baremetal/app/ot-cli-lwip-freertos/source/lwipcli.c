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

#include "lwip/api.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"

#include "lwip-port.h"

#include "lwip_freertos_demo.h"

enum
{
	DEMO_PORT   = 51700,
	CLI_MAXARGS = 32,
	CLI_MAXLEN  = 256
};

struct CliCommand
{
	size_t argc;
	char * argv[CLI_MAXARGS];
	char   buf[CLI_MAXLEN];
};

static int           demo_osocket          = -1;
static volatile bool demo_socket_connected = false;
static otCliCommand  sOtCliCommand;

static SemaphoreHandle_t sCliMutex;
static TaskHandle_t      sAppTask;
static struct CliCommand sCliCmd;

/**
 * Handle a new incoming TCP connection on the demosocket thread. Print all received TCP messages.
 * @param sock socket handle
 */
static void handle_connection(int sock)
{
	char buffer[33] = {0};
	int  len;

	COMMS_LOCKED(otCliOutputFormat("Incoming TCP Connection opened\r\n"));
	demo_socket_connected = true;

	while ((len = recv(sock, &buffer, sizeof(buffer) - 1, 0)) > 0)
	{
		buffer[len] = '\0';
		LockCommsThread();
		ca_log_note("Received TCP Data");
		otCliOutputFormat("TCP In: %s\r\n", buffer);
		UnlockCommsThread();
	}

	COMMS_LOCKED(otCliOutputFormat("TCP Connection Closed by client\r\n"));

	// Connection closed by client - clean up
	demo_socket_connected = false;
	close(sock);
}

/**
 * A incoming socket thread for handling incoming connections using the socket API.
 * @param arg not used
 */
static void demosocket_thread(void *arg)
{
	int                 sock = -1;
	struct sockaddr_in6 addr = {};
	int                 err;

	(void)arg;

	// Create a new connection
	sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

	// Bind connection to well known port number and listen on it
	addr.sin6_len    = sizeof(addr);
	addr.sin6_family = AF_INET6;
	addr.sin6_port   = htons(DEMO_PORT);
	bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	listen(sock, 1);

	while (1)
	{
		//Wait for new incoming connection
		int newsock = accept(sock, NULL, NULL);

		//Process new connection if formed
		if (newsock != -1)
		{
			handle_connection(newsock);
		}
	}
}

/**
 * Handle lwip CLI command using the socket api.
 * @param argc argcount
 * @param argv argvector
 */
void handle_cli_lwipdemo(int argc, char *argv[])
{
	if (argc <= 0)
	{
		LockCommsThread();
		otCliOutputFormat("lwip - print help\r\n");
		otCliOutputFormat("lwip tcp - print status\r\n");
		otCliOutputFormat("lwip tcp con <ip> - open tcp connection to port %d\r\n", 51700);
		otCliOutputFormat("lwip tcp sen <msg> - send text string over tcp connection\r\n");
		otCliOutputFormat("lwip tcp clo - close tcp connection\r\n");
		otCliOutputFormat("lwip dns <hostname> - Resolve IP address of hostname using DNS\r\n");
		otCliOutputFormat("lwip dns ser <ipv6 addr> - Set the IPv6 address of the DNS server\r\n");
		UnlockCommsThread();
		return;
	}

	if (strcmp(argv[0], "tcp") == 0)
	{
		if (argc <= 1)
		{
			LockCommsThread();
			otCliOutputFormat("incoming socket ");
			if (demo_socket_connected)
				otCliOutputFormat("connected\r\n");
			else
				otCliOutputFormat("listening\r\n");

			otCliOutputFormat("outgoing socket ");
			if (demo_osocket == -1)
				otCliOutputFormat("dis");
			otCliOutputFormat("connected\r\n");
			UnlockCommsThread();

			return;
		}
		//strncmp is used so 'con', 'connect' etc are all accepted
		if (strncmp(argv[1], "con", 3) == 0)
		{
			struct sockaddr_in6 daddr = {};

			daddr.sin6_len    = sizeof(daddr);
			daddr.sin6_family = AF_INET6;
			daddr.sin6_port   = htons(DEMO_PORT);

			if (argc < 3)
			{
				COMMS_LOCKED(otCliOutputFormat("'tcp con' expects IP address argument\r\n"));
				return;
			}
			if (demo_osocket != -1)
			{
				COMMS_LOCKED(otCliOutputFormat("CLI demo program only supports single outgoing TCP connection\r\n"));
				return;
			}
			if (inet_pton(AF_INET6, argv[2], &(daddr.sin6_addr)) != 1)
			{
				COMMS_LOCKED(otCliOutputFormat("Invalid IP address\r\n"));
				return;
			}
			if ((demo_osocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
			{
				COMMS_LOCKED(otCliOutputFormat("Failed to allocate TCP Socket\r\n"));
				return;
			}
			if (connect(demo_osocket, (struct sockaddr *)&daddr, sizeof(daddr)) == -1)
			{
				COMMS_LOCKED(otCliOutputFormat("Failed to open outgoing TCP connection\r\n"));
				return;
			}
			else
			{
				COMMS_LOCKED(otCliOutputFormat("Outgoing TCP Connection opened\r\n"));
			}
		}
		//strncmp is used so 'sen', 'send' etc are all accepted
		else if (strncmp(argv[1], "sen", 3) == 0)
		{
			int err = 0;
			if (demo_osocket == -1)
			{
				COMMS_LOCKED(otCliOutputFormat("Failed to send: Not connected\r\n"));
				return;
			}

			for (int i = 2; i < argc; i++)
			{
				u8_t flags  = TCP_WRITE_FLAG_COPY;
				bool isLast = (i == argc - 1);

				err = send(demo_osocket, argv[i], strlen(argv[i]), MSG_MORE);

				if (err == -1)
					break;

				if (isLast)
					err = send(demo_osocket, "\0", 1, 0);
				else
					err = send(demo_osocket, " ", 1, MSG_MORE);

				if (err == -1)
					break;
			}
			if (err == -1)
			{
				COMMS_LOCKED(otCliOutputFormat("Terminated outgoing TCP Socket\r\n"));
				close(demo_osocket);
				demo_osocket = -1;
			}
		}
		//strncmp is used so 'clo', 'close' etc are all accepted
		else if (strncmp(argv[1], "clo", 3) == 0)
		{
			int err      = close(demo_osocket);
			demo_osocket = -1;

			if (err == -1)
				COMMS_LOCKED(otCliOutputFormat("Failed to close TCP Socket\r\n"));
			else
				COMMS_LOCKED(otCliOutputFormat("Closed outgoing TCP Socket\r\n"));
		}
	}
	else if (strcmp(argv[0], "dns") == 0)
	{
		if (argc == 2)
		{
			struct addrinfo  ai_in  = {};
			struct addrinfo *ai_out = NULL;
			int              rval   = 0;

			ai_in.ai_family = AF_INET6;
			ai_in.ai_flags  = (AI_V4MAPPED | AI_ADDRCONFIG);

			rval = getaddrinfo(argv[1], NULL, &ai_in, &ai_out);

			if (rval == 0)
			{
				COMMS_LOCKED(otCliOutputFormat("DNS success, results:\r\n"));
				for (struct addrinfo *cur_ai = ai_out; cur_ai; cur_ai = cur_ai->ai_next)
				{
					if (cur_ai->ai_addr && cur_ai->ai_family == AF_INET6)
					{
						char                 buf[INET6_ADDRSTRLEN + 1] = {};
						struct sockaddr_in6 *addr6                     = (struct sockaddr_in6 *)cur_ai->ai_addr;

						inet_ntop(AF_INET6, &(addr6->sin6_addr), buf, sizeof(buf));
						COMMS_LOCKED(otCliOutputFormat("   %s\r\n", buf));
					}
				}
			}

			else
			{
				COMMS_LOCKED(otCliOutputFormat("Failed to resolve dns query\r\n"));
			}

			freeaddrinfo(ai_out);
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
				LOCK_TCPIP_CORE();
				dns_setserver(0, &dnsServer);
				UNLOCK_TCPIP_CORE();
				COMMS_LOCKED(otCliOutputFormat("Done\r\n"));
			}
			else
			{
				COMMS_LOCKED(otCliOutputFormat("Invalid ip\r\n"));
			}
		}
		else
		{
			COMMS_LOCKED(otCliOutputFormat("'lwip dns' parse error\r\n"));
		}
	}
}

/**
 * This is the application thread function. It is responsible for handling the lwip CLI commands.
 * It simply waits until arguments are passed to it from the Comms thread, and handles them
 * accordingly using the socket API.
 * @param arg
 */
static void app_thread(void *arg)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		xSemaphoreTake(sCliMutex, portMAX_DELAY);
		handle_cli_lwipdemo(sCliCmd.argc, sCliCmd.argv);
		xSemaphoreGive(sCliMutex);
	}
}

/**
 * The cli command is handled in the Comms Thread, which is not compatible with the socket api.
 * Therefore we pass the command to the app thread, which handles requesting the transmission.
 * @param argc
 * @param argv
 */
static void handle_cli_copy(int argc, char *argv[])
{
	static char * curPos  = &sCliCmd.buf[0];
	static size_t bufLeft = sizeof(sCliCmd.buf);

	xSemaphoreTake(sCliMutex, portMAX_DELAY);

	sCliCmd.argc = argc;
	for (int i = 0; i < argc; i++)
	{
		size_t arglen = strlen(argv[i]) + 1;

		if (arglen > bufLeft)
		{
			ca_log_warn("CLI command too long!");
			break;
		}

		sCliCmd.argv[i] = curPos;
		memcpy(sCliCmd.argv[i], argv[i], arglen);
		bufLeft -= arglen;
		curPos += arglen;
	}

	xSemaphoreGive(sCliMutex);
	xTaskNotifyGive(sAppTask);
}

ca_error init_lwipdemo(otInstance *aInstance, struct ca821x_dev *pDeviceRef)
{
	sCliMutex = xSemaphoreCreateMutex();

	xTaskCreate(demosocket_thread, "DemoSock", 1024, NULL, 2, NULL);
	xTaskCreate(app_thread, "DemoApp", 1024, NULL, 2, &sAppTask);

	sOtCliCommand.mCommand = handle_cli_copy;
	sOtCliCommand.mName    = "lwip";
	otCliSetUserCommands(&sOtCliCommand, 1);

	return CA_ERROR_SUCCESS;
}
