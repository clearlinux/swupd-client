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
#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

static int requested_version = -1;

int nonpack;

void increment_retries(int *retries, int *timeout)
{
	(*retries)++;
	sleep(*timeout);
	*timeout *= 2;
}

/* This loads the upstream Clear Manifest.Full and local
 * Manifest.full, and then checks that there are no conflicts between
 * the files they both include */
static int check_manifests_uniqueness(int clrver, int mixver)
{
	struct manifest *clear = load_manifest_full(clrver, false);
	struct manifest *mixer = load_manifest_full(mixver, true);
	if (!clear || !mixer) {
		fprintf(stderr, "ERROR: Could not load full manifests\n");
		return -1;
	}

	struct file **clearfull = manifest_files_to_array(clear);
	struct file **mixerfull = manifest_files_to_array(mixer);

	if (clearfull == NULL || mixerfull == NULL) {
		fprintf(stderr, "Could not convert full manifest to array\n");
		return -1;
	}

	int ret = enforce_compliant_manifest(mixerfull, clearfull, mixer->filecount, clear->filecount);

	free_manifest_array(clearfull);
	free_manifest_array(mixerfull);
	free_manifest(clear);
	free_manifest(mixer);

	return ret;
}

static int update_loop(struct list *updates, struct manifest *server_manifest)
{
	int ret;
	struct file *file;
	struct list *iter;

	ret = download_fullfiles(updates, &nonpack);
	if (ret) {
		fprintf(stderr, "ERROR: Could not download all files, aborting update\n");
		return ret;
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
	printf("Applying update\n");
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

	/* Pass the current version here, not the new, otherwise we will never
	 * hit the Manifest delta path. */
	ret = add_subscriptions(subbed, subs, mom, false, 0);
	if (ret & (add_sub_ERR | add_sub_BADNAME)) {
		ret = -ret;
	} else {
		ret = 0;
	}
	list_free_list(subbed);

	return ret;
}

static int re_exec_update(bool versions_match)
{
	if (!versions_match) {
		fprintf(stderr, "ERROR: Inconsistency between version files, exiting now.\n");
		return 1;
	}

	if (!swupd_cmd) {
		fprintf(stderr, "ERROR: Unable to determine re-update command, exiting now.\n");
		return 1;
	}

	/* Run the swupd_cmd saved from main */
	if (system(swupd_cmd) != 0) {
		return 1;
	}

	return 0;
}

static bool need_new_upstream(int server)
{
	if (!access(MIX_DIR "upstreamversion", R_OK)) {
		int version = read_mix_version_file(MIX_DIR "upstreamversion", path_prefix);
		if (version < server) {
			return true;
		}
	}
	return false;
}

static int main_update()
{
	int current_version = -1, server_version = -1;
	int mix_current_version = -1, mix_server_version = -1;
	struct manifest *current_manifest = NULL, *server_manifest = NULL;
	struct list *updates = NULL;
	struct list *current_subs = NULL;
	struct list *latest_subs = NULL;
	int ret;
	int retries = 0;
	int timeout = 10;
	struct timespec ts_start, ts_stop; // For main swupd update time
	double delta;
	bool mix_exists;
	bool re_update = false;
	bool versions_match = false;

	srand(time(NULL));

	ret = swupd_init();
	if (ret != 0) {
		/* being here means we already close log by a previously caught error */
		fprintf(stderr, "Updater failed to initialize, exiting now.\n");
		return ret;
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);
	timelist_timer_start(global_times, "Main Update");

	mix_exists = check_mix_exists();

	fprintf(stderr, "Update started.\n");

	timelist_timer_start(global_times, "Update Step 1: get versions");

	read_subscriptions(&current_subs);

	handle_mirror_if_stale();

/* Step 1: get versions */
version_check:
	ret = check_versions(&current_version, &server_version, requested_version, path_prefix);

	if (ret < 0) {
		ret = EXIT_FAILURE;
		goto clean_curl;
	}

	if (mix_exists) {
		check_mix_versions(&mix_current_version, &mix_server_version, path_prefix);
		if (mix_current_version == -1 || mix_server_version == -1) {
			ret = EXIT_FAILURE;
			goto clean_curl;
		}
		/* Check if a new upstream version is available so we can update to it still */
		if (need_new_upstream(server_version)) {
			printf("NEW CLEAR AVAILABLE %d\n", server_version);
			/* Update the upstreamversion that will be used to generate the new mix content */
			FILE *verfile = fopen(MIX_DIR "upstreamversion", "w+");
			if (!verfile) {
				fprintf(stderr, "ERROR: fopen() %s/upstreamversion returned %s\n", MIX_DIR, strerror(errno));
			} else {
				fprintf(verfile, "%d", server_version);
				fclose(verfile);
			}

			if (run_command("/usr/bin/mixin", "build", NULL) != 0) {
				fprintf(stderr, "ERROR: Could not execute mixin\n");
				ret = EXIT_FAILURE;
				goto clean_curl;
			}

			// new mix version
			check_mix_versions(&mix_current_version, &mix_server_version, path_prefix);
			ret = check_manifests_uniqueness(server_version, mix_server_version);
			if (ret > 0) {
				printf("\n\t!! %i collisions were found between mix and upstream, please re-create mix !!\n", ret);
				if (!allow_mix_collisions) {
					ret = EXIT_FAILURE;
					goto clean_curl;
				}
			} else if (ret < 0) {
				ret = EXIT_FAILURE;
				goto clean_curl;
			}

			goto version_check;
		}
		current_version = mix_current_version;
		server_version = mix_server_version;
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
	timelist_timer_stop(global_times);
	timelist_timer_stop(global_times); // Close step 1
	timelist_timer_start(global_times, "Load Manifests:");
	int manifest_err;
load_current_mom:
	/* Step 3: setup manifests */

	/* get the from/to MoM manifests */
	if (system_on_mix()) {
		current_manifest = load_mom(current_version, false, mix_exists, &manifest_err);
	} else {
		current_manifest = load_mom(current_version, false, false, &manifest_err);
	}
	if (!current_manifest) {
		/* TODO: possibly remove this as not getting a "from" manifest is not fatal
		 * - we just don't apply deltas */
		if (manifest_err == -ENET404 || manifest_err == -ENOENT) {
			fprintf(stderr, "The current MoM manifest was not found\n");
		} else if (retries < MAX_TRIES && manifest_err != -EIO) {
			increment_retries(&retries, &timeout);
			fprintf(stderr, "Retry #%d downloading current MoM manifest\n", retries);
			goto load_current_mom;
		}
		ret = EMOM_LOAD;
		goto clean_exit;
	}

	/*  Reset the retries and timeout for subsequent download calls */
	retries = 0;
	timeout = 10;

load_server_mom:
	timelist_timer_stop(global_times); // Close step 2
	timelist_timer_start(global_times, "Recurse and Consolidate Manifests");
	server_manifest = load_mom(server_version, true, mix_exists, &manifest_err);
	if (!server_manifest) {
		if (manifest_err == -ENET404 || manifest_err == -ENOENT) {
			fprintf(stderr, "The server MoM manifest was not found\n");
			fprintf(stderr, "Version %d not available\n", server_version);
		} else if (retries < MAX_TRIES && manifest_err != -EIO) {
			increment_retries(&retries, &timeout);
			fprintf(stderr, "Retry #%d downloading server MoM manifest\n", retries);
			goto load_server_mom;
		}
		ret = EMOM_LOAD;
		goto clean_exit;
	}

	retries = 0;
	timeout = 10;

load_current_submanifests:
	/* Read the current collective of manifests that we are subscribed to.
	 * First load up the old (current) manifests. Statedir could have been cleared
	 * or corrupt, so don't assume things are already there. Updating subscribed
	 * manifests is done as part of recurse_manifest */
	current_manifest->submanifests = recurse_manifest(current_manifest, current_subs, NULL, false, &manifest_err);
	if (!current_manifest->submanifests) {
		if (retries < MAX_TRIES && manifest_err != -EIO) {
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
	timelist_timer_start(global_times, "Add Included Manifests");
	ret = add_included_manifests(server_manifest, &latest_subs);
	timelist_timer_stop(global_times);
	if (ret) {
		if (ret == -add_sub_BADNAME) {
			/* this means a bundle(s) was removed in a future version */
			printf("WARNING: One or more installed bundles are no longer available at version %d.\n",
			       server_version);
		} else {
			ret = ERECURSE_MANIFEST;
			goto clean_exit;
		}
	}

load_server_submanifests:
	/* read the new collective of manifests that we are subscribed to in the new MoM */
	server_manifest->submanifests = recurse_manifest(server_manifest, latest_subs, NULL, false, &manifest_err);
	if (!server_manifest->submanifests) {
		if (retries < MAX_TRIES && manifest_err != -EIO) {
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
	timelist_timer_stop(global_times);
	/* Step 4: check disk state before attempting update */
	timelist_timer_start(global_times, "Pre-Update Scripts");
	run_preupdate_scripts(server_manifest);
	timelist_timer_stop(global_times);

	/* Step 5: get the packs and untar */
	timelist_timer_start(global_times, "Download Packs");
	download_subscribed_packs(latest_subs, server_manifest, false);
	timelist_timer_stop(global_times);

	timelist_timer_start(global_times, "Apply deltas");
	apply_deltas(current_manifest);
	timelist_timer_stop(global_times);

	/* Step 6: some more housekeeping */
	/* TODO: consider trying to do less sorting of manifests */

	timelist_timer_start(global_times, "Create Update List");

	updates = create_update_list(server_manifest);

	print_statistics(current_version, server_version);
	timelist_timer_stop(global_times);
	/* Step 7: apply the update */

	/*
	 * need update list in filename order to insure directories are
	 * created before their contents
	 */
	timelist_timer_start(global_times, "Update Loop");
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
		goto clean_exit;
	}

	delete_motd();
	timelist_timer_stop(global_times);
	/* Run any scripts that are needed to complete update */
	timelist_timer_start(global_times, "Run Scripts");

	/* Determine if another update is needed so the scripts block */
	int new_current_version = get_current_version(path_prefix);
	if (on_new_format() && (requested_version == -1 || (requested_version > new_current_version))) {
		re_update = true;
	}

	run_scripts(re_update);
	timelist_timer_stop(global_times);

	/* Create the state file that will tell swupd it's on a mix on future runs */
	if (mix_exists && !system_on_mix()) {
		int fd = open(MIXED_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd == -1) {
			fprintf(stderr, "ERROR: Failed to create 'mixed' statefile\n");
			ret = -1;
		}
		close(fd);
	}

clean_exit:
	list_free_list(updates);
	free_manifest(current_manifest);
	free_manifest(server_manifest);

clean_curl:
	timelist_timer_stop(global_times);
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
	timelist_print_stats(global_times);

	/* version_files_match must be done before swupd_deinit to use globals */
	if (re_update) {
		versions_match = version_files_consistent();
	}
	if (latest_subs) {
		list_free_list(current_subs);
	} else {
		free_subscriptions(&current_subs);
	}
	/* clean up our cached content from the update. It is likely much more than
	 * we need and the clean helps us prevent cache bloat. */
	if (!keepcache) {
		clean_statedir(false, false);
	}
	free_subscriptions(&latest_subs);
	swupd_deinit();

	if (ret == 0) {
		if (nonpack > 0) {
			printf("%i files were not in a pack\n", nonpack);
		}
		if (!download_only) {
			if (current_version < server_version) {
				printf("Update successful. System updated from version %d to version %d\n",
				       current_version, server_version);
			} else {
				printf("Update complete. System already up-to-date at version %d\n", current_version);
			}
		}
	} else {
		printf("Update failed.\n");
	}

	if (re_update && ret == 0) {
		ret = re_exec_update(versions_match);
	}

	/* free swupd_cmd now that is no longer needed */
	free_string(&swupd_cmd);

	return ret;
}

static bool cmd_line_status = false;

static const struct option prog_opts[] = {
	{ "download", no_argument, 0, 'd' },
	{ "manifest", required_argument, 0, 'm' },
	{ "status", no_argument, 0, 's' },
	{ "keepcache", no_argument, 0, 'k' },
	{ "migrate", no_argument, 0, 'T' },
	{ "allow-mix-collisions", no_argument, 0, 'a' },
};

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd update [OPTION...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "   -m, --manifest=M        Update to version M, also accepts 'latest' (default)\n");
	fprintf(stderr, "   -d, --download          Download all content, but do not actually install the update\n");
	fprintf(stderr, "   -s, --status            Show current OS version and latest version available on server\n");
	fprintf(stderr, "   -k, --keepcache         Do not delete the swupd state directory content after updating the system\n");
	fprintf(stderr, "   -T, --migrate           Migrate to augmented upstream/mix content\n");
	fprintf(stderr, "   -a, --allow-mix-collisions	Ignore and continue if custom user content conflicts with upstream provided content\n");
	fprintf(stderr, "\n");
}

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'm':
		if (strcmp("latest", optarg) == 0) {
			requested_version = -1;
			return true;
		}

		err = strtoi_err(optarg, &requested_version);
		if (err < 0 || requested_version < 0) {
			fprintf(stderr, "Invalid --manifest argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 'a':
		allow_mix_collisions = true;
		return true;
	case 'd':
		download_only = true;
		return true;
	case 's':
		cmd_line_status = true;
		return true;
	case 'T':
		migrate = true;
		fprintf(stderr, "Attempting to migrate to your mix content...\n\n");
		return true;
	case 'k':
		keepcache = true;
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
		fprintf(stderr, "Error: unexpected arguments\n\n");
		return false;
	}

	return true;
}

/* return 0 if server_version is ahead of current_version,
 * 1 if the current_version is current or ahead
 * 2 if one or more versions can't be found
 */
static int print_versions()
{
	int current_version, server_version, ret = 0;

	check_root();
	(void)init_globals();
	swupd_curl_init();

	if (read_versions(&current_version, &server_version, path_prefix) != 0) {
		ret = 2;
	} else {
		if (server_version <= current_version) {
			ret = 1;
		}
	}
	if (current_version > 0) {
		fprintf(stderr, "Current OS version: %d\n", current_version);
	}
	if (server_version > 0) {
		fprintf(stderr, "Latest server version: %d\n", server_version);
	}

	telemetry(ret ? TELEMETRY_WARN : TELEMETRY_INFO,
		  "check",
		  "result=%d\n"
		  "version_current=%d\n"
		  "version_server=%d\n",
		  ret,
		  current_version,
		  server_version);

	return ret;
}

int update_main(int argc, char **argv)
{
	int ret = 0;

	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}

	if (cmd_line_status) {
		ret = print_versions();
	} else {
		ret = main_update();
	}
	return ret;
}
