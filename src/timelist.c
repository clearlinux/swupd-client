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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "memory.h"
#include "swupd.h"
#include "timelist.h"

struct time {
	struct timespec procstart;
	struct timespec procstop;
	struct timespec rawstart;
	struct timespec rawstop;
	char *name;
	bool complete;
	TAILQ_ENTRY(time)
	times;
};

timelist *timelist_new(void)
{
	timelist *head = malloc(sizeof(timelist));
	ON_NULL_ABORT(head);
	TAILQ_INIT(head);

	timelist_timer_start(head, "Total execution time");

	return head;
}

static struct time *alloc_time()
{
	struct time *t = calloc(1, sizeof(struct time));
	ON_NULL_ABORT(t);

	return t;
}

static bool timer_stop(struct time *t)
{
	if (t->complete) {
		return false;
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &t->rawstop);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t->procstop);
	t->complete = true;
	return true;
}

static void timer_stop_all(timelist *head)
{
	struct time *t;

	TAILQ_FOREACH(t, head, times)
	{
		timer_stop(t);
	}
}

/* Fill the time struct for later processing */
void timelist_timer_start(timelist *head, const char *name)
{
	struct time *prev_t;
	char *decorated_name = NULL;
	int count = 0;
	int number_of_spaces = 0;

	if (!head) {
		return;
	}

	/* Only create one element for each start/stop block */
	struct time *t = alloc_time();

	clock_gettime(CLOCK_MONOTONIC_RAW, &t->rawstart);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t->procstart);

	/* look for all elements in the queue and count how many of those
	 * are still not complete, those are "parent" times */
	TAILQ_FOREACH_REVERSE(prev_t, head, timelist, times)
	{
		if (prev_t->complete == false) {
			count++;
		}
	}
	/* we want to indent each one of the child times with four spaces,
	 * starting on the second level child, we don't want any space in the
	 * first child. */
	if (count) {
		number_of_spaces = (count - 1) * 4;
	}

	/* build the decorated name that will be inserted */
	string_or_die(&decorated_name, "%*s%s%s", number_of_spaces, "", count ? "|-- " : "", name);
	t->name = decorated_name;
	t->complete = false;

	TAILQ_INSERT_HEAD(head, t, times);
}

void timelist_timer_stop(timelist *head)
{
	if (!head) {
		return;
	}

	struct time *t;

	TAILQ_FOREACH(t, head, times)
	{
		if (timer_stop(t)) {
			return;
		}
	}
}

void timelist_print_stats(timelist *head)
{
	if (!head) {
		return;
	}

	double delta = 0;
	struct time *t;

	timer_stop_all(head);

	fprintf(stderr, "\nRaw elapsed time stats:\n");
	TAILQ_FOREACH_REVERSE(t, head, timelist, times)
	{
		if (t->complete == true) {
			delta = (t->rawstop.tv_sec - t->rawstart.tv_sec) * 1000 + (t->rawstop.tv_nsec / 1000000.0) - (t->rawstart.tv_nsec / 1000000.0);
			fprintf(stderr, "%10.2f ms: %s\n", delta, t->name);
		}
	}
	fprintf(stderr, "\nCPU process time stats:\n");
	TAILQ_FOREACH_REVERSE(t, head, timelist, times)
	{
		if (t->complete == true) {
			delta = (t->procstop.tv_sec - t->procstart.tv_sec) * 1000 + (t->procstop.tv_nsec / 1000000.0) - (t->procstart.tv_nsec / 1000000.0);
			fprintf(stderr, "%10.2f ms: %s\n", delta, t->name);
		}
	}
}

void timelist_free(timelist *head)
{
	if (!head) {
		return;
	}

	struct time *t;

	while (!TAILQ_EMPTY(head)) {
		t = TAILQ_LAST(head, timelist);
		free_string(&t->name);
		TAILQ_REMOVE(head, t, times);
		free(t);
	}

	free(head);
}
