/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2012-2016 Intel Corporation.
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
 *   Authors:
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
 *
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"
#include "target_root.h"

#define VERIFY_PICKY 1

static char **bundles;
static bool cmdline_option_force = false;
static bool cmdline_option_recursive = false;

void bundle_remove_set_option_force(bool opt)
{
	cmdline_option_force = opt;
}

void bundle_remove_set_option_recursive(bool opt)
{
	cmdline_option_recursive = opt;
}

static void print_help(void)
{
	print("Removes software bundles\n\n");
	print("Usage:\n");
	print("   swupd bundle-remove [OPTION...] [bundle1 bundle2 (...)]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -x, --force            Removes a bundle along with all the bundles that depend on it\n");
	print("   -R, --recursive        Removes a bundle and its dependencies recursively\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "force", no_argument, 0, 'x' },
	{ "recursive", no_argument, 0, 'R' },
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

	bundles = argv + ind;

	return true;
}

static bool is_installed(const void *bundle_name)
{
	return is_installed_bundle(bundle_name);
}

static bool is_deletable_dependency(const void *dependency)
{
	if (str_cmp(dependency, "os-core") == 0) {
		return false;
	}
	if (!is_installed_bundle(dependency)) {
		return false;
	}
	if (is_tracked_bundle(dependency)) {
		return false;
	}
	return true;
}

/*
 * remove_tracked removes the tracking file in
 * path_prefix/state_dir_parent/bundles if it exists to untrack as manually
 * installed if the file exists
 */
static void remove_tracked(const char *bundle)
{
	char *destdir = get_tracking_dir();
	char *tracking_file = sys_path_join("%s/%s", destdir, bundle);
	FREE(destdir);
	/* we don't care about failures here since any weird state in the tracking
	 * dir MUST be handled gracefully */
	sys_rm(tracking_file);
	FREE(tracking_file);
}

static int filter_file_to_delete(const void *a, const void *b)
{
	struct file *A, *B;
	int ret;

	/* matched items will be removed from the list of files to be deleted */

	A = (struct file *)a;
	B = (struct file *)b;

	/* if the file we are looking at is marked as already deleted
	 * it can be removed from the list, so return a match */
	if (A->is_deleted) {
		return 0;
	}

	ret = str_cmp(A->filename, B->filename);
	if (ret) {
		return ret;
	}

	/* if the file is marked as not deleted in B, the file is still
	 * needed in the system, so return a match */
	if (!B->is_deleted) {
		return 0;
	}

	return -1;
}

static enum swupd_code validate_bundle(const char *bundle, struct manifest *mom, struct list *bundles, struct list **reqd_by)
{
	enum swupd_code ret = SWUPD_OK;
	char *msg;
	int number_of_reqd;
	const bool DONT_INCLUDE_OPTIONAL_DEPS = false;

	/* os-core bundle not allowed to be removed */
	if (str_cmp(bundle, "os-core") == 0) {
		warn("\nBundle \"os-core\" not allowed to be removed, skipping it...\n");
		return SWUPD_REQUIRED_BUNDLE_ERROR;
	}

	if (!mom_search_bundle(mom, bundle)) {
		warn("\nBundle \"%s\" is invalid, skipping it...\n", bundle);
		return SWUPD_INVALID_BUNDLE;
	}

	if (!is_installed_bundle(bundle)) {
		warn("\nBundle \"%s\" is not installed, skipping it...\n", bundle);
		return SWUPD_BUNDLE_NOT_TRACKED;
	}

	/* check if bundle is required by another installed bundle */
	string_or_die(&msg, "\nBundle \"%s\" is required by the following bundles:\n", bundle);
	number_of_reqd = required_by(reqd_by, bundle, mom, 0, bundles, msg, DONT_INCLUDE_OPTIONAL_DEPS);
	FREE(msg);
	if (number_of_reqd > 0) {
		/* the bundle is required by other bundles */
		return SWUPD_NO;
	}

	return ret;
}

static void get_removable_dependencies(struct manifest *mom, struct list **bundles_to_remove)
{
	char *bundle;
	struct list *iter;
	struct list *candidates_for_removal = NULL;
	struct manifest *m;

	/* for each of the bundles requested to be deleted
	 * get a list of their dependencies we are allowed to remove */
	for (iter = *bundles_to_remove; iter; iter = iter->next) {
		m = iter->data;
		bundle = m->component;
		recurse_dependencies(mom, bundle, &candidates_for_removal, is_deletable_dependency);
	}

	/* get a list of dependencies still required by bundles
	 * that won't be removed */
	struct list *required_bundles = NULL;
	for (iter = mom->submanifests; iter; iter = iter->next) {
		m = iter->data;
		bundle = m->component;
		if (!list_search(candidates_for_removal, bundle, cmp_manifest_component_string)) {
			recurse_dependencies(mom, bundle, &required_bundles, is_installed);
		}
	}

	/* filter out those required bundles from the
	 * candidates for removal */
	iter = candidates_for_removal;
	while (iter) {
		m = iter->data;
		bundle = m->component;
		if (list_search(required_bundles, bundle, cmp_manifest_component_string)) {
			if (iter == candidates_for_removal) {
				if (iter->next) {
					candidates_for_removal = iter->next;
				} else {
					candidates_for_removal = NULL;
				}
			}
			iter = list_free_item(iter, manifest_free_data);
			continue;
		}
		iter = iter->next;
	}

	/* the list of dependencies is ready, move the
	 * manifests to the bundles to be removed */
	for (iter = candidates_for_removal; iter; iter = iter->next) {
		m = iter->data;
		bundle = m->component;
		list_move_item(bundle, &mom->submanifests, bundles_to_remove, cmp_manifest_component_string);
	}
	*bundles_to_remove = list_sort(*bundles_to_remove, cmp_manifest_component);

	list_free_list_and_data(candidates_for_removal, manifest_free_data);
	list_free_list_and_data(required_bundles, manifest_free_data);
}

static void print_remove_summary(unsigned int requested, unsigned int bad, unsigned int total_removed)
{
	int deps_removed;

	if (bad > 0) {
		info("\nFailed to remove %i of %i bundles\n", bad, requested);
	} else {
		info("\nSuccessfully removed %i bundle%s\n", requested, (requested > 1 ? "s" : ""));
	}

	deps_removed = total_removed + bad - requested;
	if (deps_removed > 0) {
		if (cmdline_option_force) {
			info("%i bundle%s\n", deps_removed, deps_removed > 1 ? "s that depended on the specified bundle(s) were removed" : " that depended on the specified bundle(s) was removed");
		} else {
			info("%i bundle%s\n", deps_removed, deps_removed > 1 ? "s that were installed as a dependency were removed" : " that was installed as a dependency was removed");
		}
	}
}

/*  The function removes one or more bundles
 *  passed in the bundles list.
 */
enum swupd_code execute_remove_bundles_extra(struct list *bundles, remove_extra_proc_fn_t post_remove_fn)
{
	enum swupd_code ret_code = SWUPD_OK;
	enum swupd_code ret = SWUPD_OK;
	unsigned int bad = 0;
	unsigned int total = 0;
	int current_version = CURRENT_OS_VERSION;
	struct manifest *current_mom = NULL;
	struct list *subs = NULL;
	struct list *bundles_to_remove = NULL;
	struct list *common_files = NULL;
	struct list *files_to_remove = NULL;
	struct list *iter = NULL;
	char *bundles_list_str = NULL;

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		ret_code = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto out;
	}

	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	current_mom = load_mom(current_version, NULL);
	if (!current_mom) {
		error("Unable to download/verify %d Manifest.MoM\n", current_version);
		ret_code = SWUPD_COULDNT_LOAD_MOM;
		goto out;
	}

	/* load all installed bundles into memory */
	read_subscriptions(&subs);
	set_subscription_versions(current_mom, NULL, &subs);

	/* load all submanifests */
	current_mom->submanifests = recurse_manifest(current_mom, subs, NULL, false, NULL);
	if (!current_mom->submanifests) {
		error("Cannot load MoM sub-manifests\n");
		ret_code = SWUPD_RECURSE_MANIFEST;
		goto out_subs;
	}

	/* inform user about the effects of using the options */
	if (cmdline_option_force) {
		warn("\nThe --force option was used, the specified bundle%s and all bundles "
		     "that require %s will be removed from the system\n",
		     total > 1 ? "s" : "",
		     total > 1 ? "them" : "it");
	} else if (cmdline_option_recursive) {
		warn("\nThe --recursive option was used, the specified bundle%s and %s "
		     "dependencies will be removed from the system\n",
		     total > 1 ? "s" : "",
		     total > 1 ? "their" : "its");
	}

	for (; bundles; bundles = bundles->next, total++) {

		char *bundle = bundles->data;
		struct list *reqd_by = NULL;

		/* check if the bundle is allowed to be removed */
		ret = validate_bundle(bundle, current_mom, bundles, &reqd_by);
		if (ret) {
			if (ret != SWUPD_NO) {
				/* invalid bundle, skip it */
				ret_code = ret;
				bad++;
				continue;
			}

			/* there are bundles that depend on bundle */
			int number_of_reqd = list_len(reqd_by);
			if (!cmdline_option_force) {
				error("\nBundle \"%s\" is required by %d bundle%s, skipping it...\n", bundle, number_of_reqd, number_of_reqd == 1 ? "" : "s");
				info("Use \"swupd bundle-remove --force %s\" to remove \"%s\" and all bundles that require it\n", bundle, bundle);
				list_free_list_and_data(reqd_by, free);
				ret_code = SWUPD_REQUIRED_BUNDLE_ERROR;
				bad++;
				continue;
			} else {
				/* the --force option was specified */
				char *dep;
				for (iter = list_head(reqd_by); iter; iter = iter->next) {
					dep = iter->data;
					list_move_item(dep, &current_mom->submanifests, &bundles_to_remove, cmp_manifest_component_string);
					remove_tracked(dep);
				}
				list_free_list_and_data(reqd_by, free);
			}
		}

		/* if we have reached this point the bundle is
		 * "deletable" move the manifest of the bundle
		 * to be remove from the list of subscribed
		 * bundles to the list of bundles to be removed */
		list_move_item(bundle, &current_mom->submanifests, &bundles_to_remove, cmp_manifest_component_string);
		remove_tracked(bundle);
	}

	/* if recursive, the dependencies need to be removed too */
	if (cmdline_option_recursive) {
		get_removable_dependencies(current_mom, &bundles_to_remove);
	}

	/* if there are no bundles to remove we are done */
	if (bundles_to_remove) {

		/* inform user about extra bundles being removed (if any) */
		info("\nThe following bundles are being removed:\n");
		for (iter = list_head(bundles_to_remove); iter; iter = iter->next) {
			struct manifest *m = iter->data;
			info(" - %s\n", m->component);
		}

		/* get the list of all files required by the installed bundles (except the ones to be removed) */
		current_mom->files = consolidate_files_from_bundles(current_mom->submanifests);

		/* get the list of the files contained in the bundles to be removed */
		files_to_remove = consolidate_files_from_bundles(bundles_to_remove);

		/* sanitize files to remove; if a file is needed by a bundle that
		 * is installed, it should be kept in the system */
		files_to_remove = list_sorted_split_common_elements(files_to_remove, current_mom->files, &common_files, filter_file_to_delete, NULL);

		if (list_len(files_to_remove) > 0) {
			info("\nDeleting bundle files...\n");
			progress_next_step("remove_files", PROGRESS_BAR);
			int deleted = target_root_remove_files(files_to_remove);
			info("Total deleted files: %i\n", deleted);
		}
	}

	if (post_remove_fn) {
		ret_code = post_remove_fn(files_to_remove, common_files);
	}

	/* print a summary of the remove operation */
	print_remove_summary(total, bad, list_len(bundles_to_remove));

	/* cleanup */
	list_free_list(common_files);
	list_free_list(files_to_remove);
	list_free_list_and_data(bundles_to_remove, manifest_free_data);
	manifest_free(current_mom);
	free_subscriptions(&subs);

	return ret_code;

out_subs:
	manifest_free(current_mom);
	free_subscriptions(&subs);
out:
	bundles_list_str = str_join(", ", bundles);
	telemetry(TELEMETRY_CRIT,
		  "bundleremove",
		  "bundle=%s\n"
		  "current_version=%d\n"
		  "result=%d\n"
		  "bytes=%ld\n",
		  bundles_list_str,
		  current_version,
		  ret_code,
		  total_curl_sz);
	FREE(bundles_list_str);
	print("\nFailed to remove bundle(s)\n");

	return ret_code;
}

enum swupd_code execute_remove_bundles(struct list *bundles)
{
	return execute_remove_bundles_extra(bundles, NULL);
}

enum swupd_code bundle_remove_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_bundle_remove = 2;
	struct list *bundles_list = NULL;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	/* initialize swupd */
	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret;
	}

	/* move the bundles provided in the command line into a
	 * list so it is easier to handle them */
	for (; *bundles; ++bundles) {
		char *bundle = *bundles;
		bundles_list = list_append_data(bundles_list, bundle);
	}
	bundles_list = list_head(bundles_list);

	/*
	 * Steps for bundle-remove:
	 *  1) load_manifests
	 *  2) remove_files
	 */
	progress_init_steps("bundle-remove", steps_in_bundle_remove);

	ret = execute_remove_bundles(bundles_list);

	list_free_list(bundles_list);
	progress_finish_steps(ret);
	swupd_deinit();

	return ret;
}
