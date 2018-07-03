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
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define add_sub_ERR 1
#define add_sub_NEW 2
#define add_sub_BADNAME 4

/*
* list_installable_bundles()
* Parse the full manifest for the current version of the OS and print
*   all available bundles.
*/
int list_installable_bundles()
{
	struct list *list;
	struct file *file;
	struct manifest *MoM = NULL;
	int current_version;
	bool mix_exists;

	int ret = swupd_curl_check_network();
	if (ret) {
		fprintf(stderr, "Error: Network issue, unable to download manifest\n");
		return ret;
	}

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		return ECURRENT_VERSION;
	}

	mix_exists = (check_mix_exists() & system_on_mix());
	MoM = load_mom(current_version, false, mix_exists);
	if (!MoM) {
		return EMOM_NOTFOUND;
	}

	list = MoM->manifests = list_sort(MoM->manifests, file_sort_filename);
	while (list) {
		file = list->data;
		list = list->next;
		printf("%s\n", file->filename);
	}

	free_manifest(MoM);
	return 0;
}

/* bundle_name: this is the name for the bundle we want to be loaded
* version: this is the MoM version from which we pull last changed for bundle manifest
* submanifest: where bundle manifest is going to be loaded
*
* Basically we read MoM version then get the submanifest only for our bundle (component)
* put it into submanifest pointer, then dispose MoM data.
*/
static int load_bundle_manifest(const char *bundle_name, struct list *subs, int version, struct manifest **submanifest)
{
	struct list *sub_list = NULL;
	struct manifest *mom = NULL;
	int ret = 0;

	*submanifest = NULL;

	mom = load_mom(version, false, false);
	if (!mom) {
		return EMOM_NOTFOUND;
	}

	sub_list = recurse_manifest(mom, subs, bundle_name, false);
	if (!sub_list) {
		ret = ERECURSE_MANIFEST;
		goto free_out;
	}

	*submanifest = sub_list->data;
	sub_list->data = NULL;
	ret = 0;

free_out:
	list_free_list(sub_list);
	free_manifest(mom);
	return ret;
}

/* Finds out whether bundle_name is tracked bundle on
*  current system.
*/
bool is_tracked_bundle(const char *bundle_name)
{
	struct stat statb;
	char *filename = NULL;
	bool ret = true;

	string_or_die(&filename, "%s/%s/%s", path_prefix, BUNDLES_DIR, bundle_name);

	if (stat(filename, &statb) == -1) {
		ret = false;
	}

	free_string(&filename);
	return ret;
}

/* When loading all tracked bundles into memory, they happen
 * to be hold in subs global var, for some reasons it is
 * needed to just pop out one or more of this loaded tracked
 * bundles, this function search for bundle_name into subs
 * struct and if it found then free it from the list.
 */
static int unload_tracked_bundle(const char *bundle_name, struct list **subs)
{
	struct list *bundles;
	struct list *cur_item;
	struct sub *bundle;

	bundles = list_head(*subs);
	while (bundles) {
		bundle = bundles->data;
		cur_item = bundles;
		bundles = bundles->next;
		if (strcmp(bundle->component, bundle_name) == 0) {
			/* unlink (aka untrack) matching bundle name from tracked ones */
			*subs = free_bundle(cur_item);
			return EXIT_SUCCESS;
		}
	}

	return EBUNDLE_NOT_TRACKED;
}

/* Return list of bundles that include bundle_name */
static void required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int recursion)
{
	struct list *b, *i;
	// track recursion level for indentation
	recursion++;

	b = list_head(mom->submanifests);
	while (b) {
		struct manifest *bundle = b->data;
		b = b->next;

		int indent = 0;
		i = list_head(bundle->includes);
		while (i) {
			char *name = i->data;
			i = i->next;

			if (strcmp(name, bundle_name) == 0) {
				char *bundle_str = NULL;
				indent = (recursion - 1) * 4;
				if (recursion == 1) {
					string_or_die(&bundle_str, "%*s* %s\n", indent + 2, "", bundle->component);
				} else {
					string_or_die(&bundle_str, "%*s|-- %s\n", indent, "", bundle->component);
				}

				*reqd_by = list_append_data(*reqd_by, bundle_str);
				required_by(reqd_by, bundle->component, mom, recursion);
			}
		}
	}
}
/* Return recursive list of included bundles */
int show_included_bundles(char *bundle_name)
{
	int ret = 0;
	int current_version = CURRENT_OS_VERSION;
	struct list *subs = NULL;
	struct list *deps = NULL;
	struct manifest *mom = NULL;

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		ret = ECURRENT_VERSION;
		goto out;
	}

	mom = load_mom(current_version, false, false);
	if (!mom) {
		fprintf(stderr, "Cannot load official manifest MoM for version %i\n", current_version);
		ret = EMOM_NOTFOUND;
		goto out;
	}

	// add_subscriptions takes a list, so construct one with only bundle_name
	struct list *bundles = NULL;
	bundles = list_prepend_data(bundles, bundle_name);
	ret = add_subscriptions(bundles, &subs, current_version, mom, true, 0);
	list_free_list(bundles);
	if (ret != add_sub_NEW) {
		// something went wrong or there were no includes, print a message and exit
		char *m = NULL;
		if (ret & add_sub_ERR) {
			string_or_die(&m, "Processing error");
		} else if (ret & add_sub_BADNAME) {
			string_or_die(&m, "Bad bundle name detected");
		} else {
			string_or_die(&m, "Unknown error");
		}

		fprintf(stderr, "Error: %s - Aborting\n", m);
		free_string(&m);
		ret = 1;
		goto out;
	}
	deps = recurse_manifest(mom, subs, NULL, false);
	if (!deps) {
		fprintf(stderr, "Error: Cannot load included bundles\n");
		ret = ERECURSE_MANIFEST;
		goto out;
	}

	/* deps now includes the bundle indicated by bundle_name
	 * if deps only has one bundle in it, no included packages were found */
	if (list_len(deps) == 1) {
		fprintf(stderr, "No included bundles\n");
		ret = 0;
		goto out;
	}

	fprintf(stderr, "Bundles included by %s:\n\n", bundle_name);

	struct list *iter;
	iter = list_head(deps);
	while (iter) {
		struct manifest *included_bundle = iter->data;
		iter = iter->next;
		// deps includes the bundle_name bundle, skip it
		if (strcmp(bundle_name, included_bundle->component) == 0) {
			continue;
		}

		printf("%s\n", included_bundle->component);
	}

	ret = 0;

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

int show_bundle_reqd_by(const char *bundle_name, bool server)
{
	int ret = 0;
	int version = CURRENT_OS_VERSION;
	struct manifest *current_manifest = NULL;
	struct list *subs = NULL;
	struct list *reqd_by = NULL;

	if (!server && !is_tracked_bundle(bundle_name)) {
		fprintf(stderr, "Error: Bundle \"%s\" does not seem to be installed\n", bundle_name);
		fprintf(stderr, "       try passing --all to check uninstalled bundles\n");
		ret = EBUNDLE_NOT_TRACKED;
		goto out;
	}

	version = get_current_version(path_prefix);
	if (version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		ret = ECURRENT_VERSION;
		goto out;
	}

	current_manifest = load_mom(version, server, false);
	if (!current_manifest) {
		fprintf(stderr, "Unable to download/verify %d Manifest.MoM\n", version);
		ret = EMOM_NOTFOUND;
		goto out;
	}

	if (!search_bundle_in_manifest(current_manifest, bundle_name)) {
		fprintf(stderr, "Bundle name %s is invalid, aborting dependency list\n", bundle_name);
		ret = EBUNDLE_REMOVE;
		goto out;
	}

	if (server) {
		ret = add_included_manifests(current_manifest, version, &subs);
		if (ret) {
			fprintf(stderr, "Unable to load server manifest");
			ret = EMANIFEST_LOAD;
			goto out;
		}

	} else {
		/* load all tracked bundles into memory */
		read_subscriptions(&subs);
		/* now popout the one to be processed */
		ret = unload_tracked_bundle(bundle_name, &subs);
		if (ret != 0) {
			fprintf(stderr, "Unable to untrack %s\n", bundle_name);
			goto out;
		}
	}

	/* load all submanifests */
	current_manifest->submanifests = recurse_manifest(current_manifest, subs, NULL, server);
	if (!current_manifest->submanifests) {
		fprintf(stderr, "Error: Cannot load MoM sub-manifests\n");
		ret = ERECURSE_MANIFEST;
		goto out;
	}

	required_by(&reqd_by, bundle_name, current_manifest, 0);
	if (reqd_by == NULL) {
		fprintf(stderr, "No bundles have %s as a dependency\n", bundle_name);
		ret = 0;
		goto out;
	}

	fprintf(stderr, server ? "All installable and installed " : "Installed ");
	fprintf(stderr, "bundles that have %s as a dependency:\n", bundle_name);
	fprintf(stderr, "format:\n");
	fprintf(stderr, " # * is-required-by\n");
	fprintf(stderr, " #   |-- is-required-by\n");
	fprintf(stderr, " # * is-also-required-by\n # ...\n\n");

	struct list *iter;
	char *bundle;
	iter = list_head(reqd_by);
	while (iter) {
		bundle = iter->data;
		iter = iter->next;
		printf("%s", bundle);
		free_string(&bundle);
	}

	ret = 0;

out:
	if (current_manifest) {
		free_manifest(current_manifest);
	}

	if (ret) {
		fprintf(stderr, "Error: Bundle list failed\n");
	}

	if (reqd_by) {
		list_free_list(reqd_by);
	}

	if (subs) {
		free_subscriptions(&subs);
	}

	return ret;
}

/*  This function is a fresh new implementation for a bundle
 *  remove without being tied to verify loop, this means
 *  improved speed and space as well as more roubustness and
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
int remove_bundles(char **bundles)
{
	int lock_fd;
	int ret = 0;
	int ret_code = 0;
	int bad = 0;
	int total = 0;
	int current_version = CURRENT_OS_VERSION;
	struct manifest *current_mom, *bundle_manifest = NULL;
	struct list *subs = NULL;
	bool mix_exists;

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		fprintf(stderr, "Failed updater initialization, exiting now.\n");
		return ret;
	}

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		ret = ECURRENT_VERSION;
		telemetry(TELEMETRY_CRIT,
			  "bundleremove",
			  "bundle=%s\n"
			  "current_version=%d\n"
			  "result=%d\n",
			  *bundles,
			  current_version,
			  ret);
		swupd_deinit(lock_fd, &subs);
		return ret;
	}

	mix_exists = (check_mix_exists() & system_on_mix());

	for (; *bundles; ++bundles, total++) {

		char *bundle = *bundles;

		/* os-core bundle not allowed to be removed...
		* although this is going to be caught later because of all files
		* being marked as 'duplicated' and note removing anything
		* anyways, better catch here and return success, no extra work to be done.
		*/
		if (strcmp(bundle, "os-core") == 0) {
			fprintf(stderr, "Warning: Bundle \"os-core\" not allowed to be removed\n");
			ret = EBUNDLE_NOT_TRACKED;
			bad++;
			goto out_free_curl;
		}

		if (!is_tracked_bundle(bundle)) {
			fprintf(stderr, "Warning: Bundle \"%s\" is not installed, skipping it...\n", bundle);
			ret = EBUNDLE_NOT_TRACKED;
			bad++;
			goto out_free_curl;
		}

		/* only show this message if there are multiple bundles to be removed */
		if (*(bundles + 1) || total > 0) {
			fprintf(stderr, "Removing bundle: %s\n", bundle);
		}

		current_mom = load_mom(current_version, false, mix_exists);
		if (!current_mom) {
			fprintf(stderr, "Unable to download/verify %d Manifest.MoM\n", current_version);
			ret = EMOM_NOTFOUND;
			bad++;
			goto out_free_curl;
		}

		if (!search_bundle_in_manifest(current_mom, bundle)) {
			fprintf(stderr, "Bundle name is invalid, aborting removal\n");
			ret = EBUNDLE_REMOVE;
			bad++;
			goto out_free_mom;
		}

		/* load all tracked bundles into memory */
		read_subscriptions(&subs);

		/* popout the bundle to be removed from memory */
		ret = unload_tracked_bundle(bundle, &subs);
		if (ret != 0) {
			bad++;
			goto out_free_mom;
		}

		set_subscription_versions(current_mom, NULL, &subs);

		/* load all submanifests minus the one to be removed */
		current_mom->submanifests = recurse_manifest(current_mom, subs, NULL, false);
		if (!current_mom->submanifests) {
			fprintf(stderr, "Error: Cannot load MoM sub-manifests\n");
			ret = ERECURSE_MANIFEST;
			bad++;
			goto out_free_mom;
		}

		/* check if bundle is required by another installed bundle */
		struct list *reqd_by = NULL;
		required_by(&reqd_by, bundle, current_mom, 0);
		if (reqd_by != NULL) {
			struct list *iter;
			char *bundle;
			iter = list_head(reqd_by);
			fprintf(stderr, "Error: bundle requested to be removed is required by the following bundles:\n");
			printf("format:\n");
			printf(" # * is-required-by\n");
			printf(" #   |-- is-required-by\n");
			printf(" # * is-also-required-by\n # ...\n\n");
			while (iter) {
				bundle = iter->data;
				iter = iter->next;
				fprintf(stderr, "%s", bundle);
			}

			list_free_list_and_data(reqd_by, free);
			ret = EBUNDLE_REMOVE;
			bad++;
			goto out_free_mom;
		}

		current_mom->files = files_from_bundles(current_mom->submanifests);
		current_mom->files = consolidate_files(current_mom->files);

		/* Now that we have the consolidated list of all files, load bundle to be removed submanifest */
		ret = load_bundle_manifest(bundle, subs, current_version, &bundle_manifest);
		if (ret != 0 || !bundle_manifest) {
			fprintf(stderr, "Error: Cannot load %s sub-manifest (ret = %d)\n", bundle, ret);
			bad++;
			goto out_free_mom;
		}

		/* deduplication needs file list sorted by filename, do so */
		bundle_manifest->files = list_sort(bundle_manifest->files, file_sort_filename);
		deduplicate_files_from_manifest(&bundle_manifest, current_mom);

		fprintf(stderr, "Deleting bundle files...\n");
		remove_files_in_manifest_from_fs(bundle_manifest);

		fprintf(stderr, "Untracking bundle from system...\n");
		rm_bundle_file(bundle);

		free_manifest(bundle_manifest);
	out_free_mom:
		free_manifest(current_mom);
	out_free_curl:
		telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
			  "bundleremove",
			  "bundle=%s\n"
			  "current_version=%d\n"
			  "result=%d\n",
			  bundle,
			  current_version,
			  ret);
		if (ret) {
			ret_code = ret;
		}
	}

	if (bad > 0) {
		fprintf(stderr, "Failed to remove %i of %i bundles\n", bad, total);
	} else {
		fprintf(stderr, "Successfully removed %i bundle%s\n", total, (total > 1 ? "s" : ""));
	}

	swupd_deinit(lock_fd, &subs);

	return ret_code;
}

/* bitmapped return
   1 error happened
   2 new subscriptions
   4 bad name given
*/
int add_subscriptions(struct list *bundles, struct list **subs, int current_version, struct manifest *mom, bool find_all, int recursion)
{
	char *bundle;
	int ret = 0;
	int retries = 0;
	int timeout = 10;
	struct file *file;
	struct list *iter;
	struct manifest *manifest;

	srand(time(NULL));

	iter = list_head(bundles);
	while (iter) {
		bundle = iter->data;
		iter = iter->next;

		file = search_bundle_in_manifest(mom, bundle);
		if (!file) {
			fprintf(stderr, "Warning: Bundle \"%s\" is invalid, skipping it...\n", bundle);
			ret |= add_sub_BADNAME; /* Use this to get non-zero exit code */
			continue;
		}

		/*
		 * If we're recursing a tree of includes, we need to cut out early
		 * if the bundle we're looking at is already subscribed...
		 * Because if it is, we'll visit it soon anyway at the top level.
		 *
		 * We can't do this for the toplevel of the recursion because
		 * that is how we initiallly fill in the include tree.
		 */
		if (component_subscribed(*subs, bundle) && recursion > 0) {
			continue;
		}

	retry_manifest_download:
		manifest = load_manifest(current_version, file->last_change, file, mom, true);
		if (!manifest) {
			if (retries < MAX_TRIES && !content_url_is_local) {
				increment_retries(&retries, &timeout);
				goto retry_manifest_download;
			}
			fprintf(stderr, "Unable to download manifest %s version %d, exiting now\n", bundle, file->last_change);
			ret |= add_sub_ERR;
			goto out;
		}

		if (manifest->includes) {
			int r = add_subscriptions(manifest->includes, subs, current_version, mom, find_all, recursion + 1);
			if (r & add_sub_ERR) {
				free_manifest(manifest);
				goto out;
			}
			ret |= r; /* merge in recursive call results */
		}
		free_manifest(manifest);

		if (!find_all && is_tracked_bundle(bundle)) {
			continue;
		}

		if (component_subscribed(*subs, bundle)) {
			continue;
		}
		create_and_append_subscription(subs, bundle);
		ret |= add_sub_NEW; /* We have added at least one */
	}
out:
	return ret;
}

static int install_bundles(struct list *bundles, struct list **subs, int current_version, struct manifest *mom)
{
	int ret;
	int retries = 0;
	int timeout = 10;
	int bundles_failed = 0;
	int already_installed = 0;
	int bundles_installed = 0;
	int bundles_requested = list_len(bundles);
	long bundle_size = 0;
	long fs_free = 0;
	struct file *file;
	struct list *iter;
	struct list *to_install_bundles = NULL;
	struct list *to_install_files = NULL;
	struct list *ptr;
	struct manifest *full_mom = NULL;
	timelist times;

	times = init_timelist();

	/* step 1: check bundle args are valid if so populate subs struct */
	grabtime_start(&times, "Add bundles and recurse");
	ret = add_subscriptions(bundles, subs, current_version, mom, false, 0);

	/* print a message if any of the requested bundles is already installed */
	iter = list_head(bundles);
	while (iter) {
		char *bundle;
		bundle = iter->data;
		iter = iter->next;
		if (is_tracked_bundle(bundle)) {
			fprintf(stderr, "Warning: Bundle \"%s\" is already installed, skipping it...\n", bundle);
			already_installed++;
		}
	}

	/* Use a bitwise AND with the add_sub_NEW mask to determine if at least
	 * one new bundle was subscribed */
	if (!(ret & add_sub_NEW)) {
		/* something went wrong, print a message and exit */
		ret = EBUNDLE_INSTALL;
		goto out;
	}

	set_subscription_versions(mom, NULL, subs);

	to_install_bundles = recurse_manifest(mom, *subs, NULL, false);
	if (!to_install_bundles) {
		fprintf(stderr, "Error: Cannot load to install bundles\n");
		ret = ERECURSE_MANIFEST;
		goto out;
	}
	grabtime_stop(&times);

	grabtime_start(&times, "Consolidate files from bundles");
	to_install_files = files_from_bundles(to_install_bundles);
	to_install_files = consolidate_files(to_install_files);
	to_install_files = filter_out_existing_files(to_install_files);
	grabtime_stop(&times);

	/* Step 1.5: Now that we know the bundle is valid, check if we have enough space */
	if (!skip_diskspace_check) {
		struct statvfs stat;
		char *filepath = NULL;

		ptr = list_head(to_install_bundles);
		while (ptr) {
			struct manifest *subman;
			subman = ptr->data;
			ptr = ptr->next;

			bundle_size += subman->contentsize;
		}

		filepath = mk_full_filename(path_prefix, "/usr/");

		/* Calculate free space on filepath */
		if (statvfs(filepath, &stat) != 0) {
			fs_free = -1;
		} else {
			fs_free = stat.f_bsize * stat.f_bavail;
		}
		free_string(&filepath);

		/* Add 10% to bundle_size as a 'fudge factor' */
		if (((bundle_size * 1.1) > fs_free && !skip_diskspace_check) || fs_free < 0) {
			ret = EBUNDLE_INSTALL;

			if (fs_free > 0) {
				fprintf(stderr, "Error: Bundle too large by %ldM.\n", (bundle_size - fs_free) / 1000 / 1000);
			} else {
				fprintf(stderr, "Error: Unable to determine free space on filesystem.\n");
			}

			fprintf(stderr, "NOTE: currently, swupd only checks /usr/ "
					"(or the passed-in path with /usr/ appended) for available space.\n");
			fprintf(stderr, "To skip this error and install anyways, "
					"add the --skip-diskspace-check flag to your command.\n");

			goto out;
		}
	}

	/* step 2: download neccessary packs */
	grabtime_start(&times, "Download packs");
	(void)rm_staging_dir_contents("download");

download_subscribed_packs:
	if (list_longer_than(to_install_files, 10) && download_subscribed_packs(*subs, mom, true, true)) {
		if (retries < MAX_TRIES && !content_url_is_local) {
			increment_retries(&retries, &timeout);
			printf("\nRetry #%d downloading subscribed packs\n", retries);
			goto download_subscribed_packs;
		}
	}
	grabtime_stop(&times);

	/* step 2.5: Download missing files */
	grabtime_start(&times, "Download missing files");
	ret = download_fullfiles(to_install_files, MAX_TRIES, 10);
	if (ret) {
		fprintf(stderr, "ERROR: Could not download some files from bundles, aborting bundle installation.\n");
		goto out;
	}
	grabtime_stop(&times);

	mom->files = files_from_bundles(mom->submanifests);
	mom->files = consolidate_files(mom->files);
	grabtime_stop(&times);

	/* step 4: Install all bundle(s) files into the fs */
	grabtime_start(&times, "Installing bundle(s) files onto filesystem");
	fprintf(stderr, "Installing bundle(s) files...\n");

	/* Do an initial check to
	 * 1. make sure the file actually downloaded, if not continue on to the
	 *    verify_fix_path below.
	 * 2. verify the file hash
	 * Do not install any files to the system until the hash has been
	 * verified. The verify_fix_path also verifies the hashes. */
	char *hashpath;
	iter = list_head(to_install_files);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		string_or_die(&hashpath, "%s/staged/%s", state_dir, file->hash);

		if (access(hashpath, F_OK) < 0) {
			/* fallback to verify_fix_path below, which will check the hash
			 * itself */
			free_string(&hashpath);
			continue;
		}

		ret = verify_file(file, hashpath);
		if (!ret) {
			fprintf(stderr, "Warning: hash check failed for %s\n", file->filename);
			fprintf(stderr, "         will attempt to download fullfile for %s\n", file->filename);
			ret = swupd_rm(hashpath);
			if (ret) {
				fprintf(stderr, "Error: could not remove bad file %s\n", hashpath);
				ret = EBUNDLE_INSTALL;
				free_string(&hashpath);
				goto out;
			}
			// successfully removed, continue and check the next file
			free_string(&hashpath);
			continue;
		}
		free_string(&hashpath);
	}

	iter = list_head(to_install_files);
	unsigned int list_length = list_len(to_install_files);
	unsigned int complete = 0;
	while (iter) {
		file = iter->data;
		iter = iter->next;
		complete++;

		if (file->is_deleted || file->do_not_update || ignore(file)) {
			continue;
		}

		/* apply the heuristics for the file so the correct post-actions can
		 * be completed */
		apply_heuristics(file);
		ret = do_staging(file, full_mom);
		if (ret) {
			if (!full_mom) {
				/* step 3: Add tracked bundles */
				grabtime_start(&times, "Add tracked bundles");
				read_subscriptions(subs);
				set_subscription_versions(mom, NULL, subs);
				mom->submanifests = recurse_manifest(mom, *subs, NULL, false);
				if (!mom->submanifests) {
					fprintf(stderr, "Error: Cannot load installed bundles\n");
					ret = ERECURSE_MANIFEST;
					goto out;
				}

				mom->files = files_from_bundles(mom->submanifests);
				mom->files = consolidate_files(mom->files);
				grabtime_stop(&times);
				full_mom = mom;
			}
			ret = verify_fix_path(file->filename, full_mom);
		}
		if (ret) {
			ret = EBUNDLE_INSTALL;
			goto out;
		}

		/* two loops are necessary, first to stage, then to rename. Total is
		 * list_length * 2 */
		print_progress(complete, list_length * 2);
	}

	iter = list_head(to_install_files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		complete++;

		if (file->is_deleted || file->do_not_update || ignore(file)) {
			continue;
		}

		/* This was staged by verify_fix_path */
		if (!file->staging) {
			if (!full_mom) {
				/* step 3: Add tracked bundles */
				grabtime_start(&times, "Add tracked bundles");
				read_subscriptions(subs);
				set_subscription_versions(mom, NULL, subs);
				mom->submanifests = recurse_manifest(mom, *subs, NULL, false);
				if (!mom->submanifests) {
					fprintf(stderr, "Error: Cannot load installed bundles\n");
					ret = ERECURSE_MANIFEST;
					goto out;
				}

				mom->files = files_from_bundles(mom->submanifests);
				mom->files = consolidate_files(mom->files);
				grabtime_stop(&times);
				full_mom = mom;
			}
			file = search_file_in_manifest(full_mom, file->filename);
		}

		rename_staged_file_to_final(file);
		// This is the second half of this process
		print_progress(complete, list_length * 2);
	}
	print_progress(list_length * 2, list_length * 2); /* Force out 100% complete */
	printf("\n");
	sync();
	grabtime_stop(&times);
	/* step 5: Run any scripts that are needed to complete update */
	run_scripts(false);

	ret = 0;
	print_time_stats(&times);

out:
	/* count how many of the requested bundles were actually installed, note that the
	 * to_install_bundles list could also have extra dependencies */
	iter = list_head(to_install_bundles);
	while (iter) {
		struct manifest *to_install_manifest;
		to_install_manifest = iter->data;
		iter = iter->next;
		if (string_in_list(to_install_manifest->component, bundles)) {
			bundles_installed++;
		}
	}

	/* print totals */
	if (ret && bundles_installed != 0) {
		/* if this point is reached with a nonzero return code and bundles_installed=0 it means that
		* while trying to install the bundles some error occurred which caused the whole installation
		* process to be aborted, so none of the bundles got installed. */
		bundles_failed = bundles_requested - already_installed;
	} else {
		bundles_failed = bundles_requested - bundles_installed - already_installed;
	}
	if (bundles_failed > 0) {
		ret = EBUNDLE_INSTALL;
		fprintf(stderr, "Failed to install %i of %i bundles\n", bundles_failed, bundles_requested - already_installed);
	} else if (bundles_installed) {
		fprintf(stderr, "Successfully installed %i bundle%s\n", bundles_installed, (bundles_installed > 1 ? "s" : ""));
	}
	if (already_installed) {
		ret = EBUNDLE_INSTALL;
		fprintf(stderr, "%i bundle%s already installed\n", already_installed, (already_installed > 1 ? "s were" : " was"));
	}

	if (to_install_files) {
		list_free_list(to_install_files);
	}
	if (to_install_bundles) {
		list_free_list_and_data(to_install_bundles, free_manifest_data);
	}
	return ret;
}

/* Bundle install one ore more bundles passed in bundles
 * param as a null terminated array of strings
 */
int install_bundles_frontend(char **bundles)
{
	int lock_fd;
	int ret = 0;
	int current_version;
	struct list *bundles_list = NULL;
	struct manifest *mom;
	struct list *subs = NULL;
	char *bundles_list_str = NULL;
	timelist times;
	bool mix_exists;

	/* initialize swupd and get current version from OS */
	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		fprintf(stderr, "Failed updater initialization, exiting now.\n");
		return ret;
	}

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		ret = ECURRENT_VERSION;
		goto clean_and_exit;
	}

	mix_exists = (check_mix_exists() & system_on_mix());

	mom = load_mom(current_version, false, mix_exists);
	if (!mom) {
		fprintf(stderr, "Cannot load official manifest MoM for version %i\n", current_version);
		ret = EMOM_NOTFOUND;
		goto clean_and_exit;
	}

	times = init_timelist();

	grabtime_start(&times, "Prepend bundles to list");
	for (; *bundles; ++bundles) {
		bundles_list = list_prepend_data(bundles_list, *bundles);
		if (*bundles) {
			char *tmp = bundles_list_str;
			if (bundles_list_str) {
				string_or_die(&bundles_list_str, "%s, %s", bundles_list_str, *bundles);
			} else {
				string_or_die(&bundles_list_str, "%s", *bundles);
			}
			free_string(&tmp);
		}
	}
	grabtime_stop(&times);
	grabtime_start(&times, "Install bundles");
	ret = install_bundles(bundles_list, &subs, current_version, mom);
	list_free_list(bundles_list);
	grabtime_stop(&times);

	print_time_stats(&times);

	free_manifest(mom);
clean_and_exit:
	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
		  "bundleadd",
		  "bundles=%s\n"
		  "current_version=%d\n"
		  "result=%d\n",
		  bundles_list_str,
		  current_version,
		  ret);

	free_string(&bundles_list_str);
	swupd_deinit(lock_fd, &subs);

	return ret;
}

static int lex_sort(const void *a, const void *b)
{
	const char *name1 = (char *)a;
	const char *name2 = (char *)b;
	return strcmp(name1, name2);
}

/*
 * This function will read the BUNDLES_DIR (by default
 * /usr/share/clear/bundles/), get the list of local bundles and print
 * them sorted.
 */
int list_local_bundles()
{
	char *path = NULL;
	DIR *dir;
	struct dirent *ent;
	struct list *bundles = NULL;
	struct list *item = NULL;

	string_or_die(&path, "%s/%s", path_prefix, BUNDLES_DIR);

	dir = opendir(path);
	if (!dir) {
		perror("couldn't open bundles directory");
		free_string(&path);
		return EXIT_FAILURE;
	}
	while ((ent = readdir(dir))) {
		if (ent->d_name[0] == '.') {
			continue;
		}
		/* Need to dup the strings as the directory
		 * may be bigger than the size of the I/O buffer */
		char *name = strdup(ent->d_name);
		if (!name) {
			abort();
		}
		bundles = list_append_data(bundles, name);
	}

	closedir(dir);

	item = bundles = list_sort(bundles, lex_sort);

	while (item) {
		printf("%s\n", (char *)item->data);
		free(item->data);
		item = item->next;
	}

	list_free_list(bundles);

	free_string(&path);

	return 0;
}
