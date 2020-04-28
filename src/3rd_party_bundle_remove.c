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

#include "3rd_party_repos.h"
#include "swupd.h"

#include <errno.h>

#ifdef THIRDPARTY

static char **cmdline_bundles;
static char *cmdline_option_repo = NULL;
static bool cmdline_option_force = false;
static bool cmdline_option_recursive = false;

static void print_help(void)
{
	print("Removes software bundles installed from 3rd-party repositories\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party bundle-remove [OPTION...] [bundle1 bundle2 (...)]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -e, --repo             Specify the 3rd-party repository to use\n");
	print("   -x, --force            Removes a bundle along with all the bundles that depend on it\n");
	print("   -R, --recursive        Removes a bundle and its dependencies recursively\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "force", no_argument, 0, 'x' },
	{ "recursive", no_argument, 0, 'R' },
	{ "repo", required_argument, 0, 'e' },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case 'x':
		cmdline_option_force = optarg_to_bool(optarg);
		return true;
	case 'R':
		cmdline_option_recursive = optarg_to_bool(optarg);
		return true;
	case 'e':
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

	if (argc <= ind) {
		error("missing bundle(s) to be removed\n\n");
		return false;
	}

	cmdline_bundles = argv + ind;

	return true;
}

static enum swupd_code remove_bundle_binaries(struct list *removed_files, struct list *common_files)
{
	enum swupd_code ret_code = SWUPD_OK;
	int ret;
	char *script = NULL;
	struct list *iter = NULL;
	struct file *file = NULL;

	/* remove scripts associated with all the binaries removed */
	ret_code = third_party_process_files(removed_files, "\nRemoving 3rd-party bundle binaries...\n", "remove_binaries", third_party_remove_binary);

	/* after removing the bundle, some binary files may not have been removed
	 * due to being needed by bundles still installed in the system. However,
	 * if those bundles that are still installed didn't intend to export those
	 * binaries (by setting the is_exported flag) then the scripts from those
	 * binaries must be deleted so they are no longer "exported" */
	for (iter = common_files; iter; iter = iter->next) {
		file = iter->data;
		if (!file->is_file || !is_binary(file->filename) || file->is_exported) {
			/* not a binary or the binary is still required to be exported */
			continue;
		}

		/* the binary is no longer required to be exported, we can remove its script */
		script = third_party_get_binary_path(sys_basename(file->filename));
		ret = sys_rm(script);
		if (ret != 0 && ret != -ENOENT) {
			error("The script %s could not be removed\n\n", script);
			ret_code = SWUPD_COULDNT_REMOVE_FILE;
		}
		FREE(script);
	}

	return ret_code;
}

static enum swupd_code remove_bundle(char *bundle_name)
{
	struct list *bundle_to_remove = NULL;
	enum swupd_code ret = SWUPD_OK;

	/* execute_remove_bundles_extra expects a list */
	bundle_to_remove = list_append_data(bundle_to_remove, bundle_name);

	ret = execute_remove_bundles_extra(bundle_to_remove, remove_bundle_binaries);

	list_free_list(bundle_to_remove);
	return ret;
}

enum swupd_code third_party_bundle_remove_main(int argc, char **argv)
{
	enum swupd_code ret_code = SWUPD_OK;
	const int steps_in_bundle_remove = 3;
	struct list *bundles = NULL;

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

	/* move the bundles provided in the command line into a
	 * list so it is easier to handle them */
	for (; *cmdline_bundles; ++cmdline_bundles) {
		char *bundle = *cmdline_bundles;
		bundles = list_append_data(bundles, bundle);
	}
	bundles = list_head(bundles);

	/* set the command options */
	bundle_remove_set_option_force(cmdline_option_force);
	bundle_remove_set_option_recursive(cmdline_option_recursive);

	/*
	 * Steps for bundle-remove:
	 *  1) load_manifests
	 *  2) remove_files
	 *  3) remove_binaries
	 */
	progress_init_steps("3rd-party-bundle-remove", steps_in_bundle_remove);

	/* try removing bundles one by one */
	ret_code = third_party_run_operation(bundles, cmdline_option_repo, remove_bundle);

	list_free_list(bundles);
	swupd_deinit();
	progress_finish_steps(ret_code);

	return ret_code;
}

#endif
