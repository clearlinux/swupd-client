/*
 *   Software Updater - client side
 *
 *      Copyright © 2019-2020 Intel Corporation.
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

#define FLAG_DEPENDENCIES 2000
#define FLAG_FILES 2001

static bool cmdline_option_dependencies = false;
static bool cmdline_option_files = false;
static int cmdline_option_version = 0;
static char *cmdline_option_repo = NULL;
static char *cmdline_option_bundle = NULL;

static void print_help(void)
{
	print("Displays detailed information about a bundle from a 3rd-party repository\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party bundle-info [OPTIONS...] BUNDLE\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo       Specify the 3rd-party repository to use\n");
	print("   -V, --version=V  Show the bundle info for the specified version V (current by default), also accepts 'latest'\n");
	print("   --dependencies   Show the bundle dependencies\n");
	print("   --files          Show the files installed by this bundle\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "version", required_argument, 0, 'V' },
	{ "dependencies", no_argument, 0, FLAG_DEPENDENCIES },
	{ "files", no_argument, 0, FLAG_FILES },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	int err;

	switch (opt) {
	case 'V':
		if (str_cmp("latest", optarg) == 0) {
			cmdline_option_version = -1;
			return true;
		}
		err = str_to_int(optarg, &cmdline_option_version);
		if (err < 0 || cmdline_option_version < 0) {
			error("Invalid --version argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case FLAG_DEPENDENCIES:
		cmdline_option_dependencies = optarg_to_bool(optarg);
		return true;
	case FLAG_FILES:
		cmdline_option_files = optarg_to_bool(optarg);
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

	if (argc <= optind) {
		error("Please specify the bundle you wish to display information from\n\n");
		return false;
	}

	if (optind + 1 < argc) {
		error("Please specify only one bundle at a time\n\n");
		return false;
	}

	cmdline_option_bundle = *(argv + optind);

	return true;
}

enum swupd_code third_party_bundle_info_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_bundleinfo = 0;
	struct list *bundles = NULL;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret_code;
	}

	/* set the command options */
	bundle_info_set_option_version(cmdline_option_version);
	bundle_info_set_option_dependencies(cmdline_option_dependencies);
	bundle_info_set_option_files(cmdline_option_files);

	progress_init_steps("3rd-party-bundle-info", steps_in_bundleinfo);

	bundles = list_append_data(bundles, cmdline_option_bundle);
	ret_code = third_party_run_operation(bundles, cmdline_option_repo, bundle_info);

	list_free_list(bundles);
	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
