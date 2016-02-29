/*
 *   Software Updater - client side
 *
 *      Copyright © 2014-2016 Intel Corporation.
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
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *         Bryce Patel <bryce.patel@intel.com>
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "swupd.h"
#define PROTECTED_FILE "./test/protected_data"
#define NUM_THREADS 8
#define NUM_ACCESS 4

/* Attempt to get a write lock protecting the file test/protected_data using
 * p_lockfile(). If successful, no two processess should be able to hold the
 * lock similtaneously.
 */
static void work(int id)
{
	int lock_fd;
	int successes = 0;

	FILE *file;
	char buffer[LINE_MAX];

	while (successes < NUM_ACCESS) {
		int prev_id = -1;
		char prev_action = ' ';
		char c = ' ';

		// Sleep for a random amount of time, up to half a second so
		// not all processes attempt to access lock at once.
		usleep(rand() % 500000);

		// Attempt to access the lock
		lock_fd = p_lockfile();
		if (lock_fd < 0) {
			printf("process %i unable to acquire lock\n", id);
			fflush(stdout);

			continue;
		}

		printf("process %i acquired lock\n", id);

		file = fopen(PROTECTED_FILE, "a+");

		fseek(file, 0, SEEK_END);

		// Check if file is empty
		if (ftell(file) != 0) {
			// Go to start of last line
			while (c != '\n' && fseek(file, -2, SEEK_CUR) == 0) {
				c = getc(file);
			}

			// If the file only has one line, return to start of file
			if (c != '\n') {
				rewind(file);
			}

			// Read last line of protected_data
			sscanf(fgets(buffer, LINE_MAX, file), "%i%c\n", &prev_id, &prev_action);
		}

		fprintf(file, "%ip\n", id);
		fflush(file);

		if (prev_action == 'p') {
			printf("process %i accessed lock being held by process %i\n", id, prev_id);
			printf("process %i aborting\n\n", id);
			return;
		}

		// Sleep for up to a fifth of a second while holding lock. During this time
		// other processes should try to access lock and fail. This sleep is shorter
		// than the one before attempting to get the lock because we don't need to
		// wait for all other processes to attempt to access lock, just some of them
		usleep(rand() % 200000);

		fseek(file, 0, SEEK_END);

		if (ftell(file) == 0) {
			printf("process %i failed to write to protected_data", id);
			printf("process %i aborting\n\n", id);
		} else {
			char c = ' ';
			while (c != '\n' && fseek(file, -2, SEEK_CUR) == 0) {
				c = getc(file);
			}

			if (c != '\n') {
				rewind(file);
			}

			sscanf(fgets(buffer, LINE_MAX, file), "%i%c\n", &prev_id, &prev_action);
		}

		// Log that this thread released the lock
		fprintf(file, "%iv\n", id);
		fclose(file);

		if (prev_action != 'p' || prev_id != id) {
			if (prev_id == -1) {
				printf("process %i released lock without holding it\n", id);
			} else {
				printf("process %i and process %i held lock similtaneously\n", id, prev_id);
			}
			printf("process %i aborting\n\n", id);
			return;
		}

		printf("process %i released lock\n\n", id);
		v_lockfile(lock_fd);
		successes++;
	}
	return;
}

int main(int UNUSED_PARAM argc, char UNUSED_PARAM **argv)
{
	int ret = EXIT_FAILURE;
	int t_cnt;
	pid_t pid[NUM_THREADS];
	FILE *file;
	int id, prev_id = -1;
	char action, prev_action = ' ';
	char buffer[LINE_MAX];

	unlink(PROTECTED_FILE);

	// Create multiple processess to attempt to access the lock
	for (t_cnt = 0; t_cnt < NUM_THREADS; t_cnt++) {
		pid[t_cnt] = fork();

		// Child processes will execute work() then exit
		if (pid[t_cnt] == 0) {
			work(t_cnt);
			return EXIT_SUCCESS;
		}
	}

	// From here on is executed only by parent process

	for (t_cnt = 0; t_cnt < NUM_THREADS; t_cnt++) {
		wait(&ret);
	}

	file = fopen(PROTECTED_FILE, "r+");
	if (file == NULL) {
		printf("Log file could not be opened\n");
		ret = -1;
	}

	printf("Test results:\n");
	// Makes sure each time the lock is accessed, no other processes can access it until
	// the process holding it released it.
	while (fgets(buffer, LINE_MAX, file) != NULL && ret >= 0) {
		sscanf(buffer, "%i%c\n", &id, &action);
		if (prev_action == 'p') {
			if (action == 'p') {
				if (prev_id == id) {
					// Happens when a process holding the lock accesses it again
					// protected_data will look like "0p0p"
					printf("process %i accessed lock twice without closing\n", id);
					ret = -1;
				} else {
					// Happens when two processes hold the lock at the same time
					// protected_data will look like "0p1p"
					// In this case, lock is likely not locking properly
					// If the lock isn't working, this is the most likely error message
					printf("process %i accessed lock being held by process %i\n", id, prev_id);
					ret = -1;
				}
			} else if (prev_id != id) {
				// Happens when a lock being held by one process is closed by another process.
				// protected_data will look like "0p1v"
				// In this case, lock is likely not locking properly
				printf("process %i released lock being held by process %i\n", id, prev_id);
				ret = -1;
			}
		} else if (action == 'v') {
			if (prev_action == 'v') {
				// Happens when two processes hold the lock at the same time and then release it
				// protected_data will look like "0v1v"
				// In this case, lock is likely not locking properly
				printf("process %i released lock after process %i released it, without accessing first\n", id, prev_id);
				ret = -1;
			} else if (prev_id == -1) {
				// Happens if a lock is released without any process ever accessing it
				// protected_data will start with something like "0v"
				printf("process %i released lock without holding it\n", id);
				ret = -1;
			}
		}

		prev_action = action;
		prev_id = id;
	}

	fclose(file);

	if (ret >= 0) {
		printf("Lock passed the test\n");
	} else {
		printf("Lock failed the test\n");
	}
	return ret;
}
