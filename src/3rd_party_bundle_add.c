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

#define FLAG_SKIP_OPTIONAL 2000
#define FLAG_SKIP_DISKSPACE_CHECK 2001

static char **cmdline_bundles;
static char *cmdline_repo = NULL;

static void print_help(void)
{
	print("Installs new software bundles from 3rd-party repositories\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party bundle-add [OPTION...] [bundle1 bundle2 (...)]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo              Specify the 3rd-party repository to use\n");
	print("   --skip-optional         Do not install optional bundles (also-add flag in Manifests)\n");
	print("   --skip-diskspace-check  Do not check free disk space before adding bundle\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "skip-optional", no_argument, 0, FLAG_SKIP_OPTIONAL },
	{ "skip-diskspace-check", no_argument, 0, FLAG_SKIP_DISKSPACE_CHECK },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case FLAG_SKIP_OPTIONAL:
		globals.skip_optional_bundles = optarg_to_bool(optarg);
		return true;
	case FLAG_SKIP_DISKSPACE_CHECK:
		globals.skip_diskspace_check = optarg_to_bool(optarg);
		return true;
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
	int optind = global_parse_options(argc, argv, &opts);

	if (optind < 0) {
		return false;
	}

	if (argc <= optind) {
		error("missing bundle(s) to be installed\n\n");
		return false;
	}

	cmdline_bundles = argv + optind;

	return true;
}

static enum swupd_code add_bundle(char *bundle)
{
	struct list *bundle_to_install = NULL;
	enum swupd_code ret = SWUPD_OK;

	/* execute_bundle_add expects a list */
	bundle_to_install = list_append_data(bundle_to_install, bundle);

	info("\nBundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons\n\n");
	globals.no_scripts = true;

	ret = execute_bundle_add(bundle_to_install);

	list_free_list(bundle_to_install);
	return ret;
}

enum swupd_code third_party_bundle_add_main(int argc, char **argv)
{
	struct list *bundles = NULL;
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_bundleadd = 8;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	/* initialize swupd */
	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret_code;
	}

	/*
	 * Steps for 3rd-party bundle-add:
	 *  1) load_manifests
	 *  2) download_packs
	 *  3) extract_packs
	 *  4) validate_fullfiles
	 *  5) download_fullfiles
	 *  6) extract_fullfiles
	 *  7) install_files
	 *  8) run_postupdate_scripts
	 */
	progress_init_steps("3rd-party-bundle-add", steps_in_bundleadd);

	/* move the bundles provided in the command line into a
	 * list so it is easier to handle them */
	for (; *cmdline_bundles; ++cmdline_bundles) {
		char *bundle = *cmdline_bundles;
		bundles = list_append_data(bundles, bundle);
	}
	bundles = list_head(bundles);

	/* try installing bundles one by one */
	ret_code = third_party_run_operation(bundles, cmdline_repo, add_bundle);

	list_free_list(bundles);
	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
