/*
 *   Software Updater - client side
 *
 *      Copyright © 2017-2020 Intel Corporation.
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
#include "verifytime.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DAY_SECONDS 86400

/*
 * Functions and macros copied from sys.c and string.h to maintain compatibility
 * without adding more dependencies.
 */
#define PATH_SEPARATOR '/'
#define str_len(_str) strnlen(_str, INT64_MAX)
static char *sys_path_join(const char *fmt, ...)
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

	len = str_len(path);
	char *pretty_path = calloc(1, str_len(path) + 1);
	if (!pretty_path) {
		abort();
	}

	/* remove all duplicated PATH_SEPARATOR from the path */
	for (i = j = 0; i < len; i++) {
		if (path[i] == PATH_SEPARATOR && // Is separator and
		    // Next is also a separator or
		    (path[i + 1] == PATH_SEPARATOR ||
		     // Is a trailing separator, but not root
		     (path[i + 1] == '\0' && j != 0))) {
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

static int get_versionstamp(char *path_prefix, time_t *timestamp)
{
	FILE *fp = NULL;
	char data[11];
	char *filename;
	int err = 0;

	filename = sys_path_join("%s/usr/share/clear/versionstamp", path_prefix ? path_prefix : "");

	errno = 0;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		err = -errno;
		*timestamp = 0;
		goto exit;
	}

	if (fgets(data, 11, fp) == NULL) {
		err = -EIO;
		*timestamp = 0;
		goto close_and_exit;
	}

	/* If we read a 0 the versionstamp is wrong/corrupt */
	errno = 0;
	*timestamp = strtol(data, NULL, 10);
	if (errno != 0) {
		err = -errno;
		*timestamp = 0;
	}
close_and_exit:
	fclose(fp);
exit:
	free(filename);
	return err;
}

bool set_time(time_t mtime)
{
	struct timespec clock_time = {
		.tv_sec = mtime,
		.tv_nsec = 0,
	};

	if (clock_settime(CLOCK_REALTIME, &clock_time) < 0) {
		return false;
	}

	return true;
}

char *time_to_string(time_t t)
{
	struct tm *timeinfo;
	char time_str[50] = { 0 };

	timeinfo = localtime(&t);
	if (!timeinfo) {
		return NULL;
	}

	strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", timeinfo);

	return strdup(time_str);
}

/* TODO: Get even better time than the versionstamp using a collection of servers,
 * and fallback to using versionstamp time if it does not work or seem reasonable.
 * The system time wasn't sane, so set it here and try again */
int verify_time(char *path_prefix, time_t *system_time, time_t *clear_time)
{
	time_t currtime;
	time_t versiontime;
	int err;

	time(&currtime);
	if (system_time) {
		*system_time = currtime;
	}

	err = get_versionstamp(path_prefix, &versiontime);
	if (err || versiontime == 0) {
		return -ENOENT;
	}
	if (clear_time) {
		*clear_time = versiontime;
	}

	/* Give it a day's worth of tolerance */
	if (currtime < (versiontime - DAY_SECONDS)) {
		return -ETIME;
	}

	return 0;
}
