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

#ifdef THIRDPARTY

#define FLAG_DEPS 2000

static bool cmdline_local = true;
static bool cmdline_option_all = false;
static char *cmdline_option_has_dep = NULL;
static char *cmdline_option_deps = NULL;
static char *cmdline_repo = NULL;

static void free_has_dep(void)
{
	free_string(&cmdline_option_has_dep);
}

static void free_deps(void)
{
	free_string(&cmdline_option_deps);
}

static void print_help(void)
{
	print("Lists all installed and installable software bundles from 3rd-party repositories\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party bundle-list [OPTIONS...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo              Specify the 3rd-party repository to use\n");
	print("   -a, --all               List all available bundles for the current version of Clear Linux\n");
	print("   -D, --has-dep=[BUNDLE]  List all bundles which have BUNDLE as a dependency\n");
	print("   --deps=[BUNDLE]         List bundles included by BUNDLE\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "all", no_argument, 0, 'a' },
	{ "deps", required_argument, 0, FLAG_DEPS },
	{ "has-dep", required_argument, 0, 'D' },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 'a':
		cmdline_option_all = optarg_to_bool(optarg);
		cmdline_local = !cmdline_option_all;
		return true;
	case 'D':
		string_or_die(&cmdline_option_has_dep, "%s", optarg);
		atexit(free_has_dep);
		cmdline_local = false;
		return true;
	case 'R':
		cmdline_repo = strdup_or_die(optarg);
		return true;
	case FLAG_DEPS:
		string_or_die(&cmdline_option_deps, "%s", optarg);
		atexit(free_deps);
		cmdline_local = false;
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
		error("unexpected arguments\n\n");
		return false;
	}

	return true;
}

static enum swupd_code list_repo_bundles(UNUSED_PARAM char *unused)
{
	static enum swupd_code ret_code = SWUPD_NO;
	int ret;

	ret = list_bundles();

	/* When using the --deps or --has-dep flags which take a BUNDLE as argument,
	 * one or more repositories may not have BUNDLE, but we should not propagate
	 * the SWUPD_INVALID_BUNDLE error unless the bundle is not found in any repository. */
	if (ret != SWUPD_INVALID_BUNDLE || ret_code == SWUPD_NO) {
		ret_code = ret;
	}

	return ret_code;
}

enum swupd_code third_party_bundle_list_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_bundlelist = 2;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	if (cmdline_local && !is_root()) {
		ret_code = swupd_init(SWUPD_NO_ROOT);
	} else {
		ret_code = swupd_init(SWUPD_ALL);
	}
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret_code;
	}

	/* set the command options */
	bundle_list_set_option_all(cmdline_option_all);
	bundle_list_set_option_has_dep(cmdline_option_has_dep);
	bundle_list_set_option_deps(cmdline_option_deps);

	/* list the bundles */
	ret_code = third_party_run_operation_multirepo(cmdline_repo, list_repo_bundles, SWUPD_OK, "bundle-list", steps_in_bundlelist);

	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
