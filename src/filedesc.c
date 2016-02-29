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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "swupd.h"

void dump_file_descriptor_leaks(void)
{
	DIR *dir;
	struct dirent *entry;

	dir = opendir("/proc/self/fd");
	if (!dir) {
		return;
	}

	while (1) {
		char *filename;
		char buffer[PATH_MAXLEN + 1];
		entry = readdir(dir);
		size_t size;
		if (!entry) {
			break;
		}
		if (strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		/* skip stdin/out/err */
		if (strcmp(entry->d_name, "0") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "1") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "2") == 0) {
			continue;
		}

		/* we hold an fd open, the one from opendir above */
		sprintf(buffer, "%i", dirfd(dir));
		if (strcmp(entry->d_name, buffer) == 0) {
			continue;
		}

		string_or_die(&filename, "/proc/self/fd/%s", entry->d_name);
		memset(&buffer, 0, sizeof(buffer));
		size = readlink(filename, buffer, PATH_MAXLEN);
		if (size) {
			printf("Possible filedescriptor leak: fd_number=\"%s\",fd_details=\"%s\"\n", entry->d_name, buffer);
		}
		free(filename);
	}

	closedir(dir);
}
