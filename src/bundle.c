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

#include "config.h"
#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

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
	int lock_fd;
	int ret;

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		fprintf(stderr, "Error: Failed updater initialization. Exiting now\n");
		return ret;
	}

	if (!check_network()) {
		fprintf(stderr, "Error: Network issue, unable to download manifest\n");
		v_lockfile(lock_fd);
		return ENOSWUPDSERVER;
	}

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		v_lockfile(lock_fd);
		return ECURRENT_VERSION;
	}

	swupd_curl_set_current_version(current_version);

	MoM = load_mom(current_version, false);
	if (!MoM) {
		v_lockfile(lock_fd);
		return EMOM_NOTFOUND;
	}

	list = MoM->manifests;
	while (list) {
		file = list->data;
		list = list->next;
		printf("%s\n", file->filename);
	}

	free_manifest(MoM);
	v_lockfile(lock_fd);
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

	swupd_curl_set_current_version(version);
	mom = load_mom(version, false);
	if (!mom) {
		return EMOM_NOTFOUND;
	}

	sub_list = recurse_manifest(mom, subs, bundle_name);
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

	free(filename);
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
void required_by(struct list **reqd_by, const char *bundle_name, struct manifest *mom, int rl)
{
	struct list *b, *i;
	// track recursion level for indentation
	rl++;

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
				indent = (rl - 1) * 4;
				if (rl == 1) {
					string_or_die(&bundle_str, "%*s* %s\n", indent + 2, "", bundle->component);
				} else {
					string_or_die(&bundle_str, "%*s|-- %s\n", indent, "", bundle->component);
				}

				*reqd_by = list_append_data(*reqd_by, bundle_str);
				required_by(reqd_by, bundle->component, mom, rl);
			}
		}
	}
}

/*  This function is a fresh new implementation for a bundle
 *  remove without being tied to verify loop, this means
 *  improved speed and space as well as more roubustness and
 *  flexibility. What it does is basically:
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
int remove_bundle(const char *bundle_name)
{
	int lock_fd;
	int ret = 0;
	int current_version = CURRENT_OS_VERSION;
	struct manifest *current_mom, *bundle_manifest = NULL;
	struct list *subs = NULL;

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		fprintf(stderr, "Failed updater initialization, exiting now.\n");
		return ret;
	}

	/* os-core bundle not allowed to be removed...
	 * although this is going to be caught later because of all files
	 * being marked as 'duplicated' and note removing anything
	 * anyways, better catch here and return success, no extra work to be done.
	 */
	if (strcmp(bundle_name, "os-core") == 0) {
		ret = EBUNDLE_NOT_TRACKED;
		goto out_free_curl;
	}

	if (!is_tracked_bundle(bundle_name)) {
		ret = EBUNDLE_NOT_TRACKED;
		fprintf(stderr, "Warning: Bundle \"%s\" does not seem to be installed\n", bundle_name);
		goto out_free_curl;
	}

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		ret = ECURRENT_VERSION;
		goto out_free_curl;
	}

	swupd_curl_set_current_version(current_version);

	current_mom = load_mom(current_version, false);
	if (!current_mom) {
		fprintf(stderr, "Unable to download/verify %d Manifest.MoM\n", current_version);
		ret = EMOM_NOTFOUND;
		goto out_free_curl;
	}

	if (!search_bundle_in_manifest(current_mom, bundle_name)) {
		fprintf(stderr, "Bundle name is invalid, aborting removal\n");
		ret = EBUNDLE_REMOVE;
		goto out_free_mom;
	}

	/* load all tracked bundles into memory */
	read_subscriptions_alt(&subs);
	/* now popout the one to be removed */
	ret = unload_tracked_bundle(bundle_name, &subs);
	if (ret != 0) {
		goto out_free_mom;
	}

	set_subscription_versions(current_mom, NULL, &subs);

	/* load all submanifest minus the one to be removed */
	current_mom->submanifests = recurse_manifest(current_mom, subs, NULL);
	if (!current_mom->submanifests) {
		fprintf(stderr, "Error: Cannot load MoM sub-manifests\n");
		ret = ERECURSE_MANIFEST;
		goto out_free_mom;
	}

	struct list *reqd_by = NULL;
	required_by(&reqd_by, bundle_name, current_mom, 0);
	if (reqd_by != NULL) {
		struct list *iter;
		char *bundle;
		iter = list_head(reqd_by);
		fprintf(stderr, "Error: bundle requested to be removed is required by the following bundles:\n");
		fprintf(stderr, "format:\n");
		fprintf(stderr, " # * is-included-by\n #   |-- is-included-by\n # * is-also-included-by\n # ...\n\n");
		while (iter) {
			bundle = iter->data;
			iter = iter->next;
			fprintf(stderr, "%s", bundle);
		}

		list_free_list(reqd_by);
		ret = EBUNDLE_REMOVE;
		goto out_free_mom;
	}

	current_mom->files = files_from_bundles(current_mom->submanifests);

	current_mom->files = consolidate_files(current_mom->files);

	/* Now that we have the consolidated list of all files, load bundle to be removed submanifest*/
	ret = load_bundle_manifest(bundle_name, subs, current_version, &bundle_manifest);
	if (ret != 0 || !bundle_manifest) {
		fprintf(stderr, "Error: Cannot load %s sub-manifest (ret = %d)\n", bundle_name, ret);
		goto out_free_mom;
	}

	/* deduplication needs file list sorted by filename, do so */
	bundle_manifest->files = list_sort(bundle_manifest->files, file_sort_filename);
	deduplicate_files_from_manifest(&bundle_manifest, current_mom);

	fprintf(stderr, "Deleting bundle files...\n");
	remove_files_in_manifest_from_fs(bundle_manifest);

	fprintf(stderr, "Untracking bundle from system...\n");
	rm_bundle_file(bundle_name);

	fprintf(stderr, "Success: Bundle removed\n");

	free_manifest(bundle_manifest);
out_free_mom:
	free_manifest(current_mom);
out_free_curl:
	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
		  "bundleremove",
		  "bundle=%s\n"
		  "current_version=%d\n"
		  "result=%d\n",
		  bundle_name,
		  current_version,
		  ret);
	if (ret) {
		fprintf(stderr, "Error: Bundle remove failed\n");
	}

	swupd_deinit(lock_fd, &subs);

	return ret;
}

/* bitmapped return
   1 error happened
   2 new subscriptions
   4 bad name given
*/
#define add_sub_ERR 1
#define add_sub_NEW 2
#define add_sub_BADNAME 4
int add_subscriptions(struct list *bundles, struct list **subs, int current_version, struct manifest *mom, int recursion)
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
			fprintf(stderr, "%s bundle name is invalid, skipping it...\n", bundle);
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
			if (retries < MAX_TRIES) {
				increment_retries(&retries, &timeout);
				goto retry_manifest_download;
			}
			fprintf(stderr, "Unable to download manifest %s version %d, exiting now\n", bundle, file->last_change);
			ret |= add_sub_ERR;
			goto out;
		}

		if (manifest->includes) {
			int r = add_subscriptions(manifest->includes, subs, current_version, mom, recursion + 1);
			if (r & add_sub_ERR) {
				free_manifest(manifest);
				goto out;
			}
			ret |= r; /* merge in recursive call results */
		}
		free_manifest(manifest);

		if (is_tracked_bundle(bundle)) {
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
	struct file *file;
	struct list *iter;
	struct list *to_install_bundles = NULL;
	struct list *to_install_files = NULL;
	timelist times;

	times = init_timelist();

	/* step 1: check bundle args are valid if so populate subs struct */
	grabtime_start(&times, "Add bundles and recurse");
	ret = add_subscriptions(bundles, subs, current_version, mom, 0);

	if (ret != add_sub_NEW) {
		/* something went wrong, print a message and exit */
		const char *m;
		if (ret == 0) { /* no bad names, no new packages */
			fprintf(stderr, "nothing to add, exiting now\n");
			goto out;
		}
		if (ret & add_sub_ERR) {
			m = "Processing error";
		} else if (ret & add_sub_BADNAME) {
			m = "Bad bundle name(s) detected";
		} else {
			m = "Unknown error";
		}
		ret = EBUNDLE_INSTALL;
		fprintf(stderr, "Error: %s - Aborting\n", m); /* Should be to stderr */
		goto out;
	}

	set_subscription_versions(mom, NULL, subs);

	to_install_bundles = recurse_manifest(mom, *subs, NULL);
	if (!to_install_bundles) {
		fprintf(stderr, "Error: Cannot load to install bundles\n");
		ret = ERECURSE_MANIFEST;
		goto out;
	}
	grabtime_stop(&times);

	grabtime_start(&times, "Consolidate files from bundles");
	to_install_files = files_from_bundles(to_install_bundles);
	to_install_files = consolidate_files(to_install_files);
	grabtime_stop(&times);

	/* step 2: download neccessary packs */
	grabtime_start(&times, "Download packs");
	(void)rm_staging_dir_contents("download");

download_subscribed_packs:
	if (download_subscribed_packs(*subs, true)) {
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			printf("\nRetry #%d downloading subscribed packs\n", retries);
			goto download_subscribed_packs;
		}
	}
	grabtime_stop(&times);

	/* step 3: Add tracked bundles */
	grabtime_start(&times, "Add tracked bundles");
	read_subscriptions_alt(subs);
	set_subscription_versions(mom, NULL, subs);
	mom->submanifests = recurse_manifest(mom, *subs, NULL);
	if (!mom->submanifests) {
		fprintf(stderr, "Error: Cannot load installed bundles\n");
		ret = ERECURSE_MANIFEST;
		goto out;
	}

	mom->files = files_from_bundles(mom->submanifests);
	mom->files = consolidate_files(mom->files);
	grabtime_stop(&times);

	/* step 4: Install all bundle(s) files into the fs */
	grabtime_start(&times, "Installing bundle(s) files onto filesystem");
	fprintf(stderr, "Installing bundle(s) files...\n");
	unsigned int list_length = list_len(to_install_files);
	unsigned int complete = 0;
	iter = list_head(to_install_files);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		if (file->is_deleted || file->do_not_update || ignore(file)) {
			continue;
		}

		ret = do_staging(file, mom);
		if (ret) {
			ret = verify_fix_path(file->filename, mom);
		}
		if (ret) {
			ret = EBUNDLE_INSTALL;
			goto out;
		}

		/* two loops are necessary, first to stage, then to rename. Total is
		 * list_length * 2 */
		print_progress(++complete, list_length * 2);
	}

	iter = list_head(to_install_files);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		if (file->is_deleted || file->do_not_update || ignore(file)) {
			continue;
		}

		/* This was staged by verify_fix_path */
		if (!file->staging) {
			file = search_file_in_manifest(mom, file->filename);
		}

		rename_staged_file_to_final(file);
		// This is the second half of this process
		print_progress(++complete, list_length * 2);
	}

	printf("\n");
	sync();
	grabtime_stop(&times);
	/* step 5: Run any scripts that are needed to complete update */
	run_scripts(false);

	ret = 0;
	fprintf(stderr, "Bundle(s) installation done.\n");
	print_time_stats(&times);

out:
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

	swupd_curl_set_current_version(current_version);

	mom = load_mom(current_version, false);
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
			free(tmp);
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

	swupd_deinit(lock_fd, &subs);

	return ret;
}

/*
 * This function will read /usr/share/clear/bundles/
 * in order to get the bundle names, these will be
 * saved in a list.
 */
void read_local_bundles(struct list **list_bundles)
{
	char *path = NULL;
	DIR *dir;
	struct dirent *ent;

	string_or_die(&path, "%s/%s", path_prefix, BUNDLES_DIR);

	dir = opendir(path);
	if (dir) {
		while ((ent = readdir(dir))) {
			if ((strcmp(ent->d_name, ".") == 0) ||
			    (strcmp(ent->d_name, "..") == 0)) {
				continue;
			}

			*list_bundles = list_append_data(*list_bundles, ent->d_name);
		}

		closedir(dir);
	}

	free(path);
}
