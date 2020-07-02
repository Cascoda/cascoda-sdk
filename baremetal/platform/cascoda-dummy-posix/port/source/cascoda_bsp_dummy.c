/**
 * @file
 * @brief Board Support Package (BSP)\n
 *        Micro: Nuvoton M2351\n
 *        Board: Chili Module
 */
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
//System
#include <stdio.h>
#include <time.h>

//Cascoda
#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_spi.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"

const struct FlashInfo BSP_FlashInfo = {0};
struct FlashInfo       BSP_GetFlashInfo(void)
{
	return BSP_FlashInfo;
}
const struct ModuleSpecialPins BSP_ModuleSpecialPins = {MSP_DEFAULT};

struct ModuleSpecialPins BSP_GetModuleSpecialPins(void)
{
	return BSP_ModuleSpecialPins;
}
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
void BSP_USBSerialWrite(u8_t *pBuffer)
{
}

u8_t *BSP_USBSerialRxPeek(void)
{
	return NULL;
}

void BSP_USBSerialRxDequeue(void)
{
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

void BSP_PowerDown(u32_t sleeptime_ms, u8_t use_timer0, u8_t dpd, struct ca821x_dev *pDeviceRef)
{
	BSP_WaitTicks(sleeptime_ms);
}

wakeup_reason BSP_GetWakeupReason(void)
{
	return WAKEUP_POWERON;
}

void BSP_Initialise(struct ca821x_dev *pDeviceRef)
{
}

void BSP_UseExternalClock(u8_t useExternalClock)
{
}

ca_error BSP_ModuleRegisterGPIOInput(struct gpio_input_args *args)
{
	return CA_ERROR_NOT_HANDLED;
}

ca_error BSP_ModuleRegisterGPIOOutput(u8_t mpin, module_pin_type isled)
{
	return CA_ERROR_NOT_HANDLED;
}

ca_error BSP_ModuleDeregisterGPIOPin(u8_t mpin)
{
	return CA_ERROR_NOT_HANDLED;
}

u8_t BSP_ModuleIsGPIOPinRegistered(u8_t mpin)
{
	return CA_ERROR_NOT_HANDLED;
}

ca_error BSP_ModuleSetGPIOPin(u8_t mpin, u8_t val)
{
	return CA_ERROR_NOT_HANDLED;
}

ca_error BSP_ModuleSenseGPIOPin(u8_t mpin, u8_t *val)
{
	return CA_ERROR_NOT_HANDLED;
}

ca_error BSP_ModuleReadVoltsPin(u8_t mpin, u32_t *val)
{
	return CA_ERROR_NOT_HANDLED;
}

void BSP_SystemReset(sysreset_mode resetMode)
{
}

bool BSP_IsInsideInterrupt(void)
{
	return false;
}

void BSP_Waiting(void)
{
}

u64_t BSP_GetUniqueId(void)
{
	return 0;
}

const char *BSP_GetPlatString(void)
{
	return "Dummy";
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

void BSP_WatchdogEnable(u32_t timeout_ms)
{
}

void BSP_WatchdogReset(void)
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

void BSP_SystemConfig(fsys_mhz fsys, u8_t enable_comms)
{
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
