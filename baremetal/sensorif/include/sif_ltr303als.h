/*
 * Copyright (C) 2019  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Sensor interface for LITEON LTR-303ALS-01 ambient light sensor
*/

#ifndef SIF_LTR303ALS_H
#define SIF_LTR303ALS_H

/* slave addresses */
#define SIF_SAD_LTR303ALS 0x29

/* part id and manufacturer id */
#define SIF_LTR303ALS_PARTID 0xA0
#define SIF_LTR303ALS_MANFID 0x05

/* master measurement (read access) modes */
enum sif_ltr303als_mode
{
	SIF_LTR303ALS_MODE_POLL_ONE_SHOT, /* one-shot   conversion - sensor in standby mode otherwise */
	SIF_LTR303ALS_MODE_CONTINUOUS,    /* continuous conversion - sensor always active */
};

/* measurement mode */
#define SIF_LTR303ALS_MODE SIF_LTR303ALS_MODE_POLL_ONE_SHOT

/* startup times */
#define SIF_LTR303ALS_TSTUP_POWERUP_MS 100 /* startup time [ms] after power-up */
#define SIF_LTR303ALS_TSTUP_STANDBY_MS  10 /* startup time [ms] standby to active */

/* register addresses */
enum sif_ltr303als_reg_address
{
	REG_LTR303ALS_CONTR       = 0x80,
	REG_LTR303ALS_MEAS_RATE   = 0x85,
	REG_LTR303ALS_PART_ID     = 0x86,
	REG_LTR303ALS_MANUFAC_ID  = 0x87,
	REG_LTR303ALS_DATA_CH1_0  = 0x88,
	REG_LTR303ALS_DATA_CH1_1  = 0x89,
	REG_LTR303ALS_DATA_CH0_0  = 0x8A,
	REG_LTR303ALS_DATA_CH0_1  = 0x8B,
	REG_LTR303ALS_STATUS      = 0x8C,
	REG_LTR303ALS_INTERRUPT   = 0x8F,
	REG_LTR303ALS_THRES_UP_0  = 0x97,
	REG_LTR303ALS_THRES_UP_1  = 0x98,
	REG_LTR303ALS_THRES_LOW_0 = 0x99,
	REG_LTR303ALS_THRES_LOW_1 = 0x9A,
	REG_LTR303ALS_INT_PERS    = 0x9E,
};

/* ALS gain */
enum sif_ltr303als_gain
{
	LTR303ALS_GAIN_1X        = 0x00,
	LTR303ALS_GAIN_2X        = 0x01,
	LTR303ALS_GAIN_4X        = 0x02,
	LTR303ALS_GAIN_8X        = 0x03,
	LTR303ALS_GAIN_48X       = 0x06,
	LTR303ALS_GAIN_96X       = 0x07,
};

/* ALS integration time [ms] (default 100) */
enum sif_ltr303als_tint
{
	LTR303ALS_TINT_50        = 0x01,
	LTR303ALS_TINT_100       = 0x00,
	LTR303ALS_TINT_150       = 0x04,
	LTR303ALS_TINT_200       = 0x02,
	LTR303ALS_TINT_250       = 0x05,
	LTR303ALS_TINT_300       = 0x06,
	LTR303ALS_TINT_350       = 0x07,
	LTR303ALS_TINT_400       = 0x03,
};

/* ALS measurement time (period) [ms] (default 500) */
enum sif_ltr303als_tmeas
{
	LTR303ALS_TMEAS_50       = 0x00,
	LTR303ALS_TMEAS_100      = 0x01,
	LTR303ALS_TMEAS_200      = 0x02,
	LTR303ALS_TMEAS_500      = 0x03,
	LTR303ALS_TMEAS_1000     = 0x04,
	LTR303ALS_TMEAS_2000     = 0x05,
};

/* gain, measurement time and integration period default setting */
#define SIF_LTR303ALS_GAIN  LTR303ALS_GAIN_1X
#define SIF_LTR303ALS_TINT  LTR303ALS_TINT_100
#define SIF_LTR303ALS_TMEAS LTR303ALS_TMEAS_200

/* functions */
u8_t  SIF_LTR303ALS_Initialise(void);                            /* initialise sensor, shutdown mode */
u8_t  SIF_LTR303ALS_Configure(u8_t gain, u8_t tint, u8_t tmeas); /* configure sensor */
u8_t  SIF_LTR303ALS_ReadLight(u16_t *ch0, u16_t *ch1);           /* measure light */

#endif // SIF_LTR303ALS_H
