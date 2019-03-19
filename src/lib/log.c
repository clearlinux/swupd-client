/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#define DEBUG_HEADER_SIZE 50

static void log_internal(FILE *out, const char *file, int line, const char *label, const char *format, va_list args_list);
static int cur_log_level = LOG_INFO;
static log_fn_t log_function = log_internal;

static void log_internal(FILE *out, const char *file, int line, const char *label, const char *format, va_list args_list)
{
	char time_str[50];
	struct tm *timeinfo;
	int printed;

	// In debug mode, print more information
	if (cur_log_level == LOG_DEBUG) {
		time_t currtime;
		time(&currtime);
		timeinfo = localtime(&currtime);
		strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

		printed = fprintf(out, "%s (%s:%d)", time_str, file, line);
		if (printed < DEBUG_HEADER_SIZE) {
			fprintf(out, "%*c", 50 - printed, ' ');
		}
	}

	if (label) {
		fprintf(out, "%s: ", label);
	}
	vfprintf(out, format, args_list);
}

void log_set_level(int log_level)
{
	cur_log_level = log_level;
}

void log_set_function(log_fn_t log_fn)
{
	if (log_fn) {
		log_function = log_fn;
	} else {
		log_function = log_internal;
	}
}

void log_full(int log_level, FILE *out, const char *file, int line, const char *label, const char *format, ...)
{
	if (cur_log_level < log_level) {
		return;
	}

	va_list ap;
	va_start(ap, format);

	log_function(out, file, line, label, format, ap);

	/* clean memory assigned to arguments list */
	va_end(ap);
}
