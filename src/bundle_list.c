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

#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <string.h>

#include "config.h"
#include "swupd.h"

#define FLAG_DEPS 2000

static bool cmdline_option_all = false;
static char *cmdline_option_has_dep = NULL;
static char *cmdline_option_deps = NULL;
static bool cmdline_local = true;

static void free_has_dep(void)
{
	free_string(&cmdline_option_has_dep);
}

static void free_deps(void)
{
	free_string(&cmdline_option_deps);
}

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd bundle-list [OPTIONS...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	print("Options:\n");
	print("   -a, --all               List all available bundles for the current version of Clear Linux\n");
	print("   -D, --has-dep=[BUNDLE]  List all bundles which have BUNDLE as a dependency\n");
	print("   --deps=[BUNDLE]         List bundles included by BUNDLE\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "all", no_argument, 0, 'a' },
	{ "deps", required_argument, 0, FLAG_DEPS },
	{ "has-dep", required_argument, 0, 'D' },
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

/*
 * This function will read the BUNDLES_DIR (by default
 * /usr/share/clear/bundles/), get the list of local bundles and print
 * them sorted.
 */
static enum swupd_code list_local_bundles()
{
	char *name;
	char *path = NULL;
	struct list *bundles = NULL;
	struct list *item = NULL;
	struct manifest *MoM = NULL;
	struct file *bundle_manifest = NULL;
	int current_version;
	bool mix_exists;
	int count = 0;

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		goto skip_mom;
	}

	mix_exists = (check_mix_exists() & system_on_mix());
	MoM = load_mom(current_version, mix_exists, NULL);
	if (!MoM) {
		warn("Could not determine which installed bundles are experimental\n");
	}

skip_mom:
	string_or_die(&path, "%s/%s", globals.path_prefix, BUNDLES_DIR);

	errno = 0;
	bundles = get_dir_files_sorted(path);
	if (!bundles && errno) {
		error("couldn't open bundles directory");
		free_string(&path);
		return SWUPD_COULDNT_LIST_DIR;
	}

	info("Installed bundles:\n");
	item = bundles;
	while (item) {
		if (MoM) {
			bundle_manifest = mom_search_bundle(MoM, sys_basename((char *)item->data));
		}
		if (bundle_manifest) {
			name = get_printable_bundle_name(bundle_manifest->filename, bundle_manifest->is_experimental);
			info(" - ");
			print("%s\n", name);
			free(name);
		} else {
			info(" - ");
			print("%s\n", sys_basename((char *)item->data));
		}
		count++;
		item = item->next;
	}
	info("\nTotal: %d\n", count);

	list_free_list_and_data(bundles, free);

	free_string(&path);
	manifest_free(MoM);

	return SWUPD_OK;
}

/* Return recursive list of included bundles */
static enum swupd_code show_included_bundles(char *bundle_name)
{
	int ret = 0;
	int current_version = CURRENT_OS_VERSION;
	struct list *subs = NULL;
	struct list *deps = NULL;
	struct manifest *mom = NULL;
	int count = 0;

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto out;
	}

	mom = load_mom(current_version, false, NULL);
	if (!mom) {
		error("Cannot load official manifest MoM for version %i\n", current_version);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto out;
	}

	// add_subscriptions takes a list, so construct one with only bundle_name
	struct list *bundles = NULL;
	bundles = list_prepend_data(bundles, bundle_name);
	ret = add_subscriptions(bundles, &subs, mom, true, 0);
	list_free_list(bundles);
	if (ret != add_sub_NEW) {
		// something went wrong or there were no includes, print a message and exit
		char *m = NULL;
		if (ret & add_sub_ERR) {
			string_or_die(&m, "Processing error");
			ret = SWUPD_COULDNT_LOAD_MANIFEST;
		} else if (ret & add_sub_BADNAME) {
			string_or_die(&m, "Bad bundle name detected");
			ret = SWUPD_INVALID_BUNDLE;
		} else {
			string_or_die(&m, "Unknown error");
			ret = SWUPD_UNEXPECTED_CONDITION;
		}

		error("%s - Aborting\n", m);
		free_string(&m);
		goto out;
	}
	deps = recurse_manifest(mom, subs, NULL, false, NULL);
	if (!deps) {
		error("Cannot load included bundles\n");
		ret = SWUPD_RECURSE_MANIFEST;
		goto out;
	}

	/* deps now includes the bundle indicated by bundle_name
	 * if deps only has one bundle in it, no included packages were found */
	if (list_len(deps) == 1) {
		info("No included bundles\n");
		ret = SWUPD_OK;
		goto out;
	}

	info("Bundles included by %s:\n", bundle_name);

	struct list *iter;
	iter = list_head(deps);
	while (iter) {
		struct manifest *included_bundle = iter->data;
		iter = iter->next;
		// deps includes the bundle_name bundle, skip it
		if (strcmp(bundle_name, included_bundle->component) == 0) {
			continue;
		}

		info(" - ");
		print("%s\n", included_bundle->component);
		count++;
	}
	info("\nTotal: %d\n", count);

	ret = SWUPD_OK;

out:
	if (mom) {
		manifest_free(mom);
	}

	if (deps) {
		list_free_list_and_data(deps, manifest_free_data);
	}

	if (subs) {
		free_subscriptions(&subs);
	}

	return ret;
}

/*
* list_installable_bundles()
* Parse the full manifest for the current version of the OS and print
*   all available bundles.
*/
static enum swupd_code list_installable_bundles()
{
	char *name;
	struct list *list;
	struct file *file;
	struct manifest *MoM = NULL;
	int current_version;
	int count;
	bool mix_exists;

	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		return SWUPD_CURRENT_VERSION_UNKNOWN;
	}

	mix_exists = (check_mix_exists() & system_on_mix());
	MoM = load_mom(current_version, mix_exists, NULL);
	if (!MoM) {
		return SWUPD_COULDNT_LOAD_MOM;
	}

	info("All available bundles:\n");
	list = MoM->manifests = list_sort(MoM->manifests, file_sort_filename);
	while (list) {
		file = list->data;
		list = list->next;
		name = get_printable_bundle_name(file->filename, file->is_experimental);
		info(" - ");
		print("%s\n", name);
		free_string(&name);
		count++;
	}
	info("\nTotal: %d\n", count);

	manifest_free(MoM);
	return 0;
}

static enum swupd_code show_bundle_reqd_by(const char *bundle_name, bool server)
{
	int ret = 0;
	int version = CURRENT_OS_VERSION;
	struct manifest *current_manifest = NULL;
	struct list *subs = NULL;
	struct list *reqd_by = NULL;
	int number_of_reqd = 0;

	if (!server && !is_installed_bundle(bundle_name)) {
		info("Bundle \"%s\" does not seem to be installed\n", bundle_name);
		info("       try passing --all to check uninstalled bundles\n");
		ret = SWUPD_BUNDLE_NOT_TRACKED;
		goto out;
	}

	version = get_current_version(globals.path_prefix);
	if (version < 0) {
		error("Unable to determine current OS version\n");
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
		goto out;
	}

	current_manifest = load_mom(version, false, NULL);
	if (!current_manifest) {
		error("Unable to download/verify %d Manifest.MoM\n", version);
		ret = SWUPD_COULDNT_LOAD_MOM;
		goto out;
	}

	if (!mom_search_bundle(current_manifest, bundle_name)) {
		error("Bundle name %s is invalid, aborting dependency list\n", bundle_name);
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

	char *msg;
	string_or_die(&msg, "%s bundles that have %s as a dependency:\n", server ? "All" : "Installed", bundle_name);
	number_of_reqd = required_by(&reqd_by, bundle_name, current_manifest, 0, NULL, msg);
	free_string(&msg);
	if (reqd_by == NULL) {
		info("No bundles have %s as a dependency\n", bundle_name);
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

	if (ret) {
		print("Bundle list failed\n");
	}

	if (subs) {
		free_subscriptions(&subs);
	}

	return ret;
}

enum swupd_code bundle_list_main(int argc, char **argv)
{
	int ret;
	const int steps_in_bundlelist = 1;

	/* there is no need to report in progress for bundle-list at this time */

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("bundle-list", steps_in_bundlelist);

	if (cmdline_local && !is_root()) {
		ret = swupd_init(SWUPD_NO_ROOT);
	} else {
		ret = swupd_init(SWUPD_ALL);
	}
	/* if swupd fails to initialize, the only list command we can still attempt is
	 * listing locally installed bundles (with the limitation of not showing what
	 * bundles are experimental) */
	if (ret != 0) {
		error("Failed updater initialization. Exiting now\n");
		goto finish;
	}

	if (cmdline_local) {
		ret = list_local_bundles();
	} else if (cmdline_option_deps != NULL) {
		ret = show_included_bundles(cmdline_option_deps);
	} else if (cmdline_option_has_dep != NULL) {
		ret = show_bundle_reqd_by(cmdline_option_has_dep, cmdline_option_all);
	} else {
		ret = list_installable_bundles();
	}

	swupd_deinit();

finish:
	progress_finish_steps(ret);
	return ret;
}
