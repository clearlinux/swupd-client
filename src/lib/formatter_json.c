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

#include <stdlib.h>
#include <string.h>

#include "formatter_json.h"
#include "log.h"
#include "macros.h"
#include "strings.h"

static bool json_stream_active = false;

static void json_sanitize_string(char *full_msg)
{
	int i;

	/* sanitize characters in the string */
	for (i = 0; full_msg[i]; i++) {
		/* if the message has double quotes replace them with single quotes
		 * since double quotes have to be escaped in JSON */
		if (full_msg[i] == '"') {
			full_msg[i] = '\'';
		}
		/* replace all '\n' and '\r' from the message with spaces */
		if (full_msg[i] == '\n' || full_msg[i] == '\r') {
			full_msg[i] = ' ';
		}
	}

	for (i = i - 1; i >= 0 && full_msg[i] == ' '; i--) {
		full_msg[i] = '\0';
	}
}

static void json_message(const char *msg_type, const char *msg, va_list args_list)
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

	json_sanitize_string(full_msg);

	/* add the JSON format and print immediately */
	if (strcmp(full_msg, "") != 0) {
		fprintf(stdout, "{ \"type\" : \"%s\", \"msg\" : \"%s\" },\n", type ? type : "info", full_msg);
		fflush(stdout);
	}

	free_string(&full_msg);
	if (type) {
		free_string(&type);
	}
}

void json_start(const char *op)
{
	fprintf(stdout, "[\n{ \"type\" : \"start\", \"section\" : \"%s\" },\n", op);
	fflush(stdout);
	json_stream_active = true;
}

void json_end(const char *op, int status)
{
	json_stream_active = false;

	fprintf(stdout, "{ \"type\" : \"end\", \"section\" : \"%s\", \"status\" : %d }\n]\n", op, status);
	fflush(stdout);
}

void log_json(FILE *out UNUSED_PARAM, const char *file UNUSED_PARAM, int line UNUSED_PARAM, const char *label, const char *format, va_list args_list)
{
	if (!json_stream_active) {
		return;
	}

	json_message(label, format, args_list);
}

void json_progress(const char *step_description, unsigned int current_step, unsigned int total_steps, int percentage)
{
	if (!json_stream_active) {
		return;
	}

	fprintf(stdout, "{ \"type\" : \"progress\", "
			"\"currentStep\" : %d, "
			"\"totalSteps\" : %d, "
			"\"stepCompletion\" : %d, "
			"\"stepDescription\" : \"%s\" },\n",
		current_step, total_steps, percentage, step_description);
}
