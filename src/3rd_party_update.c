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

#define FLAG_DOWNLOAD_ONLY 2000

static int cmdline_option_version = -1;
static bool cmdline_option_download_only = false;
static bool cmdline_option_keepcache = false;
static bool cmdline_option_status = false;
static char *cmdline_option_repo = NULL;

static void print_help(void)
{
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

static enum swupd_code update_repos(const char *state_dir, const char *path_prefix, struct repo *repo)
{
	enum swupd_code ret = SWUPD_OK;

	/* set the appropriate global variables for the selected 3rd-party repo */
	ret = third_party_set_repo(state_dir, path_prefix, repo);
	if (ret) {
		return ret;
	}

	if (cmdline_option_status) {
		return check_update();
	} else {
		return execute_update();
	}
}

enum swupd_code thir_party_update_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	struct list *repos = NULL;
	struct list *iter = NULL;
	struct repo *repo = NULL;
	char *state_dir;
	char *path_prefix;
	int ret;

	/*
	 * Steps for update:
	 *
	 *   1) load_manifests
	 *   2) run_preupdate_scripts
	 *   3) download_packs
	 *   4) extract_packs
	 *   5) prepare_for_update
	 *   6) validate_fullfiles
	 *   7) download_fullfiles
	 *   8) extract_fullfiles
	 *   9) update_files
	 *  10) run_postupdate_scripts
	 */
	const int steps_in_update = 10;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("3rd-party-update", steps_in_update);

	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		goto exit;
	}

	/* Update should always ignore optional bundles */
	globals.skip_optional_bundles = true;

	/* set the command options */
	update_set_option_version(cmdline_option_version);
	update_set_option_download_only(cmdline_option_download_only);
	update_set_option_keepcache(cmdline_option_keepcache);

	/* load the existing 3rd-party repos from the repo.ini config file */
	repos = third_party_get_repos();

	/* backup the original state_dir and path_prefix values */
	state_dir = strdup_or_die(globals.state_dir);
	path_prefix = strdup_or_die(globals.path_prefix);

	if (cmdline_option_repo) {

		/* a repo was specified, use that one only */
		repo = list_search(repos, cmdline_option_repo, repo_name_cmp);
		if (!repo) {
			error("3rd-party repository %s was not found\n\n", cmdline_option_repo);
			ret_code = SWUPD_INVALID_REPOSITORY;
			goto clean_and_exit;
		}
		ret_code = update_repos(state_dir, path_prefix, repo);

	} else {

		/* no repo was specified, update all repos */
		ret_code = SWUPD_NO;

		for (iter = repos; iter; iter = iter->next) {
			repo = iter->data;
			third_party_repo_header(repo->name);
			ret = update_repos(state_dir, path_prefix, repo);
			/* if all repos are up to date we should return SWUPD_NO,
			 * if at least one repo needs an update we should return SWUPD_OK,
			 * in case of a problem, we should return the appropriate error code */
			if (ret == SWUPD_OK || ret != SWUPD_NO) {
				ret_code = ret;
			}
			info("\n");
		}
	}

clean_and_exit:
	free_string(&state_dir);
	free_string(&path_prefix);
	list_free_list_and_data(repos, repo_free_data);
	swupd_deinit();

exit:
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
