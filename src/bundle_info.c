/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2019 Intel Corporation.
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
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "swupd.h"

#define FLAG_DEPENDENCIES 2000
#define FLAG_FILES 2001

static bool cmdline_option_dependencies = false;
static bool cmdline_option_files = false;
static int cmdline_option_version = 0;
static char *bundle;

void bundle_info_set_option_version(int opt)
{
	cmdline_option_version = opt;
}

void bundle_info_set_option_dependencies(bool opt)
{
	cmdline_option_dependencies = opt;
}

void bundle_info_set_option_files(bool opt)
{
	cmdline_option_files = opt;
}

static void print_help(void)
{
	print("Displays detailed information about a bundle\n\n");
	print("Usage:\n");
	print("   swupd bundle-info [OPTIONS...] BUNDLE\n\n");

	global_print_help();

	print("Options:\n");
	print("   -V, --version=V  Show the bundle info for the specified version V (current by default), also accepts 'latest'\n");
	print("   --dependencies   Show the bundle dependencies\n");
	print("   --files          Show the files installed by this bundle\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "version", required_argument, 0, 'V' },
	{ "dependencies", no_argument, 0, FLAG_DEPENDENCIES },
	{ "files", no_argument, 0, FLAG_FILES },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	int err;

	switch (opt) {
	case 'V':
		if (strcmp("latest", optarg) == 0) {
			cmdline_option_version = -1;
			return true;
		}
		err = strtoi_err(optarg, &cmdline_option_version);
		if (err < 0 || cmdline_option_version < 0) {
			error("Invalid --version argument: %s\n\n", optarg);
			return false;
		}
		return true;
	case FLAG_DEPENDENCIES:
		cmdline_option_dependencies = optarg_to_bool(optarg);
		return true;
	case FLAG_FILES:
		cmdline_option_files = optarg_to_bool(optarg);
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

	if (argc <= optind) {
		error("Please specify the bundle you wish to display information from\n\n");
		return false;
	}

	if (optind + 1 < argc) {
		error("Please specify only one bundle at a time\n\n");
		return false;
	}

	bundle = *(argv + optind);

	return true;
}

static void print_bundle_dependencies(struct manifest *manifest, struct list *indirect_includes)
{
	struct list *iter;
	struct sub *sub;
	char *dep;

	/* print direct includes/also-add first */
	if (manifest->includes || manifest->optional) {
		info("\nDirect dependencies (%d):\n", list_len(manifest->includes) + list_len(manifest->optional));
		for (iter = list_head(manifest->includes); iter; iter = iter->next) {
			dep = iter->data;
			info(" - ");
			print("%s\n", dep);
		}
		for (iter = list_head(manifest->optional); iter; iter = iter->next) {
			dep = iter->data;
			/* optional dependencies can be installed or not, so we
			 * can also provide this info to the user, this only make
			 * sense if the requested bundle is also installed */
			info(" - ");
			print("%s", dep);
			if (is_installed_bundle(manifest->component)) {
				info(" (optional, %s)", is_installed_bundle(dep) ? "installed" : "not installed");
			} else {
				info(" (optional)");
			}
			print("\n");
		}
	}

	/* now print indirect includes (if any) */
	if (indirect_includes) {
		info("\nIndirect dependencies (%d):\n", list_len(indirect_includes));
		for (iter = list_head(indirect_includes); iter; iter = iter->next) {
			sub = iter->data;
			info(" - ");
			print("%s", sub->component);
			if (sub->is_optional) {
				if (is_installed_bundle(manifest->component)) {
					info(" (optional, %s)", is_installed_bundle(sub->component) ? "installed" : "not installed");
				} else {
					info(" (optional)");
				}
			}
			print("\n");
		}
	}
}

static void print_bundle_files(struct list *files)
{
	struct list *iter;
	struct file *file;
	long count = 0;

	info("\nFiles in bundle");
	if (cmdline_option_dependencies) {
		info(" (includes dependencies):\n");
	} else {
		info(":\n");
	}

	for (iter = list_head(files); iter; iter = iter->next, count++) {
		file = iter->data;
		info(" - ");
		print("%s\n", file->filename);
	}

	info("\nTotal files: %ld\n", count);
}

static void print_bundle_size(struct manifest *manifest, long size, bool bundle_installed)
{
	char *pretty_size;

	info("\nBundle size:\n");
	prettify_size(manifest->contentsize, &pretty_size);
	info(" - Size of bundle: %s\n", pretty_size);
	free_string(&pretty_size);

	prettify_size(size, &pretty_size);
	if (bundle_installed) {
		info(" - Size bundle takes on disk (includes dependencies): %s\n", pretty_size);
	} else {
		info(" - Maximum amount of disk size the bundle will take if installed (includes dependencies): %s\n", pretty_size);
	}
	free_string(&pretty_size);
}

static long get_bundle_size(struct manifest *mom, bool bundle_installed)
{
	long bundle_size;
	struct list *bundles_not_installed = NULL;
	struct list *iter;
	struct manifest *manifest;
	char *dep;

	if (bundle_installed) {
		/* if the bundle is installed, get the size the bundle
		 * takes on disk (considering its dependencies) */
		bundle_size = get_manifest_list_contentsize(mom->submanifests);
	} else {
		/* if the bundle is not installed, get the max size it
		 * would take on disk if installed */
		for (iter = list_head(mom->submanifests); iter; iter = iter->next) {
			manifest = iter->data;
			dep = manifest->component;
			if (!is_installed_bundle(dep)) {
				bundles_not_installed = list_prepend_data(bundles_not_installed, manifest);
			}
		}
		bundle_size = get_manifest_list_contentsize(bundles_not_installed);
		list_free_list(bundles_not_installed);
	}

	return bundle_size;
}

static enum swupd_code get_bundle_files(struct manifest *manifest, struct manifest *mom, struct list **files)
{

	/* when the --includes flag is also used, the list needs to include
	 * the files from all bundles pulled in by the requested bundle */
	if (cmdline_option_dependencies) {
		*files = consolidate_files_from_bundles(mom->submanifests);
	} else {
		*files = list_clone(manifest->files);
	}
	*files = filter_out_deleted_files(*files);

	return SWUPD_OK;
}

static enum swupd_code get_bundle_dependencies(struct manifest *manifest, struct list *subs, struct list **indirect_includes, const char *bundle)
{
	struct list *iter;
	struct sub *sub;
	char *included_bundle;

	/* build a list of dependencies */
	for (iter = list_head(subs); iter; iter = iter->next) {
		sub = iter->data;
		included_bundle = sub->component;
		/* we don't need to add the requested bundle to the list */
		if (strcmp(included_bundle, bundle) == 0) {
			continue;
		}
		*indirect_includes = list_append_data(*indirect_includes, sub);
	}

	/* from the list of dependencies, remove those that were
	 * directly pulled by the bundle so we are left only with the ones
	 * indirectly pulled */
	*indirect_includes = list_sort(*indirect_includes, subscription_sort_component);
	manifest->includes = list_sort(manifest->includes, strcmp_wrapper);
	manifest->optional = list_sort(manifest->optional, strcmp_wrapper);
	*indirect_includes = list_sorted_filter_common_elements(*indirect_includes, manifest->includes, subscription_bundlename_strcmp, NULL);
	*indirect_includes = list_sorted_filter_common_elements(*indirect_includes, manifest->optional, subscription_bundlename_strcmp, NULL);

	return SWUPD_OK;
}

enum swupd_code bundle_info(char *bundle)
{
	enum swupd_code ret = SWUPD_OK;
	int requested_version, latest_version;
	struct manifest *mom = NULL;
	struct manifest *latest_mom = NULL;
	struct manifest *manifest = NULL;
	struct manifest *latest_manifest = NULL;
	struct file *file = NULL;
	struct list *subs = NULL;
	long bundle_size;
	bool mix_exists;
	bool installed = is_installed_bundle(bundle);
	bool tracked = is_tracked_bundle(bundle);

	/* get the very latest version available */
	latest_version = get_latest_version(NULL);
	if (latest_version < 0) {
		error("Unable to determine the server version\n");
		return SWUPD_SERVER_CONNECTION_ERROR;
	}

	/* if no version was specified,
	 * get the version the system is currently at */
	if (cmdline_option_version == 0) {
		requested_version = get_current_version(globals.path_prefix);
		if (requested_version < 0) {
			error("Unable to determine current OS version\n");
			return SWUPD_CURRENT_VERSION_UNKNOWN;
		}
	} else if (cmdline_option_version == -1) {
		/* user selected "latest" */
		requested_version = latest_version;
	} else {
		/* user specified an arbitrary version */
		requested_version = cmdline_option_version;
	}

	mix_exists = (check_mix_exists() & system_on_mix());

	/* get the MoM for the requested version */
	mom = load_mom(requested_version, mix_exists, NULL);
	if (!mom) {
		error("Cannot load official manifest MoM for version %i\n", requested_version);
		return SWUPD_COULDNT_LOAD_MOM;
	}

	/* validate the bundle provided and get its manifest */
	file = mom_search_bundle(mom, bundle);
	if (!file) {
		error("Bundle \"%s\" is invalid\n", bundle);
		ret = SWUPD_INVALID_BUNDLE;
		goto clean;
	}
	manifest = load_manifest(file->last_change, file, mom, false, NULL);
	if (!manifest) {
		error("Unable to download the manifest for %s version %d, exiting now\n", bundle, file->last_change);
		ret = SWUPD_COULDNT_LOAD_MANIFEST;
		goto clean;
	}

	/* if there is a newer version of the bundle, get the manifests */
	if (requested_version != latest_version) {
		/* get mom from the latest version */
		latest_mom = load_mom(latest_version, mix_exists, NULL);
		if (!latest_mom) {
			error("Cannot load official manifest MoM for version %i\n", latest_version);
			ret = SWUPD_COULDNT_LOAD_MOM;
			goto clean;
		}

		/* get the latest version of the bundle manifest */
		file = mom_search_bundle(latest_mom, bundle);
		if (file) {
			latest_manifest = load_manifest(file->last_change, file, latest_mom, false, NULL);
			if (!latest_manifest) {
				error("Unable to download the manifest for %s version %d, exiting now\n", bundle, file->last_change);
				ret = SWUPD_COULDNT_LOAD_MANIFEST;
				goto clean;
			}
		} else {
			debug("Bundle \"%s\" is no longer available at version %d\n", bundle, latest_version);
		}
	}

	/* get all bundles pulled in by the requested bundle */
	struct list *bundle_temp = NULL;
	bundle_temp = list_prepend_data(bundle_temp, bundle);
	ret = add_subscriptions(bundle_temp, &subs, mom, true, 0);
	list_free_list(bundle_temp);
	if (ret & add_sub_ERR) {
		ret = SWUPD_COULDNT_LOAD_MANIFEST;
		goto clean;
	} else if (ret & add_sub_BADNAME) {
		ret = SWUPD_INVALID_BUNDLE;
		goto clean;
	} else {
		/* add_subscriptions return add_sub_NEW when successful,
		 * convert it to a swupd_code */
		ret = SWUPD_OK;
	}
	set_subscription_versions(mom, NULL, &subs);
	mom->submanifests = recurse_manifest(mom, subs, NULL, false, NULL);

	/* print header */
	char *header = NULL;
	string_or_die(&header, " Info for bundle: %s", bundle);
	print_header(header);
	free_string(&header);

	/* status info */
	char *status = NULL;
	if (tracked) {
		string_or_die(&status, "Explicitly installed");
	} else if (installed) {
		string_or_die(&status, "Installed");
	}
	info("Status: %s%s\n", status ? status : "Not installed", file->is_experimental ? " (experimental)" : "");
	free_string(&status);

	/* version info */
	if (installed) {
		if (manifest->version < file->last_change) {
			info("\nThere is an update for bundle %s:\n", bundle);
		} else {
			info("\nBundle %s is up to date:\n", bundle);
		}
		info(" - Installed bundle last updated in version: %d\n", manifest->version);
		info(" - ");
	}
	info("Latest available version: %d\n", file->last_change);

	/* size info */
	bundle_size = get_bundle_size(mom, installed);
	print_bundle_size(manifest, bundle_size, installed);

	/* optional info */

	/* the --includes flag was used */
	if (cmdline_option_dependencies) {

		struct list *indirect_includes = NULL;
		ret = get_bundle_dependencies(manifest, subs, &indirect_includes, bundle);
		if (ret) {
			error("Could not get all bundles included by %s\n", bundle);
			goto clean;
		}
		print_bundle_dependencies(manifest, indirect_includes);
		list_free_list(indirect_includes);
	}

	/* the --files flag was used */
	if (cmdline_option_files) {

		struct list *bundle_files = NULL;
		ret = get_bundle_files(manifest, mom, &bundle_files);
		if (ret) {
			error("Could not get all files included by %s\n", bundle);
			goto clean;
		}
		print_bundle_files(bundle_files);
		list_free_list(bundle_files);
	}

	info("\n");

clean:
	free_subscriptions(&subs);
	manifest_free(latest_manifest);
	manifest_free(latest_mom);
	manifest_free(manifest);
	manifest_free(mom);

	return ret;
}

enum swupd_code bundle_info_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_bundleinfo = 0;
	/* there is no need to report in progress for bundle-info at this time */

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

	progress_init_steps("bundle-info", steps_in_bundleinfo);

	ret = bundle_info(bundle);

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
