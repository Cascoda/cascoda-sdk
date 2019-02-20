/**
 * @file cascoda_debug.c
 * @brief Debug printing functions for Cascoda API code and applications.
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
// void EVBME_Message( uint8_t Count, uint8_t *pBuffer, void *pDeviceRef );

#if defined(USE_USB) || defined(USE_UART)

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send buffered message upstream
 *******************************************************************************
 * \param pDeviceRef - Device reference
 *******************************************************************************
 ******************************************************************************/
static void dpFlush(void *pDeviceRef)
{
	if (PutcCount != 0)
	{
		if (EVBME_Message != NULL)
		{
			EVBME_Message((char *)PutcBuffer, PutcCount, pDeviceRef);
		}
		PutcCount = 0;
	}
} // End of dpFlush()

/******************************************************************************/
/***************************************************************************/ /**
 * \brief putchar override for printf messages
 *******************************************************************************
 * Writes characters to PutcBuffer until a newline is received, at which point
 * dpFlush is called.
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
		dpFlush(NULL);
		return (int)OutChar;
	}
	PutcBuffer[PutcCount] = OutChar;
	PutcCount++;
	if (PutcCount == MAX_PUTC_CHARS)
	{
		dpFlush(NULL);
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

#endif

/******************************************************************************/
/******************************************************************************/
/****** dptime()                                                         ******/
/******************************************************************************/
/****** Brief:  DP Time HH:MM:SS.MS                                      ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: -                                                        ******/
/******************************************************************************/
/******************************************************************************/
void dptime(void)
{
	/* TODO */
	// u16_t MSecs;
	// Putc( (Hours/10)+'0' );
	// Putc( (Hours%10)+'0' );
	// Putc( ':' );
	// Putc( (Minutes/10)+'0' );
	// Putc( (Minutes%10)+'0' );
	// Putc( ':' );
	// Putc( (Seconds/10)+'0' );
	// Putc( (Seconds%10)+'0' );
	// Putc( '.' );
	// MSecs = Ticks;
	// Putc( (MSecs/100)+'0' );
	// Putc( ((MSecs%100)/10)+'0' );
	// dps("  ");
} // End of dptime()
