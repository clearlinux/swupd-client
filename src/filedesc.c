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
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"

static void foreach_open_fd(void(pf)(int, void *), void *arg)
{
	DIR *dir;
	struct dirent *entry;
	int dir_fd;

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

		n = strtol(entry->d_name, &ep, 10);
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
	string_or_die(&filename, "/proc/self/fd/%d", n);
	if ((size = readlink(filename, buffer, PATH_MAXLEN)) != -1) {
		buffer[size] = '\0'; /* Supply the terminator */
		if (!strstr(buffer, "socket")) {
			fprintf(stderr, "Possible filedescriptor leak: fd_number=\"%d\",fd_details=\"%s\"\n", n, buffer);
		}
	}
	free_string(&filename);
}

void dump_file_descriptor_leaks(void)
{
	foreach_open_fd(&dump_file_descriptor_leaks_int, NULL);
}

/* Internal function */
static void close_fds_int(int n, void *arg)
{
	arg = arg; /* silence warning */
	close(n);
}

void close_fds(void)
{
	foreach_open_fd(&close_fds_int, NULL);
}
