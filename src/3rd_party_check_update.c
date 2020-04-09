/*/*
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

#include "3rd_party_repos.h"
#include "swupd.h"

#ifdef THIRDPARTY

static char *cmdline_option_repo = NULL;

static void print_help(void)
{
	print("Checks whether an update is available for the 3rd-party repositories and prints out the information if so\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party check-update [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo       Specify the 3rd-party repository to use\n");
}

static const struct option prog_opts[] = {
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 'R':
		cmdline_option_repo = strdup_or_die(optarg);
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
	int optind = global_parse_options(argc, argv, &opts);

	if (optind < 0) {
		return false;
	}

	if (argc > optind) {
		error("unexpected arguments\n\n");
		return false;
	}

	return true;
}

static enum swupd_code check_update_repo(UNUSED_PARAM char *unused)
{
	return check_update();
}

enum swupd_code third_party_check_update_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_checkupdate = 0;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		return ret_code;
	}

	/* run check-update */
	ret_code = third_party_run_operation_multirepo(cmdline_option_repo, check_update_repo, SWUPD_NO, "check-update", steps_in_checkupdate);

	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
