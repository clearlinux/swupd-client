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
 *
 *   Authors:
 *         Otavio Pontes <otavio.pontes@intel.com>
 *
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"
#include "tp.h"

/*
 * To keep this thread pool implementation simple, pipes were used instead of
 * mutex locks using the fact that read() and write() operations are thread safe.
 * So in order to schedule a task to be executed we just need to write() a task
 * in to the pipe and the first thread to read() that data will execute the
 * processing.
 */

struct tp {
	int num_threads;
	int pipe_fds[2];
	pthread_t threads[];
};

struct task {
	tp_task_run_t run;
	void *data;
};

static void *thread_run(void *data)
{
	int *fd;
	ssize_t r;
	struct task task;

	fd = data;

	while (1) {
		r = read(*fd, &task, sizeof(task));
		if (r == 0) {
			// EOF: Close thread
			return NULL;
		} else if (r < 0 || r != sizeof(task)) {
			if (errno == EINTR) {
				// Not an error, we can continue
				continue;
			}
			fprintf(stderr, "Error - Thread communication failed: %d - %s\n",
				errno, strerror(errno));
			return NULL;
		}

		// Run task
		task.run(task.data);
	}
	return NULL;
}

struct tp *tp_start(int num_threads)
{
	int i;
	struct tp *tp;

	tp = calloc(1, sizeof(struct tp) + num_threads * sizeof(pthread_t *));
	ON_NULL_ABORT(tp);

	tp->num_threads = num_threads;
	if (pipe2(tp->pipe_fds, O_DIRECT | O_CLOEXEC) < 0) {
		goto error;
	}

	// Create threads
	for (i = 0; i < num_threads; i++) {
		if (pthread_create(&tp->threads[i], NULL, thread_run, &tp->pipe_fds[0]) != 0) {
			tp->num_threads = i;
			goto error_threads;
		}
	}

	return tp;

error_threads:
	tp_complete(tp);
	return NULL;

error:
	free(tp);
	return NULL;
}

int tp_task_schedule(struct tp *tp, tp_task_run_t run, void *data)
{
	int r = -1;
	struct task task;

	task.run = run;
	task.data = data;

	while (r < 0) {
		r = write(tp->pipe_fds[1], &task, sizeof(task));
		if (r < 0) {
			if (errno == EINTR) {
				// Not an error, we can continue
				continue;
			}
			fprintf(stderr, "Error: thread pool task scheduling failed: %d - %s",
				errno, strerror(errno));
			return r;
		}
	}

	return 0;
}

void tp_complete(struct tp *tp)
{
	int i;

	// Close pipe so threads will get an EOF when all tasks are completed
	close(tp->pipe_fds[1]);

	for (i = 0; i < tp->num_threads; i++) {
		pthread_join(tp->threads[i], NULL);
	}

	close(tp->pipe_fds[0]);
	free(tp);
}
