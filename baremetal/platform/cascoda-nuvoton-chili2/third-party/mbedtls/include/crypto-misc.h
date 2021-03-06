/* mbed Microcontroller Library
 * Copyright (c) 2017-2018 Nuvoton
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MBED_CRYPTO_MISC_H
#define MBED_CRYPTO_MISC_H

#include <stdbool.h>
#include "M2351.h"
#include "partition_M2351.h"

/* The pointer to the correct (secure or non-secure) CRPT module is determined at link-time. This declaration
 * is how the symbol gets passed along to MBEDTLS. The alternative to this is moving MBEDTLS higher up
 * in the dependency chain, near the BSP, or finding a way to inject the partition file from the secure
 * binaries into the library.
 */
extern CRPT_T *CRPT_dyn;

#define MBEDTLS_CONFIG_HW_SUPPORT

/* Policy for configuring secure attribute of CRYPTO/CRPT module:
 *
 * There's only one CRYPTO/CRPT module and we have the following policy for configuring its secure attribute:
 *
 * 1. TRNG or mbedtls H/W support can be enabled on either secure target or non-secure target, but not both.
 * 2. TRNG and mbedtls H/W supports cannot be enabled on different targets.
 * 3. On secure target, if TRNG or mbedtls H/W support is enabled, CRYPTO/CRPT must configure to secure.
 * 4. On non-secure target, if TRNG or mbedtls H/W support is enabled, CRYPTO/CRPT must configure to non-secure.
 */
#if DEVICE_TRNG || defined(MBEDTLS_CONFIG_HW_SUPPORT)
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#if defined(SCU_INIT_PNSSET1_VAL) && (SCU_INIT_PNSSET1_VAL & (1 << 18))
#error("CRYPTO/CRPT must configure to secure for secure target which supports TRNG or mbedtls H/W")
#endif
#else
#if (!defined(SCU_INIT_PNSSET1_VAL)) || (!(SCU_INIT_PNSSET1_VAL & (1 << 18)))
//            #error("CRYPTO/CRPT must configure to non-secure for non-secure target which supports TRNG or mbedtls H/W")
#endif
#endif
#endif

#if DEVICE_TRNG || defined(MBEDTLS_CONFIG_HW_SUPPORT)

#ifdef __cplusplus
extern "C" {
#endif

/* Get Crypto module base dependent on security state */
__STATIC_INLINE CRPT_T *CRYPTO_MODBASE(void)
{
	return CRPT_dyn;
}

/** Platform calls these to force linker to include mbedtls plat objects */
void targetm2351_aes_register(void);
void targetm2351_ecp_register(void);
void targetm2351_hwpoll_register(void);

/* Init/Uninit crypto module */
void crypto_init(void);
void crypto_uninit(void);

/* Clear buffer to zero
 * Implementation that should never be optimized out by the compiler */
void crypto_zeroize(void *v, size_t n);
void crypto_zeroize32(uint32_t *v, size_t n);

/* Acquire/release ownership of crypto sub-module
 *
 * \note            "acquire" is blocking until ownership is acquired
 *
 * \note            "acquire"/"release" must be paired.
 *
 * \note            Recursive "acquire" is not allowed because the underlying synchronization
 *                  primitive mutex does not support it.
 */
void crypto_aes_acquire(void);
void crypto_aes_release(void);
void crypto_ecc_acquire(void);
void crypto_ecc_release(void);

/**
 * Recursive counter for ecc initialisation
 * @return
 */
int crypto_ecc_init(void);
int crypto_ecc_deinit(void);

/* Flow control between crypto/xxx start and crypto/xxx ISR
 *
 * crypto_xxx_prestart/crypto_xxx_wait encapsulate control flow between crypto/xxx start and crypto/xxx ISR.
 *
 * crypto_xxx_prestart will also address synchronization issue with memory barrier instruction.
 *
 * On finish, return of crypto_xxx_wait indicates success or not:
 *   true if successful
 *   false if failed
 *
 * Example: Start AES H/W and wait for its finish
 *   crypto_aes_prestart();
 *   AES_Start();
 *   crypto_aes_wait();
 */
void crypto_prng_prestart(void);
bool crypto_prng_wait(void);
void crypto_aes_prestart(void);
bool crypto_aes_wait(void);
void crypto_ecc_prestart(void);
bool crypto_ecc_wait(void);

/* Check if buffer can be used for crypto DMA. It has the following requirements:
 * (1) Word-aligned buffer base address
 * (2) Crypto submodule (AES, DES, SHA, etc.) dependent buffer size alignment. Must be 2 power.
 * (3) Located in 0x20000000-0x2FFFFFFF region
 */
bool crypto_dma_buff_compat(const void *buff, size_t buff_size, size_t size_aligned_to);

/* Check if input/output buffers are overlapped */
bool crypto_dma_buffs_overlap(const void *in_buff, size_t in_buff_size, const void *out_buff, size_t out_buff_size);

#ifdef __cplusplus
}
#endif

#endif /* #if DEVICE_TRNG || defined(MBEDTLS_CONFIG_HW_SUPPORT) */

#endif
