/*
 *   Software Updater - client side
 *
 *      Copyright © 2018-2020 Intel Corporation.
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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "macros.h"
#include "thread_pool.h"

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
			error("Thread communication failed: %d - %s\n",
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

	if (num_threads < 0) {
		error("Number of threads (%d) shouldn't be negative\n", num_threads);
		return NULL;
	}

	tp = malloc_or_die(sizeof(struct tp) + num_threads * sizeof(pthread_t *));

	tp->num_threads = num_threads;
	if (num_threads == 0) {
		//We don't need to create threads
		return tp;
	}

	// Create threads communication
	if (pipe2(tp->pipe_fds, O_DIRECT | O_CLOEXEC) < 0) {
		debug("No support for pipe2(..., O_DIRECT | O_CLOEXEC) syscall\n");
		debug("Error %d - %s\n", errno, strerror(errno));
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
	FREE(tp);
	return NULL;
}

int tp_task_schedule(struct tp *tp, tp_task_run_t run, void *data)
{
	int r = -1;
	struct task task;

	if (tp->num_threads == 0) {
		// Run task
		run(data);
		return 0;
	}

	// Use threads
	task.run = run;
	task.data = data;

	while (r < 0) {
		r = write(tp->pipe_fds[1], &task, sizeof(task));
		if (r < 0) {
			if (errno == EINTR) {
				// Not an error, we can continue
				continue;
			}
			error("Thread pool task scheduling failed: %d - %s\n",
			      errno, strerror(errno));
			return r;
		}
	}

	return 0;
}

void tp_complete(struct tp *tp)
{
	int i;
	if (!tp) {
		return;
	}

	if (tp->num_threads == 0) {
		goto free_tp;
	}

	// Close pipe so threads will get an EOF when all tasks are completed
	close(tp->pipe_fds[1]);

	for (i = 0; i < tp->num_threads; i++) {
		pthread_join(tp->threads[i], NULL);
	}

	close(tp->pipe_fds[0]);

free_tp:
	FREE(tp);
}

int tp_get_num_threads(struct tp *tp)
{
	if (!tp) {
		error("Invalid thread pool: NULL\n");
		return 0;
	}

	return tp->num_threads;
}
