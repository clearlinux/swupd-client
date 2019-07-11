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
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

static int requested_version = -1;
static bool download_only;

int nonpack;

/* This loads the upstream Clear Manifest.Full and local
 * Manifest.full, and then checks that there are no conflicts between
 * the files they both include */
static int check_manifests_uniqueness(int clrver, int mixver)
{
	struct manifest *clear = load_manifest_full(clrver, false);
	struct manifest *mixer = load_manifest_full(mixver, true);
	if (!clear || !mixer) {
		error("Could not load full manifests\n");
		return -1;
	}

	struct file **clearfull = manifest_files_to_array(clear);
	struct file **mixerfull = manifest_files_to_array(mixer);

	if (clearfull == NULL || mixerfull == NULL) {
		error("Could not convert full manifest to array\n");
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
	struct step step;

	step = progress_get_step();
	progress_set_step(step.current, "download_fullfiles");
	ret = download_fullfiles(updates, &nonpack);
	if (ret) {
		error("Could not download all files, aborting update\n");
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
	info("Staging file content\n");
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
		if (ret != 0) {
			error("File staging failed: %s\n", file->filename);
			return ret;
		}
	}

	/* check policy, and if policy says, "ask", ask the user at this point */
	/* check for reboot need - if needed, wait for reboot */

	/* sync */
	sync();

	/* rename to apply update */
	progress_set_step((step.current) + 1, "update_files");
	info("Applying update\n");
	ret = rename_all_files_to_final(updates);
	if (ret != 0) {
		ret = SWUPD_COULDNT_RENAME_FILE;
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

static enum swupd_code check_versions(int *current_version, int *server_version, int requested_version, char *path_prefix)
{
	int ret;

	ret = read_versions(current_version, server_version, path_prefix);
	if (ret != SWUPD_OK) {
		return ret;
	}
	if (*current_version == 0) {
		error("Update from version 0 not supported yet.\n");
		return SWUPD_INVALID_OPTION;
	}
	if (requested_version != -1) {
		if (requested_version < *current_version) {
			error("Requested version for update (%d) must be greater than current version (%d)\n",
			      requested_version, *current_version);
			return SWUPD_INVALID_OPTION;
		}
		if (requested_version < *server_version) {
			*server_version = requested_version;
		}
	}

	return SWUPD_OK;
}

static enum swupd_code main_update()
{
	int current_version = -1, server_version = -1;
	int mix_current_version = -1, mix_server_version = -1;
	struct manifest *current_manifest = NULL, *server_manifest = NULL;
	struct list *updates = NULL;
	struct list *current_subs = NULL;
	struct list *latest_subs = NULL;
	int ret;
	struct timespec ts_start, ts_stop; // For main swupd update time
	double delta;
	bool mix_exists;
	bool re_update = false;
	bool versions_match = false;

	/* start the timer used to report the total time to telemetry */
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);

	/* Step 1: Preparation steps */
	timelist_timer_start(global_times, "Prepare for update");
	progress_set_step(1, "prepare_for_update");
	info("Update started.\n");

	mix_exists = check_mix_exists();

	read_subscriptions(&current_subs);

	handle_mirror_if_stale();
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Prepare for update

	/* Step 2: get versions */
	timelist_timer_start(global_times, "Get versions");
	progress_set_step(2, "get_versions");
version_check:
	ret = check_versions(&current_version, &server_version, requested_version, path_prefix);
	if (ret != SWUPD_OK) {
		goto clean_curl;
	}

	if (mix_exists) {
		check_mix_versions(&mix_current_version, &mix_server_version, path_prefix);
		if (mix_current_version == -1 || mix_server_version == -1) {
			ret = SWUPD_CURRENT_VERSION_UNKNOWN;
			goto clean_curl;
		}
		/* Check if a new upstream version is available so we can update to it still */
		if (need_new_upstream(server_version)) {
			info("NEW CLEAR AVAILABLE %d\n", server_version);
			/* Update the upstreamversion that will be used to generate the new mix content */
			FILE *verfile = fopen(MIX_DIR "upstreamversion", "w+");
			if (!verfile) {
				error("fopen() %s/upstreamversion returned %s\n", MIX_DIR, strerror(errno));
			} else {
				fprintf(verfile, "%d", server_version);
				fclose(verfile);
			}

			if (run_command("/usr/bin/mixin", "build", NULL) != 0) {
				error("Could not execute mixin\n");
				ret = SWUPD_SUBPROCESS_ERROR;
				goto clean_curl;
			}

			// new mix version
			check_mix_versions(&mix_current_version, &mix_server_version, path_prefix);
			ret = check_manifests_uniqueness(server_version, mix_server_version);
			if (ret > 0) {
				info("\n");
				warn("%i collisions were found between mix and upstream, please re-create mix !!\n", ret);
				if (!allow_mix_collisions) {
					ret = SWUPD_MIX_COLLISIONS;
					goto clean_curl;
				}
			} else if (ret < 0) {
				ret = SWUPD_COULDNT_LOAD_MANIFEST;
				goto clean_curl;
			}

			goto version_check;
		}
		current_version = mix_current_version;
		server_version = mix_server_version;
	}

	if (server_version <= current_version) {
		if (requested_version == server_version) {
			info("Requested version (%i)", requested_version);
		} else {
			info("Version on server (%i)", server_version);
		}
		info(" is not newer than system version (%i)\n", current_version);
		ret = SWUPD_OK;
		goto clean_curl;
	}

	info("Preparing to update from %i to %i\n", current_version, server_version);
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Get versions

	/* Step 3: housekeeping */
	timelist_timer_start(global_times, "Clean up download directory");
	progress_set_step(3, "cleanup_download_dir");
	if (rm_staging_dir_contents("download")) {
		error("There was a problem cleaning download directory\n");
		ret = SWUPD_COULDNT_REMOVE_FILE;
		goto clean_curl;
	}
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Clean up download directory

	/* Step 4: setup manifests */
	timelist_timer_start(global_times, "Load manifests");
	progress_set_step(4, "load_manifests");
	timelist_timer_start(global_times, "Load MoM manifests");
	int manifest_err;

	/* get the from/to MoM manifests */
	if (system_on_mix()) {
		current_manifest = load_mom(current_version, false, mix_exists, &manifest_err);
	} else {
		current_manifest = load_mom(current_version, false, false, &manifest_err);
	}
	if (!current_manifest) {
		/* TODO: possibly remove this as not getting a "from" manifest is not fatal
		 * - we just don't apply deltas */
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto clean_exit;
	}

	server_manifest = load_mom(server_version, true, mix_exists, &manifest_err);
	if (!server_manifest) {
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto clean_exit;
	}
	timelist_timer_stop(global_times); // closing: Load MoM manifests

	timelist_timer_start(global_times, "Recurse and consolidate bundle manifests");
	/* Read the current collective of manifests that we are subscribed to.
	 * First load up the old (current) manifests. Statedir could have been cleared
	 * or corrupt, so don't assume things are already there. Updating subscribed
	 * manifests is done as part of recurse_manifest */
	current_manifest->submanifests = recurse_manifest(current_manifest, current_subs, NULL, false, &manifest_err);
	if (!current_manifest->submanifests) {
		ret = SWUPD_RECURSE_MANIFEST;
		error("Cannot load current MoM sub-manifests, exiting\n");
		goto clean_exit;
	}

	/* consolidate the current collective manifests down into one in memory */
	current_manifest->files = files_from_bundles(current_manifest->submanifests);
	current_manifest->files = consolidate_files(current_manifest->files);
	latest_subs = list_clone(current_subs);
	set_subscription_versions(server_manifest, current_manifest, &latest_subs);
	link_submanifests(current_manifest, server_manifest, current_subs, latest_subs, false);

	/* The new subscription is seeded from the list of currently installed bundles
	 * This calls add_subscriptions which recurses for new includes */
	timelist_timer_start(global_times, "Add included bundle manifests");
	ret = add_included_manifests(server_manifest, &latest_subs);
	if (ret) {
		if (ret == -add_sub_BADNAME) {
			/* this means a bundle(s) was removed in a future version */
			warn("One or more installed bundles are no longer available at version %d.\n",
			     server_version);
		} else {
			ret = SWUPD_RECURSE_MANIFEST;
			goto clean_exit;
		}
	}
	timelist_timer_stop(global_times); // closing: Add included bundle manifests

	/* read the new collective of manifests that we are subscribed to in the new MoM */
	server_manifest->submanifests = recurse_manifest(server_manifest, latest_subs, NULL, false, &manifest_err);
	if (!server_manifest->submanifests) {
		ret = SWUPD_RECURSE_MANIFEST;
		error("Cannot load server MoM sub-manifests, exiting\n");
		goto clean_exit;
	}

	/* consolidate the new collective manifests down into one in memory */
	server_manifest->files = files_from_bundles(server_manifest->submanifests);
	server_manifest->files = consolidate_files(server_manifest->files);
	set_subscription_versions(server_manifest, current_manifest, &latest_subs);
	link_submanifests(current_manifest, server_manifest, current_subs, latest_subs, true);

	/* prepare for an update process based on comparing two in memory manifests */
	link_manifests(current_manifest, server_manifest);
	timelist_timer_stop(global_times); // closing: Recurse and consolidate bundle manifests
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Load manifests

	/* Step 5: check disk state before attempting update */
	timelist_timer_start(global_times, "Run pre-update scripts");
	progress_set_step(5, "run_preupdate_scripts");
	scripts_run_pre_update(server_manifest);
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Run pre-update scripts

	/* Step 6: get the packs and untar */
	timelist_timer_start(global_times, "Download packs");
	progress_set_step(6, "download_packs");
	download_subscribed_packs(latest_subs, server_manifest, false);
	timelist_timer_stop(global_times); // closing: Download packs

	/* Step	 7: apply deltas */
	timelist_timer_start(global_times, "Apply deltas");
	progress_set_step(7, "apply_deltas");
	apply_deltas(current_manifest);
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Apply deltas

	/* Step 8: some more housekeeping */
	/* TODO: consider trying to do less sorting of manifests */
	timelist_timer_start(global_times, "Create update list");
	progress_set_step(8, "create_update_list");
	updates = create_update_list(server_manifest);

	print_statistics(current_version, server_version);
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Create update list

	/* Steps 9 & 10: downloading and applying updates */
	/* need update list in filename order to insure directories are
	 * created before their contents */
	timelist_timer_start(global_times, "Update loop");
	/* two steps are part of this stage, so only pass the
	 * initial step number and we will update the description
	 * from within the update_loop function */
	progress_set_step(9, "");
	updates = list_sort(updates, file_sort_filename);

	ret = update_loop(updates, server_manifest);
	if (ret == 0 && !download_only) {
		/* Failure to write the version file in the state directory
		 * should not affect exit status. */
		(void)update_device_latest_version(server_version);
		info("Update was applied.\n");
	} else if (ret != 0) {
		// Ensure a positive exit status for the main program.
		ret = abs(ret);
		goto clean_exit;
	}

	delete_motd();
	timelist_timer_stop(global_times); // closing: Update loop

	/* Step 11: Run any scripts that are needed to complete update */
	timelist_timer_start(global_times, "Run post-update scripts");
	progress_set_step(11, "run_postupdate_scripts");

	/* Determine if another update is needed so the scripts block */
	int new_current_version = get_current_version(path_prefix);
	if (on_new_format() && (requested_version == -1 || (requested_version > new_current_version))) {
		re_update = true;
	}
	scripts_run_post_update(re_update);
	progress_complete_step();
	timelist_timer_stop(global_times); // closing: Run post-update scripts

	/* Create the state file that will tell swupd it's on a mix on future runs */
	if (mix_exists && !system_on_mix()) {
		int fd = open(MIXED_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd == -1) {
			error("Failed to create 'mixed' statefile\n");
			ret = SWUPD_COULDNT_CREATE_FILE;
		}
		close(fd);
	}

clean_exit:
	list_free_list(updates);
	free_manifest(current_manifest);
	free_manifest(server_manifest);

clean_curl:
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts_stop);
	delta = ts_stop.tv_sec - ts_start.tv_sec + ts_stop.tv_nsec / 1000000000.0 - ts_start.tv_nsec / 1000000000.0;
	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
		  "update",
		  "current_version=%d\n"
		  "server_version=%d\n"
		  "result=%d\n"
		  "time=%5.1f\n"
		  "bytes=%ld\n",
		  current_version,
		  server_version,
		  ret,
		  delta,
		  total_curl_sz);

	if (server_version > current_version) {
		info("Update took %0.1f seconds, %ld MB transferred\n", delta,
		     total_curl_sz / 1000 / 1000);
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

	if (ret == 0) {
		if (nonpack > 0) {
			info("%i files were not in a pack\n", nonpack);
		}
		if (!download_only) {
			if (current_version < server_version) {
				print("Update successful. System updated from version %d to version %d\n",
				      current_version, server_version);
			} else {
				print("Update complete. System already up-to-date at version %d\n", current_version);
			}
		}
	} else {
		print("Update failed.\n");
	}

	if (re_update && ret == 0) {

		if (!versions_match) {
			error("Inconsistency between version files, exiting now.\n");
			return SWUPD_CURRENT_VERSION_UNKNOWN;
		}

		if (!swupd_argv) {
			error("Unable to determine re-update command, exiting now.\n");
			return SWUPD_INVALID_BINARY;
		}

		/* we need to resolve the whole path to swupd first, proc/self/exe
		 * is a symbolic link to the executable that is running the current process */
		char swupd_binary[LINE_MAX];
		int path_length;
		path_length = readlink("/proc/self/exe", swupd_binary, sizeof(swupd_binary));
		if (path_length <= 0 || path_length >= LINE_MAX) {
			error("Could not determine the swupd path\n");
			return -1;
		}
		swupd_binary[path_length] = '\0';

		/* Run the swupd_argv saved from main */
		return execv(swupd_binary, swupd_argv);
	}

	return ret;
}

static bool cmd_line_status = false;

static const struct option prog_opts[] = {
	{ "download", no_argument, 0, 'd' },
	{ "version", required_argument, 0, 'V' },
	{ "manifest", required_argument, 0, 'm' },
	{ "status", no_argument, 0, 's' },
	{ "keepcache", no_argument, 0, 'k' },
	{ "migrate", no_argument, 0, 'T' },
	{ "allow-mix-collisions", no_argument, 0, 'a' },
};

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd update [OPTION...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	// TODO(castulo): remove the superseded -m option from the help menu by end of November 2019,
	// so it is not visible but the option must remain available in the back so we don't break users
	print("Options:\n");
	print("   -V, --version=V         Update to version V, also accepts 'latest' (default)\n");
	print("   -d, --download          Download all content, but do not actually install the update\n");
	print("   -s, --status            Show current OS version and latest version available on server. Equivalent to \"swupd check-update\"\n");
	print("   -k, --keepcache         Do not delete the swupd state directory content after updating the system\n");
	print("   -T, --migrate           Migrate to augmented upstream/mix content\n");
	print("   -a, --allow-mix-collisions	Ignore and continue if custom user content conflicts with upstream provided content\n");
	print("   -m, --manifest=V        NOTE: this flag has been superseded. Please use -V instead\n");
	print("\n");
}

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'm':
	case 'V':
		if (strcmp("latest", optarg) == 0) {
			requested_version = -1;
			return true;
		}

		err = strtoi_err(optarg, &requested_version);
		if (err < 0 || requested_version < 0) {
			error("Invalid --manifest argument: %s\n\n", optarg);
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
		error("Attempting to migrate to your mix content...\n\n");
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
		error("unexpected arguments\n\n");
		return false;
	}

	return true;
}

enum swupd_code update_main(int argc, char **argv)
{
	int ret = SWUPD_OK;
	const int steps_in_update = 11;

	/*
	 * Steps for update:
	 *
	 * 1) prepare_for_update
	 * 2) get_versions
	 * 3) cleanup_download_dir
	 * 4) load_manifests
	 * 5) run_preupdate_scripts
	 * 6) download_packs
	 * 7) apply_deltas
	 * 8) create_update_list
	 * 9) download_fullfiles
	 * 10) update_files
	 * 11) run_postupdate_scripts
	 */

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	/* Update should always ignore optional bundles */
	skip_optional_bundles = true;
	progress_init_steps("update", steps_in_update);

	ret = swupd_init(SWUPD_ALL);
	if (ret != 0) {
		error("Updater failed to initialize, exiting now.\n");
		return ret;
	}

	if (cmd_line_status) {
		ret = check_update();
	} else {
		ret = main_update();
	}

	swupd_deinit();

	progress_finish_steps("update", ret);
	return ret;
}
