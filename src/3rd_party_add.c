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

#include <errno.h>

#include "3rd_party_repos.h"
#include "swupd.h"

#ifdef THIRDPARTY

static void print_help(void)
{
	/* TODO(castulo): we need to change this description to match that of the
	 * documentation once the documentation for this content is added */
	print("Adds a repository from which 3rd-party content can be installed\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party add [repo-name] [repo-URL]\n\n");

	global_print_help();
	print("\n");
}

static const struct global_options opts = {
	NULL,
	0,
	NULL,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
#define EXACT_ARG_COUNT 2
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	int positional_args = argc - ind;
	/* Ensure that repo add expects only two args: repo-name, repo-url */
	switch (positional_args) {
	case 0:
		error("The positional args: repo-name and repo-url are missing\n\n");
		return false;
	case 1:
		error("The positional args: repo-url is missing\n\n");
		return false;
	case EXACT_ARG_COUNT:
		return true;
	default:
		error("Unexpected arguments\n\n");
		return false;
	}
	return false;
}

static int remove_repo(const char *repo_name)
{
	int ret;

	ret = third_party_remove_repo(repo_name);
	if (ret != 0 && ret != -ENOENT) {
		return ret;
	}

	ret = third_party_remove_repo_directory(repo_name);
	if (ret != 0 && ret != -ENOENT) {
		return ret;
	}

	return 0;
}

enum swupd_code third_party_add_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int step_in_third_party_add = 9;
	const char *name, *url;
	struct list *repos = NULL;
	struct repo *repo = NULL;
	bool revert = false;
	int repo_version;
	int ret;
	const bool DONT_VERIFY_CERTIFICATE = false;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		return ret_code;
	}

	/*
	 * Steps for repo add:
	 *  1) add_repo,
	 *  2-9) 8 steps for adding bundle os-core
	 */
	progress_init_steps("third-party-add", step_in_third_party_add);

	name = argv[argc - 2];
	url = argv[argc - 1];

	if (!is_url_allowed(url)) {
		ret_code = SWUPD_INVALID_OPTION;
		goto finish;
	}

	/* The last two in reverse are the repo-name, repo-url */
	progress_next_step("add_repo", PROGRESS_UNDEFINED);
	info("Adding 3rd-party repository %s...\n\n", name);
	ret = third_party_add_repo(name, url);
	if (ret) {
		if (ret != -EEXIST) {
			error("Failed to add repository %s to config\n\n", name);
			ret_code = SWUPD_COULDNT_WRITE_FILE;
		} else {
			ret_code = SWUPD_INVALID_OPTION;
		}
		goto finish;
	}

	/* at this point the repo has been added to the repo.ini file */
	repos = third_party_get_repos();
	repo = list_search(repos, name, cmp_repo_name_string);
	if (!repo) {
		/* this should not happen */
		ret_code = SWUPD_UNEXPECTED_CONDITION;
		revert = true;
		goto finish;
	}

	/* set the appropriate content_dir and state_dir for the selected 3rd-party repo */
	ret_code = third_party_set_repo(repo, DONT_VERIFY_CERTIFICATE);
	if (ret_code) {
		revert = true;
		goto finish;
	}

	/* get repo's latest version */
	repo_version = get_latest_version(repo->url);
	if (repo_version < 0) {
		error("Unable to determine the latest version for repository %s\n\n", repo->name);
		ret_code = SWUPD_INVALID_REPOSITORY;
		revert = true;
		goto finish;
	}

	/* the repo's "os-core" bundle needs to be installed at this moment
	 * so we can track the version of the repo */
	info("Installing the required bundle 'os-core' from the repository...\n");
	info("Note that all bundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons\n");
	globals.no_scripts = true;
	struct list *bundle_to_install = NULL;
	bundle_to_install = list_append_data(bundle_to_install, "os-core");
	ret_code = bundle_add(bundle_to_install, repo_version);
	if (ret_code) {
		revert = true;
	}
	list_free_list(bundle_to_install);

finish:
	if (revert) {
		/* there was an error adding the repo, revert the action,
		 * we don't want to keep a corrupt repo */
		ret = remove_repo(name);
		if (ret) {
			error("The corrupt repository failed to be removed (errno: %d)\n\n", ret);
		}
	}

	if (ret_code == SWUPD_OK && !revert) {
		print("\nRepository added successfully\n");
	} else {
		print("\nFailed to add repository\n");
	}

	list_free_list_and_data(repos, repo_free_data);
	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
