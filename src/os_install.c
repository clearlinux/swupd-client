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

#include "swupd.h"

#define FLAG_DOWNLOAD_ONLY 2000
#define FLAG_SKIP_OPTIONAL 2001

static bool cmdline_option_download = false;
static bool cmdline_option_force = false;
static bool cmdline_option_skip_optional = false;
static struct list *cmdline_bundles = NULL;
static char *path;
static int cmdline_option_version = -1;
static char *cmdline_option_statedir_cache = NULL;

static const struct option prog_opts[] = {
	{ "force", no_argument, 0, 'x' },
	{ "bundles", required_argument, 0, 'B' },
	{ "version", required_argument, 0, 'V' },
	{ "manifest", required_argument, 0, 'm' },
	{ "statedir-cache", required_argument, 0, 's' },
	{ "download", no_argument, 0, FLAG_DOWNLOAD_ONLY },
	{ "skip-optional", no_argument, 0, FLAG_SKIP_OPTIONAL },
};

static void print_help(void)
{
	print("Performs a system software installation in the specified location\n\n");
	print("Usage:\n");
	print("   swupd os-install [OPTION...] PATH\n\n");

	global_print_help();

	print("Options:\n");
	print("   -V, --version=[VER]         If the version to install is not the latest, it can be specified with this option\n");
	print("   -x, --force                 Attempt to proceed even if non-critical errors found\n");
	print("   -B, --bundles=[BUNDLES]     Include the specified BUNDLES in the OS installation. Example: --bundles=xterm,vim\n");
	print("   -s, --statedir-cache=[PATH] After checking for content in the statedir, check the statedir-cache before downloading it over the network\n");
	print("   --download                  Download all content, but do not actually install it\n");
	print("   --skip-optional             Do not install optional bundles (also-add flag in Manifests)\n");
	print("\n");
}

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'm':
	case 'V':
		/* even though the default version is "latest", allow users to
		 * specify it as an option */
		if (str_cmp("latest", optarg) == 0) {
			return true;
		}
		err = str_to_int(optarg, &cmdline_option_version);
		if (err < 0 || cmdline_option_version <= 0) {
			error("Invalid --version argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'x':
		cmdline_option_force = optarg_to_bool(optarg);
		return true;
	case 's':
		cmdline_option_statedir_cache = strdup_or_die(optarg);
		return true;
	case 'B': {
		char *ctx = NULL;
		char *arg_copy = strdup_or_die(optarg);
		char *token = strtok_r(arg_copy, ",", &ctx);
		while (token) {
			cmdline_bundles = list_prepend_data(cmdline_bundles, strdup_or_die(token));
			token = strtok_r(NULL, ",", &ctx);
		}
		free(arg_copy);
		if (!cmdline_bundles) {
			error("Missing --bundles argument\n\n");
			return false;
		}
		return true;
	}
	case FLAG_DOWNLOAD_ONLY:
		cmdline_option_download = optarg_to_bool(optarg);
		return true;
	case FLAG_SKIP_OPTIONAL:
		cmdline_option_skip_optional = optarg_to_bool(optarg);
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

	/* users need to specify a PATH for the installation as mandatory argument
	 * either explicitly or using the --path global argument */
	if ((argc == optind) && (globals.path_prefix == NULL)) {
		error("the path where the OS will be installed needs to be specified\n\n");
		return false;
	} else if (argc > optind) {
		path = *(argv + optind);
		if (globals.path_prefix) {
			error("cannot specify a PATH and use the --path option at the same time\n\n");
			return false;
		}
		/* set the path_prefix with the provided path */
		if (!set_path_prefix(path)) {
			error("Invalid --path argument\n\n");
			return false;
		}
	}

	/* currently there are no flag restrictions for "os-install" */

	return true;
}

enum swupd_code install_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	int steps_in_os_install;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret;
	}

	/* Initialize the default state dir of the system to be installed */
	char *new_os_state = sys_path_join("%s/%s", globals.path_prefix, "/var/lib/swupd");
	if (create_state_dirs(new_os_state)) {
		ret = SWUPD_COULDNT_CREATE_DIR;
		free_and_clear_pointer(&new_os_state);
		return ret;
	}
	free_and_clear_pointer(&new_os_state);

	/* set options needed for the install in the verify command */
	verify_set_option_quick(true);
	verify_set_option_install(true);
	verify_set_option_statedir_cache(cmdline_option_statedir_cache);
	verify_set_option_download(cmdline_option_download);
	verify_set_option_skip_optional(cmdline_option_skip_optional);
	verify_set_option_force(cmdline_option_force);
	verify_set_option_bundles(cmdline_bundles);
	verify_set_option_version(cmdline_option_version);

	/*
	 * Steps for os-install:
	 *  1) load_manifests
	 *  2) download_packs
	 *  3) extract_packs
	 *  4) check_files_hash
	 *  5) validate_fullfiles
	 *  6) download_fullfiles
	 *  7) extract_fullfiles (with --download finishes here)
	 *  8) add_missing_files
	 *  9) run_postupdate_scripts
	 */
	if (cmdline_option_download) {
		steps_in_os_install = 7;
	} else {
		steps_in_os_install = 9;
	}
	progress_init_steps("os-install", steps_in_os_install);

	/* install */
	ret = execute_verify();

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
