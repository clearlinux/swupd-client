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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "signature.h"
#include "swupd.h"

#define FLAG_DOWNLOAD_ONLY 2000
#define FLAG_UPDATE_SEARCH_FILE_INDEX 2001

static int requested_version = -1;
static bool download_only = false;
static bool update_search_file_index = false;
static bool keepcache = false;
static char swupd_binary[LINE_MAX] = { 0 };

int nonpack;

void update_set_option_version(int opt)
{
	requested_version = opt;
}

void update_set_option_download_only(bool opt)
{
	download_only = opt;
}

void update_set_option_keepcache(bool opt)
{
	keepcache = opt;
}

static void save_swupd_binary_path()
{
	/* we need to resolve the whole path to swupd first, proc/self/exe
		 * is a symbolic link to the executable that is running the current process */
	int path_length;
	path_length = readlink("/proc/self/exe", swupd_binary, sizeof(swupd_binary));
	if (path_length <= 0 || path_length >= LINE_MAX) {
		// On errors fallback to default location
		strncpy(swupd_binary, "/usr/bin/swupd", sizeof(swupd_binary));
	} else {
		swupd_binary[path_length] = '\0';
	}
}

static int update_loop(struct list *updates, struct manifest *server_manifest, extra_proc_fn_t file_validation_fn)
{
	int ret;

	ret = download_fullfiles(updates, &nonpack);
	if (ret) {
		error("Could not download all files, aborting update\n");
		return ret;
	}

	if (download_only) {
		return 0;
	}

	/* now that we have the files, perform extra validation on them
	 * before installing them (if applicable) */
	if (file_validation_fn) {
		ret = file_validation_fn(updates);
		if (ret) {
			info("Aborting update...\n\n");
			return ret;
		}
	}

	progress_next_step("update_files", PROGRESS_BAR);
	return staging_install_all_files(updates, server_manifest);
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
	ret = add_subscriptions(subbed, subs, mom, true, 0);
	list_free_list(subbed);
	if (ret & (add_sub_ERR | add_sub_BADNAME)) {
		return ret;
	}

	return 0;
}

static enum swupd_code check_versions(int *current_version, int *server_version, int requested_version, char *path_prefix)
{
	int ret;

	ret = read_versions(current_version, server_version, path_prefix);
	if (ret != SWUPD_OK) {
		return ret;
	}
	if (*current_version == 0) {
		error("Update from version 0 not supported yet\n");
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

/* Make a copy of the Manifest for the completion code */
static void save_manifest(int version)
{
	char *momdir, *momfile, *original;

	string_or_die(&momdir, "%s/var/tmp/swupd", globals.path_prefix);
	string_or_die(&momfile, "%s/Manifest.MoM", momdir);
	string_or_die(&original, "%s/%i/Manifest.MoM", globals.state_dir, version);
	sys_rm(momfile);
	mkdir_p(momdir);

	link_or_copy(original, momfile);
	free_and_clear_pointer(&momdir);
	free_and_clear_pointer(&momfile);
	free_and_clear_pointer(&original);
}

/* Checks to see if the given file is installed under the path_prefix. */
static bool is_installed_and_verified(struct file *file)
{
	/* Not safe to perform the hash check if there was a type change
	 * involving symlinks. */
	if (file->is_link != file->peer->is_link) {
		return false;
	}

	char *fullname = sys_path_join("%s/%s", globals.path_prefix, file->filename);

	if (verify_file(file, fullname)) {
		free_and_clear_pointer(&fullname);
		return true;
	}
	free_and_clear_pointer(&fullname);
	return false;
}

/* Find files which need updated based on differences in last_change.
   Should let further do_not_update policy be handled in the caller, but for
   now some hacky exclusions are done here. */
static struct list *create_update_list(struct manifest *server)
{
	struct list *output = NULL;
	struct list *list;

	globals.update_count = 0;
	globals.update_skip = 0;
	list = list_head(server->files);
	while (list) {
		struct file *file;
		file = list->data;
		list = list->next;

		/* Look for potential short circuit, if something has the same
		 * flags and the same hash, then conclude they are the same. */
		if (file->peer &&
		    file->is_deleted == file->peer->is_deleted &&
		    file->is_file == file->peer->is_file &&
		    file->is_dir == file->peer->is_dir &&
		    file->is_link == file->peer->is_link &&
		    hash_equal(file->hash, file->peer->hash)) {
			if (file->last_change == file->peer->last_change) {
				/* Nothing to do; the file did not change */
				continue;
			}
			/* When file and its peer have matching hashes
			 * but different versions, this indicates a
			 * minversion bump was performed server-side.
			 * Skip updating them if installed with the
			 * correct hash. */
			if (is_installed_and_verified(file)) {
				continue;
			}
		}

		/* Note: at this stage, "untracked" files are always "new"
		 * files, so they will not have a peer. */
		if (!file->peer ||
		    (file->peer && file->last_change > file->peer->last_change)) {
			/* check and if needed mark as do_not_update */
			(void)ignore(file);
			/* check if we need to run scripts/update the bootloader/etc */
			apply_heuristics(file);

			output = list_prepend_data(output, file);
			continue;
		}
	}
	globals.update_count = list_len(output) - globals.update_skip;

	return output;
}

enum swupd_code execute_update_extra(extra_proc_fn_t post_update_fn, extra_proc_fn_t file_validation_fn)
{
	int current_version = -1, server_version = -1;
	struct manifest *current_manifest = NULL, *server_manifest = NULL;
	struct list *updates = NULL;
	struct list *current_subs = NULL;
	struct list *latest_subs = NULL;
	int ret;
	struct timespec ts_start, ts_stop; // For main swupd update time
	double delta;
	bool re_update = false;
	bool versions_match = false;

	/* start the timer used to report the total time to telemetry */
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts_start);

	save_swupd_binary_path();

	/* Preparation steps */
	timelist_timer_start(globals.global_times, "Prepare for update");
	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	info("Update started\n");

	read_subscriptions(&current_subs);

	if (handle_mirror_if_stale() < 0) {
		ret = SWUPD_SERVER_CONNECTION_ERROR;
		goto clean_curl;
	}
	timelist_timer_stop(globals.global_times); // closing: Prepare for update

	/* get versions */
	timelist_timer_start(globals.global_times, "Get versions");
	ret = check_versions(&current_version, &server_version, requested_version, globals.path_prefix);
	if (ret != SWUPD_OK) {
		goto clean_curl;
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
	timelist_timer_stop(globals.global_times); // closing: Get versions

	/* housekeeping */
	timelist_timer_start(globals.global_times, "Clean up download directory");
	if (rm_staging_dir_contents("download")) {
		error("There was a problem cleaning download directory\n");
		ret = SWUPD_COULDNT_REMOVE_FILE;
		goto clean_curl;
	}
	timelist_timer_stop(globals.global_times); // closing: Clean up download directory

	/* setup manifests */
	timelist_timer_start(globals.global_times, "Load manifests");
	timelist_timer_start(globals.global_times, "Load MoM manifests");
	int manifest_err;

	current_manifest = load_mom(current_version, &manifest_err);
	if (!current_manifest) {
		/* TODO: possibly remove this as not getting a "from" manifest is not fatal
		 * - we just don't apply deltas */
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto clean_exit;
	}

	server_manifest = load_mom(server_version, &manifest_err);
	if (!server_manifest) {
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto clean_exit;
	}
	save_manifest(server_version);
	timelist_timer_stop(globals.global_times); // closing: Load MoM manifests

	timelist_timer_start(globals.global_times, "Recurse and consolidate bundle manifests");
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
	current_manifest->files = consolidate_files_from_bundles(current_manifest->submanifests);
	latest_subs = list_clone(current_subs);
	set_subscription_versions(server_manifest, current_manifest, &latest_subs);
	link_submanifests(current_manifest, server_manifest, current_subs, latest_subs, false);

	/* The new subscription is seeded from the list of currently installed bundles
	 * This calls add_subscriptions which recurses for new includes */
	timelist_timer_start(globals.global_times, "Add included bundle manifests");
	ret = add_included_manifests(server_manifest, &latest_subs);
	if (ret) {
		if (ret & add_sub_BADNAME) {
			/* this means a bundle(s) was removed in a future version */
			warn("One or more installed bundles are no longer available at version %d\n",
			     server_version);
		} else {
			ret = SWUPD_RECURSE_MANIFEST;
			goto clean_exit;
		}
	}
	timelist_timer_stop(globals.global_times); // closing: Add included bundle manifests

	/* read the new collective of manifests that we are subscribed to in the new MoM */
	server_manifest->submanifests = recurse_manifest(server_manifest, latest_subs, NULL, false, &manifest_err);
	if (!server_manifest->submanifests) {
		ret = SWUPD_RECURSE_MANIFEST;
		error("Cannot load server MoM sub-manifests, exiting\n");
		goto clean_exit;
	}

	/* consolidate the new collective manifests down into one in memory */
	server_manifest->files = consolidate_files_from_bundles(server_manifest->submanifests);
	set_subscription_versions(server_manifest, current_manifest, &latest_subs);
	link_submanifests(current_manifest, server_manifest, current_subs, latest_subs, true);

	/* prepare for an update process based on comparing two in memory manifests */
	link_manifests(current_manifest, server_manifest);
	timelist_timer_stop(globals.global_times); // closing: Recurse and consolidate bundle manifests
	timelist_timer_stop(globals.global_times); // closing: Load manifests

	/* check disk state before attempting update */
	timelist_timer_start(globals.global_times, "Run pre-update scripts");
	progress_next_step("run_preupdate_scripts", PROGRESS_UNDEFINED);
	scripts_run_pre_update(server_manifest);
	timelist_timer_stop(globals.global_times); // closing: Run pre-update scripts

	/* get the packs and untar */
	timelist_timer_start(globals.global_times, "Download packs");
	download_subscribed_packs(latest_subs, server_manifest, false);
	timelist_timer_stop(globals.global_times); // closing: Download packs

	/* apply deltas */
	progress_next_step("prepare_for_update", PROGRESS_UNDEFINED);
	timelist_timer_start(globals.global_times, "Apply deltas");
	apply_deltas(current_manifest);
	timelist_timer_stop(globals.global_times); // closing: Apply deltas

	/* some more housekeeping */
	/* TODO: consider trying to do less sorting of manifests */
	timelist_timer_start(globals.global_times, "Create update list");
	updates = create_update_list(server_manifest);

	print_statistics(current_version, server_version);
	timelist_timer_stop(globals.global_times); // closing: Create update list

	/* downloading and applying updates */
	/* need update list in filename order to insure directories are
	 * created before their contents */
	timelist_timer_start(globals.global_times, "Update loop");
	updates = list_sort(updates, cmp_file_filename_is_deleted);

	ret = update_loop(updates, server_manifest, file_validation_fn);
	if (ret == 0 && !download_only) {
		/* Failure to write the version file in the state directory
		 * should not affect exit status. */
		(void)update_device_latest_version(server_version);
		info("Update was applied\n");
	} else if (ret != 0) {
		// Ensure a positive exit status for the main program.
		ret = abs(ret);
		goto clean_exit;
	}

	delete_motd();
	timelist_timer_stop(globals.global_times); // closing: Update loop

	if (!download_only) {
		/* execute post-update processing (if any) */
		if (post_update_fn) {
			ret = post_update_fn(updates);
		}

		/* Run any scripts that are needed to complete update */
		timelist_timer_start(globals.global_times, "Run post-update scripts");
		progress_next_step("run_postupdate_scripts", PROGRESS_UNDEFINED);

		/* Determine if another update is needed so the scripts block */
		int new_current_version = get_current_version(globals.path_prefix);
		if (on_new_format() && (requested_version == -1 || (requested_version > new_current_version))) {
			re_update = true;
		}
		scripts_run_post_update(re_update || globals.wait_for_scripts);
		timelist_timer_stop(globals.global_times); // closing: Run post-update scripts
	}

	/* Downloading all manifests to be used as search-file index */
	if (update_search_file_index) {
		timelist_timer_start(globals.global_times, "Updating search file index");
		progress_next_step("update_search_index", PROGRESS_BAR);
		info("Downloading all Clear Linux manifests...\n");
		mom_get_manifests_list(server_manifest, NULL, NULL);
		timelist_timer_stop(globals.global_times); // closing: Updating search file index
	}

clean_exit:
	list_free_list(updates);
	manifest_free(current_manifest);
	manifest_free(server_manifest);

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
	timelist_print_stats(globals.global_times);

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
				info("Update successful - System updated from version %d to version %d\n",
				     current_version, server_version);
			} else {
				info("Update complete - System already up-to-date at version %d\n", current_version);
			}
		}
	} else {
		print("Update failed\n");
	}

	if (re_update && ret == 0) {

		if (!versions_match) {
			error("Inconsistency between version files, exiting now\n");
			return SWUPD_CURRENT_VERSION_UNKNOWN;
		}

		if (!globals.swupd_argv) {
			error("Unable to determine re-update command, exiting now\n");
			return SWUPD_INVALID_BINARY;
		}

		/* Run the swupd_argv saved from main */
		return execv(swupd_binary, globals.swupd_argv);
	}

	return ret;
}

enum swupd_code execute_update(void)
{
	return execute_update_extra(NULL, NULL);
}

static bool cmd_line_status = false;

static const struct option prog_opts[] = {
	{ "download", no_argument, 0, FLAG_DOWNLOAD_ONLY },
	{ "update-search-file-index", no_argument, 0, FLAG_UPDATE_SEARCH_FILE_INDEX },
	{ "version", required_argument, 0, 'V' },
	{ "manifest", required_argument, 0, 'm' },
	{ "status", no_argument, 0, 's' },
	{ "keepcache", no_argument, 0, 'k' },
};

static void print_help(void)
{
	print("Performs a system software update\n\n");
	print("Usage:\n");
	print("   swupd update [OPTION...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	print("Options:\n");
	print("   -V, --version=V         Update to version V, also accepts 'latest' (default)\n");
	print("   -s, --status            Show current OS version and latest version available on server. Equivalent to \"swupd check-update\"\n");
	print("   -k, --keepcache         Do not delete the swupd state directory content after updating the system\n");
	print("   --download              Download all content, but do not actually install the update\n");
	print("   --update-search-file-index Update the index used by search-file to speed up searches (Don't enable this if you have download or space restrictions)\n");
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
			error("Invalid --version argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case 's':
		cmd_line_status = optarg_to_bool(optarg);
		return true;
	case 'k':
		keepcache = optarg_to_bool(optarg);
		return true;
	case FLAG_DOWNLOAD_ONLY:
		download_only = optarg_to_bool(optarg);
		return true;
	case FLAG_UPDATE_SEARCH_FILE_INDEX:
		update_search_file_index = optarg_to_bool(optarg);
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
	enum swupd_code ret = SWUPD_OK;
	int steps_in_update;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret;
	}

	/* Update should always ignore optional bundles */
	globals.skip_optional_bundles = true;

	/*
	 * Steps for update:
	 *   1) load_manifests
	 *   2) run_preupdate_scripts
	 *   3) download_packs
	 *   4) extract_packs
	 *   5) prepare_for_update
	 *   6) validate_fullfiles
	 *   7) download_fullfiles
	 *   8) extract_fullfiles (finishes here on --download)
	 *   9) update_files
	 *  10) run_postupdate_scripts
	 *  11) update_search_index (only with --update-search-file-index)
	 */
	if (cmd_line_status) {
		steps_in_update = 0;
	} else if (download_only) {
		steps_in_update = 8;
	} else {
		steps_in_update = 10;
	}
	if (update_search_file_index) {
		steps_in_update++;
	}
	progress_init_steps("update", steps_in_update);

	if (cmd_line_status) {
		ret = check_update();
	} else {
		ret = execute_update();
	}

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
