/**
 * @file
 * @brief Debug printing functions for Cascoda API code and applications.
 *
 * Used for binding to the system's printf functions etc.
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

#include <stdint.h>
#include <time.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"

/** Max characters that can be buffered before being sent upstream */
#define MAX_PUTC_CHARS (128)

uint8_t PutcCount = 0;              //!< Current length of PutcBuffer
uint8_t PutcBuffer[MAX_PUTC_CHARS]; //!< Debug message buffer

// EVBME_Message Function for Upstream Communications in:
// - cascoda_serial_uart.c
// - cascoda_serial_usb.c

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send buffered message upstream
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
static void flushbuf(void *pDeviceRef)
{
	if (PutcCount != 0)
	{
		if (EVBME_Message != NULL)
		{
			EVBME_Message((char *)PutcBuffer, PutcCount, pDeviceRef);
		}
		PutcCount = 0;
	}
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief putchar override for printf messages
 *******************************************************************************
 * Writes characters to PutcBuffer until a newline is received, at which point
 * HOST_flush is called.
 *******************************************************************************
 * \param OutChar - Character to put in output buffer
 *******************************************************************************
 * \return Character written to output buffer
 *******************************************************************************
 ******************************************************************************/
int putchar(int OutChar)
{
	if (OutChar == '\n')
	{
		flushbuf(NULL);
		return (int)OutChar;
	}
	PutcBuffer[PutcCount] = OutChar;
	PutcCount++;
	if (PutcCount == MAX_PUTC_CHARS)
	{
		flushbuf(NULL);
	}
	return (int)OutChar;
} // End of putchar()

int fputc(int c, void *stream)
{
	(void)stream;
	return putchar(c);
}

#if defined(__NEWLIB__)
int _write(int file, char *ptr, int len)
{
	int i;

	(void)file;
	for (i = 0; i < len; i++)
	{
		putchar(ptr[i]);
	}
	return i;
}
#endif
