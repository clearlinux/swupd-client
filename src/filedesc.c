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
 *         Arjan van de Ven <arjan@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"

/* global structure to hold open file descriptors at startup time */
static fd_set open_fds;

/* This function takes 2 parameters, one is a function pointer and the
 * second is void* and so can be almost anything.
 *
 * This loops over all the file descriptors that are open according to
 * /proc/self/fd, ignoring stdin, stdout, stderr and the file
 * descriptor that is being used to read the directory. For any
 * additional ones it calls the user supplied function with 2
 * parameters, the file descriptor number and the void* that is passed
 * in.
 *
 * Currently there is no immediate uses for this void*, but by passing
 * a pointer to a struct additional parameters could be passed in, and
 * results returned via elements of the struct rather than using
 * global values.
 */
static void foreach_open_fd(void(pf)(int, void *), void *arg)
{
	DIR *dir;
	struct dirent *entry;
	int dir_fd;
	int err;

	dir = opendir("/proc/self/fd");
	if (!dir) {
		return;
	}
	/* we hold an fd open, the one from opendir above */
	dir_fd = dirfd(dir);

	while ((entry = readdir(dir)) != NULL) {
		int n;
		char *ep;

		/* Convert it to an integer */
		if ((entry->d_name[0] < '0') || (entry->d_name[0] > '9')) {
			/* ignore anthing that is not a number */
			continue;
		}

		err = strtoi_err_endptr(entry->d_name, &ep, &n);
		if (err != 0) {
			warn("invalid fd\n");
		}
		if (*ep) {
			continue; /* Trailing non digits */
		}

		/* Skip stdin, stdout, stderr and the file descriptor we are using */
		if (n == 0 || n == 1 || n == 2 || n == dir_fd) {
			continue;
		}
		(*pf)(n, arg); /* Call user function */
	}

	closedir(dir);
}

/* Provide for passing and returning an arg
 * Internal function
 */
static void dump_file_descriptor_leaks_int(int n, void *a)
{
	char *filename;
	char buffer[PATH_MAXLEN + 1];
	ssize_t size;
	a = a; /* Silence warning */
	if (FD_ISSET(n, &open_fds)) {
		return; /* Was already open */
	}
	string_or_die(&filename, "/proc/self/fd/%d", n);
	if ((size = readlink(filename, buffer, PATH_MAXLEN)) != -1) {
		buffer[size] = '\0'; /* Supply the terminator */
		if (!strstr(buffer, "socket") &&
		    !strstr(buffer, "/dev/random") &&
		    !strstr(buffer, "/dev/urandom")) {
			info("Possible filedescriptor leak: fd_number=\"%d\",fd_details=\"%s\"\n", n, buffer);
		}
	}
	free_string(&filename);
}

void dump_file_descriptor_leaks(void)
{
	foreach_open_fd(&dump_file_descriptor_leaks_int, NULL);
}

/* Internal function */
static void record_fds_int(int n, void *arg)
{
	arg = arg; /* silence warning */
	FD_SET(n, &open_fds);
}

void record_fds(void)
{
	FD_ZERO(&open_fds); /* Clean slate */
	foreach_open_fd(&record_fds_int, NULL);
}
