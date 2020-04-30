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

#include "lib/log.h"
#include "lib/macros.h"
#include "lib/sys.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define DAY_SECONDS 86400

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
	FREE(filename);
	return err;
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
	time_t versiontime;
	int err;

	time(&currtime);
	timeinfo = localtime(&currtime);

	err = get_versionstamp(path_prefix, &versiontime);
	if (err || versiontime == 0) {
		return false;
	}

	/* Give it a day's worth of tolerance */
	if (currtime < (versiontime - DAY_SECONDS)) {
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
