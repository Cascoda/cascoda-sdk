/**
 * @file cascoda_bsp_chili.c
 * @brief Board Support Package (BSP)\n
 *        Micro: Nuvoton M2351\n
 *        Board: Chili Module
 * @author Wolfgang Bruchner
 * @date 21/01/16
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
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
//System
#include <stdio.h>
#include <time.h>

//Cascoda
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_time.h"
#include "cascoda-bm/cascoda_types.h"
#include "ca821x_api.h"

const struct FlashInfo   BSP_FlashInfo = {0};
static struct ca821x_dev internal_device;

void BSP_WaitUs(u32_t us)
{
	struct timespec ts;
	ts.tv_sec  = 0;
	ts.tv_nsec = us * 1000;
	nanosleep(&ts, NULL);
}

void BSP_ResetRF(u8_t ms)
{
}

u8_t BSP_SenseRFIRQ(void)
{
	return 0;
}

void BSP_DisableRFIRQ(void)
{
}

void BSP_EnableRFIRQ(void)
{
}

void BSP_SetRFSSBHigh(void)
{
}

void BSP_SetRFSSBLow(void)
{
}

void BSP_EnableSerialIRQ(void)
{
}

void BSP_DisableSerialIRQ(void)
{
}

#if defined(USE_USB)
void BSP_SerialWrite(u8_t *pBuffer)
{
}

u8_t BSP_SerialRead(u8_t *pBuffer)
{
	return 0;
}
#endif // USE_USB

#if defined(USE_UART)

void BSP_SerialWriteAll(u8_t *pBuffer, u32_t BufferSize)
{
	if (pBuffer[0] == 0)
	{
		//Only print EVBME messages
		printf("%.*s, BufferSize, pBuffer");
	}
}

u32_t BSP_SerialRead(u8_t *pBuffer, u32_t BufferSize)
{
	return 0;
}

void CHILI_UARTInit(void)
{
}

void BSP_UARTDeinit(void)
{
}

#endif // USE_UART

void BSP_SPIInit(void)
{
}

u8_t BSP_SPIExchangeByte(u8_t OutByte)
{
	u8_t InByte;
	while (!BSP_SPIPushByte(OutByte))
		;
	while (!BSP_SPIPopByte(&InByte))
		;
	return InByte;
}

u8_t BSP_SPIPushByte(u8_t OutByte)
{
	return 1;
}

u8_t BSP_SPIPopByte(u8_t *InByte)
{
	*InByte = 0xFF;
	return 1;
}

void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0)
{
	TIME_WaitTicks(sleeptime_ms);
}

void BSP_Initialise(struct ca821x_dev *pDeviceRef)
{
}

void BSP_UseExternalClock(u8_t useExternalClock)
{
}

void BSP_Waiting(void)
{
}

u8_t BSP_GetChargeStat(void)
{
	return 0;
}

i32_t BSP_GetTemperature(void)
{
	return 200;
}

u32_t BSP_ADCGetVolts(void)
{
	return 0;
}

void BSP_LEDSigMode(u8_t mode)
{
}

void BSP_WatchdogEnable(u32_t timeout_ms)
{
}

void BSP_WatchdogReset(u32_t timeout_ms)
{
}

u8_t BSP_IsWatchdogTriggered(void)
{
	return 0;
}

void BSP_WatchdogDisable(void)
{
}

u8_t BSP_GPIOSenseSwitch(void)
{
	return 0;
}

void BSP_EnableUSB(void)
{
}

void BSP_DisableUSB(void)
{
}

u8_t BSP_IsUSBPresent(void)
{
	return 0;
}

void BSP_WriteDataFlashInitial(u32_t startaddr, u32_t *data, u32_t datasize)
{
}

void BSP_EraseDataFlashPage(u32_t startaddr)
{
}

void BSP_ReadDataFlash(u32_t startaddr, u32_t *data, u32_t datasize)
{
}

void BSP_ClearDataFlash(void)
{
}

void SENSORIF_I2C_Init(void)
{
}

void SENSORIF_I2C_Deinit(void)
{
}

enum sensorif_i2c_status SENSORIF_I2C_Write(u8_t slaveaddr, u8_t *data, u32_t *len)
{
	(void)slaveaddr;
	(void)data;
	(void)len;
	return SENSORIF_I2C_ST_NOT_IMPLEMENTED;
}

enum sensorif_i2c_status SENSORIF_I2C_Read(u8_t slaveaddr, u8_t *pdata, u32_t *plen)
{
	(void)slaveaddr;
	(void)pdata;
	(void)plen;
	return SENSORIF_I2C_ST_NOT_IMPLEMENTED;
}
