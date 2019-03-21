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
	info("Installed version: %d\n", current_version);
	info("Version URL:       %s\n", version_url);
	info("Content URL:       %s\n", content_url);
}

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd info [OPTION...]\n\n");

	global_print_help();
	fprintf(stderr, "\n");
}

static const struct global_options opts = {
	NULL,
	0,
	NULL,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	if (argc > ind) {
		error("Unexpected arguments\n\n");
		return false;
	}

	return true;
}

enum swupd_code info_main(int UNUSED_PARAM argc, char UNUSED_PARAM **argv)
{
	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	if (!init_globals()) {
		return SWUPD_INIT_GLOBALS_FAILED;
	}

	print_update_conf_info();

	free_globals();
	return 0;
}
