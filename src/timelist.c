/*
 *   Software Updater - client side
 *
 *      Copyright © 2018 Intel Corporation.
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
 */

#define _GNU_SOURCE

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "swupd.h"
#include "timelist.h"

struct time {
	struct timespec procstart;
	struct timespec procstop;
	struct timespec rawstart;
	struct timespec rawstop;
	const char *name;
	bool complete;
	TAILQ_ENTRY(time)
	times;
};

timelist init_timelist(void)
{
	timelist head = TAILQ_HEAD_INITIALIZER(head);
	TAILQ_INIT(&head);
	return head;
}

static struct time *alloc_time()
{
	struct time *t = calloc(1, sizeof(struct time));
	ON_NULL_ABORT(t);

	return t;
}

/* Fill the time struct for later processing */
void grabtime_start(timelist *head, const char *name)
{
	if (verbose_time == false) {
		return;
	}

	/* Only create one element for each start/stop block */
	struct time *t = alloc_time();

	clock_gettime(CLOCK_MONOTONIC_RAW, &t->rawstart);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t->procstart);
	t->name = name;
	t->complete = false;
	TAILQ_INSERT_HEAD(head, t, times);
}

void grabtime_stop(timelist *head)
{
	if (verbose_time == false) {
		return;
	}

	struct time *t;

	TAILQ_FOREACH(t, head, times)
	{
		if (!t->complete) {
			clock_gettime(CLOCK_MONOTONIC_RAW, &t->rawstop);
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t->procstop);
			t->complete = true;
			return;
		}
	}
}

void print_time_stats(timelist *head)
{
	if (verbose_time == false || TAILQ_FIRST(head) == NULL) {
		return;
	}

	double delta = 0;
	struct time *t;

	fprintf(stderr, "\nRaw elapsed time stats:\n");
	TAILQ_FOREACH_REVERSE(t, head, timelist, times)
	{
		if (t->complete == true) {
			delta = (t->rawstop.tv_sec - t->rawstart.tv_sec) * 1000 + (t->rawstop.tv_nsec / 1000000.0) - (t->rawstart.tv_nsec / 1000000.0);
			fprintf(stderr, "%10.2f ms: %s\n", delta, t->name);
		}
	}
	fprintf(stderr, "\nCPU process time stats:\n");
	while (!TAILQ_EMPTY(head)) {
		t = TAILQ_LAST(head, timelist);
		if (t->complete == true) {
			delta = (t->procstop.tv_sec - t->procstart.tv_sec) * 1000 + (t->procstop.tv_nsec / 1000000.0) - (t->procstart.tv_nsec / 1000000.0);
			fprintf(stderr, "%10.2f ms: %s\n", delta, t->name);
		}
		TAILQ_REMOVE(head, t, times);
		free(t);
	}
}
