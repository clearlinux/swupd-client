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
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "alias.h"
#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define VERIFY_NOPICKY 0

#define FLAG_SKIP_OPTIONAL 2000
#define FLAG_SKIP_DISKSPACE_CHECK 2001

static char **bundles;

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd bundle-add [OPTION...] [bundle1 bundle2 (...)]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	print("Options:\n");
	print("   --skip-optional         Do not install optional bundles (also-add flag in Manifests)\n");
	print("   --skip-diskspace-check  Do not check free disk space before adding bundle\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "skip-optional", no_argument, 0, FLAG_SKIP_OPTIONAL },
	{ "skip-diskspace-check", no_argument, 0, FLAG_SKIP_DISKSPACE_CHECK },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case FLAG_SKIP_OPTIONAL:
		globals.skip_optional_bundles = optarg_to_bool(optarg);
		return true;
	case FLAG_SKIP_DISKSPACE_CHECK:
		globals.skip_diskspace_check = optarg_to_bool(optarg);
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
	int optind = global_parse_options(argc, argv, &opts);

	if (optind < 0) {
		return false;
	}

	if (argc <= optind) {
		error("missing bundle(s) to be installed\n\n");
		return false;
	}

	bundles = argv + optind;

	return true;
}

/*
 * track_installed creates a tracking file in path_prefix/var/lib/bundles If
 * there are no tracked files in that directory (directory is empty or does not
 * exist) copy the tracking directory at path_prefix/usr/share/clear/bundles to
 * path_prefix/var/lib/bundles to initiate the tracking files.
 *
 * This function does not return an error code because weird state in this
 * directory must be handled gracefully whenever encountered.
 */
static void track_installed(const char *bundle_name)
{
	int ret = 0;
	char *dst = get_tracking_dir();
	char *src;

	/* if state_dir_parent/bundles doesn't exist or is empty, assume this is
	 * the first time tracking installed bundles. Since we don't know what the
	 * user installed themselves just copy the entire system tracking directory
	 * into the state tracking directory. */
	if (!is_populated_dir(dst)) {
		char *rmfile;
		ret = rm_rf(dst);
		if (ret) {
			goto out;
		}
		src = mk_full_filename(globals.path_prefix, "/usr/share/clear/bundles");
		/* at the point this function is called <bundle_name> is already
		 * installed on the system and therefore has a tracking file under
		 * /usr/share/clear/bundles. A simple cp -a of that directory will
		 * accurately track that bundle as manually installed. */
		ret = copy_all(src, globals.state_dir);
		free_string(&src);
		if (ret) {
			goto out;
		}
		/* remove uglies that live in the system tracking directory */
		rmfile = mk_full_filename(dst, ".MoM");
		(void)unlink(rmfile);
		free_string(&rmfile);
		/* set perms on the directory correctly */
		ret = chmod(dst, S_IRWXU);
		if (ret) {
			goto out;
		}
	}

	char *tracking_file = mk_full_filename(dst, bundle_name);
	int fd = open(tracking_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	free_string(&tracking_file);
	if (fd < 0) {
		ret = -1;
		goto out;
	}
	close(fd);

out:
	if (ret) {
		debug("Issue creating tracking file in %s for %s\n", dst, bundle_name);
	}
	free_string(&dst);
}

static enum swupd_code install_bundles(struct list *bundles, struct list **subs, struct manifest *mom)
{
	int ret;
	int bundles_failed = 0;
	int already_installed = 0;
	int bundles_installed = 0;
	int dependencies_installed = 0;
	int bundles_requested = list_len(bundles);
	long bundle_size = 0;
	long fs_free = 0;
	struct file *file;
	struct list *iter;
	struct list *installed_bundles = NULL;
	struct list *installed_files = NULL;
	struct list *to_install_bundles = NULL;
	struct list *to_install_files = NULL;
	struct list *current_subs = NULL;
	bool invalid_bundle_provided = false;

	/* step 1: get subscriptions for bundles to be installed */
	info("Loading required manifests...\n");
	timelist_timer_start(globals.global_times, "Add bundles and recurse");
	progress_set_step(1, "load_manifests");
	ret = add_subscriptions(bundles, subs, mom, false, 0);

	/* print a message if any of the requested bundles is already installed */
	iter = list_head(bundles);
	while (iter) {
		char *bundle;
		bundle = iter->data;
		iter = iter->next;
		if (is_installed_bundle(bundle)) {
			warn("Bundle \"%s\" is already installed, skipping it...\n", bundle);
			already_installed++;
			/* track as installed since the user tried to install */
			track_installed(bundle);
		}
		/* warn the user if the bundle to be installed is experimental */
		file = search_bundle_in_manifest(mom, bundle);
		if (file && file->is_experimental) {
			warn("Bundle %s is experimental\n", bundle);
		}
	}

	/* Use a bitwise AND with the add_sub_NEW mask to determine if at least
	 * one new bundle was subscribed */
	if (!(ret & add_sub_NEW)) {
		/* something went wrong, nothing will be installed */
		if (ret & add_sub_ERR) {
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
		} else if (ret & add_sub_BADNAME) {
			ret = SWUPD_INVALID_BUNDLE;
		} else {
			/* this means the user tried to add a bundle that
			 * was already installed, nothing to do here */
			ret = SWUPD_OK;
		}
		goto out;
	}
	/* If at least one of the provided bundles was invalid set this flag
	 * so we can check it before exiting the program */
	if (ret & add_sub_BADNAME) {
		invalid_bundle_provided = true;
	}

	/* Set the version of the subscribed bundles to the one they last changed */
	set_subscription_versions(mom, NULL, subs);

	/* Load the manifest of all bundles to be installed */
	to_install_bundles = recurse_manifest(mom, *subs, NULL, false, NULL);
	if (!to_install_bundles) {
		error("Cannot load to install bundles\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto out;
	}

	/* Load the manifest of all bundles already installed */
	read_subscriptions(&current_subs);
	set_subscription_versions(mom, NULL, &current_subs);
	installed_bundles = recurse_manifest(mom, current_subs, NULL, false, NULL);
	if (!installed_bundles) {
		error("Cannot load installed bundles\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto out;
	}
	mom->submanifests = installed_bundles;

	progress_complete_step();
	timelist_timer_stop(globals.global_times); // closing: Add bundles and recurse

	/* Step 2: Get a list with all files needed to be installed for the requested bundles */
	timelist_timer_start(globals.global_times, "Consolidate files from bundles");
	progress_set_step(2, "consolidate_files");

	/* get all files already installed in the target system */
	installed_files = files_from_bundles(installed_bundles);
	installed_files = consolidate_files(installed_files);
	mom->files = installed_files;
	installed_files = filter_out_deleted_files(installed_files);

	/* get all the files included in the bundles to be added */
	to_install_files = files_from_bundles(to_install_bundles);
	to_install_files = consolidate_files(to_install_files);
	to_install_files = filter_out_deleted_files(to_install_files);

	/* from the list of files to be installed, remove those files already in the target system */
	to_install_files = filter_out_existing_files(to_install_files, installed_files);

	progress_complete_step();
	timelist_timer_stop(globals.global_times); // closing: Consolidate files from bundles

	/* Step 3: Check if we have enough space */
	progress_set_step(3, "check_disk_space_availability");
	if (!globals.skip_diskspace_check) {
		timelist_timer_start(globals.global_times, "Check disk space availability");
		char *filepath = NULL;

		bundle_size = get_manifest_list_contentsize(to_install_bundles);
		filepath = mk_full_filename(globals.path_prefix, "/usr/");

		/* Calculate free space on filepath */
		fs_free = get_available_space(filepath);
		free_string(&filepath);

		/* Add 10% to bundle_size as a 'fudge factor' */
		if (((bundle_size * 1.1) > fs_free) || fs_free < 0) {
			ret = SWUPD_DISK_SPACE_ERROR;

			if (fs_free > 0) {
				error("Bundle too large by %ldM\n", (bundle_size - fs_free) / 1000 / 1000);
			} else {
				error("Unable to determine free space on filesystem\n");
			}

			info("NOTE: currently, swupd only checks /usr/ "
			     "(or the passed-in path with /usr/ appended) for available space\n");
			info("To skip this error and install anyways, "
			     "add the --skip-diskspace-check flag to your command\n");

			goto out;
		}
		timelist_timer_stop(globals.global_times); // closing: Check disk space availability
	}
	progress_complete_step();

	/* step 4: download necessary packs */
	timelist_timer_start(globals.global_times, "Download packs");
	progress_set_step(4, "download_packs");

	(void)rm_staging_dir_contents("download");

	if (list_longer_than(to_install_files, 10)) {
		download_subscribed_packs(*subs, mom, true);
	} else {
		/* the progress would be completed within the
		 * download_subscribed_packs function, since we
		 * didn't run it, manually mark the step as completed */
		info("No packs need to be downloaded\n");
		progress_complete_step();
	}
	timelist_timer_stop(globals.global_times); // closing: Download packs

	/* step 5: Download missing files */
	timelist_timer_start(globals.global_times, "Download missing files");
	progress_set_step(5, "download_fullfiles");
	ret = download_fullfiles(to_install_files, NULL);
	if (ret) {
		/* make sure the return code is positive */
		ret = abs(ret);
		error("Could not download some files from bundles, aborting bundle installation\n");
		goto out;
	}
	timelist_timer_stop(globals.global_times); // closing: Download missing files

	/* step 6: Install all bundle(s) files into the fs */
	timelist_timer_start(globals.global_times, "Installing bundle(s) files onto filesystem");
	progress_set_step(6, "install_files");

	info("Installing bundle(s) files...\n");

	/* This loop does an initial check to verify the hash of every downloaded file to install,
	 * if the hash is wrong it is removed from staging area so it can be re-downloaded */
	char *hashpath;
	iter = list_head(to_install_files);
	while (iter) {
		file = iter->data;
		iter = iter->next;

		string_or_die(&hashpath, "%s/staged/%s", globals.state_dir, file->hash);

		if (access(hashpath, F_OK) < 0) {
			/* the file does not exist in the staged directory, it will need
			 * to be downloaded again */
			free_string(&hashpath);
			continue;
		}

		/* make sure the file is not corrupt */
		if (!verify_file(file, hashpath)) {
			warn("hash check failed for %s\n", file->filename);
			info("         will attempt to download fullfile for %s\n", file->filename);
			ret = sys_rm(hashpath);
			if (ret != -ENOENT) {
				error("could not remove bad file %s\n", hashpath);
				ret = SWUPD_COULDNT_REMOVE_FILE;
				free_string(&hashpath);
				goto out;
			}
			// successfully removed, continue and check the next file
			free_string(&hashpath);
			continue;
		}
		free_string(&hashpath);
	}

	/*
	 * NOTE: The following two loops are used to install the files in the target system:
	 *  - the first loop stages the file
	 *  - the second loop renames the files to their final name in the target system
	 *
	 * This process is done in two separate loops to reduce the chance of end up
	 * with a corrupt system if for some reason the process is aborted during this stage
	 */
	unsigned int list_length = list_len(to_install_files) * 2; // we are using two loops so length is times 2
	unsigned int complete = 0;

	/* Copy files to their final destination */
	iter = list_head(to_install_files);
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

		/* stage the file:
		 *  - make sure the directory where the file will be copied to exists
		 *  - if the file being staged already exists in the system make sure its
		 *    type hasn't changed, if it has, remove it so it can be replaced
		 *  - copy the file/directory to its final destination; if it is a file,
		 *    keep its name with a .update prefix, if it is a directory, it will already
		 *    be copied with its final name
		 * Note: to avoid too much recursion, do not send the mom to do_staging so it
		 *       doesn't try to fix failures, we will handle those below */
		ret = do_staging(file, mom);
		if (ret) {
			goto out;
		}

		progress_report(complete, list_length);
	}

	/* Rename the files to their final form */
	iter = list_head(to_install_files);
	while (iter) {
		file = iter->data;
		iter = iter->next;
		complete++;

		if (file->is_deleted || file->do_not_update || ignore(file)) {
			continue;
		}

		/* This was staged by verify_fix_path */
		if (!file->staging && !file->is_dir) {
			/* the current file struct doesn't have the name of the "staging" file
			 * since it was staged by verify_fix_path, the staged file is in the
			 * file struct in the MoM, so we need to load that one instead
			 * so rename_staged_file_to_final works properly */
			file = search_file_in_manifest(mom, file->filename);
		}

		rename_staged_file_to_final(file);

		progress_report(complete, list_length);
	}
	sync();
	timelist_timer_stop(globals.global_times); // closing: Installing bundle(s) files onto filesystem

	/* step 7: Run any scripts that are needed to complete update */
	timelist_timer_start(globals.global_times, "Run Scripts");
	progress_set_step(7, "run_scripts");
	scripts_run_post_update(globals.wait_for_scripts);
	timelist_timer_stop(globals.global_times); // closing: Run Scripts
	progress_complete_step();

	ret = SWUPD_OK;

out:
	/* count how many of the requested bundles were actually installed, note that the
	 * to_install_bundles list could also have extra dependencies */
	iter = list_head(to_install_bundles);
	while (iter) {
		struct manifest *to_install_manifest;
		to_install_manifest = iter->data;
		iter = iter->next;
		if (is_installed_bundle(to_install_manifest->component)) {
			if (string_in_list(to_install_manifest->component, bundles)) {
				bundles_installed++;
				track_installed(to_install_manifest->component);
			} else {
				dependencies_installed++;
			}
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
		print("Failed to install %i of %i bundles\n", bundles_failed, bundles_requested - already_installed);
	} else if (bundles_installed > 0) {
		print("Successfully installed %i bundle%s\n", bundles_installed, (bundles_installed > 1 ? "s" : ""));
	}
	if (dependencies_installed > 0) {
		print("%i bundle%s\n", dependencies_installed, (dependencies_installed > 1 ? "s were installed as dependencies" : " was installed as dependency"));
	}
	if (already_installed > 0) {
		print("%i bundle%s already installed\n", already_installed, (already_installed > 1 ? "s were" : " was"));
	}

	/* cleanup */
	if (current_subs) {
		free_subscriptions(&current_subs);
	}
	if (to_install_files) {
		list_free_list(to_install_files);
	}
	if (to_install_bundles) {
		list_free_list_and_data(to_install_bundles, free_manifest_data);
	}
	/* if one or more of the requested bundles was invalid, and
	 * there is no other error return SWUPD_INVALID_BUNDLE */
	if (invalid_bundle_provided && !ret) {
		ret = SWUPD_INVALID_BUNDLE;
	}
	return ret;
}

/* Bundle install one ore more bundles passed in bundles
 * param as a null terminated array of strings
 */
enum swupd_code install_bundles_frontend(char **bundles)
{
	int ret = 0;
	int current_version;
	struct list *aliases = NULL;
	struct list *bundles_list = NULL;
	struct manifest *mom;
	struct list *subs = NULL;
	char *bundles_list_str = NULL;
	bool mix_exists;

	/* initialize swupd and get current version from OS */
	ret = swupd_init(SWUPD_ALL);
	if (ret != 0) {
		error("Failed updater initialization, exiting now\n");
		return ret;
	}

	timelist_timer_start(globals.global_times, "Load MoM");
	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto clean_and_exit;
	}

	mix_exists = (check_mix_exists() & system_on_mix());

	mom = load_mom(current_version, mix_exists, NULL);
	if (!mom) {
		error("Cannot load official manifest MoM for version %i\n", current_version);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto clean_and_exit;
	}
	timelist_timer_stop(globals.global_times); // closing: Load MoM

	timelist_timer_start(globals.global_times, "Prepend bundles to list");
	aliases = get_alias_definitions();
	for (; *bundles; ++bundles) {
		struct list *alias_bundles = get_alias_bundles(aliases, *bundles);
		char *alias_list_str = string_join(", ", alias_bundles);

		if (strcmp(*bundles, alias_list_str) != 0) {
			info("Alias %s will install bundle(s): %s\n", *bundles, alias_list_str);
		}
		free_string(&alias_list_str);
		bundles_list = list_concat(alias_bundles, bundles_list);
	}
	list_free_list_and_data(aliases, free_alias_lookup);
	timelist_timer_stop(globals.global_times); // closing: Prepend bundles to list

	timelist_timer_start(globals.global_times, "Install bundles");
	ret = install_bundles(bundles_list, &subs, mom);
	timelist_timer_stop(globals.global_times); // closing: Install bundles

	timelist_print_stats(globals.global_times);

	free_manifest(mom);
clean_and_exit:
	bundles_list_str = string_join(", ", bundles_list);
	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
		  "bundleadd",
		  "bundles=%s\n"
		  "current_version=%d\n"
		  "result=%d\n"
		  "bytes=%ld\n",
		  bundles_list_str,
		  current_version,
		  ret,
		  total_curl_sz);

	list_free_list_and_data(bundles_list, free);
	free_string(&bundles_list_str);
	free_subscriptions(&subs);
	swupd_deinit();

	return ret;
}

enum swupd_code bundle_add_main(int argc, char **argv)
{
	int ret;
	const int steps_in_bundleadd = 7;

	/*
	 * Steps for bundle-add:
	 *
	 *  1) load_manifests
	 *  2) consolidate_files
	 *  3) check_disk_space_availability
	 *  4) download_packs
	 *  5) download_fullfiles
	 *  6) install_files
	 *  7) progress_set_step
	 */

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("bundle-add", steps_in_bundleadd);

	ret = install_bundles_frontend(bundles);

	progress_finish_steps(ret);
	return ret;
}
