/*
 *  Copyright (c) 2021, Cascoda Ltd.
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

#ifndef CASCODA_SDK_MBEDTLS_USER_CONFIG_H
#define CASCODA_SDK_MBEDTLS_USER_CONFIG_H

#if CASCODA_BUILD_SECURE_LWM2M
#if MBEDTLS_SSL_MAX_CONTENT_LEN < 1024
#undef MBEDTLS_SSL_MAX_CONTENT_LEN
#define MBEDTLS_SSL_MAX_CONTENT_LEN 1024
#endif

#ifndef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#endif

#ifndef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif

#ifndef MBEDTLS_BASE64_C
#define MBEDTLS_BASE64_C
#endif

#ifndef MBEDTLS_ECDH_C
#define MBEDTLS_ECDH_C
#endif

#ifndef MBEDTLS_ECDSA_C
#define MBEDTLS_ECDSA_C
#endif

#ifndef MBEDTLS_OID_C
#define MBEDTLS_OID_C
#endif

#ifndef MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_PARSE_C
#endif

#ifndef MBEDTLS_X509_USE_C
#define MBEDTLS_X509_USE_C
#endif

#ifndef MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#endif

#ifndef MBEDTLS_X509_USE_C
#define MBEDTLS_X509_USE_C
#endif

#ifndef MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#endif
#endif //CASCODA_BUILD_SECURE_LWM2M

#if CASCODA_BUILD_LWIP
#ifndef MBEDTLS_OID_C
#define MBEDTLS_OID_C
#endif

#ifndef MBEDTLS_X509_USE_C
#define MBEDTLS_X509_USE_C
#endif

#ifndef MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#endif
#endif //CASCODA_BUILD_LWIP

#if defined(MBEDTLS_DEVICE_CONFIG_FILE)
#include MBEDTLS_DEVICE_CONFIG_FILE
#endif

#endif //CASCODA_SDK_MBEDTLS_USER_CONFIG_H
