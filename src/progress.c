/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2020 Intel Corporation.
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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "lib/log.h"
#include "lib/macros.h"
#include "progress.h"
#include "swupd_exit_codes.h"

#define PERCENTAGE_UNDEFINED -1
#define PERCENTAGE_OFF -2
#define PERCENTAGE_ON -3

static void default_progress_function(const char *step_description, int local_current_step, int local_total_steps, int percentage);

static const char *title = NULL;
static int total_steps = 0;
static int current_step = 0;
static const char *current_step_title = "";
static int last_percentage;
static bool progress_report_enabled = true;
enum progress_type current_type = PROGRESS_UNDEFINED;
static progress_fn_t progress_function = default_progress_function;
static start_fn_t start_function = NULL;
static end_fn_t end_function = NULL;

struct spinner_data {
	time_t begin;
	int index;
} spinner_data;

void clean_spinner(void)
{
	if (current_type == PROGRESS_UNDEFINED) {
		info("    \r");
	}
}

static int progress_spinner_callback(void *clientp, int64_t UNUSED_PARAM dltotal, int64_t UNUSED_PARAM dlnow, int64_t UNUSED_PARAM ultotal, int64_t UNUSED_PARAM ulnow)
{
	const char spinner[4] = { '|', '/', '-', '\\' };
	struct spinner_data *data = clientp;

	if (time(NULL) > data->begin) {
		data->begin = time(NULL);
		info(" [%c]\r", spinner[data->index]);
		data->index = (data->index + 1) % 4;
		fflush(stdout);
	}

	return 0;
}

static void progress_spinner_end(void)
{
	if (!isatty(fileno(stdout)) || log_get_level() == LOG_DEBUG) {
		return;
	}

	/* clean the spinner from the screen */
	clean_spinner();

	swupd_curl_download_set_progress_callback(NULL, NULL);
}

static void progress_spinner_start(void)
{
	if (!isatty(fileno(stdout)) || log_get_level() == LOG_DEBUG) {
		return;
	}

	spinner_data.index = 0;
	spinner_data.begin = 0;
	swupd_curl_download_set_progress_callback(progress_spinner_callback, &spinner_data);
}

static void default_progress_function(const char UNUSED_PARAM *step_description, int UNUSED_PARAM local_current_step, int UNUSED_PARAM local_total_steps, int percentage)
{
	if (percentage != PERCENTAGE_UNDEFINED &&
	    (percentage < 0 || percentage > 100)) {
		return;
	}

	// Turn spinner on
	if (percentage == PERCENTAGE_UNDEFINED) {
		progress_spinner_start();
		return;
	}

	// Turn spinner off
	if (last_percentage == PERCENTAGE_UNDEFINED) {
		progress_spinner_end();
		return;
	}

	// Don't show 0% on first call (just initizalize function)
	if (last_percentage == PERCENTAGE_OFF) {
		last_percentage = PERCENTAGE_ON;
		return;
	}

	// Don't show 100% if 0 wasn't print
	if (last_percentage < 0) {
		return;
	}

	// Print progress bar
	if (isatty(fileno(stdout))) {
		info(" [%3d%%]\r", percentage);
		if (percentage == 100) {
			info("\n\n");
		}
	} else {
		/* if printing to a file print every percentage
		 * in its own line */
		info(" [%3d%%]\n", percentage);
	}

	fflush(stdout);
}

static void complete_previous_step(void)
{
	if (!progress_report_enabled) {
		return;
	}

	if (current_step > 0 && last_percentage != 100) {
		progress_function(current_step_title, current_step, total_steps, 100);
	}
}

static void start_step()
{
	if (!progress_report_enabled) {
		return;
	}

	if (current_type == PROGRESS_UNDEFINED) {
		progress_function(current_step_title, current_step, total_steps,
				  PERCENTAGE_UNDEFINED);
		last_percentage = PERCENTAGE_UNDEFINED;
	} else {
		progress_function(current_step_title, current_step, total_steps,
				  0);
		last_percentage = 0;
	}
}

void progress_set_format(progress_fn_t progress_fn, start_fn_t start_fn, end_fn_t end_fn)
{
	if (!progress_fn) {
		progress_function = default_progress_function;
	} else {
		progress_function = progress_fn;
	}
	start_function = start_fn;
	end_function = end_fn;
}

void progress_init_steps(const char *steps_title, int steps_in_process)
{
	// If steps_title is invalid or if steps have already been initialized
	if (!steps_title || title) {
		return;
	}

	title = steps_title;
	total_steps = steps_in_process;
	if (start_function) {
		start_function(title);
	}
}

void progress_finish_steps(int status)
{
	// If steps hasn't been initialized
	if (!title) {
		return;
	}

	complete_previous_step();

	if (end_function) {
		end_function(title, status);
	}

	if (current_step != total_steps) {
		debug("Warning: Less steps than expected were executed for the current operation: expected (%d), executed (%d)\n", current_step, total_steps);
	}

	/* reset value of static variables in case a new progress is used */
	title = NULL;
	total_steps = 0;
	current_step = 0;
}

void progress_next_step(const char *step_title, enum progress_type type)
{
	// If steps not initilized
	if (!title) {
		debug("Please initialize the steps\n");
		return;
	}
	debug("Current step name: %s\n", step_title);
	debug("Current step number: %d (out of %d)\n", current_step + 1, total_steps);
	if (current_step >= total_steps) {
		debug("Warning: The current steps exceed the number of steps defined for the current operation\n");
		debug("Increasing the number of total steps by one\n");
		total_steps++;
	}

	complete_previous_step();

	current_step++;
	current_step_title = step_title;
	current_type = type;
	last_percentage = PERCENTAGE_OFF;

	start_step();
}

void progress_report(double count, double max)
{
	int percentage;

	// Ignore progress report if:
	// - progress is disabled
	// - values are incorrect
	// - progress step is undefined
	if (!progress_report_enabled || !progress_function ||
	    max <= 0 || count <= 0 || current_type == PROGRESS_UNDEFINED) {
		return;
	}

	/* Only print when the percentage changes, so a maximum of 100 times per run */
	percentage = (int)(100 * (count / max));

	/* we should never have a percentage bigger than 100% */
	if (percentage > 100) {
		debug("Progress percentage overflow %d%% (Count: %ld, Max: %ld)\n", percentage, (long)count, (long)max);
		percentage = 100;
	}

	if (percentage != last_percentage) {
		progress_function(current_step_title, current_step, total_steps, percentage);
		last_percentage = percentage;
	}
}

void progress_set_enabled(bool enabled)
{
	progress_report_enabled = enabled;
}
