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

static int check_disk_space_availability(struct list *to_install_bundles)
{
	char *filepath = NULL;
	long fs_free = 0;
	long bundle_size = 0;

	if (globals.skip_diskspace_check) {
		return 0;
	}

	timelist_timer_start(globals.global_times, "Check disk space availability");
	bundle_size = get_manifest_list_contentsize(to_install_bundles);
	filepath = mk_full_filename(globals.path_prefix, "/usr/");

	/* Calculate free space on filepath */
	fs_free = get_available_space(filepath);
	free_string(&filepath);
	timelist_timer_stop(globals.global_times); // closing: Check disk space availability

	/* Add 10% to bundle_size as a 'fudge factor' */
	if (((bundle_size * 1.1) > fs_free) || fs_free < 0) {
		if (fs_free > 0) {
			error("Bundle too large by %ldM\n", (bundle_size - fs_free) / 1000 / 1000);
		} else {
			error("Unable to determine free space on filesystem\n");
		}

		info("NOTE: currently, swupd only checks /usr/ "
		     "(or the passed-in path with /usr/ appended) for available space\n");
		info("To skip this error and install anyways, "
		     "add the --skip-diskspace-check flag to your command\n");

		return -1;
	}

	return 0;
}

void print_summary(int bundles_requested, int already_installed, int bundles_installed, int dependencies_installed)
{
	/* print totals */
	int bundles_failed = bundles_requested - bundles_installed - already_installed;
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
}

static bool is_not_installed_bundle(const void *bundle)
{
	return !is_installed_bundle(bundle);
}

static bool is_installed_bundle_data(const void *bundle)
{
	return is_installed_bundle(bundle);
}

static int compute_bundle_dependecies(struct manifest *mom, struct list *bundles, struct list **to_install_bundles, int *already_installed, int *invalid_bundles)
{
	struct list *iter;
	int err;

	/* print a message if any of the requested bundles is already installed
	   or is invalid.
	 */
	for (iter = bundles; iter; iter = iter->next) {
		struct file *file;
		char *bundle = iter->data;

		if (is_installed_bundle(bundle)) {
			warn("Bundle \"%s\" is already installed, skipping it...\n", bundle);
			(*already_installed)++;
			/* track as installed since the user tried to install */
			track_installed(bundle);
			continue;
		}

		file = mom_search_bundle(mom, bundle);
		if (!file) {
			warn("Bundle \"%s\" is invalid, skipping it...\n", bundle);
			(*invalid_bundles)++;

			continue;
		}

		/* warn the user if the bundle to be installed is experimental */
		if (file->is_experimental) {
			warn("Bundle %s is experimental\n", bundle);
		}

		err = recurse_dependencies(mom, bundle, to_install_bundles, is_not_installed_bundle);
		if (err) {
			return err;
		}
	}

	return 0;
}

static enum swupd_code install_bundles(struct list *bundles, struct manifest *mom)
{
	enum swupd_code ret = SWUPD_OK;
	int err = 0;
	int already_installed = 0;
	int bundles_installed = 0;
	int dependencies_installed = 0;

	int bundles_requested;
	struct list *iter;
	struct list *installed_files = NULL;
	struct list *to_install_files = NULL;
	int invalid_bundles = 0;
	struct list *installed_bundles = NULL;
	struct list *to_install_bundles = NULL;

	/* step 1: get subscriptions for bundles to be installed */
	info("Loading required manifests...\n");
	timelist_timer_start(globals.global_times, "Add bundles and recurse");
	progress_set_next_step("load_manifests");

	err = mom_get_manifests_list(mom, &installed_bundles, is_installed_bundle_data);
	if (err) {
		ret = SWUPD_COULDNT_LOAD_MANIFEST;
		goto out;
	}

	// check for errors
	err = compute_bundle_dependecies(mom, bundles, &to_install_bundles, &already_installed, &invalid_bundles);
	if (err) {
		ret = SWUPD_RECURSE_MANIFEST;
		goto out;
	}
	if (!to_install_bundles) {
		goto out;
	}
	timelist_timer_stop(globals.global_times); // closing: Add bundles and recurse

	/* Step 2: Get a list with all files needed to be installed for the requested bundles */
	timelist_timer_start(globals.global_times, "Consolidate files from bundles");
	progress_set_next_step("consolidate_files");

	installed_files = files_from_bundles(installed_bundles);
	installed_files = filter_out_deleted_files(installed_files);
	installed_files = consolidate_files(installed_files);

	to_install_files = files_from_bundles(to_install_bundles);
	to_install_files = filter_out_deleted_files(to_install_files);
	to_install_files = consolidate_files(to_install_files);

	progress_complete_step();
	timelist_timer_stop(globals.global_times); // closing: Consolidate files from bundles

	if (!to_install_files) {
		goto out;
	}

	///* Step 3: Check if we have enough space */
	progress_set_next_step("check_disk_space_availability");
	err = check_disk_space_availability(to_install_bundles);
	progress_complete_step();
	if (err) {
		ret = SWUPD_DISK_SPACE_ERROR;
		goto out;
	}

	/* step 4: download necessary packs */
	timelist_timer_start(globals.global_times, "Download packs");
	progress_set_next_step("download_packs");

	if (rm_staging_dir_contents("download") < 0) {
		debug("rm_staging_dir_contents failed - resuming operation");
	}

	if (list_longer_than(to_install_files, 10 * list_len(to_install_bundles))) {
		download_zero_packs(to_install_bundles, mom);
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
	progress_set_next_step("download_fullfiles");
	err = download_fullfiles(to_install_files, NULL);
	timelist_timer_stop(globals.global_times); // closing: Download missing files
	if (err) {
		/* make sure the return code is positive */
		error("Could not download some files from bundles, aborting bundle installation\n");
		ret = SWUPD_COULDNT_DOWNLOAD_FILE;
		goto out;
	}

	/* step 6: Install all bundle(s) files into the fs */
	timelist_timer_start(globals.global_times, "Installing bundle(s) files onto filesystem");
	progress_set_next_step("install_files");

	// Apply heuristics to all files
	timelist_timer_start(globals.global_times, "Appling heuristics");
	for (iter = to_install_files; iter; iter = iter->next) {
		struct file *file = iter->data;
		(void)ignore(file);
		apply_heuristics(file);
	}
	timelist_timer_stop(globals.global_times); // closing: Appling heuristics

	//TODO: Improve staging functions so we won't need this hack
	globals.update_count = list_len(to_install_files);
	globals.update_skip = 0;
	mom->files = installed_files;
	ret = staging_install_all_files(to_install_files, mom);
	mom->files = NULL;
	if (ret) {
		error("Failed to install required files");
		goto out;
	}
	timelist_timer_stop(globals.global_times); // closing: Installing bundles

	/* step 7: Run any scripts that are needed to complete update */
	timelist_timer_start(globals.global_times, "Run Scripts");
	progress_set_next_step("run_scripts");
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

	bundles_requested = list_len(bundles);
	print_summary(bundles_requested, already_installed, bundles_installed, dependencies_installed);

	/* if one or more of the requested bundles was invalid, and
	 * there is no other error return SWUPD_INVALID_BUNDLE */
	if (invalid_bundles > 0 && ret == SWUPD_OK) {
		ret = SWUPD_INVALID_BUNDLE;
	}

	list_free_list(installed_files);
	list_free_list(to_install_files);
	list_free_list_and_data(to_install_bundles, manifest_free_data);
	list_free_list_and_data(installed_bundles, manifest_free_data);

	return ret;
}

static struct list *generate_bundles_to_install(char **bundles)
{
	struct list *aliases = NULL;
	struct list *bundles_list = NULL;

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
	bundles_list = list_sort(bundles_list, list_strcmp);
	bundles_list = list_deduplicate(bundles_list, list_strcmp, free);

	return bundles_list;
}

/* Bundle install one ore more bundles passed in bundles
 * param as a null terminated array of strings
 */
enum swupd_code main_bundle_add()
{
	int ret = 0;
	int current_version;
	struct list *bundles_list = NULL;
	struct manifest *mom;
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
	bundles_list = generate_bundles_to_install(bundles);
	timelist_timer_stop(globals.global_times); // closing: Prepend bundles to list

	timelist_timer_start(globals.global_times, "Install bundles");
	ret = install_bundles(bundles_list, mom);
	timelist_timer_stop(globals.global_times); // closing: Install bundles

	timelist_print_stats(globals.global_times);

	manifest_free(mom);
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
	 *  7) progress_set_next_step
	 */

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("bundle-add", steps_in_bundleadd);

	ret = main_bundle_add();

	progress_finish_steps(ret);
	return ret;
}
