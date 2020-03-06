/*
 *   Software Updater - client side
 *
 *      Copyright © 2020 Intel Corporation.
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

#ifdef THIRDPARTY

static char *cmdline_repo = NULL;

static void print_help(void)
{
	print("Shows the current version of 3rd-party repositories and the URLs used for updates\n\n");
	print("Usage:\n");
	print("   swupd info [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo              Specify the 3rd-party repository to use\n");
}

static const struct option prog_opts[] = {
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 'R':
		cmdline_repo = strdup_or_die(optarg);
		return true;
	default:
		return false;
	}
	return false;
}

static const struct global_options opts = {
	prog_opts,
	sizeof(prog_opts) / sizeof(struct option),
	parse_opt,
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

static enum swupd_code show_repo_info(UNUSED_PARAM char *unused)
{
	return print_update_conf_info();
}

enum swupd_code third_party_info_main(int UNUSED_PARAM argc, char UNUSED_PARAM **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_third_party_info = 0;
	/* there is no need to report in progress for init at this time */

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	/* initialize swupd */
	ret_code = swupd_init(SWUPD_NO_ROOT);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret_code;
	}

	progress_init_steps("third-party-info", steps_in_third_party_info);

	/* show info for each repo */
	ret_code = third_party_run_operation_multirepo(cmdline_repo, show_repo_info, SWUPD_OK, "info", steps_in_third_party_info);

	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
