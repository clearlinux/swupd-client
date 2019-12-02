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

#define FLAG_DEPS 2000

static bool cmdline_local = true;
static bool cmdline_option_all = false;
static char *cmdline_option_has_dep = NULL;
static char *cmdline_option_deps = NULL;

static void free_has_dep(void)
{
	free_string(&cmdline_option_has_dep);
}

static void free_deps(void)
{
	free_string(&cmdline_option_deps);
}

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd 3rd-party bundle-list [OPTIONS...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -a, --all               List all available bundles for the current version of Clear Linux\n");
	print("   -D, --has-dep=[BUNDLE]  List all bundles which have BUNDLE as a dependency\n");
	print("   --deps=[BUNDLE]         List bundles included by BUNDLE\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "all", no_argument, 0, 'a' },
	{ "deps", required_argument, 0, FLAG_DEPS },
	{ "has-dep", required_argument, 0, 'D' },
};

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 'a':
		cmdline_option_all = optarg_to_bool(optarg);
		cmdline_local = !cmdline_option_all;
		return true;
	case 'D':
		string_or_die(&cmdline_option_has_dep, "%s", optarg);
		atexit(free_has_dep);
		cmdline_local = false;
		return true;
	case FLAG_DEPS:
		string_or_die(&cmdline_option_deps, "%s", optarg);
		atexit(free_deps);
		cmdline_local = false;
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

	return true;
}

static void print_repo_header(const char *repo_name)
{
	char *header = NULL;
	int header_length;

	string_or_die(&header, " 3rd-Party Repo: %s", repo_name);
	header_length = strlen(header);
	print_pattern("_", header_length + 1);
	info("%s\n", header);
	print_pattern("_", header_length + 1);
	info("\n");
	free_string(&header);
}

static enum swupd_code list_bundles(struct repo *repo)
{
	enum swupd_code ret;

	if (cmdline_local) {
		ret = list_local_bundles(repo->version);
	} else if (cmdline_option_deps != NULL) {
		ret = show_included_bundles(cmdline_option_deps, repo->version);
	} else if (cmdline_option_has_dep != NULL) {
		ret = show_bundle_reqd_by(cmdline_option_has_dep, cmdline_option_all, repo->version);
	} else {
		ret = list_installable_bundles(repo->version);
	}
	info("\n");

	return ret;
}

enum swupd_code third_party_bundle_list_main(int argc, char **argv)
{
	struct list *repos = NULL;
	struct list *iter = NULL;
	char *state_dir;
	char *path_prefix;
	enum swupd_code ret;
	const int steps_in_bundlelist = 1;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("3rd-party-bundle-list", steps_in_bundlelist);

	if (cmdline_local && !is_root()) {
		ret = swupd_init(SWUPD_NO_ROOT);
	} else {
		ret = swupd_init(SWUPD_ALL);
	}

	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		goto finish;
	}

	/* load the existing 3rd-party repos from the repo.ini config file */
	repos = third_party_get_repos();

	/* backup the original state_dir and path_prefix values */
	state_dir = strdup_or_die(globals.state_dir);
	path_prefix = strdup_or_die(globals.path_prefix);

	for (iter = repos; iter; iter = iter->next) {
		struct repo *repo = iter->data;

		/* set the appropriate content_dir and state_dir for the selected 3rd-party repo */
		if (third_party_set_repo(state_dir, path_prefix, repo)) {
			ret = SWUPD_COULDNT_CREATE_DIR;
			goto clean_and_exit;
		}

		print_repo_header(repo->name);
		ret = list_bundles(repo);
	}

clean_and_exit:
	free_string(&state_dir);
	free_string(&path_prefix);
	list_free_list_and_data(repos, repo_free_data);
	swupd_deinit();

finish:
	progress_finish_steps(ret);
	return ret;
}

#endif
