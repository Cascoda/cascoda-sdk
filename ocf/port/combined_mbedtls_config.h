#ifndef MBEDTLS_COMBINED_CONFIG_H
#define MBEDTLS_COMBINED_CONFIG_H

// Iotivity's configuration lives in the patched mbedTLS config file.
#include <mbedtls/config.h>
#undef MBEDTLS_PLATFORM_EXIT_ALT
#undef MBEDTLS_PLATFORM_STD_EXIT

// OpenThread's configuration comes next. It has been adapted for compatibility with Iotivity.

#include <stdio.h>
#include <stdlib.h>

#include <openthread/config.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/memory.h>

#define MBEDTLS_PLATFORM_SNPRINTF_MACRO snprintf

#define MBEDTLS_AES_C
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CCM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CMAC_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECJPAKE_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED
#define MBEDTLS_MD_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_SMALLER
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_DTLS_ANTI_REPLAY
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY
#define MBEDTLS_SSL_EXPORT_KEYS
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_SSL_TLS_C

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_SSL_SRV_C
#endif

//#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
//#endif

#define MBEDTLS_BASE64_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C

#if OPENTHREAD_ENABLE_ECDSA
#define MBEDTLS_BASE64_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#endif

#define MBEDTLS_MPI_WINDOW_SIZE 1       /**< Maximum windows size used. */
#define MBEDTLS_MPI_MAX_SIZE 32         /**< Maximum number of bytes for usable MPIs. */
#define MBEDTLS_ECP_MAX_BITS 256        /**< Maximum bit size of groups */
#define MBEDTLS_ECP_WINDOW_SIZE 2       /**< Maximum window size used */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM 0 /**< Enable fixed-point speed-up */

#undef MBEDTLS_FS_IO

#undef MBEDTLS_MAX_ENTROPY_SOURCES
#define MBEDTLS_MAX_ENTROPY_SOURCES 3

#define MBEDTLS_SSL_CIPHERSUITES MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8

#if defined(MBEDTLS_USER_CONFIG_FILE)
#include MBEDTLS_USER_CONFIG_FILE
#endif

#if defined(MBEDTLS_ECP_ALT) && !defined(MBEDTLS_ECP_RESTARTABLE)
typedef void mbedtls_ecp_restart_ctx;
#endif

// Cascoda additions
#define MBEDTLS_PKCS5_C
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#define MBEDTLS_ERROR_C
#define MBEDTLS_DEBUG_C

#undef MBEDTLS_SSL_MAX_CONTENT_LEN
#define MBEDTLS_SSL_MAX_CONTENT_LEN (1700)

#define MBEDTLS_ECDSA_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_PEM_WRITE_C
#define MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE
#define MBEDTLS_X509_CSR_WRITE_C
#define MBEDTLS_X509_CREATE_C

#include "mbedtls/check_config.h"

#endif // MBEDTLS_CONFIG_H
