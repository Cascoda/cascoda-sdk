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
/**
 * @file
 * Declarations for core platform abstraction functions.
 */
/**
 * @ingroup bm-core
 * @defgroup bm-interface-core Baremetal Core platform abstraction
 * @brief  Platform abstraction for core SDK functionality, see \ref bm-interface for extended functionality.
 *
 * @{
 */

#ifndef CASCODA_INTERFACE_CORE_H
#define CASCODA_INTERFACE_CORE_H

#include "cascoda_types.h"
#include "mac_messages.h"

/**
 * \brief Wait for specified Time in Microseconds (max. 999)
 *
 * \param us - Time in microseconds
 *
 */
void BSP_WaitUs(u32_t us);

/**
 * \brief This function will be called repeatedly when the Baremetal drivers
 * are blocking & waiting (eg. WAIT_ms or WAIT_Callback), in case the
 * BSP needs to do any system maintenance or wants to reduce power consumption.
 *
 */
void BSP_Waiting(void);

/**
 * \brief Get the number of milliseconds since program start.
 *
 * \returns The number of milliseconds since program start
 *
 */
u32_t BSP_ReadAbsoluteTime(void);

/**
 * \brief Wait for specified Time in Milliseconds
 *
 * \param ticks - Time in milliseconds
 *
 */
void BSP_WaitTicks(u32_t ticks);

/**
 * \brief Reset CAX RF Chip
 *
 * \param ms - Reset Low Time in [ms]
 *
 */
void BSP_ResetRF(u8_t ms);

/**
 * \brief Sense whether SPI IRQ is high or low
 *
 * \return State of IRQ pin of the CA821x
 *
 */
u8_t BSP_SenseRFIRQ(void);

/**
 * \brief Inhibit SPI IRQ, suppresses interrupt but still latches it
 *
 */
void BSP_DisableRFIRQ(void);

/**
 * \brief Allow SPI IRQ, re-enabling interrupt after BSP_DisableRFIRQ()
 *
 */
void BSP_EnableRFIRQ(void);

/**
 * \brief Put SPI Select (SSB) Pin high
 *
 */
void BSP_SetRFSSBHigh(void);

/**
 * \brief Put SPI Select (SSB) Pin low
 *
 */
void BSP_SetRFSSBLow(void);

/**
 * \brief Initialise GPIO and SPI Pins for Comms with CA-821X
 *
 */
void BSP_SPIInit(void);

/**
 * Exchange bytes over SPI, receiving into RxBuf and transferring from TxBuf.
 *
 * Any bytes received after a count of RxLen MUST be discarded. Any bytes sent
 * after a count of TxLen MUST have the value 0xFF. A total of MAX(RxLen, TxLen)
 * bytes MUST be exchanged. When the exchange is complete, the BSP MUST call the
 * SPI_ExchangeComplete function.
 *
 * RFIRQ will be disabled before this function is called.
 *
 * @param RxBuf Buffer to fill with received bytes. Must be at least RxLen big.
 * @param TxBuf Buffer to send bytes from. Must be at least TxLen big.
 * @param RxLen Count of bytes to receive before dumping all future bytes.
 * @param TxLen Count of bytes to transmit before sending all 0xFF bytes.
 */
void BSP_SPIExchange(u8_t *RxBuf, const u8_t *TxBuf, u8_t RxLen, u8_t TxLen);

/**
 * \brief Is the code running in an interrupt context?
 * @return true if in interrupt context
 */
bool BSP_IsInsideInterrupt(void);

/**
 * \brief Connect MOSI port to SPI MOSI and disable pull-up
 *
 */
void BSP_SetSPIMOSIOutput(void);

/**
 * \brief Disconnect MOSI port from SPI MOSI and put to tristate with pull-up
 *
 */
void BSP_SetSPIMOSITristate(void);

/**
 * \brief start microseconds timer
 *
 */
void BSP_MicroTimerStart(void);

/**
 * \brief stop microseconds timer
 * @return time since start in [us]
 */
u32_t BSP_MicroTimerStop(void);

/**
 * \brief get microseconds timer while running
 * @return time since start in [us]
 */
u32_t BSP_MicroTimerGet(void);

#endif // CASCODA_INTERFACE_CORE_H

/**
 * @}
 */
