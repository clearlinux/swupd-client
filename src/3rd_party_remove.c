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
#include "3rd_party_repos.h"
#include "swupd.h"

#include <errno.h>

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

	int positional_args = argc - ind;
	/* Ensure that repo remove expects only one args: repo-name */
	switch (positional_args) {
	case 0:
		error("The positional args: repo-name is missing\n\n");
		return false;
	case EXACT_ARG_COUNT:
		return true;
	default:
		error("Unexpected arguments\n\n");
		return false;
	}
	return false;
}

enum swupd_code third_party_remove_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int step_in_third_party_remove = 0;
	int err;
	char *name = NULL;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_NO_ROOT);
	if (ret != SWUPD_OK) {
		return ret;
	}

	progress_init_steps("third-party-remove", step_in_third_party_remove);

	/* The last argument has to be the repo-name to be deleted */
	name = argv[argc - 1];
	info("Removing repository %s...\n", name);
	err = third_party_remove_repo(name);
	if (err < 0) {
		if (err == -ENOENT) {
			ret = SWUPD_INVALID_OPTION;
		} else {
			ret = SWUPD_COULDNT_WRITE_FILE;
		}
		goto exit;
	}

	if (third_party_remove_repo_directory(name) < 0) {
		ret = SWUPD_COULDNT_REMOVE_FILE;
	}

exit:
	if (ret == SWUPD_OK) {
		print("\nRepository and its content removed successfully\n");
	} else {
		print("\nFailed to remove repository\n");
	}
	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}

#endif
