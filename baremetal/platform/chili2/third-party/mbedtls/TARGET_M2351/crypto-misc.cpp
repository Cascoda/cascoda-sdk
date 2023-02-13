/* mbed Microcontroller Library
 * Copyright (c) 2017-2018 Nuvoton
 * Modifications Copyright (c) 2020 Cascoda Ltd
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

#include <limits.h>
#include <stdint.h>

#include "cascoda-bm/cascoda_os.h"
#include "ca821x_log.h"
#include "cascoda_chili.h"

#include "M2351.h"
#include "crypto-misc.h"
#include "nu_bitutil.h"

#if DEVICE_TRNG || defined(MBEDTLS_CONFIG_HW_SUPPORT)

/* Crypto (AES, DES, SHA, etc.) init counter. Crypto's keeps active as it is non-zero. */
static uint16_t crypto_init_counter = 0U;

/* Crypto done flags */
#define CRYPTO_DONE_OK BIT1  /* Done with OK */
#define CRYPTO_DONE_ERR BIT2 /* Done with error */

/* Track if AES H/W operation is done */
static volatile uint16_t crypto_aes_done;
/* Track if ECC H/W operation is done */
static volatile uint16_t crypto_ecc_done;
static uint8_t           ecc_counter = 0;

static ca_mutex aes_mutex = NULL;
static ca_mutex ecc_mutex = NULL;

static void crypto_submodule_prestart(volatile uint16_t *submodule_done);
static bool crypto_submodule_wait(volatile uint16_t *submodule_done);

static void core_util_critical_section_enter(void)
{
	CA_OS_SchedulerSuspend();
}

static void core_util_critical_section_exit(void)
{
	CA_OS_SchedulerResume();
}

/* As crypto init counter changes from 0 to 1:
 *
 * 1. Enable crypto clock
 * 2. Enable crypto interrupt
 */
void crypto_init(void)
{
	core_util_critical_section_enter();
	if (crypto_init_counter == USHRT_MAX)
	{
		core_util_critical_section_exit();
		ca_log_crit("Crypto clock enable counter would overflow (> USHRT_MAX)");
		return;
	}
	crypto_init_counter++;
	if (crypto_init_counter == 1)
	{
		/* Enable IP clock */
		CHILI_CRYPTOEnableClock();

		NVIC_EnableIRQ(CRPT_IRQn);
	}
	if (!aes_mutex)
		aes_mutex = CA_OS_MutexInit();
	if (!ecc_mutex)
		ecc_mutex = CA_OS_MutexInit();
	core_util_critical_section_exit();
}

/* As crypto init counter changes from 1 to 0:
 *
 * 1. Disable crypto interrupt
 * 2. Disable crypto clock
 */
void crypto_uninit(void)
{
	core_util_critical_section_enter();
	if (crypto_init_counter == 0)
	{
		core_util_critical_section_exit();
		ca_log_crit("Crypto clock enable counter would underflow (< 0)");
		return;
	}
	crypto_init_counter--;
	if (crypto_init_counter == 0)
	{
		NVIC_DisableIRQ(CRPT_IRQn);

		/* Disable IP clock */
		CHILI_CRYPTODisableClock();
	}
	core_util_critical_section_exit();
}

/* Implementation that should never be optimized out by the compiler */
void crypto_zeroize(void *v, size_t n)
{
	volatile unsigned char *p = (volatile unsigned char *)v;
	while (n--)
	{
		*p++ = 0;
	}
}

/* Implementation that should never be optimized out by the compiler */
void crypto_zeroize32(uint32_t *v, size_t n)
{
	volatile uint32_t *p = (volatile uint32_t *)v;
	while (n--)
	{
		*p++ = 0;
	}
}

void crypto_aes_acquire(void)
{
	CA_OS_MutexLock(&aes_mutex);
}

void crypto_aes_release(void)
{
	CA_OS_MutexUnlock(&aes_mutex);
}

void crypto_ecc_acquire(void)
{
	CA_OS_MutexLock(&ecc_mutex);
}

void crypto_ecc_release(void)
{
	CA_OS_MutexUnlock(&ecc_mutex);
}

int crypto_ecc_init(void)
{
	uint8_t rval;

	core_util_critical_section_enter();
	rval = ecc_counter++;

	if (rval == 0)
	{
		crypto_init();
		/* Enable ECC interrupt */
		ECC_ENABLE_INT(CRPT_dyn);
	}
	core_util_critical_section_exit();

	return rval;
}

int crypto_ecc_deinit(void)
{
	uint8_t rval;

	core_util_critical_section_enter();
	rval = --ecc_counter;

	if (rval == 0)
	{
		/* Disable ECC interrupt */
		ECC_DISABLE_INT(CRPT_dyn);
		crypto_uninit();
	}
	core_util_critical_section_exit();

	return rval;
}

void crypto_aes_prestart(void)
{
	crypto_submodule_prestart(&crypto_aes_done);
}

bool crypto_aes_wait(void)
{
	return crypto_submodule_wait(&crypto_aes_done);
}

void crypto_ecc_prestart(void)
{
	crypto_submodule_prestart(&crypto_ecc_done);
}

bool crypto_ecc_wait(void)
{
	return crypto_submodule_wait(&crypto_ecc_done);
}

bool crypto_dma_buff_compat(const void *buff, size_t buff_size, size_t size_aligned_to)
{
	uint32_t buff_ = (uint32_t)buff;

	return (((buff_ & 0x03) == 0) &&                      /* Word-aligned buffer base address */
	        ((buff_size & (size_aligned_to - 1)) == 0) && /* Crypto submodule dependent buffer size alignment */
#if defined(NVIC_INIT_ITNS2_VAL) && (NVIC_INIT_ITNS2_VAL & (1 << 7))
	        //Crypto nonsecure
	        (((buff_ >> 28) == 0x3) && (buff_size <= (0x40000000 - buff_)))); /* 0x30000000-0x3FFFFFFF */
#else
	        //Crypto secure
	        (((buff_ >> 28) == 0x2) && (buff_size <= (0x30000000 - buff_)))); /* 0x20000000-0x2FFFFFFF */
#endif
}

/* Overlap cases
 *
 * 1. in_buff in front of out_buff:
 *
 * in             in_end
 * |              |
 * ||||||||||||||||
 *     ||||||||||||||||
 *     |              |
 *     out            out_end
 *
 * 2. out_buff in front of in_buff:
 *
 *     in             in_end
 *     |              |
 *     ||||||||||||||||
 * ||||||||||||||||
 * |              |
 * out            out_end
 */
bool crypto_dma_buffs_overlap(const void *in_buff, size_t in_buff_size, const void *out_buff, size_t out_buff_size)
{
	uint32_t in      = (uint32_t)in_buff;
	uint32_t in_end  = in + in_buff_size;
	uint32_t out     = (uint32_t)out_buff;
	uint32_t out_end = out + out_buff_size;

	bool overlap = (in <= out && out < in_end) || (out <= in && in < out_end);

	return overlap;
}

static void crypto_submodule_prestart(volatile uint16_t *submodule_done)
{
	*submodule_done = 0;

	/* Ensure memory accesses above are completed before DMA is started
     *
     * Replacing __DSB() with __DMB() is also OK in this case.
     *
     * Refer to "multi-master systems" section with DMA in:
     * https://static.docs.arm.com/dai0321/a/DAI0321A_programming_guide_memory_barriers_for_m_profile.pdf
     */
	__DSB();
}

static bool crypto_submodule_wait(volatile uint16_t *submodule_done)
{
	while (!*submodule_done)
		;

	/* Ensure while loop above and subsequent code are not reordered */
	__DSB();

	if ((*submodule_done & CRYPTO_DONE_OK))
	{
		/* Done with OK */
		return true;
	}
	else if ((*submodule_done & CRYPTO_DONE_ERR))
	{
		/* Done with error */
		return false;
	}

	return false;
}

/* Crypto interrupt handler
 *
 * There's inconsistency in cryptography related naming, Crpt or Crypto. For example,
 * cryptography IRQ handler could be CRPT_IRQHandler or CRYPTO_IRQHandler. To override
 * default cryptography IRQ handler, see device/startup_{CHIP}.c for its correct name
 * or call NVIC_SetVector() in crypto_init() regardless of its name. */
extern "C" void CRPT_IRQHandler()
{
	uint32_t intsts;

	if ((intsts = AES_GET_INT_FLAG(CRYPTO_MODBASE())) != 0)
	{
		/* Done with OK */
		crypto_aes_done |= CRYPTO_DONE_OK;
		/* Clear interrupt flag */
		AES_CLR_INT_FLAG(CRYPTO_MODBASE());
	}
	else if ((intsts = ECC_GET_INT_FLAG(CRYPTO_MODBASE())) != 0)
	{
		/* Check interrupt flags */
		if (intsts & CRPT_INTSTS_ECCIF_Msk)
		{
			/* Done with OK */
			crypto_ecc_done |= CRYPTO_DONE_OK;
		}
		else if (intsts & CRPT_INTSTS_ECCEIF_Msk)
		{
			/* Done with error */
			crypto_ecc_done |= CRYPTO_DONE_ERR;
		}
		/* Clear interrupt flag */
		ECC_CLR_INT_FLAG(CRYPTO_MODBASE());
	}
}

#endif /* #if DEVICE_TRNG || defined(MBEDTLS_CONFIG_HW_SUPPORT) */
