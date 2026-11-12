/*
 *  Copyright (c) 2026, Cascoda Ltd.
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
 * Cascoda Interface to Vendor BSP/Library Support Package.
 * MCU:    Nuvoton M2351
 * Brown-Out Detection (BOD)
*/
/* System */
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
/* Platform */
#include "M2351.h"
/* Cascoda */
#include "cascoda-bm/cascoda_interface.h"
#include "ca821x_api.h"
#include "cascoda_chili.h"
#include "cascoda_chili_gpio.h"

/* pointer to ISR callback function */
static int (*ModuleBODCallback)(void);

/* Minimum time spent in ISR before VDD reaches 2.1V:
 *
 * VLevel  T2.1
 * [V]     [us]
 * ------------
 * 2.6     750
 * 2.4     450
 * 2.2     100
 *
 * Note: BOD low power mode (LPM) should not be used when m2351 is in active mode,
 *       as reaction is too slow to trigger ISR before minimum voltage is reached.
 */

 /* ISR0: BOD interrupt */
void BOD_IRQHandler(void)
{
	/* clear interrupt */
	SYS_CLEAR_BOD_INT_FLAG();

	/* call callback if registered */
	if (ModuleBODCallback)
		ModuleBODCallback();
}

/* enable BOD */
void BSP_BODEnable(void)
{
	/* unlock write-protected registers */
    SYS_UnlockReg();
	/* enable BOD */
	SYS_ENABLE_BOD();
	/* disable BOD low power mode */
	SYS_CLEAR_BOD_LPM();
	/* set detector voltage level to 2.6V */
	SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_2_6V);
	/* disable BOD reset / use interrupt */
	SYS_DISABLE_BOD_RST();
	/* clear interrupt just in case */
	SYS_CLEAR_BOD_INT_FLAG();
	/* enable interrupt */
	NVIC_EnableIRQ(BOD_IRQn);
	/* re-lock write-protected registers */
    SYS_LockReg();
}

/* disable BOD */
void BSP_BODDisable(void)
{
	/* unlock write-protected registers */
    SYS_UnlockReg();
	/* disable interrupt */
	NVIC_DisableIRQ(BOD_IRQn);
	/* clear interrupt just in case */
	SYS_CLEAR_BOD_INT_FLAG();
	/* disable BOD */
	SYS_DISABLE_BOD();
	/* re-lock write-protected registers */
    SYS_LockReg();
}

/* register ISR function callback */
void BSP_BODRegisterCallback(int (*callback)(void))
{
	ModuleBODCallback = callback;
}

/* enable/disable BOD low power mode (lpm) */
void BSP_BODEnableLPM(bool enable)
{
	/* unlock write-protected registers */
    SYS_UnlockReg();
	/* set/clear lpm */
	if(enable)
		SYS_SET_BOD_LPM();
	else
		SYS_CLEAR_BOD_LPM();
	/* re-lock write-protected registers */
    SYS_LockReg();
}

/* set BOD trigger voltage level */
ca_error BSP_BODSetVoltageLevel(bod_voltage_level vlevel)
{
	ca_error status = CA_ERROR_SUCCESS;

	/* unlock write-protected registers */
    SYS_UnlockReg();

	switch(vlevel)
	{
		case BOD_VLEVEL_3_0V: SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_3_0V); break;
		case BOD_VLEVEL_2_8V: SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_2_8V); break;
		case BOD_VLEVEL_2_6V: SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_2_6V); break;
		case BOD_VLEVEL_2_4V: SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_2_4V); break;
		case BOD_VLEVEL_2_2V: SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_2_2V); break;
		case BOD_VLEVEL_2_0V: SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_2_0V); break;
		default: status = CA_ERROR_INVALID_ARGS; break;
	}

	/* re-lock write-protected registers */
    SYS_LockReg();

	return(status);
}
