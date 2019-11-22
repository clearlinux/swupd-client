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
	print("Usage:\n");
	print("   swupd 3rd-party bundle-add [OPTION...] [bundle1 bundle2 (...)]\n\n");

	global_print_help();

	print("Options:\n");
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

enum swupd_code third_party_bundle_add_main(int argc, char **argv)
{
	struct list *bundles = NULL;
	struct list *iter1 = NULL;
	struct list *iter2 = NULL;
	struct list *repos = NULL;
	char *state_dir;
	char *path_prefix;
	int ret_code = SWUPD_OK;
	int ret = SWUPD_OK;

	/*
	 * Steps for 3rd-party bundle-add:
	 *
	 *  1) load_manifests
	 *  2) download_packs
	 *  3) extract_packs
	 *  4) validate_fullfiles
	 *  5) download_fullfiles
	 *  6) extract_fullfiles
	 *  7) install_files
	 */

	const int steps_in_bundleadd = 7;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("3rd-party-bundle-add", steps_in_bundleadd);

	/* initialize swupd */
	ret_code = swupd_init(SWUPD_ALL);
	if (ret_code != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret_code;
	}

	/* load the existing 3rd-party repos from the repo.ini config file */
	repos = third_party_get_repos();

	/* move the bundles provided in the command line into a
	 * list so it is easier to handle them */
	for (; *cmdline_bundles; ++cmdline_bundles) {
		char *bundle = *cmdline_bundles;
		bundles = list_append_data(bundles, bundle);
	}
	bundles = list_head(bundles);

	/* backup the original state_dir and path_prefix values */
	state_dir = strdup_or_die(globals.state_dir);
	path_prefix = strdup_or_die(globals.path_prefix);

	/* try installing bundles one by one */
	for (iter1 = bundles; iter1; iter1 = iter1->next) {
		char *bundle = iter1->data;
		struct repo *selected_repo = NULL;
		int count = 0;

		/* if the repo to be used was specified, use it, otherwise
		* search for the bundle in all the 3rd-party repos */
		if (cmdline_repo) {

			selected_repo = list_search(repos, cmdline_repo, repo_name_cmp);

		} else {

			info("Searching for bundle %s in the 3rd-party repositories...\n", bundle);
			for (iter2 = repos; iter2; iter2 = iter2->next) {
				struct repo *repo = iter2->data;
				struct manifest *mom = NULL;
				struct file *file = NULL;

				/* set the appropriate content_dir and state_dir for the selected 3rd-party repo */
				if (third_party_set_repo(state_dir, path_prefix, repo)) {
					ret_code = SWUPD_COULDNT_CREATE_DIR;
					goto clean_and_exit;
				}

				/* load the repo's MoM*/
				mom = load_mom(repo->version, false, NULL);
				if (!mom) {
					error("Cannot load manifest MoM for 3rd-party repository %s version %i\n", repo->name, repo->version);
					ret_code = SWUPD_COULDNT_LOAD_MOM;
					goto clean_and_exit;
				}

				/* search for the bundle in the MoM */
				file = mom_search_bundle(mom, bundle);
				if (file) {
					/* the bundle was found in this repo, keep a pointer to the repo */
					info("Bundle %s found in 3rd-party repository %s\n", bundle, repo->name);
					count++;
					selected_repo = repo;
				}
				manifest_free(mom);
			}

			/* if the bundle exists in only one repo, we can continue */
			if (count == 0) {
				error("bundle %s was not found in any 3rd-party repository\n\n", bundle);
				ret_code = SWUPD_INVALID_BUNDLE;
				continue;
			} else if (count > 1) {
				error("bundle %s was found in more than one 3rd-party repository\n", bundle);
				info("Please specify a repository using the --repo flag\n\n");
				ret_code = SWUPD_INVALID_OPTION;
				continue;
			}
		}

		if (!selected_repo) {
			error("3rd-party repository %s was not found\n\n", cmdline_repo);
			ret_code = SWUPD_INVALID_OPTION;
			goto clean_and_exit;
		}

		/* set the content_url and state_dir to those from the 3rd-party repo
		 * where the requested bundle was found */
		if (third_party_set_repo(state_dir, path_prefix, selected_repo)) {
			ret_code = SWUPD_COULDNT_CREATE_DIR;
			goto clean_and_exit;
		}

		/* execute_bundle_add expects a list so we can send a new list with only
		 * the one bundle we want to install in this iteration */
		struct list *bundle_to_install = NULL;
		bundle_to_install = list_append_data(bundle_to_install, bundle);

		info("\nBundles added from a 3rd-party repository are forced to run with the --no-scripts flag for security reasons\n\n");
		globals.no_scripts = true;

		ret = execute_bundle_add(bundle_to_install, selected_repo->version);

		if (ret != SWUPD_OK) {
			ret_code = ret;
		}
		list_free_list(bundle_to_install);
		info("\n");
	}

clean_and_exit:
	free_string(&state_dir);
	free_string(&path_prefix);
	list_free_list(bundles);
	list_free_list_and_data(repos, repo_free_data);

	swupd_deinit();
	progress_finish_steps(ret_code);
	return ret_code;
}

#endif
