/*
 *  Copyright (c) 2019, Cascoda Ltd.
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

#ifndef CASCODA_SECURE_H
#define CASCODA_SECURE_H

#include "cascoda-bm/cascoda_interface.h"

/** Set Use_external_clock flag */
void CHILI_SetUseExternalClock(u8_t use_ext_clock);

/** Get Use_external_clock flag */
u8_t CHILI_GetUseExternalClock(void);

/** Set enable_comms_interface flag, used by CHILI_SystemReinit */
void CHILI_SetEnableCommsInterface(u8_t enable_coms_interface);

/** Get enable_comms_interface flag, used by CHILI_SystemReinit */
u8_t CHILI_GetEnableCommsInterface(void);

/**
 * Set current system frequency
 * @param system_frequency  New system frequency
 */
void CHILI_SetSystemFrequency(fsys_mhz system_frequency);

/**
 * Get current system frequency
 * @return Current system frequency
 */
fsys_mhz CHILI_GetSystemFrequency(void);

/**
 * Get the 96-bit hardware unique ID.
 * @param uid_out Out pointer pointing to at least 3 words of memory
 */
void CHILI_GetUID(uint32_t *uid_out);

/** Enable SPI Clock. */
void CHILI_EnableSpiModuleClock();

/** Wait until system is stable after potential usb plug-in */
void CHILI_WaitForSystemStable();

/** Initialise ADC peripheral */
void CHILI_InitADC(u32_t reference);

/** Deinitialise ADC peripheral */
void CHILI_DeinitADC();

/** Initialise GPIO peripheral clock */
void CHILI_GPIOInitClock();

/** Initialise Timer IRQ priorities */
void CHILI_ReInitSetTimerPriority();

/** Configure clock for power down */
void CHILI_PowerDownSelectClock(u8_t use_timer0);

/** Process all of the secure-only power down routines */
u32_t CHILI_PowerDownSecure(u32_t sleeptime_ms, u8_t use_timer0, dpd_flag dpd);

/** Enable internal temperature sensor */
void CHILI_EnableTemperatureSensor();

/** Disable internal temperature sensor */
void CHILI_DisableTemperatureSensor();

/** Is chili currently asleep? */
u8_t CHILI_GetAsleep();

/** Set asleep state */
void CHILI_SetAsleep(u8_t new_asleep);

/** Should the Chili wake up? */
u8_t CHILI_GetWakeup();

/** Set wake up state */
void CHILI_SetWakeup(u8_t new_wakeup);

/** Has gpio interrupt occured during power-down sequence? */
u8_t CHILI_GetGPIOInt();

/** Set gpio interrrupt occured during power-down sequence flag */
void CHILI_SetGPIOInt(u8_t new_gpioint);

/** Get power-down flag (device is powering down) ? */
u8_t CHILI_GetPowerDown();

/** Set power-down flag */
void CHILI_SetPowerDown(u8_t new_powerdown);

/** Configure the chili to boot from LDROM next time */
void CHILI_SetLDROMBoot(void);

/** Configure the chili to boot from APROM next time */
void CHILI_SetAPROMBoot(void);

/** Enable the TRNG Module Clock */
void CHILI_EnableTRNGClk(void);

/** Disable the TRNG Module Clock */
void CHILI_DisableTRNGClk(void);

/** Interrupt Handler for SPI DMA */
void CHILI_SPIDMAIRQHandler(void);

/** Register the SPIComplete callback on trustzone (Compiles to nothing on non-tz) */
void CHILI_RegisterSPIComplete(void (*callback)(void));

#endif
