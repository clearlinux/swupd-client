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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define DAY_SECONDS 86400

unsigned long int get_versionstamp(void)
{
	struct stat statt;
	FILE *fp = NULL;
	char data[11];
	const char *filename = "/usr/share/clear/versionstamp";

	if (stat(filename, &statt) == -1) {
		printf("%s does not exist!\n", filename);
		return 0;
	}

	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Failed to open %s\n", filename);
		return 0;
	}

	if (fgets(data, 11, fp) == NULL) {
		printf("Failed to read %s\n", filename);
		fclose(fp);
		return 0;
	}

	/* If we read a 0 the versionstamp is wrong/corrupt */
	if (strtoul(data, NULL, 10) == 0) {
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return strtoul(data, NULL, 10);
}

bool set_time(time_t mtime, char *time)
{
	if (stime(&mtime) != 0) {
		printf("Failed to set system time");
		return false;
	}
	printf("Set system time to %s\n", time);
	return true;
}

int main()
{
	time_t currtime;
	struct tm *timeinfo;
	unsigned long int versionstamp;

	time(&currtime);
	timeinfo = localtime(&currtime);

	versionstamp = get_versionstamp();
	if (versionstamp == 0) {
		return 1;
	}
	time_t versiontime = (time_t)versionstamp;

	/* Give it a day's worth of tolerance */
	if ((unsigned long int)currtime < (versionstamp - DAY_SECONDS)) {
		/* TODO: Get even better time than the versionstamp using a collection of servers,
		 * and fallback to using versionstamp time if it does not work or seem reasonable.
		 * The system time wasn't sane, so set it here and try again */
		printf("Warning: Current time is %s\nAttempting to fix...", asctime(timeinfo));
		if (set_time(versiontime, asctime(timeinfo)) == false) {
			return 1;
		}
	}

	return 0;
}
