/**
 * @file
 * @brief mikrosdk interface
 */
/*
 *  Copyright (c) 2022, Cascoda Ltd.
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
 * Example click interface driver
*/

#ifndef SHT_CLICK_H
#define SHT_CLICK_H

#include <stdint.h>

/* use alarm pin as interrupt (1) with continuous mode instead of polling (0) with single-shot mode */
#define SHT_USE_INTERRUPT 0

/* all sgp30 maximum data length */
#define SHT3X_MAXDLEN 6
/* I2C slave address + memory address (command) */
#define SHT3X_ADDLEN 3

/* simple boolean state mapping */
#define MIKROSDK_SHT_ON 1
#define MIKROSDK_SHT_OFF 0

/* additional commands for alert function (not in original code) */
#define SHT_CMD_WR_LO_SET 0x6100 /* write low  limits set   */
#define SHT_CMD_WR_LO_CLR 0x610B /* write low  limits clear */
#define SHT_CMD_WR_HI_SET 0x611D /* write high limits set   */
#define SHT_CMD_WR_HI_CLR 0x6116 /* write high limits clear */
#define SHT_CMD_RD_LO_SET 0xE102 /* write low  limits set   */
#define SHT_CMD_RD_LO_CLR 0xE109 /* write low  limits clear */
#define SHT_CMD_RD_HI_SET 0xE11F /* write high limits set   */
#define SHT_CMD_RD_HI_CLR 0xE114 /* write high limits clear */

/* timing parameters [ms] */
#define SHT_T_COMMS 2 /* 2 ms between communication exchange */

/* alarm active high */
enum sht_alarm_state
{
	SHT_ALARM_TRIGGERED = 2,
	SHT_ALARM_CLEARED   = 1,
	SHT_ALARM_NOALARM   = 0,
};

/* default alarm limits */
/* H in %  * 100 */
/* T in 'C * 100 */
#define SHT_LIMIT_H_HI 5500  /* humidity    high */
#define SHT_LIMIT_H_LO 2000  /* humidity    low  */
#define SHT_LIMIT_T_HI 6000  /* temperature high */
#define SHT_LIMIT_T_LO -1000 /* temperature low  */
#define SHT_LIMIT_H_HYS 200  /* humidity    hysteresis for clearing alarms */
#define SHT_LIMIT_T_HYS 200  /* temperature hysteresis for clearing alarms */

/* data acquisition status */
enum sht_status
{
	SHT_ST_OK              = 0, /* read values ok */
	SHT_ST_ALARM_CLEARED   = 1, /* alarm has been cleared (values ok) */
	SHT_ST_ALARM_TRIGGERED = 2, /* alarm has been triggered (values ok) */
	SHT_ST_FAIL            = 3  /* acquisition failed */
};

/* new functions */
uint8_t MIKROSDK_SHT_start_pm(void);
uint8_t MIKROSDK_SHT_stop_pm(void);
uint8_t MIKROSDK_SHT_get_temp_hum_pm(int16_t *humidity, int16_t *temperature);
uint8_t MIKROSDK_SHT_get_temp_hum_ss(int16_t *humidity, int16_t *temperature);
uint8_t MIKROSDK_SHT_heater_control(uint8_t state);
uint8_t MIKROSDK_SHT_read_status(uint16_t *statusregister);
uint8_t MIKROSDK_SHT_clear_status(void);
void    MIKROSDK_SHT_hard_reset(void);
uint8_t MIKROSDK_SHT_soft_reset(void);
uint8_t MIKROSDK_SHT_set_hi_alert_limits(int16_t h_set, int16_t h_clr, int16_t t_set, int16_t t_clr);
uint8_t MIKROSDK_SHT_set_lo_alert_limits(int16_t h_set, int16_t h_clr, int16_t t_set, int16_t t_clr);
uint8_t MIKROSDK_SHT_get_hi_alert_limits(int16_t *h_set, int16_t *h_clr, int16_t *t_set, int16_t *t_clr);
uint8_t MIKROSDK_SHT_get_lo_alert_limits(int16_t *h_set, int16_t *h_clr, int16_t *t_set, int16_t *t_clr);
void    MIKROSDK_SHT_config(void);
void    MIKROSDK_SHT_pin_mapping(uint8_t reset, uint8_t alarm);
uint8_t MIKROSDK_SHT_get_alarm(void);
uint8_t MIKROSDK_SHT_alarm_triggered(void);
uint8_t MIKROSDK_SHT_Initialise(void);
uint8_t MIKROSDK_SHT_Reinitialise(void);
uint8_t MIKROSDK_SHT_Acquire(int16_t *humidity, int16_t *temperature);

#endif // SHT_CLICK_H
