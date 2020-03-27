/*
 *   Software Updater - client side
 *
 *      Copyright © 2017 Intel Corporation.
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
 *         Tudor Marcu <tudor.marcu@intel.com>
 *
 */
#define _GNU_SOURCE
#include "verifytime.h"

#include "lib/log.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define DAY_SECONDS 86400
#define PATH_SEPARATOR '/'

/* this function is a copy from sys_path_join() from lib/sys.c */
static char *path_join(const char *fmt, ...)
{
	char *path;
	va_list ap;
	int len;
	int i, j;

	/* merge arguments into one path */
	va_start(ap, fmt);
	if (vasprintf(&path, fmt, ap) < 0) {
		abort();
	}
	va_end(ap);

	len = strlen(path);
	char *pretty_path = malloc(strlen(path) + 1);
	if (!pretty_path) {
		abort();
	}

	/* remove all duplicated PATH_SEPARATOR from the path */
	for (i = j = 0; i < len; i++) {
		if (path[i] == PATH_SEPARATOR && path[i + 1] == PATH_SEPARATOR) {
			/* duplicated PATH_SEPARATOR, throw it away */
			continue;
		}
		pretty_path[j] = path[i];
		j++;
	}
	pretty_path[j] = '\0';

	free(path);

	return pretty_path;
}

static unsigned long int get_versionstamp(char *path_prefix)
{
	FILE *fp = NULL;
	char data[11];
	char *filename;
	unsigned long int version_num;

	filename = path_join("%s/usr/share/clear/versionstamp", path_prefix ? path_prefix : "");

	errno = 0;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		if (errno == ENOENT) {
			warn("%s does not exist!\n", filename);
		} else {
			error("Failed to open %s\n", filename);
		}
		version_num = 0;
		goto exit;
	}

	if (fgets(data, 11, fp) == NULL) {
		error("Failed to read %s\n", filename);
		version_num = 0;
		goto close_and_exit;
	}

	/* If we read a 0 the versionstamp is wrong/corrupt */
	errno = 0;
	version_num = strtoul(data, NULL, 10);
	if (errno != 0) {
		version_num = 0;
	}
close_and_exit:
	fclose(fp);
exit:
	free(filename);
	return version_num;
}

static bool set_time(time_t mtime)
{
	struct timespec clock_time = {
		.tv_sec = mtime,
		.tv_nsec = 0,
	};

	if (clock_settime(CLOCK_REALTIME, &clock_time) < 0) {
		error("Failed to set system time\n");
		return false;
	}

	info("Set system time to %s\n", ctime(&mtime));
	return true;
}

bool verify_time(char *path_prefix)
{
	time_t currtime;
	struct tm *timeinfo;
	unsigned long int versionstamp;

	time(&currtime);
	timeinfo = localtime(&currtime);

	versionstamp = get_versionstamp(path_prefix);
	if (versionstamp == 0) {
		return false;
	}
	time_t versiontime = (time_t)versionstamp;

	/* Give it a day's worth of tolerance */
	if ((unsigned long int)currtime < (versionstamp - DAY_SECONDS)) {
		/* TODO: Get even better time than the versionstamp using a collection of servers,
		 * and fallback to using versionstamp time if it does not work or seem reasonable.
		 * The system time wasn't sane, so set it here and try again */
		char time_str[50];

		strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", timeinfo);
		warn("Current time is %s\nAttempting to fix...", time_str);
		if (set_time(versiontime) == false) {
			return false;
		}
	}

	return true;
}
