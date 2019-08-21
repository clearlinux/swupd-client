/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2019 Intel Corporation.
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

#include <assert.h>

#include "swupd.h"
#include "swupd_internal.h"

#define FLAG_EXTRA_FILES_ONLY 2000

static const char picky_tree_default[] = "/usr";
static const char picky_whitelist_default[] = "/usr/lib/modules|/usr/lib/kernel|/usr/local|/usr/src";

static bool cmdline_option_force = false;
static bool cmdline_option_picky = false;
static bool cmdline_option_quick = false;
static int cmdline_option_version = 0;
static char *cmdline_option_picky_tree = NULL;
static const char *cmdline_option_picky_whitelist = picky_whitelist_default;
static bool cmdline_option_extra_files_only = false;
static struct list *cmdline_bundles = NULL;

/* picky_whitelist points to picky_whitelist_buffer if and only if regcomp() was called for it */
static regex_t *picky_whitelist;
static regex_t picky_whitelist_buffer;

static const struct option prog_opts[] = {
	{ "force", no_argument, 0, 'x' },
	{ "version", required_argument, 0, 'V' },
	{ "manifest", required_argument, 0, 'm' },
	{ "picky", no_argument, 0, 'Y' },
	{ "picky-tree", required_argument, 0, 'X' },
	{ "picky-whitelist", required_argument, 0, 'w' },
	{ "quick", no_argument, 0, 'q' },
	{ "extra-files-only", no_argument, 0, FLAG_EXTRA_FILES_ONLY },
	{ "bundles", required_argument, 0, 'B' },
};

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd repair [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -V, --version=[VER]     Compare against version VER to repair\n");
	print("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	print("   -q, --quick             Don't compare hashes, only fix missing files\n");
	print("   -B, --bundles=[BUNDLES] Forces swupd to only repair the specified BUNDLES. Example: --bundles=os-core,vi\n");
	print("   -Y, --picky             Also remove files which should not exist\n");
	print("   -X, --picky-tree=[PATH] Selects the sub-tree where --picky and --extra-files-only  looks for extra files. Default: /usr\n");
	print("   -w, --picky-whitelist=[RE] Directories that match the regex get skipped. Example: /var|/etc/machine-id\n");
	print("                           Default: %s\n", picky_whitelist_default);
	print("   --extra-files-only      Like --picky, but it only performs this task\n");
	print("\n");
}

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'm':
	case 'V':
		err = strtoi_err(optarg, &cmdline_option_version);
		if (err < 0 || cmdline_option_version < 0) {
			error("Invalid --%s argument: %s\n\n", opt == 'V' ? "version" : "manifest", optarg);
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
	case FLAG_EXTRA_FILES_ONLY:
		cmdline_option_extra_files_only = optarg_to_bool(optarg);
		return true;
	case 'B':
		/* if we are parsing a list from the command line we don't want to append it to
		 * a possible existing list parsed from a config file, we want to replace it, so
		 * we need to delete the existing list first */
		list_free_list(cmdline_bundles);
		cmdline_bundles = NULL;
		cmdline_bundles = string_split(",", optarg);
		if (!cmdline_bundles) {
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

	if (cmdline_bundles) {
		if (cmdline_option_picky) {
			error("--bundles and --picky options are mutually exclusive\n");
			return false;
		}
		if (cmdline_option_extra_files_only) {
			error("--bundles and --extra-files-only options are mutually exclusive\n");
			return false;
		}
	}

	picky_whitelist = compile_whitelist(cmdline_option_picky_whitelist);
	if (!picky_whitelist) {
		return false;
	}

	return true;
}

regex_t *compile_whitelist(const char *whitelist_pattern)
{
	int errcode;
	char *full_regex = NULL;

	/* Enforce matching the entire path. */
	string_or_die(&full_regex, "^(%s)$", whitelist_pattern);

	assert(!picky_whitelist);
	errcode = regcomp(&picky_whitelist_buffer, full_regex, REG_NOSUB | REG_EXTENDED);
	picky_whitelist = &picky_whitelist_buffer;
	if (errcode) {
		error("Problem processing --picky-whitelist=%s\n", cmdline_option_picky_whitelist);
		print_regexp_error(errcode, picky_whitelist);
		goto done;
	}

done:
	free_string(&full_regex);
	return picky_whitelist;
}

enum swupd_code repair_main(int argc, char **argv)
{
	int ret;

	string_or_die(&cmdline_option_picky_tree, "%s", picky_tree_default);

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	/* set options needed for the verify --fix command */
	verify_set_option_install(false);
	verify_set_option_fix(true);
	verify_set_option_force(cmdline_option_force);
	verify_set_option_quick(cmdline_option_quick);
	verify_set_option_picky(cmdline_option_picky);
	verify_set_option_version(cmdline_option_version);
	verify_set_picky_whitelist(picky_whitelist);
	verify_set_picky_tree(cmdline_option_picky_tree);
	verify_set_extra_files_only(cmdline_option_extra_files_only);
	verify_set_option_bundles(cmdline_bundles);

	/* run verify --fix */
	ret = verify_main();

	free_string(&cmdline_option_picky_tree);
	return ret;
}
