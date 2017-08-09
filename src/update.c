/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2016 Intel Corporation.
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
 *
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

int nonpack;

void increment_retries(int *retries, int *timeout)
{
	(*retries)++;
	sleep(*timeout);
	*timeout *= 2;
}

static void try_delta_loop(struct list *updates)
{
	struct list *iter;
	struct file *file;

	iter = list_head(updates);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		if (!file->is_file) {
			continue;
		}

		try_delta(file);
	}
}

static struct list *full_download_loop(struct list *updates, int isfailed)
{
	struct list *iter;
	struct file *file;

	iter = list_head(updates);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		if (file->is_deleted) {
			continue;
		}

		full_download(file);
	}

	if (isfailed) {
		list_free_list(updates);
	}

	return end_full_download();
}

static int update_loop(struct list *updates, struct manifest *server_manifest)
{
	int ret;
	struct file *file;
	struct list *iter;
	struct list *failed = NULL;
	int err;
	int retries = 0;  /* We only want to go through the download loop once */
	int timeout = 10; /* Amount of seconds for first download retry */

TRY_DOWNLOAD:
	err = start_full_download(true);
	if (err != 0) {
		return err;
	}

	if (failed != NULL) {
		try_delta_loop(failed);
		failed = full_download_loop(failed, 1);
	} else {
		try_delta_loop(updates);
		failed = full_download_loop(updates, 0);
	}

#if 0
	if (rm_staging_dir_contents("download")) {
		return -1;
	}
#endif

	/* Set retries only if failed downloads exist, and only retry a fixed
	   amount of &times */
	if (list_head(failed) != NULL && retries < MAX_TRIES) {
		increment_retries(&retries, &timeout);
		fprintf(stderr, "Starting download retry #%d\n", retries);
		clean_curl_multi_queue();
		goto TRY_DOWNLOAD;
	}

	if (retries >= MAX_TRIES) {
		fprintf(stderr, "ERROR: Could not download all files, aborting update\n");
		list_free_list(failed);
		return -1;
	}

	if (download_only) {
		return 0;
	}

	/*********** rootfs critical section starts ***************************
         NOTE: the next loop calls do_staging() which can remove files, starting a critical section
	       which ends after rename_all_files_to_final() succeeds
	 */

	/* from here onward we're doing real update work modifying "the disk" */

	/* starting at list_head in the filename alpha-sorted updates list
	 * means node directories are added before leaf files */
	fprintf(stderr, "Staging file content\n");
	iter = list_head(updates);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		if (file->do_not_update || file->is_deleted) {
			continue;
		}

		/* for each file: fdatasync to persist changed content over reboot, or maybe a global sync */
		/* for each file: check hash value; on mismatch delete and queue full download */
		/* todo: hash check */

		ret = do_staging(file, server_manifest);
		if (ret < 0) {
			fprintf(stderr, "File staging failed: %s\n", file->filename);
			return ret;
		}
	}

	/* check policy, and if policy says, "ask", ask the user at this point */
	/* check for reboot need - if needed, wait for reboot */

	/* sync */
	sync();

	/* rename to apply update */
	ret = rename_all_files_to_final(updates);
	if (ret != 0) {
		return ret;
	}

	/* TODO: do we need to optimize directory-permission-only changes (directories
	 *       are now sent as tar's so permissions are handled correctly, even
	 *       if less than efficiently)? */

	sync();

	/* NOTE: critical section starts when update_loop() calls do_staging() */
	/*********** critical section ends *************************************/

	return ret;
}

int add_included_manifests(struct manifest *mom, int current, struct list **subs)
{
	struct list *subbed = NULL;
	struct list *iter;
	int ret;

	iter = list_head(*subs);
	while (iter) {
		subbed = list_prepend_data(subbed, ((struct sub *)iter->data)->component);
		iter = iter->next;
	}

	/* Pass the current version here, not the new, otherwise we will never
	 * hit the Manifest delta path. */
	if (add_subscriptions(subbed, subs, current, mom, 0) >= 0) {
		ret = 0;
	} else {
		ret = -1;
	}
	list_free_list(subbed);

	return ret;
}

static int re_exec_update(bool versions_match)
{
	if (!versions_match) {
		fprintf(stderr, "ERROR: Inconsistency between version files, exiting now.\n");
		return -1;
	}

	if (!swupd_cmd) {
		fprintf(stderr, "ERROR: Unable to determine re-update command, exiting now.\n");
		return -1;
	}

	/* Run the swupd_cmd saved from main */
	return system(swupd_cmd);
}

int main_update()
{
	int current_version = -1, server_version = -1;
	struct manifest *current_manifest = NULL, *server_manifest = NULL;
	struct list *updates = NULL;
	struct list *current_subs = NULL;
	struct list *latest_subs = NULL;
	int ret;
	int lock_fd;
	int retries = 0;
	int timeout = 10;
	struct timespec ts_start, ts_stop; // For main swupd update time
	timelist times;
	double delta;
	bool re_update = false;
	bool versions_match = false;

	srand(time(NULL));

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		/* being here means we already close log by a previously caught error */
		fprintf(stderr, "Updater failed to initialize, exiting now.\n");
		return ret;
	}

	times = init_timelist();

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);
	grabtime_start(&times, "Main Update");

	if (!check_network()) {
		fprintf(stderr, "Error: Network issue, unable to proceed with update\n");
		ret = ENOSWUPDSERVER;
		goto clean_curl;
	}

	fprintf(stderr, "Update started.\n");

	grabtime_start(&times, "Update Step 1: get versions");

	read_subscriptions_alt(&current_subs);

	/* Step 1: get versions */

	ret = check_versions(&current_version, &server_version, path_prefix);

	if (ret < 0) {
		ret = EXIT_FAILURE;
		goto clean_curl;
	}
	if (server_version <= current_version) {
		fprintf(stderr, "Version on server (%i) is not newer than system version (%i)\n", server_version, current_version);
		ret = EXIT_SUCCESS;
		goto clean_curl;
	}

	fprintf(stderr, "Preparing to update from %i to %i\n", current_version, server_version);

	/* Step 2: housekeeping */

	if (rm_staging_dir_contents("download")) {
		fprintf(stderr, "Error cleaning download directory\n");
		ret = EXIT_FAILURE;
		goto clean_curl;
	}
	grabtime_stop(&times);
	grabtime_stop(&times); // Close step 1
	grabtime_start(&times, "Load Manifests:");
load_current_mom:
	/* Step 3: setup manifests */

	/* get the from/to MoM manifests */
	current_manifest = load_mom(current_version, false);
	if (!current_manifest) {
		/* TODO: possibly remove this as not getting a "from" manifest is not fatal
		 * - we just don't apply deltas */
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			fprintf(stderr, "Retry #%d downloading from/to MoM Manifests\n", retries);
			goto load_current_mom;
		}
		fprintf(stderr, "Failure retrieving manifest from server\n");
		ret = EMOM_NOTFOUND;
		goto clean_exit;
	}

	/*  Reset the retries and timeout for subsequent download calls */
	retries = 0;
	timeout = 10;

load_server_mom:
	grabtime_stop(&times); // Close step 2
	grabtime_start(&times, "Recurse and Consolidate Manifests");
	server_manifest = load_mom(server_version, true);
	if (!server_manifest) {
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			fprintf(stderr, "Retry #%d downloading server Manifests\n", retries);
			goto load_server_mom;
		}
		fprintf(stderr, "Failure retrieving manifest from server\n");
		fprintf(stderr, "Unable to load manifest after retrying (config or network problem?)\n");
		ret = EMOM_NOTFOUND;
		goto clean_exit;
	}

	retries = 0;
	timeout = 10;

load_current_submanifests:
	/* Read the current collective of manifests that we are subscribed to.
	 * First load up the old (current) manifests. Statedir could have been cleared
	 * or corrupt, so don't assume things are already there. Updating subscribed
	 * manifests is done as part of recurse_manifest */
	current_manifest->submanifests = recurse_manifest(current_manifest, current_subs, NULL);
	if (!current_manifest->submanifests) {
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			fprintf(stderr, "Retry #%d downloading current sub-manifests\n", retries);
			goto load_current_submanifests;
		}
		ret = ERECURSE_MANIFEST;
		printf("Cannot load current MoM sub-manifests, exiting\n");
		goto clean_exit;
	}
	retries = 0;
	timeout = 10;

	/* consolidate the current collective manifests down into one in memory */
	current_manifest->files = files_from_bundles(current_manifest->submanifests);

	current_manifest->files = consolidate_files(current_manifest->files);

	latest_subs = list_clone(current_subs);
	set_subscription_versions(server_manifest, current_manifest, &latest_subs);
	link_submanifests(current_manifest, server_manifest, current_subs, latest_subs, false);
	/* The new subscription is seeded from the list of currently installed bundles
	 * This calls add_subscriptions which recurses for new includes */
	grabtime_start(&times, "Add Included Manifests");
	ret = add_included_manifests(server_manifest, current_version, &latest_subs);
	grabtime_stop(&times);
	if (ret) {
		ret = EMANIFEST_LOAD;
		goto clean_exit;
	}

load_server_submanifests:
	/* read the new collective of manifests that we are subscribed to in the new MoM */
	server_manifest->submanifests = recurse_manifest(server_manifest, latest_subs, NULL);
	if (!server_manifest->submanifests) {
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			printf("Retry #%d downloading server sub-manifests\n", retries);
			goto load_server_submanifests;
		}
		ret = ERECURSE_MANIFEST;
		printf("Error: Cannot load server MoM sub-manifests, exiting\n");
		goto clean_exit;
	}
	retries = 0;
	timeout = 10;
	/* consolidate the new collective manifests down into one in memory */
	server_manifest->files = files_from_bundles(server_manifest->submanifests);

	server_manifest->files = consolidate_files(server_manifest->files);

	set_subscription_versions(server_manifest, current_manifest, &latest_subs);
	link_submanifests(current_manifest, server_manifest, current_subs, latest_subs, true);

	/* prepare for an update process based on comparing two in memory manifests */
	link_manifests(current_manifest, server_manifest);
#if 0
	debug_write_manifest(current_manifest, "debug_manifest_current.txt");
	debug_write_manifest(server_manifest, "debug_manifest_server.txt");
#endif
	grabtime_stop(&times);
	/* Step 4: check disk state before attempting update */
	grabtime_start(&times, "Pre-Update Scripts");
	run_preupdate_scripts(server_manifest);
	grabtime_stop(&times);

	grabtime_start(&times, "Download Packs");

download_packs:
	/* Step 5: get the packs and untar */
	ret = download_subscribed_packs(latest_subs, false);
	if (ret) {
		// packs don't always exist, tolerate that but not ENONET
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			printf("Retry #%d downloading packs\n", retries);
			goto download_packs;
		}
		printf("Pack download failed\n");
		ret = EDOWNLOADPACKS;
		goto clean_exit;
	}
	grabtime_stop(&times);
	grabtime_start(&times, "Create Update List");
	/* Step 6: some more housekeeping */
	/* TODO: consider trying to do less sorting of manifests */

	updates = create_update_list(current_manifest, server_manifest);

	link_renames(updates, current_manifest); /* TODO: Have special lists for candidate and renames */

	print_statistics(current_version, server_version);
	grabtime_stop(&times);
	/* Step 7: apply the update */

	/*
	 * need update list in filename order to insure directories are
	 * created before their contents
	 */
	grabtime_start(&times, "Update Loop");
	updates = list_sort(updates, file_sort_filename);

	ret = update_loop(updates, server_manifest);
	if (ret == 0 && !download_only) {
		/* Failure to write the version file in the state directory
		 * should not affect exit status. */
		(void)update_device_latest_version(server_version);
		printf("Update was applied.\n");
	} else if (ret < 0) {
		// Ensure a positive exit status for the main program.
		ret = -ret;
	}

	delete_motd();
	grabtime_stop(&times);
	/* Run any scripts that are needed to complete update */
	grabtime_start(&times, "Run Scripts");

	/* Determine if another update is needed so the scripts block */
	if (string_in_list("update", post_update_actions) && (ret == 0)) {
		re_update = true;
	}

	run_scripts(re_update);
	grabtime_stop(&times);

clean_exit:
	list_free_list(updates);
	free_manifest(current_manifest);
	free_manifest(server_manifest);

clean_curl:
	grabtime_stop(&times);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts_stop);
	delta = ts_stop.tv_sec - ts_start.tv_sec + ts_stop.tv_nsec / 1000000000.0 - ts_start.tv_nsec / 1000000000.0;
	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
		  "update",
		  "current_version=%d\n"
		  "server_version=%d\n"
		  "result=%d\n"
		  "time=%5.1f\n",
		  current_version,
		  server_version,
		  ret,
		  delta);

	if (server_version > current_version) {
		printf("Update took %0.1f seconds\n", delta);
	}
	print_time_stats(&times);

	/* version_files_match must be done before swupd_deinit to use globals */
	if (re_update) {
		versions_match = version_files_consistent();
	}
	swupd_deinit(lock_fd, &latest_subs);

	if (!download_only) {
		if ((current_version < server_version) && (ret == 0)) {
			printf("Update successful. System updated from version %d to version %d\n",
			       current_version, server_version);
		} else if (ret == 0) {
			printf("Update complete. System already up-to-date at version %d\n", current_version);
		}
	}

	if (nonpack > 0) {
		printf("%i files were not in a pack\n", nonpack);
	}

	if (re_update && ret == 0) {
		ret = re_exec_update(versions_match);
	}

	/* free swupd_cmd now that is no longer needed */
	free(swupd_cmd);
	swupd_cmd = NULL;

	return ret;
}
