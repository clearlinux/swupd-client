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
#include "3rd_party_internal.h"
#include "lib/sys.h"

#ifdef THIRDPARTY

static int remove_repo_directory(char *repo_name)
{
	char *repo_dir;
	int ret = 0;
	repo_dir = get_repo_path(repo_name);
	if (is_dir(repo_dir)) {
		ret = sys_rm_recursive(repo_dir);
		if (ret) {
			error("Failed to delete repository directory\n");
		}
	}
	free_string(&repo_dir);
	return ret;
}

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

	/* The last argument has to be the repo-name to be deleted */
	if (remove_repo_from_config(argv[argc - 1])) {
		ret = SWUPD_NO;
	} else {
		if (remove_repo_directory(argv[argc - 1])) {
			ret = SWUPD_NO;
		} else {
			info("Repository %s and its contents removed successfully\n", argv[argc - 1]);
		}
	}

	swupd_deinit();

finish:
	progress_finish_steps(ret);
	return ret;
}

#endif
