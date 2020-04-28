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
 *         Mario Alfredo Carrillo Arevalo <mario.alfredo.c.arevalo@intel.com>
 *         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
 */

#include <errno.h>
#include <getopt.h>
#include <string.h>

#include "swupd.h"

#define FLAG_DEPS 2000
#define FLAG_STATUS 2001

static bool cmdline_option_all = false;
static char *cmdline_option_has_dep = NULL;
static char *cmdline_option_deps = NULL;
static bool cmdline_local = true;
static bool cmdline_option_status = false;

void bundle_list_set_option_all(bool opt)
{
	cmdline_option_all = opt;
	cmdline_local = !cmdline_option_all;
}

void bundle_list_set_option_has_dep(char *bundle)
{
	if (!bundle) {
		return;
	}
	FREE(cmdline_option_has_dep);
	cmdline_option_has_dep = bundle;
	cmdline_local = false;
}

void bundle_list_set_option_deps(char *bundle)
{
	if (!bundle) {
		return;
	}
	FREE(cmdline_option_deps);
	cmdline_option_deps = bundle;
	cmdline_local = false;
}

void bundle_list_set_option_status(bool opt)
{
	cmdline_option_status = opt;
}

static void free_has_dep(void)
{
	FREE(cmdline_option_has_dep);
}

static void free_deps(void)
{
	FREE(cmdline_option_deps);
}

static void print_help(void)
{
	print("Lists all installed and installable software bundles\n\n");
	print("Usage:\n");
	print("   swupd bundle-list [OPTIONS...]\n\n");

	global_print_help();

	print("Options:\n");
	print("   -a, --all               List all available bundles for the current version of Clear Linux\n");
	print("   -D, --has-dep=[BUNDLE]  List all bundles which have BUNDLE as a dependency (use --verbose for tree view)\n");
	print("   --deps=[BUNDLE]         List BUNDLE dependencies (use --verbose for tree view)\n");
	print("   --status                Show the installation status of the listed bundles\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "all", no_argument, 0, 'a' },
	{ "deps", required_argument, 0, FLAG_DEPS },
	{ "has-dep", required_argument, 0, 'D' },
	{ "status", no_argument, 0, FLAG_STATUS },
};

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 'a':
		cmdline_option_all = optarg_to_bool(optarg);
		cmdline_local = !cmdline_option_all;
		return true;
	case 'D':
		string_or_die(&cmdline_option_has_dep, "%s", optarg);
		atexit(free_has_dep);
		cmdline_local = false;
		return true;
	case FLAG_DEPS:
		string_or_die(&cmdline_option_deps, "%s", optarg);
		atexit(free_deps);
		cmdline_local = false;
		return true;
	case FLAG_STATUS:
		cmdline_option_status = optarg_to_bool(optarg);
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

	/* flag restrictions */
	if (cmdline_option_status) {
		if (cmdline_option_deps) {
			error("--status and --deps options are mutually exclusive\n");
			return false;
		}
		if (cmdline_option_has_dep) {
			error("--status and --has-dep options are mutually exclusive\n");
			return false;
		}
	}

	return true;
}

/*
 * This function will read the BUNDLES_DIR (by default
 * /usr/share/clear/bundles/), get the list of local bundles and print
 * them sorted.
 */
static enum swupd_code list_local_bundles(int version)
{
	char *name;
	char *path = NULL;
	struct list *bundles = NULL;
	struct list *item = NULL;
	struct manifest *MoM = NULL;
	struct file *bundle_manifest = NULL;
	int count = 0;

	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	if (version > 0) {
		MoM = load_mom(version, NULL);
		if (!MoM) {
			warn("Could not determine which installed bundles are experimental\n\n");
		}
	}

	string_or_die(&path, "%s/%s", globals.path_prefix, BUNDLES_DIR);

	errno = 0;
	bundles = get_dir_files_sorted(path);
	if (!bundles && errno) {
		error("couldn't open bundles directory");
		FREE(path);
		return SWUPD_COULDNT_LIST_DIR;
	}

	progress_next_step("list_bundles", PROGRESS_UNDEFINED);

	info("Installed bundles:\n");
	item = bundles;
	while (item) {
		if (MoM) {
			bundle_manifest = mom_search_bundle(MoM, sys_basename((char *)item->data));
		}
		if (bundle_manifest) {
			name = get_printable_bundle_name(bundle_manifest->filename, bundle_manifest->is_experimental, cmdline_option_status && is_installed_bundle(bundle_manifest->filename), cmdline_option_status && is_tracked_bundle(bundle_manifest->filename));
			info(" - ");
			print("%s\n", name);
			FREE(name);
		} else {
			info(" - ");
			print("%s\n", sys_basename((char *)item->data))
		}
		count++;
		item = item->next;
	}
	info("\nTotal: %d\n", count);

	list_free_list_and_data(bundles, free);

	FREE(path);
	manifest_free(MoM);

	return SWUPD_OK;
}

/* Return recursive list of included bundles */
static enum swupd_code show_included_bundles(char *bundle_name, int version)
{
	enum swupd_code ret_code = SWUPD_OK;
	int ret = 0;
	struct list *bundles = NULL;
	struct list *subs = NULL;
	struct list *iter = NULL;
	struct manifest *mom = NULL;
	struct sub *bundle_sub = NULL;
	bool verbose = (log_get_level() == LOG_INFO_VERBOSE);

	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	info("Loading required manifests...\n");
	mom = load_mom(version, NULL);
	if (!mom) {
		error("Cannot load official manifest MoM for version %i\n", version);
		ret_code = SWUPD_COULDNT_LOAD_MOM;
		goto out;
	}

	// add_subscriptions takes a list, so construct one with only bundle_name
	bundles = list_prepend_data(bundles, bundle_name);
	ret = add_subscriptions(bundles, &subs, mom, true, 0);
	if (ret != add_sub_NEW) {
		// something went wrong or there were no includes, print a message and exit
		if (ret & add_sub_ERR) {
			error("Cannot load included bundles\n");
			ret_code = SWUPD_COULDNT_LOAD_MANIFEST;
		} else if (ret & add_sub_BADNAME) {
			ret_code = SWUPD_INVALID_BUNDLE;
		} else {
			error("Unknown error\n");
			ret_code = SWUPD_UNEXPECTED_CONDITION;
		}
		goto out;
	}

	progress_next_step("list_bundles", PROGRESS_UNDEFINED);

	/* the list of subscription include the bundle indicated by bundle_name and os-core,
	 * if deps has only two bundles in it, no included bundles were found */
	const int MINIMAL_SUBSCRIPTIONS = 2;
	if (str_cmp(bundle_name, "os-core") == 0 || list_len(subs) <= MINIMAL_SUBSCRIPTIONS) {
		info("\nNo included bundles\n");
		goto out;
	}

	info("\nBundles included by %s:\n", bundle_name);

	if (verbose) {
		ret = subscription_get_tree(bundles, &subs, mom, true, 0);
		if (ret != add_sub_NEW && ret != 0) {
			ret_code = SWUPD_COULDNT_LOAD_MANIFEST;
		}
	} else {
		subs = list_sort(subs, cmp_subscription_component);
		iter = list_head(subs);
		while (iter) {
			bundle_sub = iter->data;
			iter = iter->next;
			// subs include the requested bundle, skip it
			if (str_cmp(bundle_name, bundle_sub->component) == 0) {
				continue;
			}
			info(" - ");
			print("%s\n", bundle_sub->component);
		}
	}
	info("\nTotal: %d\n", list_len(subs) - 1);

out:
	list_free_list(bundles);
	manifest_free(mom);
	free_subscriptions(&subs);

	return ret_code;
}

/*
* list_installable_bundles()
* Parse the full manifest for the current version of the OS and print
*   all available bundles.
*/
static enum swupd_code list_installable_bundles(int version)
{
	char *name;
	struct list *list;
	struct file *file;
	struct manifest *MoM = NULL;
	int count = 0;

	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	MoM = load_mom(version, NULL);
	if (!MoM) {
		return SWUPD_COULDNT_LOAD_MOM;
	}

	progress_next_step("list_bundles", PROGRESS_UNDEFINED);
	info("All available bundles:\n");
	list = MoM->manifests = list_sort(MoM->manifests, cmp_file_filename_is_deleted);
	while (list) {
		file = list->data;
		list = list->next;
		name = get_printable_bundle_name(file->filename, file->is_experimental, cmdline_option_status && is_installed_bundle(file->filename), cmdline_option_status && is_tracked_bundle(file->filename));
		info(" - ");
		print("%s\n", name);
		FREE(name);
		count++;
	}
	info("\nTotal: %d\n", count);

	manifest_free(MoM);
	return 0;
}

static enum swupd_code show_bundle_reqd_by(const char *bundle_name, bool server, int version)
{
	int ret = 0;
	struct manifest *current_manifest = NULL;
	struct list *subs = NULL;
	struct list *reqd_by = NULL;
	int number_of_reqd = 0;
	const bool INCLUDE_OPTIONAL_DEPS = true;

	if (!server && !is_installed_bundle(bundle_name)) {
		info("Bundle \"%s\" does not seem to be installed\n", bundle_name);
		info("       try passing --all to check uninstalled bundles\n");
		ret = SWUPD_BUNDLE_NOT_TRACKED;
		goto out;
	}

	progress_next_step("load_manifests", PROGRESS_UNDEFINED);
	info("Loading required manifests...\n");
	current_manifest = load_mom(version, NULL);
	if (!current_manifest) {
		error("Unable to download/verify %d Manifest.MoM\n", version);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto out;
	}

	if (!mom_search_bundle(current_manifest, bundle_name)) {
		warn("Bundle \"%s\" is invalid, skipping it...\n", bundle_name);
		ret = SWUPD_INVALID_BUNDLE;
		goto out;
	}

	if (server) {
		ret = add_included_manifests(current_manifest, &subs);
		if (ret) {
			error("Unable to load server manifest");
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
			goto out;
		}

	} else {
		/* load all tracked bundles into memory */
		read_subscriptions(&subs);
	}

	/* load all submanifests */
	current_manifest->submanifests = recurse_manifest(current_manifest, subs, NULL, server, NULL);
	if (!current_manifest->submanifests) {
		error("Cannot load MoM sub-manifests\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto out;
	}

	progress_next_step("list_bundles", PROGRESS_UNDEFINED);

	char *msg;
	string_or_die(&msg, "\n%s bundles that have %s as a dependency:\n", server ? "All" : "Installed", bundle_name);
	number_of_reqd = required_by(&reqd_by, bundle_name, current_manifest, 0, NULL, msg, INCLUDE_OPTIONAL_DEPS);
	FREE(msg);
	if (reqd_by == NULL) {
		info("\nNo bundles have %s as a dependency\n", bundle_name);
		ret = SWUPD_OK;
		goto out;
	}
	list_free_list_and_data(reqd_by, free);
	info("\nTotal: %d\n", number_of_reqd);

	ret = SWUPD_OK;

out:
	if (current_manifest) {
		manifest_free(current_manifest);
	}

	if (subs) {
		free_subscriptions(&subs);
	}

	return ret;
}

enum swupd_code list_bundles(void)
{
	enum swupd_code ret;
	int version;

	/* if we cannot get the current version, the only list command we can still
	 * attempt is listing locally installed bundles (with the limitation of not
	 * showing what bundles are experimental) */
	version = get_current_version(globals.path_prefix);
	if (version < 0 && !cmdline_local) {
		error("Unable to determine current OS version\n");
		return SWUPD_CURRENT_VERSION_UNKNOWN;
	}

	if (cmdline_local) {
		ret = list_local_bundles(version);
	} else if (cmdline_option_deps != NULL) {
		ret = show_included_bundles(cmdline_option_deps, version);
	} else if (cmdline_option_has_dep != NULL) {
		ret = show_bundle_reqd_by(cmdline_option_has_dep, cmdline_option_all, version);
	} else {
		ret = list_installable_bundles(version);
	}

	return ret;
}

enum swupd_code bundle_list_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_bundlelist = 2;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	if (cmdline_local && !is_root()) {
		ret = swupd_init(SWUPD_NO_ROOT);
	} else {
		ret = swupd_init(SWUPD_ALL);
	}
	if (ret != SWUPD_OK) {
		error("Failed swupd initialization, exiting now\n");
		progress_finish_steps(ret);
		return ret;
	}

	/*
	 * Steps for bundle-list:
	 *  1) load_manifests
	 *  2) list_bundles
	 */
	progress_init_steps("bundle-list", steps_in_bundlelist);

	ret = list_bundles();

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
