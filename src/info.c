/*
 *   Software Updater - client side
 *
 *      Copyright © 2018 Intel Corporation.
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

#define _GNU_SOURCE
#include <stdio.h>

#include "swupd.h"

void print_update_conf_info()
{
	int current_version = get_current_version(path_prefix);
	printf("Installed version: %d\n", current_version);
	printf("Version URL:       %s\n", version_url);
	printf("Content URL:       %s\n", content_url);
}

int info_main(int UNUSED_PARAM argc, char UNUSED_PARAM **argv)
{
	if (!init_globals()) {
		return EINIT_GLOBALS;
	}

	print_update_conf_info();

	free_globals();
	return 0;
}
