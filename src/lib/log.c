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

static int cur_log_level = LOG_INFO;
static int json_format = 0;

void set_log_level(int log_level)
{
	cur_log_level = log_level;
}

void set_json_format(void)
{
	json_format = 1;
}

void json_start(const char *op)
{
	if (json_format) {
		fprintf(stderr, "[\n");
		fflush(stderr);
		fprintf(stderr, "{ \"%s\" : \"start\" },\n", op);
		fflush(stderr);
	}
}

void json_end(const char *op)
{
	if (json_format) {
		fprintf(stderr, "{ \"%s\" : \"end\" }\n", op);
		fflush(stderr);
		fprintf(stderr, "]\n");
		fflush(stderr);
	}
}

void format_to_json(const char *type, const char *msg, va_list args_list)
{
	const int MAX_TYPE_LENGTH = 8;
	char *full_msg;
	char ltype[MAX_TYPE_LENGTH];

	/* make sure the type is all lower case */
	if (type) {
		for (int i = 0; type[i]; i++) {
			ltype[i] = tolower(type[i]);
		}
		ltype[strlen(type)] = '\0';
	} else {
		/* if no type was provided we default to info */
		strcpy(ltype, "info");
	}

	/* build the full message based on all the arguments */
	if (vasprintf(&full_msg, msg, args_list) < 0) {
		abort();
	}

	if (strcmp(full_msg, "") != 0) {
		int j = 0;
		for (unsigned int i = 0; i <= strlen(full_msg); i++) {
			/* if the message has double quotes replace them with single quotes
			* since double quotes are not supported in JSON */
			if (full_msg[i] == '"') {
				full_msg[i] = '\'';
			}
			/* remove all '\n' from the message */
			if (full_msg[i] != '\n') {
				full_msg[j] = full_msg[i];
				j++;
			}
		}
		full_msg[j] = '\0';

		/* add the JSON format and print immediately */
		fprintf(stderr, "{ \"type\" : \"%s\", \"msg\" : \"%s\" },\n", ltype, full_msg);
		fflush(stderr);
	}

	free(full_msg);
}

void log_full(int log_level, FILE *out, const char *file, int line, const char *label, const char *format, ...)
{
	if (cur_log_level < log_level) {
		return;
	}

	char time_str[50];
	va_list ap;
	struct tm *timeinfo;
	int printed;

	// In debug mode, print more information
	if (cur_log_level == LOG_DEBUG && !json_format) {
		time_t currtime;
		time(&currtime);
		timeinfo = localtime(&currtime);
		strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

		printed = fprintf(out, "%s (%s:%d)", time_str, file, line);
		if (printed < DEBUG_HEADER_SIZE) {
			fprintf(out, "%*c", 50 - printed, ' ');
		}
	}

	/* initialize the arguments list */
	va_start(ap, format);

	if (json_format) {
		format_to_json(label, format, ap);
	} else {
		if (label) {
			fprintf(out, "%s: ", label);
		}
		vfprintf(out, format, ap);
	}

	/* clean memory assigned to arguments list */
	va_end(ap);
}
