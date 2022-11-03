/**
 * @file
 *//*
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
/*
 * Library for UART communication with the SPS30 particulate matter (PM) driver.
*/

#include "sps30.h"
#include <stdio.h>

/*-----------------------------------------------PRIVATE FUNCTION DECLARATION----*/
u32_t sps30_checksum(u8_t *buf, u32_t num);

void sps30_append(u8_t *tx_buf, u8_t *buf, u32_t buf_size, u8_t start_stop_byte, u8_t checksum);

/*-----------------------------------------------------------PUBLIC FUNCTION----*/
void sps30_uart_write(u8_t *tx_buf, u32_t tx_buf_size)
{
	u8_t  tx_buffer[tx_buf_size + 3];
	u32_t chk         = sps30_checksum(tx_buf, tx_buf_size);
	u32_t total_bytes = tx_buf_size + 3;
	u32_t total_write_bytes;

	sps30_append(tx_buffer, tx_buf, tx_buf_size, SPS30_START_STOP_BYTE, (u8_t)chk);

	total_write_bytes = SENSORIF_UART_Write(tx_buffer, total_bytes);
}

void sps30_uart_read(u8_t *rx_buf, u32_t total_buf_size)
{
	u8_t  temp_buf[30];
	u32_t num = SENSORIF_UART_Read(temp_buf, total_buf_size);
	if (!num)
	{
		ca_log_warn("*** No data received ***");
		return;
	}
	printf("Number of characters placed in buffer: %d\n", num);
	u8_t rx_buf_size = temp_buf[4];

	for (int i = 0; i < rx_buf_size; i++)
	{
		rx_buf[i] = temp_buf[i];
	}
}

void sps30_uart_start_measurement(void)
{
	u8_t tx_buf[5], rx_buf[7] = {0x00};
	u8_t data_length = 0x02;

	tx_buf[0] = SPS30_UART_ADDRESS;
	tx_buf[1] = SPS30_UART_START_MEASUREMENT;
	tx_buf[2] = 0x02;
	tx_buf[3] = 0x01; // sub-command
	tx_buf[4] = 0x03; // measurement mode

	sps30_uart_write(tx_buf, 5);
	sps30_uart_read(rx_buf, 7);
}

void sps30_uart_read_device_serial_number(u8_t *rx_buf)
{
	u8_t tx_buf[4];
	u8_t data_length = 0x01;

	tx_buf[0] = SPS30_UART_ADDRESS;
	tx_buf[1] = (u8_t)(SPS30_UART_SERIAL_NUMBER);
	tx_buf[2] = 0x01; // one data length
	tx_buf[3] = 0x03; //serial number

	sps30_uart_write(tx_buf, 4);
	sps30_uart_read(rx_buf, 28);
}

u8_t MIKROSDK_SPS30_Initialise(void)
{
	SENSORIF_UART_Init();
	BSP_WaitTicks(1000);
	sps30_uart_start_measurement();
	return 0;
}

/*-----------------------------------------------------------PRIVATE FUNCTION----*/
u32_t sps30_checksum(u8_t *buf, u32_t num)
{
	u32_t sum = 0;
	for (u8_t i = 0; i < num; ++i)
	{
		sum += buf[i];
	}
	return ~(sum);
}

void sps30_append(u8_t *tx_buf, u8_t *buf, u32_t buf_size, u8_t start_stop_byte, u8_t checksum)
{
	if (!buf)
	{
		ca_log_warn("Buffer is empty");
		return;
	}

	tx_buf[0] = start_stop_byte;
	for (int i = 0; i < buf_size; ++i)
	{
		tx_buf[i + 1] = buf[i];
	}
	tx_buf[buf_size + 1] = checksum;
	tx_buf[buf_size + 2] = start_stop_byte;
}
