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
 * Sensor interface for Silabs Si7021 temperature / humidity sensor
*/

#ifndef SENSORIF_APP_H
#define SENSORIF_APP_H

/* measurement period in [ms] */
#define SENSORIF_MEASUREMENT_PERIOD 5000
/* delta in [ms] around measurement time */
/* needs to be increased of more devices are tested */
#define SENSORIF_MEASUREMENT_DELTA 100

/* report measurement times flag */
#define SENSORIF_REPORT_TMEAS 1

/* flag which sensors to include in test */
#define SENSORIF_TEST_SI7021    0
#define SENSORIF_TEST_MAX30205  0
#define SENSORIF_TEST_LTR303ALS 1

/* functions */
u8_t SENSORIF_Initialise(struct ca821x_dev *pDeviceRef);
void SENSORIF_Handler(struct ca821x_dev *pDeviceRef);
void SENSORIF_Handler_SI7021(void);
void SENSORIF_Handler_MAX30205(void);
void SENSORIF_Handler_LTR303ALS(void);

#endif // SENSORIF_APP_H
