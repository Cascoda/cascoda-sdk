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

/*
 * For full documentation on these options:
 * See https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html for documentation
 * and include/lwip/opt.h
 */

#ifndef THIRD_PARTY_LWIP_PORT_BM_LWIPOPTS_H_
#define THIRD_PARTY_LWIP_PORT_BM_LWIPOPTS_H_

// Core defines -------

//OS-Awareness for freertos
#define NO_SYS 0

#define TCPIP_THREAD_STACKSIZE 1024

#define TCPIP_THREAD_PRIO 5

//mbox sizes from cross-thread comms
#define TCPIP_MBOX_SIZE 32
#define DEFAULT_UDP_RECVMBOX_SIZE 6
#define DEFAULT_TCP_RECVMBOX_SIZE 6
#define DEFAULT_RAW_RECVMBOX_SIZE 6
#define DEFAULT_ACCEPTMBOX_SIZE 6

//Don't redefine timeval
#define LWIP_TIMEVAL_PRIVATE 0
//LWIP should provide errno
#define LWIP_PROVIDE_ERRNO 1

// Memory usage defines -----------

// Memory allocated to internal dynamic pool
#ifndef MEM_SIZE
#define MEM_SIZE 2000
#endif

// Allow storing many ipv6 addresses
#define LWIP_IPV6_NUM_ADDRESSES 7

// 4 byte alignment
#define MEM_ALIGNMENT 4

// Max number of simultaneous outgoing ARP requests
#define MEMP_NUM_ARP_QUEUE 2

//Max number of simultaneous active TCP connections
#define MEMP_NUM_TCP_PCB 4

//Max number of simultaneous listening TCP connections
#define MEMP_NUM_TCP_PCB_LISTEN 4

// Constrained & single interface

#define LWIP_SINGLE_NETIF 1

// Module enables ------------------

#define LWIP_DNS 1
#define LWIP_UDP 1
#define LWIP_UDPLITE 0

//TCP
#define LWIP_TCP 1
#define LWIP_EVENT_API 0
#define LWIP_CALLBACK_API 1
#define TCP_MSS 200

#define LWIP_IPV4 0

#define LWIP_IPV6 1
#define LWIP_IPV6_SEND_ROUTER_SOLICIT 0
#define LWIP_IPV6_MLD 0
#define LWIP_ND6_ALLOW_RA_UPDATES 0
#define LWIP_IPV6_DHCP6 1

#define LWIP_ALTCP 1
#define LWIP_ALTCP_TLS 1
#define LWIP_ALTCP_TLS_MBEDTLS 1

#endif /* THIRD_PARTY_LWIP_PORT_BM_LWIPOPTS_H_ */
