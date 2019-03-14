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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "formatter_json.h"
#include "log.h"
#include "strings.h"

static int json_format = 0;

static void log_json(FILE *out UNUSED, const char *file UNUSED, int line UNUSED, const char *label, const char *format, va_list args_list)
{
	format_to_json(label, format, args_list);
}

void set_json_format(void)
{
	json_format = 1;
	set_log_function(log_json);
}

void json_start(const char *op)
{
	if (json_format) {
		fprintf(stderr, "[\n{ \"type\" : \"start\", \"section\" : \"%s\" },\n", op);
		fflush(stderr);
	}
}

void json_end(const char *op)
{
	if (json_format) {
		fprintf(stderr, "{ \"type\" : \"end\", \"section\" : \"%s\" }\n]\n", op);
		fflush(stderr);
	}
}

void json_status(int status)
{
	if (json_format) {
		fprintf(stderr, "{ \"type\" : \"status\", \"status\" : \"%d\" },\n", status);
		fflush(stderr);
	}
}

void format_to_json(const char *msg_type, const char *msg, va_list args_list)
{
	char *full_msg;
	char *type = NULL;

	/* make sure the type is all lower case */
	if (msg_type) {
		type = str_tolower(msg_type);
	}

	/* build the full message based on all the arguments */
	if (vasprintf(&full_msg, msg, args_list) < 0) {
		abort();
	}

	for (unsigned int i = 0; full_msg[i]; i++) {
		/* if the message has double quotes replace them with single quotes
		* since double quotes have to be escaped in JSON */
		if (full_msg[i] == '"') {
			full_msg[i] = '\'';
		}
		/* replace all '\n' from the message with spaces */
		if (full_msg[i] == '\n') {
			full_msg[i] = ' ';
		}
	}

	/* add the JSON format and print immediately */
	if (strcmp(full_msg, " ") != 0) {
		fprintf(stderr, "{ \"type\" : \"%s\", \"msg\" : \"%s\" },\n", type ? type : "info", full_msg);
		fflush(stderr);
	}

	free_string(&full_msg);
	if (type) {
		free_string(&type);
	}
}

void print_progress(unsigned int count, unsigned int max)
{
	struct step step;
	step.current = 0;
	step.total = 0;
	step.description = "";

	print_step_progress(step, count, max);
}

void complete_step(unsigned int current, unsigned int total, char *description)
{
	struct step step;

	step.current = current;
	step.total = total;
	step.description = description;

	if (json_format) {
		print_step_progress(step, 1, 1);
	}
}

void print_step_progress(struct step step, unsigned int count, unsigned int max)
{
	static int last_percentage = -1;
	static unsigned int last_step = 0;

	if (isatty(fileno(stdout)) || json_format) {
		/* Only print when the percentage changes, so a maximum of 100 times per run */
		int percentage = (int)(100 * ((float)count / (float)max));
		if (percentage != last_percentage || step.current != last_step) {
			if (json_format) {
				fprintf(stderr, "{ \"type\" : \"progress\", "
						"\"currentStep\" : \"%d\", "
						"\"totalSteps\" : \"%d\", "
						"\"stepCompletion\" : \"%d\", "
						"\"stepDescription\" : \"%s\" },\n",
					step.current, step.total, percentage, step.description);
			} else {
				printf("\r\t...%d%%", percentage);
			}
			fflush(stdout);
			last_percentage = percentage;
			last_step = step.current;
		}
	} else {
		/* Print the first one, then every 10 after that */
		if (count % 10 == 1) {
			printf(".");
			fflush(stdout);
		}
	}
}
