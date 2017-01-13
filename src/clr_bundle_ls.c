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
 *         Mario Alfredo Carrillo Arevalo <mario.alfredo.c.arevalo@intel.com>
 *         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
 */

#define _GNU_SOURCE
#include "swupd.h"
#include "config.h"

int bundle_list_main(int argc, char **argv)
{
	int current_version;
	int lock_fd;
	int ret;
	struct list *list_bundles = NULL;
	struct list *item = NULL;

	copyright_header("bundle list");

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		printf("Error: Failed updater initialization. Exiting now\n");
		return ret;
	}

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		printf("Error: Unable to determine current OS version\n");
		v_lockfile(lock_fd);
		return ECURRENT_VERSION;
	}

	read_local_bundles(&list_bundles);

	item = list_head(list_bundles);

	while (item) {
		printf("%s\n", (char *)item->data);
		item = item->next;
	}

	printf("Current OS version: %d\n", current_version);

	list_free_list(list_bundles);

	return ret;
}
