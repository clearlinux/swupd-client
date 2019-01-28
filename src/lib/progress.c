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
 *   Authors:
 *         Otavio Pontes <otavio.pontes@intel.com>
 *
 */

#define _GNU_SOURCE

#include <stdbool.h>
#include <sys/ioctl.h>
#include <unistd.h>


#include "progress.h"
#include "log.h"

#define PREFIX_MINIMUM ((int)sizeof("(100000/100000)") - 1)
#define BAR_MINIMUM ((int)sizeof(" [] ") - 1)
#define PERCENTAGE_MINIMUM ((int)sizeof("000%") - 1)
#define TEMPLATE_MINIMUM (PREFIX_MINIMUM + BAR_MINIMUM + PERCENTAGE_MINIMUM)
#define FILE_COLS (80)

struct progress {
	int total;
	const char *header;
	bool terminal;
	int next_percentage;
} progress;

static void print_bar(int percentage, int progress_bar_size)
{
	int i, progress, bar_size;

	bar_size = progress_bar_size - BAR_MINIMUM;
	progress = ((int)(((float)percentage * bar_size) / 100));

	fprintf(stderr, " [");
	for (i = 0; i < progress; i++) {
		fprintf(stderr, "%c", '=');
	}
	fprintf(stderr, "%*s", bar_size - progress, "");
	fprintf(stderr, "] ");
}

static void print_text(const char *text, int text_size)
{
	int printed;

	if (text_size <= 0) {
		return;
	}

	printed = fprintf(stderr, "%.*s", text_size, text);
	fprintf(stderr, "%*s", text_size - printed, "");
}

static void print_progress_line(int current, int percentage, int cols, const char *text)
{
	int progress_bar_size, printed, text_size;

	if (cols < TEMPLATE_MINIMUM) {
		goto percentage;
	}

	progress_bar_size = cols / 3 + BAR_MINIMUM; // 1/3 of the total area

	printed = fprintf(stderr, "(%d/%d) ", current, progress.total);
	text_size = cols - printed - progress_bar_size - PERCENTAGE_MINIMUM;
	print_text(text, text_size);
	print_bar(percentage, progress_bar_size);

percentage:
	fprintf(stderr, "%2d%%", percentage);
	fflush(stderr);
}

static void progress_update_file(int current, int percentage, const char *partial_text)
{
	if (!partial_text) {
		if (percentage < progress.next_percentage)
			return;
		partial_text = progress.header;
		progress.next_percentage = (((int)(percentage / 10)) * 10) + 10;
	}

	print_progress_line(current, percentage, FILE_COLS, partial_text);
	fprintf(stderr, "\n");
}

void progress_start(char *header, int total)
{
	progress.header = header;
	progress.total = total;
	progress.terminal = isatty(fileno(stderr));
	progress_update(0, NULL);
}

void progress_update(int current, const char *partial_text)
{
	int cols, percentage;
	struct winsize w;

	percentage = (int)((float)(current * 100)/ progress.total);

	if (!progress.terminal) {
		progress_update_file(current, percentage, partial_text);
		return;
	}

	// Get terminal size
	ioctl(0, TIOCGWINSZ, &w);
	cols = w.ws_col;

	if (cols < PERCENTAGE_MINIMUM) {
		return;
	}

	fprintf(stderr, "\r");
	print_progress_line(current, percentage, cols, progress.header);
}

void progress_end()
{
	progress_update(progress.total, NULL);
	fprintf(stderr, "\n");

	progress.header = "";
	progress.total = 0;
}
