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
 */
#include "verifytime.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	time_t clear_time = 0, system_time = 0;
	int err;
	char *system_time_str;
	char *clear_time_str;

	err = verify_time(NULL, &system_time, &clear_time);
	if (err == 0) {
		return 0; // Success
	}

	if (err == -ENOENT) {
		fprintf(stderr, "Failed to read system timestamp file\n");
	}

	system_time_str = time_to_string(system_time);
	clear_time_str = time_to_string(clear_time);
	if (system_time_str && clear_time_str) {
		printf("Fixing outdated system time from '%s' to '%s'\n", system_time_str, clear_time_str);
	}
	free(system_time_str);
	free(clear_time_str);

	if (set_time(clear_time)) {
		return 0; // Success
	}

	fprintf(stderr, "Failed to update system time\n");
	return 1; // Error
}
