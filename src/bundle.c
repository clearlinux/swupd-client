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
*         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
*         Tim Pepper <timothy.c.pepper@linux.intel.com>
*
*/

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "alias.h"
#include "config.h"
#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static bool cmdline_option_force = false;
static bool cmdline_option_recursive = false;

void remove_set_option_force(bool opt)
{
	cmdline_option_force = opt;
}

void remove_set_option_recursive(bool opt)
{
	cmdline_option_recursive = opt;
}

/* Finds out whether bundle_name is installed bundle on
*  current system.
*/
bool is_installed_bundle(const char *bundle_name)
{
	struct stat statb;
	char *filename = NULL;
	bool ret = true;

	string_or_die(&filename, "%s/%s/%s", globals.path_prefix, BUNDLES_DIR, bundle_name);

	if (stat(filename, &statb) == -1) {
		ret = false;
	}

	free_string(&filename);
	return ret;
}

static int find_manifest(const void *a, const void *b)
{
	struct manifest *A;
	char *B;
	int ret;

	A = (struct manifest *)a;
	B = (char *)b;

	ret = strcmp(A->component, B);
	if (ret != 0) {
		return ret;
	}

	/* we found a match*/
	return 0;
}

/* Return list of bundles that include bundle_name */
int required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion, struct list *exclusions, char *msg)
{
	struct list *b, *i;
	char *name;
	int count = 0;
	static bool print_msg;
	bool verbose = (log_get_level() == LOG_INFO_VERBOSE);

	// track recursion level for indentation
	if (recursion == 0) {
		print_msg = true;
	}
	recursion++;

	/* look at the manifest of all listed bundles to see if
	 * they list *bundle_name as a dependency */
	b = list_head(mom->submanifests);
	while (b) {
		struct manifest *bundle = b->data;
		b = b->next;

		if (strcmp(bundle->component, bundle_name) == 0) {
			/* circular dependencies are not allowed in manifests,
			 * so we can skip checking for dependencies in the same bundle */
			continue;
		}

		int indent = 0;
		i = list_head(bundle->includes);
		while (i) {
			name = i->data;
			i = i->next;

			if (strcmp(name, bundle_name) == 0) {

				/* this bundle has *bundle_name as a dependency */

				/* if the bundle being looked at is in the list of exclusions
				 * then don't consider it as a dependency, the user added it to
				 * the list of bundles to be removed too, but we DO want to
				 * consider its list of includes */
				if (!list_search(exclusions, bundle->component, strcmp_wrapper)) {

					/* add bundle to list of dependencies */
					char *bundle_str = NULL;
					string_or_die(&bundle_str, "%s", bundle->component);
					*reqd_by = list_append_data(*reqd_by, bundle_str);

					/* if the --verbose options was used, print the dependency as a
					 * tree element, in this view it is ok to have duplicated elements */
					if (verbose) {
						if (print_msg) {
							/* these messages should be printed only once */
							print_msg = false;
							info("%s", msg);
							info("\nformat:\n");
							info(" # * is-required-by\n");
							info(" #   |-- is-required-by\n");
							info(" # * is-also-required-by\n # ...\n");
							info("\n");
						}
						indent = (recursion - 1) * 4;
						if (recursion == 1) {
							info("%*s* %s\n", indent + 2, "", bundle->component);
						} else {
							info("%*s|-- %s\n", indent, "", bundle->component);
						}
					}
				}

				/* let's see what bundles list this new bundle as a dependency */
				required_by(reqd_by, bundle->component, mom, recursion, exclusions, msg);
			}
		}
	}

	if (recursion == 1) {
		/* get rid of duplicated dependencies */
		*reqd_by = list_sort(*reqd_by, strcmp_wrapper);
		*reqd_by = list_sorted_deduplicate(*reqd_by, strcmp_wrapper, free);

		/* if not using --verbose, we need to print the simplified
		 * list of bundles that depend on *bundle_name */
		i = list_head(*reqd_by);
		while (i) {
			name = i->data;
			count++;
			i = i->next;

			if (!verbose) {
				if (print_msg) {
					/* this message should be printed only once */
					print_msg = false;
					info("%s", msg);
				}
				info(" - %s\n", name);
			}
		}
	}

	return count;
}

void track_bundle_in_statedir(const char *bundle_name, const char *state_dir)
{
	int ret = 0;
	char *src;
	char *tracking_dir;

	tracking_dir = sys_path_join(state_dir, "bundles");

	/* if state_dir_parent/bundles doesn't exist or is empty, assume this is
	 * the first time tracking installed bundles. Since we don't know what the
	 * user installed themselves just copy the entire system tracking directory
	 * into the state tracking directory. */
	if (!is_populated_dir(tracking_dir)) {
		char *rmfile;
		ret = rm_rf(tracking_dir);
		if (ret) {
			goto out;
		}
		src = sys_path_join(globals.path_prefix, "/usr/share/clear/bundles");
		/* at the point this function is called <bundle_name> is already
		 * installed on the system and therefore has a tracking file under
		 * /usr/share/clear/bundles. A simple cp -a of that directory will
		 * accurately track that bundle as manually installed. */
		ret = copy_all(src, state_dir);
		free_string(&src);
		if (ret) {
			goto out;
		}
		/* remove uglies that live in the system tracking directory */
		rmfile = sys_path_join(tracking_dir, ".MoM");
		(void)unlink(rmfile);
		free_string(&rmfile);
		/* set perms on the directory correctly */
		ret = chmod(tracking_dir, S_IRWXU);
		if (ret) {
			goto out;
		}
	}

	char *tracking_file = sys_path_join(tracking_dir, bundle_name);
	int fd = open(tracking_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	free_string(&tracking_file);
	if (fd < 0) {
		ret = -1;
		goto out;
	}
	close(fd);

out:
	if (ret) {
		debug("Issue creating tracking file in %s for %s\n", tracking_dir, bundle_name);
	}
	free_string(&tracking_dir);
}

void track_bundle(const char *bundle_name)
{
	track_bundle_in_statedir(bundle_name, globals.state_dir);
}

/*
 * remove_tracked removes the tracking file in
 * path_prefix/state_dir_parent/bundles if it exists to untrack as manually
 * installed if the file exists
 */
static void remove_tracked(const char *bundle)
{
	char *destdir = get_tracking_dir();
	char *tracking_file = sys_path_join(destdir, bundle);
	free_string(&destdir);
	/* we don't care about failures here since any weird state in the tracking
	 * dir MUST be handled gracefully */
	sys_rm(tracking_file);
	free_string(&tracking_file);
}

static int filter_files_to_delete(const void *a, const void *b)
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

	ret = strcmp(A->filename, B->filename);
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

/*  This function is a fresh new implementation for a bundle
 *  remove without being tied to verify loop, this means
 *  improved speed and space as well as more robustness and
 *  flexibility. The function removes one or more bundles
 *  passed in the bundles param.
 */
enum swupd_code remove_bundles(struct list *bundles)
{
	int ret = SWUPD_OK;
	int ret_code = 0;
	int bad = 0;
	int total = 0;
	int current_version = CURRENT_OS_VERSION;
	struct manifest *current_mom = NULL;
	struct list *subs = NULL;
	struct list *bundles_to_remove = NULL;
	struct list *files_to_remove = NULL;
	struct list *reqd_by = NULL;
	char *bundles_list_str = NULL;
	bool mix_exists;

	ret = swupd_init(SWUPD_ALL);
	if (ret != 0) {
		error("Failed updater initialization, exiting now\n");
		return ret;
	}

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		ret_code = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto out_deinit;
	}

	mix_exists = (check_mix_exists() & system_on_mix());

	current_mom = load_mom(current_version, mix_exists, NULL);
	if (!current_mom) {
		error("Unable to download/verify %d Manifest.MoM\n", current_version);
		ret_code = SWUPD_COULDNT_LOAD_MOM;
		goto out_deinit;
	}

	/* load all tracked bundles into memory */
	read_subscriptions(&subs);
	set_subscription_versions(current_mom, NULL, &subs);

	/* load all submanifests */
	current_mom->submanifests = recurse_manifest(current_mom, subs, NULL, false, NULL);
	if (!current_mom->submanifests) {
		error("Cannot load MoM sub-manifests\n");
		ret_code = SWUPD_RECURSE_MANIFEST;
		goto out_subs;
	}

	for (; bundles; bundles = bundles->next, total++) {

		char *bundle = bundles->data;

		/* os-core bundle not allowed to be removed...
		* although this is going to be caught later because of all files
		* being marked as 'duplicated' and note removing anything
		* anyways, better catch here and return success, no extra work to be done.
		*/
		if (strcmp(bundle, "os-core") == 0) {
			warn("\nBundle \"os-core\" not allowed to be removed, skipping it...\n");
			ret_code = SWUPD_REQUIRED_BUNDLE_ERROR;
			bad++;
			continue;
		}

		if (!mom_search_bundle(current_mom, bundle)) {
			warn("\nBundle \"%s\" is invalid, skipping it...\n", bundle);
			ret_code = SWUPD_INVALID_BUNDLE;
			bad++;
			continue;
		}

		if (!is_installed_bundle(bundle)) {
			warn("\nBundle \"%s\" is not installed, skipping it...\n", bundle);
			ret_code = SWUPD_BUNDLE_NOT_TRACKED;
			bad++;
			continue;
		}

		/* check if bundle is required by another installed bundle */
		char *msg;
		string_or_die(&msg, "\nBundle \"%s\" is required by the following bundles:\n", bundle);
		int number_of_reqd = required_by(&reqd_by, bundle, current_mom, 0, bundles, msg);
		free_string(&msg);
		if (number_of_reqd > 0) {
			/* the bundle is required by other bundles, do not continue with the removal
			 * unless the --force flag is used */
			if (!cmdline_option_force) {

				error("\nBundle \"%s\" is required by %d bundle%s, skipping it...\n", bundle, number_of_reqd, number_of_reqd == 1 ? "" : "s");
				info("Use \"swupd bundle-remove --force %s\" to remove \"%s\" and all bundles that require it\n", bundle, bundle);
				ret_code = SWUPD_REQUIRED_BUNDLE_ERROR;
				bad++;
				list_free_list_and_data(reqd_by, free);
				reqd_by = NULL;
				continue;

			} else {

				/* the --force option was specified */
				info("\nThe --force option was used, bundle \"%s\" and all bundles that require it will be removed from the system\n", bundle);

				/* move the manifest of dependent bundles to the list of bundles to be removed */
				struct list *iter = NULL;
				char *dep;
				iter = list_head(reqd_by);
				while (iter) {
					dep = iter->data;
					iter = iter->next;
					list_move_item(dep, &current_mom->submanifests, &bundles_to_remove, find_manifest);
					remove_tracked(dep);
				}
				list_free_list_and_data(reqd_by, free);
				reqd_by = NULL;
			}
		}

		/* move the manifest of the bundle to be removed from the list of subscribed bundles
		 * to the list of bundles to be removed */
		list_move_item(bundle, &current_mom->submanifests, &bundles_to_remove, find_manifest);
		info("\nRemoving bundle: %s\n", bundle);
		remove_tracked(bundle);
	}

	/* if there are no bundles to remove we are done */
	if (bundles_to_remove) {

		/* get the list of all files required by the installed bundles (except the ones to be removed) */
		current_mom->files = consolidate_files_from_bundles(current_mom->submanifests);

		/* get the list of the files contained in the bundles to be removed */
		files_to_remove = consolidate_files_from_bundles(bundles_to_remove);

		/* sanitize files to remove; if a file is needed by a bundle that
		 * is installed, it should be kept in the system */
		files_to_remove = list_sorted_filter_common_elements(files_to_remove, current_mom->files, filter_files_to_delete, NULL);

		if (list_len(files_to_remove) > 0) {
			info("\nDeleting bundle files...\n");
			progress_next_step("remove_files", PROGRESS_BAR);
			int deleted = remove_files_from_fs(files_to_remove);
			info("Total deleted files: %i\n", deleted);
		}
	}

	if (bad > 0) {
		print("\nFailed to remove %i of %i bundles\n", bad, total);
	} else {
		print("\nSuccessfully removed %i bundle%s\n", total, (total > 1 ? "s" : ""));
	}

	list_free_list(files_to_remove);
	list_free_list_and_data(bundles_to_remove, manifest_free_data);
	manifest_free(current_mom);
	free_subscriptions(&subs);
	swupd_deinit();

	return ret_code;

out_subs:
	manifest_free(current_mom);
	free_subscriptions(&subs);
out_deinit:
	bundles_list_str = string_join(", ", bundles);
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
	free_string(&bundles_list_str);
	swupd_deinit();
	print("\nFailed to remove bundle(s)\n");

	return ret_code;
}
