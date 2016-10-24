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

	/* need update list in filename order to insure directories are
	 * created before their contents */
	updates = list_sort(updates, file_sort_filename);

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
	   amount of times */
	if (list_head(failed) != NULL && retries < MAX_TRIES) {
		increment_retries(&retries, &timeout);
		printf("Starting download retry #%d\n", retries);
		clean_curl_multi_queue();
		goto TRY_DOWNLOAD;
	}

	if (retries >= MAX_TRIES) {
		printf("ERROR: Could not download all files, aborting update\n");
		list_free_list(failed);
		return -1;
	}

	if (download_only) {
		return -1;
	}

	/*********** rootfs critical section starts ***************************
         NOTE: the next loop calls do_staging() which can remove files, starting a critical section
	       which ends after rename_all_files_to_final() succeeds
	 */

	/* from here onward we're doing real update work modifying "the disk" */

	/* starting at list_head in the filename alpha-sorted updates list
	 * means node directories are added before leaf files */
	printf("Staging file content\n");
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
			printf("File staging failed: %s\n", file->filename);
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

int add_included_manifests(struct manifest *mom, struct list **subs)
{
	struct list *subbed = NULL;
	struct list *iter;
	int ret;

	iter = list_head(*subs);
	while (iter) {
		subbed = list_prepend_data(subbed, ((struct sub *)iter->data)->component);
		iter = iter->next;
	}

	if (add_subscriptions(subbed, subs, mom->version, mom) >= 0) {
		ret = 0;
	} else {
		ret = -1;
	}
	list_free_list(subbed);

	return ret;
}

int main_update()
{
	int current_version = -1, server_version = -1;
	struct manifest *current_manifest = NULL, *server_manifest = NULL;
	struct list *updates = NULL;
	struct list *subs = NULL;
	int ret;
	int lock_fd;
	int retries = 0;
	int timeout = 10;

	srand(time(NULL));

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		/* being here means we already close log by a previously caught error */
		printf("Updater failed to initialize, exiting now.\n");
		return ret;
	}

	if (!check_network()) {
		printf("Error: Network issue, unable to proceed with update\n");
		ret = EXIT_FAILURE;
		goto clean_curl;
	}

	printf("Update started.\n");
	read_subscriptions_alt(&subs);

	/* Step 1: get versions */

	ret = check_versions(&current_version, &server_version, path_prefix);

	if (ret < 0) {
		goto clean_curl;
	}
	if (server_version <= current_version) {
		printf("Version on server (%i) is not newer than system version (%i)\n", server_version, current_version);
		ret = EXIT_SUCCESS;
		goto clean_curl;
	}

	printf("Preparing to update from %i to %i\n", current_version, server_version);

	/* Step 2: housekeeping */

	if (rm_staging_dir_contents("download")) {
		printf("Error cleaning download directory\n");
		ret = EXIT_FAILURE;
		goto clean_curl;
	}

load_current_manifests:
	/* Step 3: setup manifests */

	/* get the from/to MoM manifests */
	printf("Querying current manifest.\n");
	current_manifest = load_mom(current_version);
	if (!current_manifest) {
		/* TODO: possibly remove this as not getting a "from" manifest is not fatal
		 * - we just don't apply deltas */
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			printf("Retry #%d downloading from/to MoM Manifests\n", retries);
			goto load_current_manifests;
		}
		printf("Failure retrieving manifest from server\n");
		goto clean_exit;
	}

	/*  Reset the retries and timeout for subsequent download calls */
	retries = 0;
	timeout = 10;

load_server_manifests:
	printf("Querying server manifest.\n");

	server_manifest = load_mom(server_version);
	if (!server_manifest) {
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			printf("Retry #%d downloading server Manifests\n", retries);
			goto load_server_manifests;
		}
		printf("Failure retrieving manifest from server\n");
		printf("Unable to load manifest after retrying (config or network problem?)\n");
		goto clean_exit;
	}

	retries = 0;
	timeout = 10;

	ret = add_included_manifests(server_manifest, &subs);
	if (ret) {
		goto clean_exit;
	}

	subscription_versions_from_MoM(current_manifest, &subs, 1);
	subscription_versions_from_MoM(server_manifest, &subs, 0);

	link_submanifests(current_manifest, server_manifest);

	/* updating subscribed manifests is done as part of recurse_manifest */

	/* read the current collective of manifests that we are subscribed to */
	current_manifest->submanifests = recurse_manifest(current_manifest, subs, NULL);
	if (!current_manifest->submanifests) {
		ret = -1;
		printf("Cannot load current MoM sub-manifests, (%s), exiting\n", strerror(errno));
		goto clean_exit;
	}

	/* consolidate the current collective manifests down into one in memory */
	current_manifest->files = files_from_bundles(current_manifest->submanifests);

	current_manifest->files = consolidate_files(current_manifest->files);

	/* read the new collective of manifests that we are subscribed to */
	server_manifest->submanifests = recurse_manifest(server_manifest, subs, NULL);
	if (!server_manifest->submanifests) {
		ret = -1;
		printf("Error: Cannot load server MoM sub-manifests, (%s), exiting\n", strerror(errno));
		goto clean_exit;
	}

	/* consolidate the new collective manifests down into one in memory */
	server_manifest->files = files_from_bundles(server_manifest->submanifests);

	server_manifest->files = consolidate_files(server_manifest->files);

	/* prepare for an update process based on comparing two in memory manifests */
	link_manifests(current_manifest, server_manifest);
#if 0
	debug_write_manifest(current_manifest, "debug_manifest_current.txt");
	debug_write_manifest(server_manifest, "debug_manifest_server.txt");
#endif
	/* Step 4: check disk state before attempting update */

	run_preupdate_scripts(server_manifest);

download_packs:
	/* Step 5: get the packs and untar */
	ret = download_subscribed_packs(subs, false);
	if (ret) {
		// packs don't always exist, tolerate that but not ENONET
		if (retries < MAX_TRIES) {
			increment_retries(&retries, &timeout);
			printf("Retry #%d downloading packs\n", retries);
			goto download_packs;
		}
		printf("No network, or server unavailable for pack downloads\n");
		goto clean_exit;
	}

	/* Step 6: some more housekeeping */

	/* TODO: consider trying to do less sorting of manifests */

	updates = create_update_list(current_manifest, server_manifest);

	link_renames(updates, current_manifest); /* TODO: Have special lists for candidate and renames */

	print_statistics(current_version, server_version);

	/* Step 7: apply the update */

	ret = update_loop(updates, server_manifest);
	if (ret == 0) {
		ret = update_device_latest_version(server_version);
		printf("Update was applied.\n");
	}

	delete_motd();

	/* Run any scripts that are needed to complete update */
	run_scripts();

clean_exit:
	list_free_list(updates);
	free_manifest(current_manifest);
	free_manifest(server_manifest);

clean_curl:
	swupd_deinit(lock_fd, &subs);

	if ((current_version < server_version) && (ret == 0)) {
		printf("Update successful. System updated from version %d to version %d\n",
		       current_version, server_version);
	} else if (ret == 0) {
		printf("Update complete. System already up-to-date at version %d\n", current_version);
	}

	return ret;
}
