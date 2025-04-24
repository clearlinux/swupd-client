/*
 *   Software Updater - client side
 *
 *      Copyright © 2019-2025 Intel Corporation.
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

#define FLAG_EXTRA_FILES_ONLY 2000

static const char picky_tree_default[] = "/usr";
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
	print("Corrects any issues found with the current content installed from 3rd-party repositories\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party repair [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo              Specify the 3rd-party repository to use\n");
	print("   -V, --version=[VER]     Compare against version VER to repair\n");
	print("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	print("   -q, --quick             Don't compare hashes, only fix missing files\n");
	print("   -B, --bundles=[BUNDLES] Forces swupd to only repair the specified BUNDLES. Example: --bundles=xterm,vim\n");
	print("   -Y, --picky             Also remove files which should not exist. By default swupd only looks for them at /usr\n");
	print("                           skipping /usr/lib/modules, /usr/lib/kernel, /usr/local, and /usr/src\n");
	print("   -X, --picky-tree=[PATH] Changes the path where --picky and --extra-files-only look for extra files\n");
	print("   -w, --picky-whitelist=[RE] Directories that match the regex get skipped during --picky. Example: /usr/man|/usr/doc\n");
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
	{ "extra-files-only", no_argument, 0, FLAG_EXTRA_FILES_ONLY },
	{ "bundles", required_argument, 0, 'B' },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'x':
		cmdline_option_force = optarg_to_bool(optarg);
		return true;
	case 'q':
		cmdline_option_quick = optarg_to_bool(optarg);
		return true;
	case 'Y':
		cmdline_option_picky = optarg_to_bool(optarg);
		return true;
	case FLAG_EXTRA_FILES_ONLY:
		cmdline_option_extra_files_only = optarg_to_bool(optarg);
		return true;
	case 'R':
		cmdline_option_repo = strdup_or_die(optarg);
		return true;
	case 'V':
		err = str_to_int(optarg, &cmdline_option_version);
		if (err < 0 || cmdline_option_version < 0) {
			error("Invalid --version argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'X':
		if (optarg[0] != '/') {
			error("--picky-tree must be an absolute path, for example /usr\n\n");
			return false;
		}
		FREE(cmdline_option_picky_tree);
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
		cmdline_option_bundles = NULL;
		cmdline_option_bundles = str_split(",", optarg);
		if (!cmdline_option_bundles) {
			error("Missing required --bundles argument\n\n");
			return false;
		}
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

static enum swupd_code regenerate_binary_scripts(struct list *files_to_verify)
{
	return third_party_process_files(files_to_verify, "\nRegenerating 3rd-party bundle binaries...\n", "regenerate_binaries", third_party_update_wrapper_script);
}

static enum swupd_code repair_repos(UNUSED_PARAM char *unused)
{
	verify_set_option_version(cmdline_option_version);
	return execute_verify_extra(regenerate_binary_scripts);
}

enum swupd_code third_party_repair_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	int steps_in_repair;
	string_or_die(&cmdline_option_picky_tree, "%s", picky_tree_default);

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		FREE(cmdline_option_picky_tree);
		return ret_code;
	}

	/* set the command options */
	verify_set_option_fix(true);
	verify_set_option_force(cmdline_option_force);
	verify_set_option_quick(cmdline_option_quick);
	verify_set_option_picky(cmdline_option_picky);
	verify_set_extra_files_only(cmdline_option_extra_files_only);
	verify_set_picky_whitelist(picky_whitelist);
	verify_set_picky_tree(cmdline_option_picky_tree);
	verify_set_option_bundles(cmdline_option_bundles);

	/*
	 * Steps for repair:
	 *  1) load_manifests (with --extra-files-only jumps to step 9)
	 *  2) check_files_hash
	 *  3) validate_fullfiles
	 *  4) download_fullfiles
	 *  5) extract_fullfiles
	 *  6) add_missing_files (with --quick jumps to step 10)
	 *  7) fix_files
	 *  8) remove_extraneous_files
	 *  9) remove_extra_files (only with --picky or with --extra-files-only)
	 *  10) run_postupdate_scripts
	 *  11) regenerate_binaries
	 */
	if (cmdline_option_extra_files_only) {
		steps_in_repair = 4;
	} else if (cmdline_option_quick) {
		steps_in_repair = 8;
	} else if (cmdline_option_picky) {
		steps_in_repair = 11;
	} else {
		steps_in_repair = 10;
	}

	/* run repair (verify --fix) */
	ret_code = third_party_run_operation_multirepo(cmdline_option_repo, repair_repos, SWUPD_OK, "repair", steps_in_repair);

	FREE(cmdline_option_picky_tree);
	if (picky_whitelist) {
		regfree(picky_whitelist);
		picky_whitelist = NULL;
	}
	swupd_deinit();

	return ret_code;
}

#endif
