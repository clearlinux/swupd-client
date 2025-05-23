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

#include <errno.h>

#ifdef THIRDPARTY

static bool cmdline_option_force = false;

static const struct option prog_opts[] = {
	{ "force", no_argument, 0, 'x' },
};

static void print_help(void)
{
	/* TODO(castulo): we need to change this description to match that of the
	 * documentation once the documentation for this content is added */
	print("Removes a 3rd-party repository from the list of available repositories in the system\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party remove [OPTION...] [repo-name]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	print("\n");
}

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 'x':
		cmdline_option_force = optarg_to_bool(optarg);
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
#define EXACT_ARG_COUNT 1
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	int positional_args = argc - ind;
	/* Ensure that repo remove expects only one args: repo-name */
	switch (positional_args) {
	case 0:
		error("The positional args: repo-name is missing\n\n");
		return false;
	case EXACT_ARG_COUNT:
		return true;
	default:
		error("Unexpected arguments\n\n");
		return false;
	}
	return false;
}

static bool is_installed_bundle_data(const void *bundle)
{
	return is_installed_bundle(bundle);
}

enum swupd_code third_party_remove_main(int argc, char **argv)
{
	enum swupd_code ret, ret_partial = SWUPD_OK;
	const int step_in_third_party_remove = 2;
	int err;
	int version;
	char *name = NULL;
	char *repo_dir = NULL;
	struct list *installed_bundles = NULL;
	struct list *repos = NULL;
	struct repo *repo = NULL;
	struct manifest *mom = NULL;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		return ret;
	}

	/*
	 * Steps for 3rd-party remove:
	 *  1) load_manifests
	 *  2) remove_binaries
	 */

	progress_init_steps("third-party-remove", step_in_third_party_remove);

	/* The last argument has to be the repo-name to be deleted */
	name = argv[argc - 1];

	/* load the existing 3rd-party repos from the repo.ini config file
	 * and find the requested repo */
	repos = third_party_get_repos();
	repo = list_search(repos, name, cmp_repo_name_string);
	if (!repo) {
		error("3rd-party repository %s was not found\n\n", name);
		ret = SWUPD_INVALID_REPOSITORY;
		goto exit;
	}

	/* set the appropriate content_dir and state_dir for the selected 3rd-party repo */
	ret = third_party_set_repo(repo, globals.sigcheck);
	if (ret) {
		goto remove_repo;
	}

	/* get the current mom */
	version = get_current_version(globals.path_prefix);
	if (version < 0) {
		error("Unable to determine current version for repository %s\n\n", name);
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto remove_repo;
	}

	mom = load_mom(version, NULL);
	if (!mom) {
		error("Could not load the manifest for repository %s\n\n", name);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto remove_repo;
	}

	/* get a list of bundles already installed in the system */
	progress_next_step("load_manifests", PROGRESS_BAR);
	info("Loading required manifests...\n");
	ret = mom_get_manifests_list(mom, &mom->submanifests, is_installed_bundle_data);
	if (ret) {
		ret = SWUPD_COULDNT_LOAD_MANIFEST;
		goto remove_repo;
	}
	mom->files = consolidate_files_from_bundles(mom->submanifests);

	/* if there are errors at this point the repo may be corrupt,
	 * continue only if forced by the user */
remove_repo:
	if (ret) {
		char *bin_dir;
		bin_dir = third_party_get_bin_dir();
		if (cmdline_option_force) {
			warn("The --force option is specified; forcing the removal of the repository\n");
			info("Please be aware that this may leave exported files in %s\n", bin_dir);
			ret_partial = ret;
			ret = SWUPD_OK;
			FREE(bin_dir);
		} else {
			error("The 3rd-party repository %s is corrupt and cannot be removed cleanly\n", name);
			info("To force the removal of the repository use the --force option\n");
			info("Please be aware that this may leave exported files in %s\n", bin_dir);
			FREE(bin_dir);
			goto exit;
		}
	}

	/* remove the repo from repo.ini */
	info("Removing repository %s...\n", name);
	err = third_party_remove_repo(name);
	if (err < 0) {
		if (err == -ENOENT) {
			ret = SWUPD_INVALID_OPTION;
		} else {
			ret = SWUPD_COULDNT_WRITE_FILE;
		}
		goto exit;
	}

	/* remove the repo's content directory */
	if (third_party_remove_repo_directory(name) < 0) {
		ret = SWUPD_COULDNT_REMOVE_FILE;
	}

	/* remove the repo's exported binaries from the system */
	if (mom && mom->files) {
		ret = third_party_process_files(mom->files, "\nRemoving 3rd-party bundle binaries...\n", "remove_binaries", third_party_remove_binary);
	}

exit:
	if (ret == SWUPD_OK) {
		if (ret_partial == SWUPD_OK) {
			info("\nRepository and its content removed successfully\n");
		} else {
			info("\nRepository and its content partially removed\n");
			ret = ret_partial;
		}
	} else {
		info("\nFailed to remove repository\n");
	}
	manifest_free(mom);
	FREE(repo_dir);
	list_free_list_and_data(repos, repo_free_data);
	list_free_list_and_data(installed_bundles, free);
	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}

#endif
