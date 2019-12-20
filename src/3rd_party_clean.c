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

#define _GNU_SOURCE

#include "3rd_party_repos.h"
#include "swupd.h"

#ifdef THIRDPARTY

#define FLAG_ALL 2000
#define FLAG_DRY_RUN 2001

static bool cmdline_option_all = false;
static bool cmdline_option_dry_run = false;
static char *cmdline_option_repo = NULL;

static void print_help(void)
{
	print("Remove cached content used for updates from state directory of a 3rd-party repository\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party clean [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo     Specify the 3rd-party repository to use\n");
	print("   --all          Remove all the content including recent metadata\n");
	print("   --dry-run      Just print files that would be removed\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "all", no_argument, 0, FLAG_ALL },
	{ "dry-run", no_argument, 0, FLAG_DRY_RUN },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, char *optarg UNUSED_PARAM)
{
	switch (opt) {
	case FLAG_ALL:
		cmdline_option_all = optarg_to_bool(optarg);
		return true;
	case FLAG_DRY_RUN:
		cmdline_option_dry_run = optarg_to_bool(optarg);
		return true;
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

static enum swupd_code clean_repos_state(UNUSED_PARAM char *unused)
{
	enum swupd_code ret;
	int files_removed;

	ret = clean_statedir(cmdline_option_dry_run, cmdline_option_all);
	files_removed = clean_get_stats();
	if (cmdline_option_dry_run) {
		print("Would remove %d files\n", files_removed);
	} else {
		print("%d files removed\n", files_removed);
	}

	return ret;
}

enum swupd_code third_party_clean_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_clean = 1;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret_code;
	}
	progress_init_steps("3rd-party-clean", steps_in_clean);

	/* clean the cache */
	ret_code = third_party_run_operation_multirepo(cmdline_option_repo, clean_repos_state, SWUPD_OK);

	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
