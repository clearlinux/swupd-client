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

#define FLAG_EXTRA_FILES_ONLY 2000

static const char picky_whitelist_default[] = "/usr/lib/modules|/usr/lib/kernel|/usr/local|/usr/src";

static int cmdline_option_version = 0;
static bool cmdline_option_force = false;
static bool cmdline_option_quick = false;
static bool cmdline_option_picky = false;
static bool cmdline_option_extra_files_only = false;
static char *cmdline_option_picky_tree = NULL;
static char *cmdline_option_repo = NULL;
static const char *cmdline_option_picky_whitelist = picky_whitelist_default;
static struct list *cmdline_option_bundles = NULL;
static regex_t *picky_whitelist;

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd 3rd-party diagnose [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo              Specify the 3rd-party repository to use\n");
	print("   -V, --version=[VER]     Diagnose against manifest version VER\n");
	print("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	print("   -q, --quick             Don't check for corrupt files, only find missing files\n");
	print("   -B, --bundles=[BUNDLES] Forces swupd to only diagnose the specified BUNDLES. Example: --bundles=os-core,vi\n");
	print("   -Y, --picky             Also list files which should not exist\n");
	print("   -X, --picky-tree=[PATH] Selects the sub-tree where --picky and --extra-files-only look for extra files. Default: /usr\n");
	print("   -w, --picky-whitelist=[RE] Directories that match the regex get skipped. Example: /var|/etc/machine-id\n");
	print("                           Default: %s\n", picky_whitelist_default);
	print("   --extra-files-only      Like --picky, but it only performs this task\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "force", no_argument, 0, 'x' },
	{ "version", required_argument, 0, 'V' },
	{ "picky", no_argument, 0, 'Y' },
	{ "picky-tree", required_argument, 0, 'X' },
	{ "picky-whitelist", required_argument, 0, 'w' },
	{ "quick", no_argument, 0, 'q' },
	{ "bundles", required_argument, 0, 'B' },
	{ "extra-files-only", no_argument, 0, FLAG_EXTRA_FILES_ONLY },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'V':
		err = strtoi_err(optarg, &cmdline_option_version);
		if (err < 0 || cmdline_option_version < 0) {
			error("Invalid --version argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'x':
		cmdline_option_force = optarg_to_bool(optarg);
		return true;
	case 'q':
		cmdline_option_quick = optarg_to_bool(optarg);
		return true;
	case 'Y':
		cmdline_option_picky = optarg_to_bool(optarg);
		return true;
	case 'X':
		if (optarg[0] != '/') {
			error("--picky-tree must be an absolute path, for example /usr\n\n");
			return false;
		}
		free_string(&cmdline_option_picky_tree);
		cmdline_option_picky_tree = strdup_or_die(optarg);
		return true;
	case 'w':
		cmdline_option_picky_whitelist = strdup_or_die(optarg);
		return true;
	case 'B':
		/* if we are parsing a list from the command line we don't want to append it to
		 * a possible existing list parsed from a config file, we want to replace it, so
		 * we need to delete the existing list first */
		list_free_list(cmdline_option_bundles);
		cmdline_option_bundles = string_split(",", optarg);
		if (!cmdline_option_bundles) {
			error("Missing required --bundles argument\n\n");
			return false;
		}
		return true;
	case FLAG_EXTRA_FILES_ONLY:
		cmdline_option_extra_files_only = optarg_to_bool(optarg);
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
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	if (argc > ind) {
		error("unexpected arguments\n\n");
		return false;
	}

	/* flag restrictions */
	if (cmdline_option_bundles) {
		if (cmdline_option_picky) {
			error("--bundles and --picky options are mutually exclusive\n");
			return false;
		}
		if (cmdline_option_extra_files_only) {
			error("--bundles and --extra-files-only options are mutually exclusive\n");
			return false;
		}
		if (!cmdline_option_repo) {
			error("a repository needs to be specified to use the --bundles flag\n\n");
			return false;
		}
	}
	if (cmdline_option_quick) {
		if (cmdline_option_picky) {
			error("--quick and --picky options are mutually exclusive\n");
			return false;
		}
		if (cmdline_option_extra_files_only) {
			error("--quick and --extra-files-only options are mutually exclusive\n");
			return false;
		}
	}
	if (cmdline_option_extra_files_only) {
		if (cmdline_option_picky) {
			error("--extra-files-only and --picky options are mutually exclusive\n");
			return false;
		}
	}
	if (cmdline_option_version > 0) {
		if (!cmdline_option_repo) {
			error("a repository needs to be specified to use the --version flag\n\n");
			return false;
		}
	}

	picky_whitelist = compile_whitelist(cmdline_option_picky_whitelist);
	if (!picky_whitelist) {
		return false;
	}

	return true;
}

static enum swupd_code diagnose_repos(UNUSED_PARAM char *unused)
{
	verify_set_option_version(cmdline_option_version);
	return execute_verify();
}

enum swupd_code third_party_diagnose_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;

	/*
	 * Steps for diagnose:
	 *
	 *  1) load_manifests
	 *  2) add_missing_files
	 *  3) fix_files
	 *  4) remove_extraneous_files
	 */
	const int steps_in_diagnose = 4;

	string_or_die(&cmdline_option_picky_tree, "/usr");

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		free_string(&cmdline_option_picky_tree);
		return ret_code;
	}
	progress_init_steps("3rd-party-diagnose", steps_in_diagnose);

	/* set the command options */
	verify_set_option_force(cmdline_option_force);
	verify_set_option_quick(cmdline_option_quick);
	verify_set_option_bundles(cmdline_option_bundles);
	verify_set_option_picky(cmdline_option_picky);
	verify_set_picky_whitelist(picky_whitelist);
	verify_set_picky_tree(cmdline_option_picky_tree);
	verify_set_extra_files_only(cmdline_option_extra_files_only);

	/* diagnose 3rd-party bundles */
	ret_code = third_party_run_operation_multirepo(cmdline_option_repo, diagnose_repos, SWUPD_OK);

	free_string(&cmdline_option_picky_tree);
	if (picky_whitelist) {
		regfree(picky_whitelist);
		picky_whitelist = NULL;
	}
	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
