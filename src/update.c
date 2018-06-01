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

int nonpack;

void increment_retries(int *retries, int *timeout)
{
	(*retries)++;
	sleep(*timeout);
	*timeout *= 2;
}

static struct list *full_download_loop(struct list *updates, int isfailed)
{
	struct list *iter;
	struct file *file;
	char *filename;
	char *url;
	int ret = -1;
	unsigned int list_length = list_len(updates);
	unsigned int complete = 0;

	iter = list_head(updates);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		complete++;

		if (file->is_deleted) {
			continue;
		}
		/* Mix content is local, so don't queue files up for curl downloads */
		if (file->is_mix) {
			string_or_die(&url, "%s/%i/files/%s.tar", MIX_STATE_DIR, file->last_change, file->hash);
			string_or_die(&filename, "%s/download/.%s.tar", state_dir, file->hash);
			file->staging = filename;
			ret = link(url, filename);
			/* Try doing a regular rename if hardlink fails */
			if (ret) {
				if (rename(url, filename) != 0) {
					fprintf(stderr, "Failed to copy local mix file: %s\n", filename);
					continue;
				}
			}

			untar_full_download(file);
			continue;
		}
		full_download(file);
		print_progress(complete, list_length);
	}
	print_progress(list_length, list_length); /* Force out 100% */
	printf("\n");

	if (isfailed) {
		list_free_list(updates);
	}

	return end_full_download();
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
		abort();
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
		failed = full_download_loop(failed, 1);
	} else {
		failed = full_download_loop(updates, 0);
	}

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
	if (add_subscriptions(subbed, subs, current, mom, false, 0) >= 0) {
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
	int lock_fd;
	int retries = 0;
	int timeout = 10;
	struct timespec ts_start, ts_stop; // For main swupd update time
	timelist times;
	double delta;
	bool mix_exists;
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

	ret = swupd_curl_check_network();
	if (ret) {
		fprintf(stderr, "Error: Network issue, unable to proceed with update\n");
		goto clean_curl;
	}

	mix_exists = check_mix_exists();

	fprintf(stderr, "Update started.\n");

	grabtime_start(&times, "Update Step 1: get versions");

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

			if (system("/usr/bin/mixin build") != 0) {
				fprintf(stderr, "ERROR: Could not execute mixin\n");
				ret = EXIT_FAILURE;
				goto clean_curl;
			}

			// new mix version
			check_mix_versions(&mix_current_version, &mix_server_version, path_prefix);
			ret = check_manifests_uniqueness(server_version, mix_server_version);
			if (ret) {
				printf("\n\t!! %i collisions were found between mix and upstream, please re-create mix !!\n", ret);
				if (!allow_mix_collisions) {
					ret = EXIT_FAILURE;
					goto clean_curl;
				}
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
	grabtime_stop(&times);
	grabtime_stop(&times); // Close step 1
	grabtime_start(&times, "Load Manifests:");
load_current_mom:
	/* Step 3: setup manifests */

	/* get the from/to MoM manifests */
	if (system_on_mix()) {
		current_manifest = load_mom(current_version, false, mix_exists);
	} else {
		current_manifest = load_mom(current_version, false, false);
	}
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
	int server_manifest_err;
	server_manifest = load_mom_err(server_version, true, mix_exists, &server_manifest_err);
	if (!server_manifest) {
		if (retries < MAX_TRIES && server_manifest_err != -ENET404) {
			increment_retries(&retries, &timeout);
			fprintf(stderr, "Retry #%d downloading server Manifests\n", retries);
			goto load_server_mom;
		}
		fprintf(stderr, "Failure retrieving manifest from server\n");
		if (server_manifest_err == -ENET404) {
			fprintf(stderr, "Version %d not available\n", server_version);
		} else {
			fprintf(stderr, "Unable to load manifest after retrying (config or network problem?)\n");
		}
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
	current_manifest->submanifests = recurse_manifest(current_manifest, current_subs, NULL, false);
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
	server_manifest->submanifests = recurse_manifest(server_manifest, latest_subs, NULL, false);
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
	grabtime_stop(&times);
	/* Step 4: check disk state before attempting update */
	grabtime_start(&times, "Pre-Update Scripts");
	run_preupdate_scripts(server_manifest);
	grabtime_stop(&times);

	grabtime_start(&times, "Download Packs");

download_packs:
	/* Step 5: get the packs and untar */
	ret = download_subscribed_packs(latest_subs, server_manifest, false, true);
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

	grabtime_start(&times, "Apply deltas");
	apply_deltas(current_manifest);
	grabtime_stop(&times);

	/* Step 6: some more housekeeping */
	/* TODO: consider trying to do less sorting of manifests */

	grabtime_start(&times, "Create Update List");

	updates = create_update_list(server_manifest);

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
	int new_current_version = get_current_version(path_prefix);
	if (on_new_format() && (requested_version == -1 || (requested_version > new_current_version))) {
		re_update = true;
	}

	run_scripts(re_update);
	grabtime_stop(&times);

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
	if (latest_subs) {
		list_free_list(current_subs);
	} else {
		free_subscriptions(&current_subs);
	}
	swupd_deinit(lock_fd, &latest_subs);

	if (nonpack > 0) {
		printf("%i files were not in a pack\n", nonpack);
	}

	if (!download_only) {
		if ((current_version < server_version) && (ret == 0)) {
			printf("Update successful. System updated from version %d to version %d\n",
			       current_version, server_version);
		} else if (ret == 0) {
			printf("Update complete. System already up-to-date at version %d\n", current_version);
		}
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
	{ "help", no_argument, 0, 'h' },
	{ "manifest", required_argument, 0, 'm' },
	{ "url", required_argument, 0, 'u' },
	{ "port", required_argument, 0, 'P' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "status", no_argument, 0, 's' },
	{ "format", required_argument, 0, 'F' },
	{ "path", required_argument, 0, 'p' },
	{ "force", no_argument, 0, 'x' },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "ignore-time", no_argument, 0, 'I' },
	{ "statedir", required_argument, 0, 'S' },
	{ "certpath", required_argument, 0, 'C' },
	{ "time", no_argument, 0, 't' },
	{ "no-scripts", no_argument, 0, 'N' },
	{ "no-boot-update", no_argument, 0, 'b' },
	{ "migrate", no_argument, 0, 'T' },
	{ "allow-mix-collisions", no_argument, 0, 'a' },
	{ 0, 0, 0, 0 }
};

static void print_help(const char *name)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd %s [OPTION...]\n\n", basename((char *)name));
	fprintf(stderr, "Help Options:\n");
	fprintf(stderr, "   -h, --help              Show help options\n\n");
	fprintf(stderr, "Application Options:\n");
	fprintf(stderr, "   -m, --manifest=M        Update to version M, also accepts 'latest' (default)\n");
	fprintf(stderr, "   -d, --download          Download all content, but do not actually install the update\n");
#warning "remove user configurable url when alternative exists"
	fprintf(stderr, "   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	fprintf(stderr, "   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
#warning "remove user configurable content url when alternative exists"
	fprintf(stderr, "   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
#warning "remove user configurable version url when alternative exists"
	fprintf(stderr, "   -v, --versionurl=[URL]  RFC-3986 encoded url for version string download\n");
	fprintf(stderr, "   -s, --status            Show current OS version and latest version available on server\n");
	fprintf(stderr, "   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	fprintf(stderr, "   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	fprintf(stderr, "   -x, --force             Attempt to proceed even if non-critical errors found\n");
	fprintf(stderr, "   -n, --nosigcheck        Do not attempt to enforce certificate or signature checking\n");
	fprintf(stderr, "   -I, --ignore-time       Ignore system/certificate time when validating signature\n");
	fprintf(stderr, "   -S, --statedir          Specify alternate swupd state directory\n");
	fprintf(stderr, "   -C, --certpath          Specify alternate path to swupd certificates\n");
	fprintf(stderr, "   -t, --time              Show verbose time output for swupd operations\n");
	fprintf(stderr, "   -N, --no-scripts        Do not run the post-update scripts and boot update tool\n");
	fprintf(stderr, "   -b, --no-boot-update    Do not update the boot files using clr-boot-manager\n");
	fprintf(stderr, "   -T, --migrate           Migrate to augmented upstream/mix content\n");
	fprintf(stderr, "   -a, --allow-mix-collisions	Ignore and continue if custom user content conflicts with upstream provided content\n");
	fprintf(stderr, "\n");
}

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hm:xnIdtNbTau:P:c:v:sF:p:S:C:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'm':
			if (strcmp("latest", optarg) == 0) {
				requested_version = -1;
			} else if (sscanf(optarg, "%i", &requested_version) != 1) {
				fprintf(stderr, "Invalid --manifest argument\n\n");
				goto err;
			}
			break;
		case 'a':
			allow_mix_collisions = true;
			break;
		case 'd':
			download_only = true;
			break;
		case 'u':
			if (!optarg) {
				fprintf(stderr, "Invalid --url argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			set_content_url(optarg);
			break;
		case 't':
			verbose_time = true;
			break;
		case 'N':
			no_scripts = true;
			break;
		case 'b':
			no_boot_update = true;
			break;
		case 'P':
			if (sscanf(optarg, "%ld", &update_server_port) != 1) {
				fprintf(stderr, "Invalid --port argument\n\n");
				goto err;
			}
			break;
		case 'c':
			if (!optarg) {
				fprintf(stderr, "Invalid --contenturl argument\n\n");
				goto err;
			}
			set_content_url(optarg);
			break;
		case 'v':
			if (!optarg) {
				fprintf(stderr, "Invalid --versionurl argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			break;
		case 's':
			cmd_line_status = true;
			break;
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				fprintf(stderr, "Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'S':
			if (!optarg || !set_state_dir(optarg)) {
				fprintf(stderr, "Invalid --statedir argument\n\n");
				goto err;
			}
			break;
		case 'p': /* default empty path_prefix updates the running OS */
			if (!optarg || !set_path_prefix(optarg)) {
				fprintf(stderr, "Invalid --path argument\n\n");
				goto err;
			}
			break;
		case 'x':
			force = true;
			break;
		case 'T':
			migrate = true;
			fprintf(stderr, "Attempting to migrate to your mix content...\n\n");
			break;
		case 'n':
			sigcheck = false;
			break;
		case 'I':
			timecheck = false;
			break;
		case 'C':
			if (!optarg) {
				fprintf(stderr, "Invalid --certpath argument\n\n");
				goto err;
			}
			set_cert_path(optarg);
			break;
		default:
			fprintf(stderr, "Unrecognized option\n\n");
			goto err;
		}
	}

	return true;
err:
	print_help(argv[0]);
	return false;
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

	read_versions(&current_version, &server_version, path_prefix);

	if (current_version < 0) {
		fprintf(stderr, "Cannot determine current OS version\n");
		ret = 2;
	} else {
		fprintf(stderr, "Current OS version: %d\n", current_version);
	}

	if (server_version < 0) {
		fprintf(stderr, "Cannot get the latest server version. Could not reach server\n");
		ret = 2;
	} else {
		fprintf(stderr, "Latest server version: %d\n", server_version);
	}
	if ((ret == 0) && (server_version <= current_version)) {
		ret = 1;
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
