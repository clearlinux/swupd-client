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

#ifdef THIRDPARTY

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd 3rd-party add [repo-name] [repo-URL]\n\n");

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
#define EXACT_ARG_COUNT 2
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	int positional_args = argc - ind;
	/* Ensure that repo add expects only two args: repo-name, repo-url */
	switch (positional_args) {
	case 0:
		error("The positional args: repo-name and repo-url are missing\n\n");
		return false;
	case 1:
		error("The positional args: repo-url is missing\n\n");
		return false;
	case EXACT_ARG_COUNT:
		return true;
	default:
		error("Unexpected arguments\n\n");
		return false;
	}
	return false;
}

enum swupd_code third_party_add_main(int argc, char **argv)
{

	enum swupd_code ret = SWUPD_OK;
	const int step_in_third_party_add = 1;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("third-party", step_in_third_party_add);

	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		goto finish;
	}

	if (!is_valid_url(argv[argc - 1])) {
		ret = SWUPD_COULDNT_WRITE_FILE;
		error("Invalid argument: repo-url\n");
		goto finish;
	}

	/* The last two in reverse are the repo-name, repo-url */
	if (add_repo_config(argv[argc - 2], argv[argc - 1])) {
		ret = SWUPD_COULDNT_WRITE_FILE;
		error("Failed to add repo: %s to config\n", argv[argc - 2]);
	} else {
		info("Repository %s added successfully\n", argv[argc - 2]);
	}

	swupd_deinit();

finish:
	progress_finish_steps(ret);
	return ret;
}

#endif
