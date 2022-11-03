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
 * @ingroup bm-mikrosdk-click
 * @defgroup bm-mikrosdk-click-sps30 SPS30 particulate matter (PM) driver 
 * @brief Library for UART communication with the SPS30 particulate matter (PM) driver.
 *
 * @{
*/

#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_serial.h"

#define SPS30_UART_ADDRESS 0x00
#define SPS30_START_STOP_BYTE 0x7E
#define SPS30_UART_START_MEASUREMENT 0x00
#define SPS30_UART_SERIAL_NUMBER 0x03

#define SPS30_UART_ERROR 1

/**
 * @brief Write function in UART communication.
 *
 * @param tx_buf      Transmit data buffer.
 * @param buf_size    Transmit data buffer size.
 * 
 *  Function write the bytes on the TX line 
 */
void sps30_uart_write(u8_t *tx_buf, u32_t buf_size);

/**
 * @brief Read function in UART communication.
 *
 * @param rx_buf            Data buffer to be store data in.
 * @param total_buf_size    The number of bytes that you expect to receive.
 * 
 *  Function read the bytes on the RX line 
 */
void sps30_uart_read(u8_t *rx_buf, u32_t total_buf_size);

/**
 * @brief Start sps30 measurement in UART communication.
 * 
 *  Function signals sps30 to start measuring. 
 */
void sps30_uart_start_measurement(void);

/**
 * @brief Read device serial number in UART communication.
 * 
 *  Function read sps30 device serial number 
 */
void sps30_uart_read_device_serial_number(u8_t *rx_buf);

/******************************************************************************/
/***************************************************************************/ /**
 * \brief SPS30: Initialise Sensor
 *******************************************************************************
 * \return status, 0 = success
 *******************************************************************************
 ******************************************************************************/
u8_t MIKROSDK_SPS30_Initialise(void);

/**
 * @}
 */