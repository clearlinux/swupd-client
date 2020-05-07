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

#include <sys/stat.h>

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

static enum swupd_code validate_binary(struct file *file)
{
	enum swupd_code ret_code = SWUPD_OK;
	char *bin_directory = NULL;
	char *script = NULL;
	char *filename = NULL;

	if (!file || !third_party_file_is_binary(file)) {
		return ret_code;
	}

	filename = file->filename;
	bin_directory = third_party_get_bin_dir();
	script = third_party_get_binary_path(sys_basename(filename));

	/* if a file already exists, report the problem to the user */
	if (sys_file_exists(script)) {
		error("There is already a binary called %s in %s\n", sys_basename(filename), bin_directory);
		ret_code = SWUPD_COULDNT_CREATE_FILE;
	}
	FREE(script);
	FREE(bin_directory);

	return ret_code;
}

static enum swupd_code validate_permissions(struct file *file)
{
	enum swupd_code ret_code = SWUPD_OK;
	struct stat file_stats;
	char *staged_file = NULL;

	if (!file) {
		return ret_code;
	}

	staged_file = statedir_get_staged_file(file->hash);
	if (lstat(staged_file, &file_stats) == 0) {
		if ((file_stats.st_mode & S_ISUID) || (file_stats.st_mode & S_ISGID) || (file_stats.st_mode & S_ISVTX)) {
			warn("File %s has dangerous permissions\n", file->filename);
			ret_code = SWUPD_NO;
		}
	} else {
		ret_code = SWUPD_INVALID_FILE;
	}

	FREE(staged_file);

	return ret_code;
}

static enum swupd_code export_bundle_binaries(struct list *installed_files)
{
	return third_party_process_files(installed_files, "\nExporting 3rd-party bundle binaries...\n", "export_binaries", third_party_create_wrapper_script);
}

static enum swupd_code validate_bundle_binaries(struct list *files_to_be_installed)
{
	return third_party_process_files(files_to_be_installed, "\nValidating 3rd-party bundle binaries...\n", "validate_binaries", validate_binary);
}

static enum swupd_code validate_file_permissions(struct list *files_to_be_installed)
{
	static enum swupd_code ret_code = SWUPD_OK;

	ret_code = third_party_process_files(files_to_be_installed, "\nValidating 3rd-party bundle file permissions...\n", "validate_file_permissions", validate_permissions);
	if (ret_code) {
		if (ret_code == SWUPD_NO) {
			/* the bundle has files with dangerous permissions,
			 * ask the user wether to continue or not */
			warn("\nThe 3rd-party bundle you are about to install contains files with dangerous permission\n");
			if (confirm_action()) {
				ret_code = SWUPD_OK;
			} else {
				ret_code = SWUPD_INVALID_FILE;
			}
		}
	}

	return ret_code;
}

static enum swupd_code add_bundle(char *bundle_name)
{
	struct list *bundle_to_install = NULL;
	enum swupd_code ret = SWUPD_OK;

	/* execute_bundle_add expects a list */
	bundle_to_install = list_append_data(bundle_to_install, bundle_name);

	info("\nBundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons\n\n");
	globals.no_scripts = true;

	ret = execute_bundle_add_extra(bundle_to_install, validate_bundle_binaries, export_bundle_binaries, validate_file_permissions);

	list_free_list(bundle_to_install);
	return ret;
}

enum swupd_code third_party_bundle_add_main(int argc, char **argv)
{
	struct list *bundles = NULL;
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_bundleadd = 11;

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
	 *  2) validate_binaries
	 *  3) download_packs
	 *  4) extract_packs
	 *  5) validate_fullfiles
	 *  6) download_fullfiles
	 *  7) extract_fullfiles
	 *  8) validate_file_permissions
	 *  9) install_files
	 *  10) run_postupdate_scripts
	 *  11) export_binaries
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
