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

#include <assert.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#ifdef __NEWLIB__
#include <sys/errno.h>
#include <sys/stat.h>
#endif

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_os.h"

/** Max characters that can be buffered before being sent upstream */
#define MAX_PUTC_CHARS (128)

uint8_t PutcCount = 0;              //!< Current length of PutcBuffer
uint8_t PutcBuffer[MAX_PUTC_CHARS]; //!< Debug message buffer

// EVBME_Message Function for Upstream Communications in:
// - cascoda_serial_uart.c
// - cascoda_serial_usb.c

/**
 * Send buffered message upstream
 */
static void flushbuf()
{
	if (PutcCount != 0)
	{
		if (EVBME_Message != NULL)
		{
			EVBME_Message((char *)PutcBuffer, PutcCount);
		}
		PutcCount = 0;
	}
}

/**
 * \brief putchar override for printf messages
 *
 * Writes characters to PutcBuffer until a newline is received, at which point
 * HOST_flush is called.
 *
 * \param OutChar - Character to put in output buffer
 * \return Character written to output buffer
 */
int putchar(int OutChar)
{
	if (OutChar == '\n')
	{
		flushbuf();
		return (int)OutChar;
	}
	PutcBuffer[PutcCount] = OutChar;
	PutcCount++;
	if (PutcCount == MAX_PUTC_CHARS)
	{
		flushbuf();
	}
	return (int)OutChar;
} // End of putchar()

#if defined(__NEWLIB__)
int _write(int file, char *ptr, int len)
{
	int i;

	(void)file;

	assert(!BSP_IsInsideInterrupt());

	CA_OS_LockAPI();
	for (i = 0; i < len; i++)
	{
		putchar(ptr[i]);
	}
	CA_OS_UnlockAPI();

	return i;
}

int _read(int file, char *ptr, int len)
{
	(void)file;
	(void)ptr;
	(void)len;

	return 0;
}

void *_sbrk(int incr)
{
	extern char __HeapBase;
	extern char __HeapLimit;

	static char *heap_end = &__HeapBase; /* Previous end of heap or 0 if none */
	char *       prev_heap;

	prev_heap = heap_end;
	heap_end += incr;

	if (heap_end >= (&__HeapLimit))
	{
		heap_end = prev_heap;
		errno    = ENOMEM;
		return (char *)-1;
	}
	return (void *)prev_heap;

} /* _sbrk () */

int _close(int file)
{
	(void)file;
	return -1;
}

int _fstat(int file, struct stat *st)
{
	(void)file;
	//TODO: Currently we just mark everything as special char device, but we can improve this as we packetise
	st->st_mode = S_IFCHR;

	return 0;
}

int _isatty(int file)
{
	(void)file;
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	(void)file;
	(void)ptr;
	(void)dir;
	return 0;
}

void _exit(int status)
{
	(void)status;
	BSP_SystemReset(SYSRESET_APROM);

	while (1)
		; //Should never be reached
}

void _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	return;
}

int _getpid(void)
{
	return -1;
}

int _gettimeofday(struct timeval *tp, void *tzp)
{
	(void)tzp;
	if (tp)
	{
		struct RTCDateAndTime dt;
		BSP_RTCGetDateAndTime(&dt);
		tp->tv_sec  = BSP_RTCConvertDateAndTimeToSeconds(&dt);
		tp->tv_usec = 0;
	}

	return 0;
}

#else
int fputc(int c, void *stream)
{
	(void)stream;
	return putchar(c);
}
#endif // defined(__NEWLIB__)
