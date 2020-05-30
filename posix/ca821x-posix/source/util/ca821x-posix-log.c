/*
 * Copyright (c) 2017, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <time.h>

#include "ca821x_log.h"

void ca_log(ca_loglevel loglevel, const char *format, va_list argp)
{
	char *          lev_str;
	struct timespec ts;
	char            timeString[40];
	long            secs;

	clock_gettime(CLOCK_REALTIME, &ts);
	secs = ts.tv_sec;
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&secs));

	switch (loglevel)
	{
	case CA_LOGLEVEL_CRIT:
		lev_str = "CRIT: ";
		break;
	case CA_LOGLEVEL_WARN:
		lev_str = "WARN: ";
		break;
	case CA_LOGLEVEL_NOTE:
		lev_str = "NOTE: ";
		break;
	case CA_LOGLEVEL_INFO:
		lev_str = "INFO: ";
		break;
	case CA_LOGLEVEL_DEBG:
		lev_str = "DEBG: ";
		break;
	default:
		lev_str = "UNKN: ";
		break;
	}

	fprintf(stderr, "%s.%03d %s ", timeString, ts.tv_nsec / 1000000, lev_str);
	vfprintf(stderr, format, argp);
	fprintf(stderr, "\r\n");
}
