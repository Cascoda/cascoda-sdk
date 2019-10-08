/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* Standard includes. */
#include <arm_cmse.h>
#include <stdio.h>

/* Device includes. */
#include "NuMicro.h"
#include "partition_M2351.h"

/* FreeRTOS includes. */
#include "secure_port_macros.h"

#define JUMP_HERE 0xe7fee7ff /* Instruction Code of "B ." */

/*
 * Start address of non-secure application. This assumes that the veneer table
 * is placed at the end of secure flash, immediately followed by the application
 */
const extern uint32_t __top_veneer_table[];
const uint32_t        mainNONSECURE_APP_START_ADDRESS = (uint32_t)__top_veneer_table;

/* typedef for NonSecure callback functions */
typedef __NONSECURE_CALL int32_t (*NonSecure_funcptr)(uint32_t);
typedef int32_t (*Secure_funcptr)(uint32_t);
/*-----------------------------------------------------------*/

void Boot_Init(uint32_t u32BootBase);

/* Secure main. */
int main(void)
{
	/* Unlock protected registers. */
	SYS_UnlockReg();

	/* Print banner. */
	printf("\n");
	printf("+---------------------------------------------+\n");
	printf("|            Secure is running ...            |\n");
	printf("+---------------------------------------------+\n");

	/* Do not generate Systick interrupt on secure side. */
	SysTick_Config(1);

	/* Lock protected registers before booting non-secure code. */
	SYS_LockReg();

	/* Boot the non-secure code. */
	printf("Entering non-secure world ...\n");
	Boot_Init(mainNONSECURE_APP_START_ADDRESS);

	/* Non-secure software does not return, this code is not executed. */
	for (;;)
	{
		/* Should not reach here. */
	}
}
/*-----------------------------------------------------------*/

/*----------------------------------------------------------------------------
    Boot_Init function is used to jump to next boot code.
 *----------------------------------------------------------------------------*/
void Boot_Init(uint32_t u32BootBase)
{
	/* Set bit 28 to indicate non secure memory. 0x1d8 is the size of the vector table */
	const uint32_t    RESET_HANDLER_ADDRESS = mainNONSECURE_APP_START_ADDRESS + 0x100001d8;
	NonSecure_funcptr fp;

	/* SCB_NS.VTOR points to the Non-secure vector table base address. */
	SCB_NS->VTOR = u32BootBase;

	/* 1st Entry in the vector table is the Non-secure Main Stack Pointer. */
	__TZ_set_MSP_NS(*((uint32_t *)SCB_NS->VTOR)); /* Set up MSP in Non-secure code */

	/* Function pointer to the non-secure reset handler */
	fp = (NonSecure_funcptr)RESET_HANDLER_ADDRESS;

	/* Check if the Reset_Handler address is in Non-secure space */
	if (cmse_is_nsfptr(fp) && (((uint32_t)fp & 0xf0000000) == 0x10000000))
	{
		printf("Execute non-secure code ...\n");
		fp(0); /* Non-secure function call */
	}
	else
	{
		/* Something went wrong */
		printf("No code in non-secure region!\n");
		printf("CPU will halted at non-secure state\n");

		/* Set nonsecure MSP in nonsecure region */
		__TZ_set_MSP_NS(NON_SECURE_SRAM_BASE + 512);

		/* Try to halted in non-secure state (SRAM) */
		M32(NON_SECURE_SRAM_BASE) = JUMP_HERE;
		fp                        = (NonSecure_funcptr)(NON_SECURE_SRAM_BASE + 1);
		fp(0);

		while (1)
			;
	}
}