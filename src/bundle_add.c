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

static char **cmdline_bundles;

static void print_help(void)
{
	print("Installs new software bundles\n\n");
	print("Usage:\n");
	print("   swupd bundle-add [OPTION...] [bundle1 bundle2 (...)]\n\n");

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

	cmdline_bundles = argv + optind;

	return true;
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
	filepath = sys_path_join(globals.path_prefix, "/usr/");
	if (!sys_file_exists(filepath)) {
		free_string(&filepath);
		return 0;
	}

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

static void print_summary(int bundles_requested, int already_installed, int bundles_installed, int dependencies_installed, int rc)
{
	/* print totals */
	int bundles_failed = bundles_requested - bundles_installed - already_installed;
	if (bundles_failed > 0) {
		print("Failed to install %i of %i bundles\n", bundles_failed, bundles_requested - already_installed);
	} else if (bundles_installed > 0) {
		print("%snstalled %i bundle%s\n", (rc == SWUPD_OK ? "Successfully i" : "I"), bundles_installed, (bundles_installed > 1 ? "s" : ""));
	}
	if (dependencies_installed > 0) {
		print("%i bundle%s\n", dependencies_installed, (dependencies_installed > 1 ? "s were installed as dependencies" : " was installed as dependency"));
	}
	if ((bundles_installed > 0 || dependencies_installed > 0) && rc != SWUPD_OK) {
		print("Some files from the installed bundle%s may be missing\n", (bundles_installed + dependencies_installed > 1 ? "s" : ""));
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
			track_bundle(bundle);
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

static enum swupd_code install_files(struct manifest *mom, struct list *to_install_files)
{
	enum swupd_code ret = SWUPD_OK;
	struct list *iter;

	/* Install all bundle(s) files into the fs */
	timelist_timer_start(globals.global_times, "Installing bundle(s) files onto filesystem");
	progress_next_step("install_files", PROGRESS_BAR);

	// Apply heuristics to all files
	timelist_timer_start(globals.global_times, "Applying heuristics");
	for (iter = to_install_files; iter; iter = iter->next) {
		struct file *file = iter->data;
		(void)ignore(file);
		apply_heuristics(file);
	}
	timelist_timer_stop(globals.global_times); // closing: Applying heuristics

	//TODO: Improve staging functions so we won't need this hack
	globals.update_count = list_len(to_install_files) - globals.update_skip;
	ret = staging_install_all_files(to_install_files, mom);
	mom->files = NULL;
	if (ret) {
		error("Failed to install required files\n");
		goto out;
	}
	timelist_timer_stop(globals.global_times); // closing: Installing bundles

	/* Run any scripts that are needed to complete update */
	timelist_timer_start(globals.global_times, "Run Scripts");
	progress_next_step("run_postupdate_scripts", PROGRESS_UNDEFINED);
	scripts_run_post_update(globals.wait_for_scripts);
	timelist_timer_stop(globals.global_times); // closing: Run Scripts

out:
	return ret;
}

static struct list *generate_bundles_to_install(struct list *bundles)
{
	struct list *aliases = NULL;
	struct list *bundles_list = NULL;
	struct list *iter = NULL;

	aliases = get_alias_definitions();
	for (iter = bundles; iter; iter = iter->next) {
		char *bundle = iter->data;

		struct list *alias_bundles = get_alias_bundles(aliases, bundle);
		char *alias_list_str = string_join(", ", alias_bundles);

		if (strcmp(bundle, alias_list_str) != 0) {
			info("Alias %s will install bundle(s): %s\n", bundle, alias_list_str);
		}
		free_string(&alias_list_str);
		bundles_list = list_concat(alias_bundles, bundles_list);
	}

	list_free_list_and_data(aliases, free_alias_lookup);
	bundles_list = list_sort(bundles_list, strcmp_wrapper);
	bundles_list = list_sorted_deduplicate(bundles_list, strcmp_wrapper, free);

	return bundles_list;
}

static enum swupd_code download_content(struct manifest *mom, struct list *to_install_bundles, struct list *to_install_files)
{
	enum swupd_code ret;

	/* download necessary packs */
	timelist_timer_start(globals.global_times, "Download packs");

	if (rm_staging_dir_contents("download") < 0) {
		debug("rm_staging_dir_contents failed - resuming operation");
	}

	if (list_longer_than(to_install_files, 10 * list_len(to_install_bundles))) {
		download_zero_packs(to_install_bundles, mom);
	} else {
		/* the progress would be completed within the
		 * download_subscribed_packs function, since we
		 * didn't run it, manually mark the step as completed */
		progress_next_step("download_packs", PROGRESS_BAR);
		info("No packs need to be downloaded\n");
		progress_next_step("extract_packs", PROGRESS_UNDEFINED);
	}
	timelist_timer_stop(globals.global_times); // closing: Download packs

	/* Download missing files */
	timelist_timer_start(globals.global_times, "Download missing files");
	ret = download_fullfiles(to_install_files, NULL);
	timelist_timer_stop(globals.global_times); // closing: Download missing files
	if (ret) {
		/* make sure the return code is positive */
		error("Could not download some files from bundles, aborting bundle installation\n");
		ret = SWUPD_COULDNT_DOWNLOAD_FILE;
	}

	return ret;
}

/* Bundle install one ore more bundles passed in bundles
 * param as a null terminated array of strings
 */
enum swupd_code bundle_add(struct list *bundles_list, int version, extra_proc_fn_t pre_add_fn, extra_proc_fn_t post_add_fn)
{
	int ret = 0;
	struct manifest *mom;
	struct list *bundles = NULL;
	struct list *installed_bundles = NULL;
	struct list *to_install_bundles = NULL;
	struct list *installed_files = NULL;
	struct list *to_install_files = NULL;
	struct list *iter;
	int already_installed = 0;
	int invalid_bundles = 0;
	int bundles_installed = 0;
	int dependencies_installed = 0;
	int bundles_requested;

	char *bundles_list_str = NULL;
	bool mix_exists;

	/* check if the user is using an alias for a bundle */
	timelist_timer_start(globals.global_times, "Prepend bundles to list");
	bundles = generate_bundles_to_install(bundles_list);
	timelist_timer_stop(globals.global_times); // closing: Prepend bundles to list

	/* get the current Mom */
	timelist_timer_start(globals.global_times, "Load MoM");
	mix_exists = (check_mix_exists() & system_on_mix());
	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	mom = load_mom(version, mix_exists, NULL);
	if (!mom) {
		error("Cannot load official manifest MoM for version %i\n", version);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto clean_and_exit;
	}
	timelist_timer_stop(globals.global_times); // closing: Load MoM

	timelist_timer_start(globals.global_times, "Install bundles");

	timelist_timer_start(globals.global_times, "Add bundles and recurse");
	/* get a list of bundles already installed in the system */
	info("Loading required manifests...\n");
	ret = mom_get_manifests_list(mom, &installed_bundles, is_installed_bundle_data);
	if (ret) {
		ret = SWUPD_COULDNT_LOAD_MANIFEST;
		goto clean_and_exit;
	}

	/* get a list of bundles to install (get dependencies) */
	ret = compute_bundle_dependecies(mom, bundles, &to_install_bundles, &already_installed, &invalid_bundles);
	if (ret) {
		ret = SWUPD_RECURSE_MANIFEST;
		goto clean_and_exit;
	}
	if (!to_install_bundles) {
		goto clean_and_exit;
	}
	timelist_timer_stop(globals.global_times); // closing: Add bundles and recurse

	/* Get a list with all files needed to be installed for the requested bundles */
	timelist_timer_start(globals.global_times, "Consolidate files from bundles");
	installed_files = files_from_bundles(installed_bundles);
	installed_files = filter_out_deleted_files(installed_files);
	installed_files = consolidate_files(installed_files);
	to_install_files = files_from_bundles(to_install_bundles);
	to_install_files = filter_out_deleted_files(to_install_files);
	to_install_files = consolidate_files(to_install_files);
	timelist_timer_stop(globals.global_times); // closing: Consolidate files from bundles

	if (!to_install_files) {
		goto clean_and_exit;
	}

	/* execute pre-add processing (if any) */
	if (pre_add_fn) {
		ret = pre_add_fn(to_install_files);
		if (ret != SWUPD_OK) {
			info("Aborting bundle installation...\n\n");
			goto clean_and_exit;
		}
	}

	/* Check if we have enough space */
	ret = check_disk_space_availability(to_install_bundles);
	if (ret) {
		ret = SWUPD_DISK_SPACE_ERROR;
		goto clean_and_exit;
	}

	/* get necessary packs and/or files */
	ret = download_content(mom, to_install_bundles, to_install_files);
	if (ret) {
		goto clean_and_exit;
	}

	mom->files = installed_files;
	ret = install_files(mom, to_install_files);
	if (ret) {
		goto clean_and_exit;
	}

	timelist_timer_stop(globals.global_times); // closing: Install bundles

	timelist_print_stats(globals.global_times);

	/* execute post-add processing (if any) */
	if (post_add_fn) {
		ret = post_add_fn(to_install_files);
	}

clean_and_exit:
	iter = list_head(to_install_bundles);
	/* count how many of the requested bundles were actually installed, note that the
	 * to_install_bundles list could also have extra dependencies */
	while (iter) {
		struct manifest *to_install_manifest;
		to_install_manifest = iter->data;
		iter = iter->next;
		if (is_installed_bundle(to_install_manifest->component)) {
			if (string_in_list(to_install_manifest->component, bundles)) {
				bundles_installed++;
				track_bundle(to_install_manifest->component);
			} else {
				dependencies_installed++;
			}
		}
	}

	bundles_requested = list_len(bundles);
	print_summary(bundles_requested, already_installed, bundles_installed, dependencies_installed, ret);

	/* if one or more of the requested bundles was invalid, and
	 * there is no other error return SWUPD_INVALID_BUNDLE */
	if (invalid_bundles > 0 && ret == SWUPD_OK) {
		ret = SWUPD_INVALID_BUNDLE;
	}

	/* report command result to telemetry */
	bundles_list_str = string_join(", ", bundles);
	telemetry(ret ? TELEMETRY_CRIT : TELEMETRY_INFO,
		  "bundleadd",
		  "bundles=%s\n"
		  "current_version=%d\n"
		  "result=%d\n"
		  "bytes=%ld\n",
		  bundles_list_str,
		  version,
		  ret,
		  total_curl_sz);

	list_free_list(installed_files);
	list_free_list(to_install_files);
	list_free_list_and_data(to_install_bundles, manifest_free_data);
	list_free_list_and_data(installed_bundles, manifest_free_data);
	list_free_list_and_data(bundles, free);
	manifest_free(mom);
	free_string(&bundles_list_str);

	return ret;
}

enum swupd_code execute_bundle_add(struct list *bundles_list, extra_proc_fn_t pre_add_fn, extra_proc_fn_t post_add_fn)
{
	int version;

	/* get the current version of the system */
	version = get_current_version(globals.path_prefix);
	if (version < 0) {
		error("Unable to determine current OS version\n");
		return SWUPD_CURRENT_VERSION_UNKNOWN;
	}

	return bundle_add(bundles_list, version, pre_add_fn, post_add_fn);
}

enum swupd_code bundle_add_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_bundleadd = 8;
	struct list *bundles_list = NULL;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	/* initialize swupd */
	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		return ret;
	}

	/* move the bundles provided in the command line into a
	 * list so it is easier to handle them */
	for (; *cmdline_bundles; ++cmdline_bundles) {
		char *bundle = *cmdline_bundles;
		bundles_list = list_append_data(bundles_list, bundle);
	}
	bundles_list = list_head(bundles_list);

	/*
	 * Steps for bundle-add:
	 *  1) load_manifests
	 *  2) download_packs
	 *  3) extract_packs
	 *  4) validate_fullfiles
	 *  5) download_fullfiles
	 *  6) extract_fullfiles
	 *  7) install_files
	 *  8) run_postupdate_scripts
	 */
	progress_init_steps("bundle-add", steps_in_bundleadd);

	ret = execute_bundle_add(bundles_list, NULL, NULL);

	list_free_list(bundles_list);
	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
