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

#include "macros.h"
#include "timelist.h"
#include <sys/queue.h>

typedef TAILQ_HEAD(timelist_head, time) timelist_head;

struct timelist {
	timelist_head head;
};

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

struct timelist *timelist_new(void)
{
	struct timelist *list= malloc(sizeof(struct timelist));
	ON_NULL_ABORT(list);
	TAILQ_INIT(&list->head);

	timelist_timer_start(list, "Total execution time");

	return list;
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

static void timer_stop_all(struct timelist *list)
{
	struct time *t;

	TAILQ_FOREACH(t, &list->head, times)
	{
		timer_stop(t);
	}
}

/* Fill the time struct for later processing */
void timelist_timer_start(struct timelist *list, const char *name)
{
	if (!list) {
		return;
	}

	/* Only create one element for each start/stop block */
	struct time *t = alloc_time();

	clock_gettime(CLOCK_MONOTONIC_RAW, &t->rawstart);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t->procstart);
	t->name = name;
	t->complete = false;
	TAILQ_INSERT_HEAD(&list->head, t, times);
}

void timelist_timer_stop(struct timelist *list)
{
	if (!list) {
		return;
	}

	struct time *t;

	TAILQ_FOREACH(t, &list->head, times)
	{
		if (timer_stop(t)) {
			return;
		}
	}
}

void timelist_print_stats(struct timelist *list)
{
	if (!list) {
		return;
	}

	double delta = 0;
	struct time *t;

	timer_stop_all(list);

	fprintf(stderr, "\nRaw elapsed time stats:\n");
	TAILQ_FOREACH_REVERSE(t, &list->head, timelist_head, times)
	{
		if (t->complete == true) {
			delta = (t->rawstop.tv_sec - t->rawstart.tv_sec) * 1000 + (t->rawstop.tv_nsec / 1000000.0) - (t->rawstart.tv_nsec / 1000000.0);
			fprintf(stderr, "%10.2f ms: %s\n", delta, t->name);
		}
	}
	fprintf(stderr, "\nCPU process time stats:\n");
	TAILQ_FOREACH_REVERSE(t, &list->head, timelist_head, times)
	{
		if (t->complete == true) {
			delta = (t->procstop.tv_sec - t->procstart.tv_sec) * 1000 + (t->procstop.tv_nsec / 1000000.0) - (t->procstart.tv_nsec / 1000000.0);
			fprintf(stderr, "%10.2f ms: %s\n", delta, t->name);
		}
	}
}

void timelist_free(struct timelist *list)
{
	if (!list) {
		return;
	}

	struct time *t;

	while (!TAILQ_EMPTY(&list->head)) {
		t = TAILQ_LAST(&list->head, timelist_head);
		TAILQ_REMOVE(&list->head, t, times);
		free(t);
	}

	free(list);
}
