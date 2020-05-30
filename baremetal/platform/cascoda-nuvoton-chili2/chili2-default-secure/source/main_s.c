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

/* Standard includes. */
#include <arm_cmse.h>
#include <stdio.h>

/* Device includes. */
#include "NuMicro.h"
#include "partition_M2351.h"

#ifdef __NEWLIB__
#include <sys/errno.h>
#include <sys/stat.h>
#endif

/* FreeRTOS includes. */
#include "secure_port_macros.h"

/*
 * Start address of non-secure application. This assumes that the veneer table
 * is placed at the end of secure flash, immediately followed by the application
 */
const extern uint32_t __top_veneer_table[];
const uint32_t        mainNONSECURE_APP_START_ADDRESS = (uint32_t)__top_veneer_table + 0x10000000;

/* typedef for NonSecure callback functions */
typedef __NONSECURE_CALL int32_t (*NonSecure_funcptr)(uint32_t);
typedef int32_t (*Secure_funcptr)(uint32_t);
/*-----------------------------------------------------------*/

/*----------------------------------------------------------------------------
    Boot_Init function is used to jump to next boot code.
 *----------------------------------------------------------------------------*/
static void Boot_Init(void)
{
	const uint32_t *  vecTableNs = (uint32_t *)mainNONSECURE_APP_START_ADDRESS;
	NonSecure_funcptr fp;

	/* SCB_NS.VTOR points to the Non-secure vector table base address. */
	SCB_NS->VTOR = (uint32_t)vecTableNs;

	/* 1st Entry in the vector table is the Non-secure Main Stack Pointer. */
	__TZ_set_MSP_NS(vecTableNs[0]); /* Set up MSP in Non-secure code */

	/* Function pointer to the non-secure reset handler */
	fp = (NonSecure_funcptr)vecTableNs[1];

	/* Check if the Reset_Handler address is in Non-secure space */
	if ((uint32_t)fp & 0x10000000)
	{
		fp(0); /* Non-secure function call */
	}
}

__NONSECURE_ENTRY int do_nothing_entry_func(void)
{
	static volatile int x = 0;
	return x;
}

static void catch_gcc_csme_bug()
{
	//This function is a runtime check to catch a bug found in gcc when calling CSME entry functions
	//It has only been found when using the -Os command line option, and causes the hi registers to be wiped.
	//The current workaround is to make sure secure code is compiled with an optimisation option that is not -Os
	//At the time of writing, we use -O2 in the cascoda SDK to work around. See priv issue #259 for more details.

	volatile register int R8 __asm("r8");
	int                   restore = R8;

	R8 = 0xCA5C0DAA;
	do_nothing_entry_func();

	while (R8 != 0xCA5C0DAA)
		;
	R8 = restore;
}

/* Secure main. */
int main(void)
{
	/* Catch misconfiguration */
	catch_gcc_csme_bug();

	/* Boot the non-secure code. */
	Boot_Init();

	/* Non-secure software does not return, this code is not executed. */
	for (;;)
	{
		/* Should not reach here. */
	}
}
/*-----------------------------------------------------------*/

#if defined(__NEWLIB__)
int _write(int file, char *ptr, int len)
{
	(void)file;
	(void)ptr;
	(void)len;

	//We lie and just dump everything in secure mode.
	return len;
}

int _read(int file, char *ptr, int len)
{
	(void)file;
	(void)ptr;
	(void)len;

	return 0;
}

void *_sbrk(int incr)
{
	extern char __HeapBase;
	extern char __HeapLimit;

	static char *heap_end = &__HeapBase; /* Previous end of heap or 0 if none */
	char *       prev_heap;

	prev_heap = heap_end;
	heap_end += incr;

	if (heap_end >= (&__HeapLimit))
	{
		heap_end = prev_heap;
		errno    = ENOMEM;
		return (char *)-1;
	}
	return (void *)prev_heap;

} /* _sbrk () */

void _exit(int status)
{
	(void)status;
	NVIC_SystemReset();

	while (1)
		; //Should never be reached
}

#endif
