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
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
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

void remove_set_option_force(bool opt)
{
	cmdline_option_force = opt;
}

/*
* list_installable_bundles()
* Parse the full manifest for the current version of the OS and print
*   all available bundles.
*/
enum swupd_code list_installable_bundles()
{
	char *name;
	struct list *list;
	struct file *file;
	struct manifest *MoM = NULL;
	int current_version;
	bool mix_exists;

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		return SWUPD_CURRENT_VERSION_UNKNOWN;
	}

	mix_exists = (check_mix_exists() & system_on_mix());
	MoM = load_mom(current_version, mix_exists, NULL);
	if (!MoM) {
		return SWUPD_COULDNT_LOAD_MOM;
	}

	list = MoM->manifests = list_sort(MoM->manifests, file_sort_filename);
	while (list) {
		file = list->data;
		list = list->next;
		name = get_printable_bundle_name(file->filename, file->is_experimental);
		print("%s\n", name);
		free_string(&name);
	}

	free_manifest(MoM);
	return 0;
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
static int required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion, struct list *exclusions, char *msg)
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
				if (!list_search(exclusions, bundle->component, list_strcmp)) {

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
		*reqd_by = list_str_deduplicate(*reqd_by);

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

/* Return recursive list of included bundles */
enum swupd_code show_included_bundles(char *bundle_name)
{
	int ret = 0;
	int current_version = CURRENT_OS_VERSION;
	struct list *subs = NULL;
	struct list *deps = NULL;
	struct manifest *mom = NULL;

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto out;
	}

	mom = load_mom(current_version, false, NULL);
	if (!mom) {
		error("Cannot load official manifest MoM for version %i\n", current_version);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto out;
	}

	// add_subscriptions takes a list, so construct one with only bundle_name
	struct list *bundles = NULL;
	bundles = list_prepend_data(bundles, bundle_name);
	ret = add_subscriptions(bundles, &subs, mom, true, 0);
	list_free_list(bundles);
	if (ret != add_sub_NEW) {
		// something went wrong or there were no includes, print a message and exit
		char *m = NULL;
		if (ret & add_sub_ERR) {
			string_or_die(&m, "Processing error");
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
		} else if (ret & add_sub_BADNAME) {
			string_or_die(&m, "Bad bundle name detected");
			ret = SWUPD_INVALID_BUNDLE;
		} else {
			string_or_die(&m, "Unknown error");
			ret = SWUPD_UNEXPECTED_CONDITION;
		}

		error("%s - Aborting\n", m);
		free_string(&m);
		goto out;
	}
	deps = recurse_manifest(mom, subs, NULL, false, NULL);
	if (!deps) {
		error("Cannot load included bundles\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto out;
	}

	/* deps now includes the bundle indicated by bundle_name
	 * if deps only has one bundle in it, no included packages were found */
	if (list_len(deps) == 1) {
		info("No included bundles\n");
		ret = SWUPD_OK;
		goto out;
	}

	info("Bundles included by %s:\n\n", bundle_name);

	struct list *iter;
	iter = list_head(deps);
	while (iter) {
		struct manifest *included_bundle = iter->data;
		iter = iter->next;
		// deps includes the bundle_name bundle, skip it
		if (strcmp(bundle_name, included_bundle->component) == 0) {
			continue;
		}

		print("%s\n", included_bundle->component);
	}

	ret = SWUPD_OK;

out:
	if (mom) {
		free_manifest(mom);
	}

	if (deps) {
		list_free_list_and_data(deps, free_manifest_data);
	}

	if (subs) {
		free_subscriptions(&subs);
	}

	return ret;
}

enum swupd_code show_bundle_reqd_by(const char *bundle_name, bool server)
{
	int ret = 0;
	int version = CURRENT_OS_VERSION;
	struct manifest *current_manifest = NULL;
	struct list *subs = NULL;
	struct list *reqd_by = NULL;
	int number_of_reqd = 0;

	if (!server && !is_installed_bundle(bundle_name)) {
		info("Bundle \"%s\" does not seem to be installed\n", bundle_name);
		info("       try passing --all to check uninstalled bundles\n");
		ret = SWUPD_BUNDLE_NOT_TRACKED;
		goto out;
	}

	version = get_current_version(globals.path_prefix);
	if (version < 0) {
		error("Unable to determine current OS version\n");
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto out;
	}

	current_manifest = load_mom(version, false, NULL);
	if (!current_manifest) {
		error("Unable to download/verify %d Manifest.MoM\n", version);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto out;
	}

	if (!search_bundle_in_manifest(current_manifest, bundle_name)) {
		error("Bundle \"%s\" is invalid, aborting dependency list\n", bundle_name);
		ret = SWUPD_INVALID_BUNDLE;
		goto out;
	}

	if (server) {
		ret = add_included_manifests(current_manifest, &subs);
		if (ret) {
			error("Unable to load server manifest");
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
			goto out;
		}

	} else {
		/* load all tracked bundles into memory */
		read_subscriptions(&subs);
	}

	/* load all submanifests */
	current_manifest->submanifests = recurse_manifest(current_manifest, subs, NULL, server, NULL);
	if (!current_manifest->submanifests) {
		error("Cannot load MoM sub-manifests\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto out;
	}

	char *msg;
	string_or_die(&msg, "%s bundles that have %s as a dependency:\n", server ? "All installable and installed" : "Installed", bundle_name);
	number_of_reqd = required_by(&reqd_by, bundle_name, current_manifest, 0, NULL, msg);
	free_string(&msg);
	if (reqd_by == NULL) {
		info("No bundles have %s as a dependency\n", bundle_name);
		ret = SWUPD_OK;
		goto out;
	}
	list_free_list_and_data(reqd_by, free);
	info("\nBundle '%s' is required by %d bundle%s\n", bundle_name, number_of_reqd, number_of_reqd == 1 ? "" : "s");

	ret = SWUPD_OK;

out:
	if (current_manifest) {
		free_manifest(current_manifest);
	}

	if (ret) {
		print("Bundle list failed\n");
	}

	if (subs) {
		free_subscriptions(&subs);
	}

	return ret;
}

/*
 * remove_tracked removes the tracking file in
 * path_prefix/state_dir_parent/bundles if it exists to untrack as manually
 * installed if the file exists
 */
static void remove_tracked(const char *bundle)
{
	char *destdir = get_tracking_dir();
	char *tracking_file = mk_full_filename(destdir, bundle);
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
 *
 *  For each one of the bundles to be removed what it does
 *  is basically:
 *
 *  1) Read MoM and load all submanifests except the one to be
 *  	removed and then consolidate them.
 *  2) Load the removed bundle submanifest.
 *  3) Order the file list by filename
 *  4) Deduplicate removed submanifest file list that happens
 *  	to be on the MoM (minus bundle to be removed).
 *  5) iterate over to be removed bundle submanifest file list
 *  	performing a unlink(2) for each filename.
 *  6) Done.
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

		if (!search_bundle_in_manifest(current_mom, bundle)) {
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
		files_to_remove = list_filter_common_elements(files_to_remove, current_mom->files, filter_files_to_delete, NULL);

		if (list_len(files_to_remove) > 0) {
			info("\nDeleting bundle files...\n");
			progress_set_step(1, "remove_files");
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
	list_free_list_and_data(bundles_to_remove, free_manifest_data);
	free_manifest(current_mom);
	free_subscriptions(&subs);
	swupd_deinit();

	return ret_code;

out_subs:
	free_manifest(current_mom);
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

/* bitmapped return
   1 error happened
   2 new subscriptions
   4 bad name given
*/
int add_subscriptions(struct list *bundles, struct list **subs, struct manifest *mom, bool find_all, int recursion)
{
	char *bundle;
	int manifest_err;
	int ret = 0;
	struct file *file;
	struct list *iter;
	struct manifest *manifest;

	iter = list_head(bundles);
	while (iter) {
		bundle = iter->data;
		iter = iter->next;

		file = search_bundle_in_manifest(mom, bundle);
		if (!file) {
			warn("Bundle \"%s\" is invalid, skipping it...\n", bundle);
			ret |= add_sub_BADNAME; /* Use this to get non-zero exit code */
			continue;
		}

		if (!find_all && is_installed_bundle(bundle)) {
			continue;
		}

		manifest = load_manifest(file->last_change, file, mom, true, &manifest_err);
		if (!manifest) {
			error("Unable to download manifest %s version %d, exiting now\n", bundle, file->last_change);
			ret |= add_sub_ERR;
			goto out;
		}

		/*
		 * If we're recursing a tree of includes, we need to cut out early
		 * if the bundle we're looking at is already subscribed...
		 * Because if it is, we'll visit it soon anyway at the top level.
		 *
		 * We can't do this for the toplevel of the recursion because
		 * that is how we initiallly fill in the include tree.
		 */
		if (component_subscribed(*subs, bundle)) {
			if (recursion > 0) {
				free_manifest(manifest);
				continue;
			}
		} else {
			// Just add it to a list if it doesn't exist
			create_and_append_subscription(subs, bundle);
			ret |= add_sub_NEW; /* We have added at least one */
		}

		if (manifest->includes) {
			/* merge in recursive call results */
			ret |= add_subscriptions(manifest->includes, subs, mom, find_all, recursion + 1);
		}

		if (!globals.skip_optional_bundles && manifest->optional) {
			/* merge in recursive call results */
			ret |= add_subscriptions(manifest->optional, subs, mom, find_all, recursion + 1);
		}

		free_manifest(manifest);
	}
out:
	return ret;
}

/*
 * This function will read the BUNDLES_DIR (by default
 * /usr/share/clear/bundles/), get the list of local bundles and print
 * them sorted.
 */
enum swupd_code list_local_bundles()
{
	char *name;
	char *path = NULL;
	struct list *bundles = NULL;
	struct list *item = NULL;
	struct manifest *MoM = NULL;
	struct file *bundle_manifest = NULL;
	int current_version;
	bool mix_exists;

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		goto skip_mom;
	}

	mix_exists = (check_mix_exists() & system_on_mix());
	MoM = load_mom(current_version, mix_exists, NULL);
	if (!MoM) {
		warn("Could not determine which installed bundles are experimental\n");
	}

skip_mom:
	string_or_die(&path, "%s/%s", globals.path_prefix, BUNDLES_DIR);

	errno = 0;
	bundles = get_dir_files_sorted(path);
	if (!bundles && errno) {
		error("couldn't open bundles directory");
		free_string(&path);
		return SWUPD_COULDNT_LIST_DIR;
	}

	item = bundles;

	while (item) {
		if (MoM) {
			bundle_manifest = search_bundle_in_manifest(MoM, basename((char *)item->data));
		}
		if (bundle_manifest) {
			name = get_printable_bundle_name(bundle_manifest->filename, bundle_manifest->is_experimental);
		} else {
			string_or_die(&name, basename((char *)item->data));
		}
		print("%s\n", name);
		free_string(&name);
		free(item->data);
		item = item->next;
	}

	list_free_list(bundles);

	free_string(&path);
	free_manifest(MoM);

	return SWUPD_OK;
}
