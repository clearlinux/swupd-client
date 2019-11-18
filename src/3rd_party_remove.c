/*
 *   Software Updater - client side
 *
 *      Copyright © 2019 Intel Corporation.
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
#include "swupd.h"

#ifdef THIRDPARTY

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd 3rd-party remove [repo-name]\n\n");

	global_print_help();
	print("\n");
}

static const struct global_options opts = {
	NULL,
	0,
	NULL,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
#define EXACT_ARG_COUNT 1
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	if (argc == 1) {
		error("The positional args: repo-name is missing\n\n");
		return false;
	}

	/* Ensure that repo remove only expects only one arg: repo-name */
	if ((argc - ind) != EXACT_ARG_COUNT) {
		error("Unexpected arguments\n\n");
		return false;
	}

	return true;
}

enum swupd_code third_party_remove_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int step_in_third_party_remove = 1;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("third-party", step_in_third_party_remove);

	ret = swupd_init(SWUPD_NO_ROOT);
	if (ret != SWUPD_OK) {
		goto finish;
	}

	/* TODO Implement */

	swupd_deinit();

finish:
	progress_finish_steps(ret);
	return ret;
}

#endif
