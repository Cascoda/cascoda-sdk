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

#ifndef CA821X_API_INCLUDE_CA821X_LOG_H_
#define CA821X_API_INCLUDE_CA821X_LOG_H_

#include <stdarg.h>

#include "ca821x_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Cascoda loglevel type */
typedef enum ca_loglevel
{
	CA_LOGLEVEL_CRIT = 1, //!< Critical warnings that should always be displayed
	CA_LOGLEVEL_WARN = 2, //!< Warnings that something has gone wrong
	CA_LOGLEVEL_NOTE = 3, //!< Low-frequency notes that may be of interest
	CA_LOGLEVEL_INFO = 4, //!< Semi-Regular information that may be more frequent
	CA_LOGLEVEL_DEBG = 5, //!< High Frequency debug logs, data dumps, or unimportant information
} ca_loglevel;

/**
 * Function to process logs depending on platform
 *
 * This function should not be used by applications, and the ca_log_crit, ca_log_warn etc functions should be used instead.
 *
 * @param  loglevel    The ca_loglevel log level.
 * @param  format      A pointer to the format string.
 * @param  argp        Arguments for the format specification.
 *
 */
void ca_log(ca_loglevel loglevel, const char *format, va_list argp);

static inline void ca_log_crit(const char *format, ...);
static inline void ca_log_warn(const char *format, ...);
static inline void ca_log_note(const char *format, ...);
static inline void ca_log_info(const char *format, ...);
static inline void ca_log_debg(const char *format, ...);

static inline void ca_log_crit(const char *format, ...)
{
	if (CASCODA_LOG_LEVEL >= CA_LOGLEVEL_CRIT)
	{
		va_list va_args;
		va_start(va_args, format);
		ca_log(CA_LOGLEVEL_CRIT, format, va_args);
		va_end(va_args);
	}
}
static inline void ca_log_warn(const char *format, ...)
{
	if (CASCODA_LOG_LEVEL >= CA_LOGLEVEL_WARN)
	{
		va_list va_args;
		va_start(va_args, format);
		ca_log(CA_LOGLEVEL_WARN, format, va_args);
		va_end(va_args);
	}
}
static inline void ca_log_note(const char *format, ...)
{
	if (CASCODA_LOG_LEVEL >= CA_LOGLEVEL_NOTE)
	{
		va_list va_args;
		va_start(va_args, format);
		ca_log(CA_LOGLEVEL_NOTE, format, va_args);
		va_end(va_args);
	}
}
static inline void ca_log_info(const char *format, ...)
{
	if (CASCODA_LOG_LEVEL >= CA_LOGLEVEL_INFO)
	{
		va_list va_args;
		va_start(va_args, format);
		ca_log(CA_LOGLEVEL_INFO, format, va_args);
		va_end(va_args);
	}
}
static inline void ca_log_debg(const char *format, ...)
{
	if (CASCODA_LOG_LEVEL >= CA_LOGLEVEL_DEBG)
	{
		va_list va_args;
		va_start(va_args, format);
		ca_log(CA_LOGLEVEL_DEBG, format, va_args);
		va_end(va_args);
	}
}

#ifdef __cplusplus
}
#endif

#endif /* CA821X_API_INCLUDE_CA821X_LOG_H_ */
