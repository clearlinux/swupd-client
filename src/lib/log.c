/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2019 Intel Corporation.
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
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

#define DEBUG_HEADER_SIZE 50
#define LINE_MAX _POSIX2_LINE_MAX

static void log_internal(FILE *out, const char *file, int line, const char *label, const char *format, va_list args_list);
static int cur_log_level = LOG_INFO;
static log_fn_t log_function = log_internal;

static void print_debug_info(FILE *out, const char *file, int line)
{
	char time_str[50];
	struct tm *timeinfo;
	int printed;
	time_t currtime;

	time(&currtime);
	timeinfo = localtime(&currtime);
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
	printed = fprintf(out, "%s (%s:%d)", time_str, file, line);
	if (printed < DEBUG_HEADER_SIZE) {
		fprintf(out, "%*c", 50 - printed, ' ');
	}
}

static void log_internal(FILE *out, const char *file, int line, const char *label, const char *format, va_list args_list)
{

	char msg[LINE_MAX];

	/* do not print the carriage return found at the beginning of the message if
	 * in debug mode or if printing to a file, that would mess up the output */
	while (strncmp(format, "\r", 1) == 0) {
		if (cur_log_level != LOG_DEBUG && isatty(fileno(out))) {
			fprintf(out, "%c", format[0]);
		}
		format++;
	}

	/* print the line breaks found at the beginning of the message */
	while (strncmp(format, "\n", 1) == 0) {
		if (cur_log_level == LOG_DEBUG) {
			print_debug_info(out, file, line);
		}
		fprintf(out, "%c", format[0]);
		format++;
	}

	/* if we don't have anything else to print, finish here */
	if (strcmp(format, "") == 0) {
		return;
	}

	// In debug mode, print more information
	if (cur_log_level == LOG_DEBUG) {
		print_debug_info(out, file, line);
	}

	if (label) {
		fprintf(out, "%s: ", label);
	}

	/* all debug messages need to finish with a line break, even if the
	 * user didn't add one to the message, otherwise the output will be
	 * messed up
	 * Note that the "\n" could have been provided as an argument to the
	 * string, as in this example: info("example%s", flag ? "" : "\n")
	 * so we need to pass the string to a variable first to inspect it */
	vsnprintf(msg, LINE_MAX, format, args_list);
	fprintf(out, "%s", msg);
	if ((cur_log_level == LOG_DEBUG) && (msg[strlen(msg) - 1] != '\n')) {
		fprintf(out, "\n");
	}
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
