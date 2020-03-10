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

#include <sys/stat.h>

#ifdef THIRDPARTY

#define FLAG_DOWNLOAD_ONLY 2000

static int cmdline_option_version = -1;
static bool cmdline_option_download_only = false;
static bool cmdline_option_keepcache = false;
static bool cmdline_option_status = false;
static char *cmdline_option_repo = NULL;

static void print_help(void)
{
	print("Performs a system software update for content installed from 3rd-party repositories\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party update [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -R, --repo              Specify the 3rd-party repository to use\n");
	print("   -V, --version=V         Update to version V, also accepts 'latest' (default)\n");
	print("   -s, --status            Show current OS version and latest version available on server. Equivalent to \"swupd check-update\"\n");
	print("   -k, --keepcache         Do not delete the swupd state directory content after updating the system\n");
	print("   --download              Download all content, but do not actually install the update\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "download", no_argument, 0, FLAG_DOWNLOAD_ONLY },
	{ "version", required_argument, 0, 'V' },
	{ "status", no_argument, 0, 's' },
	{ "keepcache", no_argument, 0, 'k' },
	{ "repo", required_argument, 0, 'R' },
};

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'V':
		if (strcmp("latest", optarg) == 0) {
			cmdline_option_version = -1;
			return true;
		}

		err = strtoi_err(optarg, &cmdline_option_version);
		if (err < 0 || cmdline_option_version < 0) {
			error("Invalid --version argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 's':
		cmdline_option_status = optarg_to_bool(optarg);
		return true;
	case 'k':
		cmdline_option_keepcache = optarg_to_bool(optarg);
		return true;
	case FLAG_DOWNLOAD_ONLY:
		cmdline_option_download_only = optarg_to_bool(optarg);
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
	if (cmdline_option_version > 0) {
		if (!cmdline_option_repo) {
			error("a repository needs to be specified to use the --version flag\n\n");
			return false;
		}
	}

	return true;
}

static enum swupd_code update_binary_script(struct file *file)
{
	enum swupd_code ret_code = 0;
	int ret;
	char *bin_directory = NULL;
	char *script = NULL;
	char *binary = NULL;
	char *filename = NULL;

	if (!file || !third_party_file_is_binary(file)) {
		return ret_code;
	}

	filename = file->filename;
	bin_directory = third_party_get_bin_dir();
	script = third_party_get_binary_path(sys_basename(filename));
	binary = sys_path_join(globals.path_prefix, filename);

	/* if the script for the binary doesn't exist it is probably a new
	 * binary, create the script */
	if (!sys_file_exists(script) && sys_file_exists(binary)) {
		ret_code = third_party_create_wrapper_script(file);
		goto close_and_exit;
	}

	/* if the binary was removed during the update then we need to
	 * remove the script that exports it too */
	if (file->is_deleted) {
		ret = sys_rm(script);
		if (ret) {
			ret_code = SWUPD_COULDNT_REMOVE_FILE;
		}
	}

close_and_exit:
	free_and_clear_pointer(&binary);
	free_and_clear_pointer(&script);
	free_and_clear_pointer(&bin_directory);

	return ret_code;
}

static enum swupd_code validate_permissions(struct file *file)
{
	enum swupd_code ret_code = SWUPD_OK;
	struct stat file_stats;
	struct stat original_file_stats;
	char *staged_file = NULL;
	char *original_file = NULL;

	if (!file || file->is_deleted) {
		return ret_code;
	}

	string_or_die(&staged_file, "%s/staged/%s", globals.state_dir, file->hash);
	if (lstat(staged_file, &file_stats) == 0) {
		/* see if the file being updated has dangerous flags */
		if ((file_stats.st_mode & S_ISUID) || (file_stats.st_mode & S_ISGID) || (file_stats.st_mode & S_ISVTX)) {
			if (!file->peer) {
				/* a new file included in the update has dangerous flags */
				warn("The update has a new file %s with dangerous permissions\n", file->filename);
				ret_code = SWUPD_NO;
			} else {
				/* an existing file has dangerous flags, do not warn unless
				 * the flags changed from non-dangerous to dangerous in the update */
				original_file = sys_path_join(globals.path_prefix, file->filename);
				if (lstat(original_file, &original_file_stats) == 0) {
					if (
					    ((file_stats.st_mode & S_ISUID) && !(original_file_stats.st_mode & S_ISUID)) ||
					    ((file_stats.st_mode & S_ISGID) && !(original_file_stats.st_mode & S_ISGID)) ||
					    ((file_stats.st_mode & S_ISVTX) && !(original_file_stats.st_mode & S_ISVTX))) {
						warn("The update sets dangerous permissions to file %s\n", file->filename);
						ret_code = SWUPD_NO;
					}
				} else {
					ret_code = SWUPD_INVALID_FILE;
				}
			}
		}
	} else {
		ret_code = SWUPD_INVALID_FILE;
	}

	free_and_clear_pointer(&staged_file);
	free_and_clear_pointer(&original_file);

	return ret_code;
}

static enum swupd_code validate_file_permissions(struct list *files_to_be_updated)
{
	static enum swupd_code ret_code = SWUPD_OK;

	ret_code = third_party_process_files(files_to_be_updated, "\nValidating 3rd-party bundle file permissions...\n", "validate_file_permissions", validate_permissions);
	if (ret_code) {
		if (ret_code == SWUPD_NO) {
			/* the bundle has files with dangerous permissions,
			 * ask the user wether to continue or not */
			warn("\nThe 3rd-party update you are about to install contains files with dangerous permission\n");
			if (confirm_action()) {
				ret_code = SWUPD_OK;
			} else {
				ret_code = SWUPD_INVALID_FILE;
			}
		}
	}

	return ret_code;
}

static enum swupd_code update_exported_binaries(struct list *updated_files)
{
	return third_party_process_files(updated_files, "\nUpdating 3rd-party bundle binaries...\n", "update_binaries", update_binary_script);
}

static enum swupd_code update_repos(UNUSED_PARAM char *unused)
{
	/* Update should always ignore optional bundles */
	globals.skip_optional_bundles = true;
	globals.no_scripts = true;

	if (cmdline_option_status) {
		return check_update();
	} else {
		info("Updates from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons\n\n");
		return execute_update_extra(update_exported_binaries, validate_file_permissions);
	}
}

enum swupd_code third_party_update_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	int steps_in_update;

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
	update_set_option_version(cmdline_option_version);
	update_set_option_download_only(cmdline_option_download_only);
	update_set_option_keepcache(cmdline_option_keepcache);

	/*
	 * Steps for update:
	 *   1) load_manifests
	 *   2) run_preupdate_scripts
	 *   3) download_packs
	 *   4) extract_packs
	 *   5) prepare_for_update
	 *   6) validate_fullfiles
	 *   7) download_fullfiles
	 *   8) extract_fullfiles (finishes here on --download)
	 *   9) update_files
	 *   10) update_binaries
	 *   11) run_postupdate_scripts
	 */
	if (cmdline_option_status) {
		steps_in_update = 0;
	} else if (cmdline_option_download_only) {
		steps_in_update = 8;
	} else {
		steps_in_update = 11;
	}

	/* update 3rd-party bundles */
	ret_code = third_party_run_operation_multirepo(cmdline_option_repo, update_repos, SWUPD_NO, "update", steps_in_update);

	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
