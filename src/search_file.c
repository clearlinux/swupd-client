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
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *         Brad Peters <brad.t.peters@intel.com>
 *
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

#define FLAG_REGEXP 2000

/*
 * TODO:
 * - Support exact match
 * - Improve bundle total size algorithm by moving total size to manifest struct
 */

enum sort_type {
	SORT_TYPE_ALPHA_BUNDLES_ONLY,
	SORT_TYPE_ALPHA,
	SORT_TYPE_SIZE
};

enum search_type {
	SEARCH_TYPE_ALL = 0,
	SEARCH_TYPE_LIB = 1,
	SEARCH_TYPE_BIN = 2,
};

struct bundle_size {
	const char *bundle_name;
	long size;
};

// Parameters
static char *search_string = NULL;
static bool csv_format = false;
static bool init = false;
static enum search_type search_type = SEARCH_TYPE_ALL;
static bool cmdline_option_library = false;
static bool cmdline_option_binary = false;
static int num_results = INT_MAX;
static int sort = SORT_TYPE_ALPHA_BUNDLES_ONLY;
static bool regexp = false;
static regex_t regexp_comp;

// Context
static struct list *bundle_size_cache = NULL;
static struct list *manifest_list = NULL;

static int bundle_cmp(const void *bundle, const void *bundle_name)
{
	const struct bundle_size *b = bundle;

	return strcmp(b->bundle_name, bundle_name);
}

static int manifest_name_cmp(const void *a, const void *b)
{
	const struct manifest *ma = a;
	const struct manifest *mb = b;

	return strcmp(ma->component, mb->component);
}

static int manifest_str_cmp(const void *manifest, const void *manifest_name)
{
	const struct manifest *m = manifest;

	return strcmp(m->component, manifest_name);
}

static int process_full_include_list(struct manifest *m, struct list **include_list)
{
	struct manifest *sub;
	struct list *l;

	for (l = m->includes; l; l = l->next) {
		const char *bundle_name = l->data;

		if (list_search(*include_list, bundle_name, manifest_str_cmp) != NULL) {
			continue;
		}

		sub = list_search(manifest_list, bundle_name, manifest_str_cmp);
		if (!sub) {
			return -1;
		}

		*include_list = list_prepend_data(*include_list, sub);
		if (process_full_include_list(sub, include_list) < 0) {
			return -1;
		}
	}

	return 0;
}

static long compute_bundle_size(const char *bundle_name)
{
	struct manifest *m;
	struct list *include_list = NULL;
	struct list *l;
	long size;
	bool is_installed;

	m = list_search(manifest_list, bundle_name, manifest_str_cmp);
	if (!m) {
		return -1;
	}

	if (process_full_include_list(m, &include_list) < 0) {
		return -1;
	}

	is_installed = is_installed_bundle(bundle_name);
	size = m->contentsize;

	for (l = include_list; l; l = l->next) {
		struct manifest *sub = l->data;

		if (is_installed == is_installed_bundle(sub->component)) {
			size += sub->contentsize;
		}
	}

	list_free_list(include_list);

	return size;
}

static long get_bundle_size(const char *bundle)
{
	struct bundle_size *b;

	b = list_search(bundle_size_cache, bundle, bundle_cmp);
	if (b) {
		return b->size;
	}

	b = calloc(1, sizeof(struct bundle_size));
	ON_NULL_ABORT(b);
	b->bundle_name = bundle;
	b->size = compute_bundle_size(bundle);
	bundle_size_cache = list_prepend_data(bundle_size_cache, b);

	return b->size;
}

static int manifest_size_cmp(const void *a, const void *b)
{
	long sa, sb;
	const struct manifest *ma = a;
	const struct manifest *mb = b;

	sa = get_bundle_size(ma->component);
	sb = get_bundle_size(mb->component);

	// smaller before larger
	if (sa < sb) {
		return -1;
	}

	if (sa > sb) {
		return 1;
	}

	return 0;
}

/* Supported default search paths */
static const char *lib_paths[] = {
	"/usr/lib",
	NULL
};

static const char *bin_paths[] = {
	"/usr/bin/",
	NULL
};

static void print_bundle(struct manifest *mom, struct manifest *m)
{
	char *name;
	bool installed;
	struct file *bundle_file;
	bool is_experimental;

	if (csv_format) {
		return;
	}

	bundle_file = mom_search_bundle(mom, m->component);
	is_experimental = bundle_file ? bundle_file->is_experimental : false;
	installed = is_installed_bundle(m->component);

	name = get_printable_bundle_name(m->component, is_experimental);
	print("\nBundle %s %s", name, installed ? "[installed] " : "");
	free_string(&name);

	print("(%li MB%s)", get_bundle_size(m->component) / 1000 / 1000,
	      installed ? " on system" : " to install");
	print("\n");
}

static void print_result(const char *bundle_name, const char *filename)
{
	if (csv_format) {
		print("%s,%s\n", filename, bundle_name);
	} else {
		print("\t%s\n", filename);
	}
}

static bool match_path_prefix(const char *path, const char *path_list[])
{
	int i;

	for (i = 0; path_list[i] != NULL; i++) {
		if (strncmp(path, path_list[i], strlen(path_list[i])) == 0) {
			return true;
		}
	}
	return false;
}

static bool is_path_in_search_type(const char *filename)
{
	bool found = false;

	if (search_type == SEARCH_TYPE_ALL) {
		return true;
	}

	if (search_type & SEARCH_TYPE_LIB) {
		found = match_path_prefix(filename, lib_paths);
	}

	if (!found && (search_type & SEARCH_TYPE_BIN)) {
		found = match_path_prefix(filename, bin_paths);
	}

	return found;
}

static bool file_matches(const char *filename, const char *search_term)
{
	if (!regexp) {
		return strcasestr(filename, search_term) != NULL;
	}

	return regexec(&regexp_comp, filename, 0, NULL, 0) == 0;
}

static int search_in_manifest(struct manifest *mom, struct manifest *manifest, const char *search_term)
{
	struct list *l;
	int count = 0;
	struct list *files = NULL;

	for (l = manifest->files; l; l = l->next) {
		struct file *file = l->data;

		if (!file->is_file && !file->is_link) {
			continue;
		}

		if (file->is_deleted) {
			continue;
		}

		if (!is_path_in_search_type(file->filename)) {
			continue;
		}

		if (file_matches(file->filename, search_term)) {
			if (count == 0) {
				print_bundle(mom, manifest);
			}
			count++;

			// Print now or save to sort and print later?
			if (sort == SORT_TYPE_ALPHA) {
				files = list_prepend_data(files, file->filename);
			} else {
				if (count > num_results) {
					break;
				}
				print_result(manifest->component, file->filename);
			}
		}
	}

	// Sort and print results if postponed
	if (sort == SORT_TYPE_ALPHA) {
		files = list_sort(files, list_strcmp);
		for (l = files; l; l = l->next) {
			if (count > num_results) {
				break;
			}
			print_result(manifest->component, l->data);
		}
	}

	list_free_list(files);

	return count;
}

static int do_search(struct manifest *mom, const char *search_term)
{
	struct list *l;
	int ret = 0;
	int found = 0;
	int err;
	int regexp_err;

	if (sort == SORT_TYPE_ALPHA_BUNDLES_ONLY || sort == SORT_TYPE_ALPHA) {
		manifest_list = list_sort(manifest_list, manifest_name_cmp);
	} else if (sort == SORT_TYPE_SIZE) {
		// pre process the bundle size of all bundles for sorting
		for (l = manifest_list; l; l = l->next) {
			struct manifest *m = l->data;
			get_bundle_size(m->component);
		}
		manifest_list = list_sort(manifest_list, manifest_size_cmp);
	}

	if (regexp) {
		regexp_err = regcomp(&regexp_comp, search_term, REG_NOSUB | REG_EXTENDED);
		if (regexp_err) {
			print_regexp_error(regexp_err, &regexp_comp);
			goto error;
		}
	}

	if (num_results != INT_MAX) {
		info("Displaying only top %d file results per bundle\n", num_results);
		info("File results may be truncated\n");
	}

	for (l = manifest_list; l; l = l->next) {
		struct manifest *manifest = l->data;
		err = search_in_manifest(mom, manifest, search_term);

		// Save error, but don't abort
		if (err < 0) {
			ret = err;
		} else {
			found += err;
		}
	}

	if (regexp) {
		regfree(&regexp_comp);
	}

error:
	return ret < 0 ? ret : found;
}

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd search [OPTION...] 'search_term'\n\n");
	print("		'search_term': A substring of a binary, library or filename (default)\n");
	print("		Return: Bundle name : filename matching search term\n\n");

	global_print_help();

	print("Options:\n");
	print("   -l, --library           Search for a match of the given file in the directories where libraries are located\n");
	print("   -B, --binary            Search for a match of the given file in the directories where binaries are located\n");
	print("   -T, --top=[NUM]         Only display the top NUM results for each bundle\n");
	print("   -m, --csv               Output all results in CSV format (machine-readable)\n");
	print("   -i, --init              Download all manifests then return, no search done\n");
	print("   -o, --order=[ORDER]     Sort the output. ORDER is one of the following values:\n");
	print("                           'alpha' to order alphabetically (default)\n");
	print("                           'size' to order by bundle size (smaller to larger)\n");
}

static const struct option prog_opts[] = {
	{ "binary", no_argument, 0, 'B' },
	{ "csv", no_argument, 0, 'm' },
	{ "init", no_argument, 0, 'i' },
	{ "library", no_argument, 0, 'l' },
	{ "order", required_argument, 0, 'o' },
	{ "top", required_argument, 0, 'T' },
	{ "regexp", no_argument, 0, FLAG_REGEXP },
};

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'o':
		if (!strcmp(optarg, "alpha")) {
			sort = SORT_TYPE_ALPHA;
		} else if (!strcmp(optarg, "size")) {
			sort = SORT_TYPE_SIZE;
		} else {
			error("Invalid --order argument\n");
			return false;
		}
		return true;
	case 'T':
		err = strtoi_err(optarg, &num_results);
		if (err != 0) {
			error("Invalid --top argument\n");
			return false;
		}
		return true;
	case 'm':
		csv_format = optarg_to_bool(optarg);
		return true;
	case 'l':
		cmdline_option_library = optarg_to_bool(optarg);
		return true;
	case 'i':
		init = optarg_to_bool(optarg);
		return true;
	case 'B':
		cmdline_option_binary = optarg_to_bool(optarg);
		return true;
	case FLAG_REGEXP:
		regexp = true;
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

	if (cmdline_option_library) {
		search_type |= SEARCH_TYPE_LIB;
	}
	if (cmdline_option_binary) {
		search_type |= SEARCH_TYPE_BIN;
	}

	search_string = optind < argc ? argv[optind] : "";

	/* Arbitrary upper limit to ensure we aren't getting handed garbage */
	if (strlen(search_string) > PATH_MAX) {
		error("Search string is too long\n");
		return false;
	}

	if (optind + 1 < argc) {
		error("Only 1 search term supported at a time\n");
		return false;
	}

	return true;
}

enum swupd_code search_file_main(int argc, char **argv)
{

	int ret = SWUPD_OK;
	int err = 0;
	struct manifest *mom = NULL;
	int current_version;
	int steps_in_search = 3;

	/* there is no need to report in progress for search at this time */
	/*
	 * Steps for search:
	 *
	 * 1) get_versions
	 * 2) load_manifests
	 * (Finishes here on --init)
	 * 3) search_term
	 */

	if (!parse_options(argc, argv)) {
		return SWUPD_INVALID_OPTION;
	}
	if (init) {
		/* if user selected the --init option the number of steps in the
		 * search process are just 2 */
		steps_in_search = 2;
	}
	progress_init_steps("search", steps_in_search);

	ret = swupd_init(SWUPD_ALL);
	if (ret != 0) {
		error("Failed swupd initialization, exiting now\n");
		goto exit;
	}

	progress_set_step(1, "get_versions");
	current_version = get_current_version(globals.path_prefix);
	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		return SWUPD_CURRENT_VERSION_UNKNOWN;
	}
	progress_complete_step();

	progress_set_step(2, "load_manifests");
	mom = load_mom(current_version, false, NULL);
	if (!mom) {
		error("Cannot load official manifest MoM for version %i\n", current_version);
		return SWUPD_COULDNT_LOAD_MOM;
	}

	info("Downloading all Clear Linux manifests\n");
	err = mom_get_manifests_list(mom, &manifest_list, NULL);
	if (init) {
		if (err != 0) {
			error("Failed to download manifests\n");
			ret = SWUPD_RECURSE_MANIFEST;
		} else {
			info("Successfully retrieved manifests. Exiting\n");
			ret = SWUPD_OK;
		}
		goto clean_exit;
	}

	if (err) {
		int failed_count = list_len(mom->manifests) - list_len(manifest_list);
		warn("Failed to download %i manifest%s - search results will be partial\n", failed_count, (failed_count > 1 ? "s" : ""));
		ret = SWUPD_RECURSE_MANIFEST;
	}

	progress_set_step(3, "search_term");
	info("Searching for '%s'\n", search_string);
	err = do_search(mom, search_string);
	progress_complete_step();

	// Keep any ret error code if search is successful
	if (ret == SWUPD_OK && err == 0) {
		info("Search term not found\n");
		ret = SWUPD_NO;
	}

clean_exit:
	manifest_free(mom);
	list_free_list_and_data(manifest_list, manifest_free_data);
	list_free_list_and_data(bundle_size_cache, free);

	swupd_deinit();

exit:
	progress_finish_steps(ret);
	return ret;
}
