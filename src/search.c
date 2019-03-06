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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "swupd.h"

char *search_string;
bool display_files = false; /* Just display all files found in Manifest set */
bool csv_format = false;
bool init = false;

enum sort_type {
	alpha,
	size
};

static char search_type = '0';
static char scope = '0';
static int num_results = INT_MAX;
static int sort = alpha;

/* bundle_result contains the information to print */
struct bundle_result {
	char bundle_name[BUNDLE_NAME_MAXLEN];
	long topsize;
	long size;
	struct list *files;
	struct list *includes;
	bool is_tracked;
	bool is_experimental;
	bool seen;
};

static struct list *results;

/* add a bundle_result to the results list */
static void add_bundle_file_result(char *bundlename, char *filename, bool is_experimental)
{
	struct bundle_result *bundle = NULL;
	struct list *ptr;

	ptr = results;
	while (ptr) {
		struct bundle_result *item;

		item = ptr->data;
		if (strcmp(item->bundle_name, bundlename) == 0) {
			bundle = item;
			break;
		}
		ptr = ptr->next;
	}
	if (!bundle) {
		bundle = calloc(sizeof(struct bundle_result), 1);
		ON_NULL_ABORT(bundle);
		results = list_append_data(results, bundle);
		strncpy(bundle->bundle_name, bundlename, BUNDLE_NAME_MAXLEN - 1);
		/* record if the bundle is installed on the system */
		bundle->is_tracked = is_installed_bundle(bundlename);
		bundle->is_experimental = is_experimental;
	}
	bundle->files = list_append_data(bundle->files, strdup_or_die(filename));
}

static int bundle_cmp(const void *a, const void *b)
{
	const struct bundle_result *A, *B;
	A = (struct bundle_result *)a;
	B = (struct bundle_result *)b;

	/* lexicographical order */
	return strcmp(A->bundle_name, B->bundle_name);
}

static int bundle_size_cmp(const void *a, const void *b)
{
	const struct bundle_result *A, *B;
	A = (struct bundle_result *)a;
	B = (struct bundle_result *)b;

	// smaller before larger
	if (A->size < B->size) {
		return -1;
	} else if (A->size > B->size) {
		return 1;
	} else {
		return 0;
	}
}

static int file_cmp(const void *a, const void *b)
{
	const char *A, *B;
	A = (char *)a;
	B = (char *)b;

	/* lexicographical order */
	return strcmp(A, B);
}

static void sort_results(void)
{
	struct list *ptr;
	struct bundle_result *b;

	results = list_sort(results, bundle_cmp);

	ptr = results;
	while (ptr) {
		b = ptr->data;
		ptr = ptr->next;

		b->files = list_sort(b->files, file_cmp);
	}
}

static void free_bundle_result_data(void *data)
{
	struct bundle_result *br = (struct bundle_result *)data;

	if (!br) {
		return;
	}

	list_free_list_and_data(br->files, free);
	list_free_list_and_data(br->includes, free);
	free(br);
}

/* unmark all bundle_results in bl as seen */
static void unsee_bundle_results(struct list *bl)
{
	struct bundle_result *br;
	struct list *ptr = list_head(bl);

	while (ptr) {
		br = ptr->data;
		br->seen = false;
		ptr = ptr->next;
	}
}

/* recursively calculate size from a complete list of bundle_result structs */
static long calculate_size(char *bname, struct list *bundle_info, bool installed)
{
	struct list *ptr;
	struct bundle_result *bi;
	long size = 0;

	ptr = list_head(bundle_info);
	while (ptr) {
		bi = ptr->data;
		ptr = ptr->next;

		if (bi->seen || strcmp(bname, bi->bundle_name) != 0) {
			continue;
		}

		/* If installed is true the initial call to this recursive
		 * function was for an installed bundle. In this case we want
		 * to calculate the contentsize of all included bundles (we
		 * know they are all installed already) so we can report the
		 * total installed size on the system.
		 *
		 * Otherwise only add the contentsize of bundles not already
		 * installed on the system. */
		if (installed || !is_installed_bundle(bname)) {
			size += bi->topsize;
			bi->seen = true;
		}

		/* add bundle sizes for includes as well */
		struct list *ptr2;
		ptr2 = list_head(bi->includes);
		while (ptr2) {
			char *inc = NULL;
			inc = ptr2->data;
			ptr2 = ptr2->next;
			/* recursively add included sizes */
			size += calculate_size(inc, bundle_info, installed);
		}
	}

	return size;
}

static void add_bundle_size(struct list *bundle_info)
{
	struct bundle_result *b;
	struct list *ptr;

	ptr = list_head(results);

	while (ptr) {
		b = ptr->data;
		ptr = ptr->next;

		/* unsee all bundles before calculating size */
		unsee_bundle_results(bundle_info);

		/* calculate bundle size for recursive includes */
		b->size = calculate_size(b->bundle_name, bundle_info, b->is_tracked);
	}
}

static void print_csv_results()
{
	struct bundle_result *bundle;
	struct list *ptr;

	ptr = list_head(results);
	while (ptr) {
		struct list *fptr;
		char *filename;
		bundle = ptr->data;
		ptr = ptr->next;
		fptr = list_head(bundle->files);
		while (fptr) {
			filename = fptr->data;
			fptr = fptr->next;
			printf("%s,%s\n", filename, bundle->bundle_name);
		}
	}
}

static void print_final_results(bool display_size)
{
	char *name = NULL;
	struct bundle_result *b;
	struct list *ptr;
	int counter = 0;

	ptr = list_head(results);
	if (num_results != INT_MAX) {
		printf("Displaying top %d file results per bundle\n\n", num_results);
	}
	while (ptr) {
		struct list *ptr2;
		char *filename;
		int counter2 = 0;

		b = ptr->data;
		ptr = ptr->next;
		counter++;
		/* do not print the size information when the scope is only one bundle ('o')
		 * because we did not load all bundles and therefore do not have include sizes
		 * for the result */
		name = get_printable_bundle_name(b->bundle_name, b->is_experimental);
		printf("Bundle %s\t%s", name, b->is_tracked ? "[installed]\t" : "");
		free_string(&name);
		if (display_size) {
			printf("(%li MB%s)",
			       b->size / 1000 / 1000, /* convert from bytes->KB->MB */
			       b->is_tracked ? " on system" : " to install");
		}
		putchar('\n');
		ptr2 = list_head(b->files);
		while (ptr2 && counter2 < num_results) {
			filename = ptr2->data;
			ptr2 = ptr2->next;
			printf("\t%s\n", filename);
			counter2++;
		}

		if (ptr2) {
			printf("\tfile results truncated...\n");
		}
		printf("\n");
	}
}

/* Supported default search paths */
static char *lib_paths[] = {
	"/usr/lib",
	NULL
};

static char *bin_paths[] = {
	"/usr/bin/",
	NULL
};

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd search [OPTION...] 'search_term'\n\n");
	fprintf(stderr, "		'search_term': A substring of a binary, library or filename (default)\n");
	fprintf(stderr, "		Return: Bundle name : filename matching search term\n\n");

	global_print_help();

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "   -l, --library           Search paths where libraries are located for a match\n");
	fprintf(stderr, "   -b, --binary            Search paths where binaries are located for a match\n");
	fprintf(stderr, "   -s, --scope=[query type] 'b' or 'o' for first hit per (b)undle, or one hit total across the (o)s\n");
	fprintf(stderr, "   -T, --top=[NUM]         Only display the top NUM results for each bundle\n");
	fprintf(stderr, "   -m, --csv               Output all results in CSV format (machine-readable)\n");
	fprintf(stderr, "   -d, --display-files	   Output full file list, no search done\n");
	fprintf(stderr, "   -i, --init              Download all manifests then return, no search done\n");
	fprintf(stderr, "   -o, --order=[ORDER]     Sort the output. ORDER is one of the following values:\n");
	fprintf(stderr, "                           'alpha' to order alphabetically (default)\n");
	fprintf(stderr, "                           'size' to order by bundle size (smaller to larger)\n");
}

static const struct option prog_opts[] = {
	{ "binary", no_argument, 0, 'b' },
	{ "csv", no_argument, 0, 'm' },
	{ "display-files", no_argument, 0, 'd' },
	{ "init", no_argument, 0, 'i' },
	{ "library", no_argument, 0, 'l' },
	{ "scope", required_argument, 0, 's' },
	{ "top", required_argument, 0, 'T' },
	//TODO: -t option is deprecated. Remove that on a Major release
	{ "order", required_argument, 0, 'o' },
	{ "", required_argument, 0, 't' },
};

static bool parse_opt(int opt, char *optarg)
{
	int err;

	switch (opt) {
	case 'o':
		if (!strcmp(optarg, "alpha")) {
			sort = alpha;
		} else if (!strcmp(optarg, "size")) {
			sort = size;
		} else {
			fprintf(stderr, "Invalid --order argument\n\n");
			return false;
		}
		return true;
	case 's':
		if (strcmp(optarg, "b") && (strcmp(optarg, "o"))) {
			fprintf(stderr, "Invalid --scope argument. Must be 'b' or 'o'\n\n");
			return false;
		}

		if (!strcmp(optarg, "b")) {
			scope = 'b';
		} else if (!strcmp(optarg, "o")) {
			scope = 'o';
		}

		return true;
	case 't':
		fprintf(stderr, "Deprecated option -t was renamed. Prefer using -T or --top.\n\n");
		/* fallthrough */
	case 'T':
		err = strtoi_err(optarg, &num_results);
		if (err != 0) {
			fprintf(stderr, "Invalid --top argument\n\n");
			return false;
		}
		return true;
	case 'm':
		csv_format = true;
		return true;
	case 'l':
		if (search_type != '0') {
			fprintf(stderr, "Error, cannot specify multiple search types "
					"(-l and -b are mutually exclusive)\n");
			return false;
		}

		search_type = 'l';
		return true;
	case 'i':
		init = true;
		return true;
	case 'b':
		if (search_type != '0') {
			fprintf(stderr, "Error, cannot specify multiple search types "
					"(-l and -b are mutually exclusive)\n");
			return false;
		}

		search_type = 'b';
		return true;
	case 'd':
		display_files = true;
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

	if ((optind == argc) && (!init) && (!display_files)) {
		fprintf(stderr, "Error: Search term missing\n\n");
		return false;
	}

	if ((optind == argc - 1) && (display_files)) {
		fprintf(stderr, "Error: Cannot supply a search term and -d, --display-files together\n");
		return false;
	}

	search_string = argv[optind];

	if (optind + 1 < argc) {
		fprintf(stderr, "Error, only 1 search term supported at a time\n");
		return false;
	}

	return true;
}

/* file_search()
 * Attempt to match path and substring of filename. Base op for 'swupd search'
 * Path must match exact case, filename is case insensitive
 */
static bool file_search(char *filename, char *path, char *search_term)
{
	char *pos;

	if (display_files) {
		printf("    %s\n", filename);
		return false;
	}

	if (path) {
		pos = strstr(filename, path);
		if (pos == NULL) {
			return false;
		}
	}

	if (path && search_term) {
		/* match filename or substring of filename */
		if (strcasestr(pos + strlen(path), search_term)) {
			return true;
		}
	}

	return false;
}

/* do_search()
 * Description: Perform a lookup of the specified search string in all Clear manifests
 * for the current os release.
 */
static enum swupd_code do_search(struct manifest *MoM, char search_type, char *search_term)
{
	struct list *list;
	struct list *sublist;
	struct file *file;
	struct file *subfile;
	struct list *bundle_info = NULL;
	struct manifest *subman = NULL;
	int i;
	int ret = SWUPD_OK;
	bool done_with_bundle, done_with_search = false;
	bool hit = false;
	bool man_load_failures = false;
	long hit_count = 0;

	list = MoM->manifests;
	while (list && !done_with_search) {
		file = list->data;
		list = list->next;
		done_with_bundle = false;

		/* Load sub-manifest */
		subman = load_manifest(file->last_change, file, MoM, false, NULL);
		if (!subman) {
			fprintf(stderr, "Failed to load manifest %s\n", file->filename);
			man_load_failures = true;
			continue;
		}

		/* record contentsize and includes for install size calculation */
		struct bundle_result *bundle = NULL;
		bundle = calloc(sizeof(struct bundle_result), 1);
		ON_NULL_ABORT(bundle);

		/* copy relevant information over for future use */
		strncpy(bundle->bundle_name, subman->component, BUNDLE_NAME_MAXLEN - 1);
		bundle->topsize = subman->contentsize;
		/* do a deep copy of the includes list */
		bundle->includes = list_deep_clone_strs(subman->includes);

		bundle_info = list_prepend_data(bundle_info, bundle);

		if (display_files) {
			/* Display bundle name. Marked up for pattern matchability */
			fprintf(stderr, "--Bundle: %s--\n", file->filename);
		}

		/* Loop through sub-manifest, searching for files matching the desired pattern */
		sublist = subman->files;
		while (sublist && !done_with_bundle) {
			subfile = sublist->data;
			sublist = sublist->next;

			if ((!subfile->is_file) && (!subfile->is_link)) {
				continue;
			}

			if (display_files) {
				/* Just display filename */
				file_search(subfile->filename, NULL, NULL);
			} else if (search_type == '0') {
				/* Search for exact match, not path addition */
				if (file_search(subfile->filename, "", search_term)) {
					add_bundle_file_result(file->filename, subfile->filename, file->is_experimental);
					hit = true;
				}
			} else if (search_type == 'l') {
				/* Check each supported library path for a match */
				for (i = 0; lib_paths[i] != NULL; i++) {
					if (file_search(subfile->filename, lib_paths[i], search_term)) {
						add_bundle_file_result(file->filename, subfile->filename, file->is_experimental);
						hit = true;
					}
				}
			} else if (search_type == 'b') {
				/* Check each supported path for binaries */
				for (i = 0; bin_paths[i] != NULL; i++) {
					if (file_search(subfile->filename, bin_paths[i], search_term)) {
						add_bundle_file_result(file->filename, subfile->filename, file->is_experimental);
						hit = true;
					}
				}
			} else {
				fprintf(stderr, "Unrecognized search type. -b or -l supported\n");
				done_with_search = true;
				break;
			}

			/* Determine the level of completion we've reached */
			if (hit) {
				if (scope == 'b') {
					done_with_bundle = true;
				} else if (scope == 'o') {
					done_with_bundle = true;
					done_with_search = true;
				}

				hit_count++;
			}
			hit = false;
		}

		free_manifest(subman);
	}

	if (!hit_count && !display_files) {
		fprintf(stderr, "Search term not found.\n");
		ret = SWUPD_NO;
	}

	bool display_size = (scope != 'o' && !man_load_failures);
	if (display_size) {
		add_bundle_size(bundle_info);
	}
	list_free_list_and_data(bundle_info, free_bundle_result_data);

	if (sort == alpha) {
		/* sort alphabetically */
		sort_results();
	} else if (sort == size) {
		/* sort by bundle size */
		results = list_sort(results, bundle_size_cmp);
	}

	if (csv_format) {
		print_csv_results();
	} else {
		print_final_results(display_size);
	}
	list_free_list_and_data(results, free_bundle_result_data);

	return ret;
}

static double query_total_download_size(struct list *list)
{
	double ret;
	double size_sum = 0;
	struct file *file = NULL;
	char *untard_file = NULL;
	char *url = NULL;

	while (list) {
		file = list->data;
		list = list->next;

		string_or_die(&untard_file, "%s/%i/Manifest.%s", state_dir, file->last_change,
			      file->filename);

		if (access(untard_file, F_OK) == -1) {
			/* Does not exist client-side. Must download */
			string_or_die(&url, "%s/%i/Manifest.%s.tar", content_url,
				      file->last_change, file->filename);

			ret = swupd_curl_query_content_size(url);
			free_string(&url);
			if (ret != -1) {
				/* Convert file size from bytes to MB */
				ret = ret / 1000000;
				size_sum += ret;
			} else {
				free_string(&untard_file);
				return ret;
			}
		}
		free_string(&untard_file);
	}

	return size_sum;
}

/* download_manifests()
 * Description: To search Clear bundles for a particular entry, a complete set of
 *		manifests must be downloaded. This function does so, asynchronously, using
 *		the curl_multi interface */
static enum swupd_code download_manifests(struct manifest **MoM)
{
	struct list *list = NULL;
	struct file *file = NULL;
	char *tarfile, *untard_file, *url = NULL;
	struct manifest *subMan = NULL;
	int current_version;
	int ret = 0;
	int count = 0;
	int manifest_err;
	double size;
	bool did_download = false;

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		return SWUPD_CURRENT_VERSION_UNKNOWN;
	}

	*MoM = load_mom(current_version, false, false, NULL);
	if (!(*MoM)) {
		fprintf(stderr, "Cannot load official manifest MoM for version %i\n", current_version);
		return SWUPD_COULDNT_LOAD_MOM;
	}

	list = (*MoM)->manifests;
	size = query_total_download_size(list);
	if (size == -1) {
		fprintf(stderr, "Downloading manifests. Expect a delay, up to 100MB may be downloaded\n");
	} else if (size > 0) {
		fprintf(stderr, "Downloading Clear Linux manifests\n");
		fprintf(stderr, "   %.2f MB total...\n\n", size);
	}

	while (list) {
		file = list->data;

		string_or_die(&untard_file, "%s/%i/Manifest.%s", state_dir, file->last_change,
			      file->filename);

		string_or_die(&tarfile, "%s/%i/Manifest.%s.tar", state_dir, file->last_change,
			      file->filename);

		if (access(untard_file, F_OK) == -1) {
			/* Do download */
			subMan = load_manifest(file->last_change, file, *MoM, false, &manifest_err);
			if (!subMan) {
				fprintf(stderr, "Cannot load %s sub-manifest for version %i\n", file->filename, current_version);
				count++;
				/* if the manifest failed to download because there is no disk space
				 * remove it from the list so we don't re-attempt while searching */
				if (manifest_err == -EIO) {
					list = list_free_item(list, free_file_data);
				}
				ret = SWUPD_RECURSE_MANIFEST;
				goto out;
			} else {
				did_download = true;
			}

			free_manifest(subMan);
		}

		if (access(untard_file, F_OK) == -1) {
			string_or_die(&url, "%s/%i/Manifest.%s.tar", content_url, current_version,
				      file->filename);

			fprintf(stderr, "Error: Failure reading from %s\n", url);
			free_string(&url);
		}

	out:
		unlink(tarfile);
		free_string(&untard_file);
		free_string(&tarfile);
		list = list->next;
	}

	if (did_download) {
		if (ret) {
			fprintf(stderr, "%i manifest%s failed to download.\n\n", count, (count > 1 ? "s" : ""));
		} else {
			fprintf(stderr, "Completed manifests download.\n\n");
		}
	}

	return ret;
}

enum swupd_code search_main(int argc, char **argv)
{
	int ret = SWUPD_OK;
	struct manifest *MoM = NULL;

	if (!parse_options(argc, argv)) {
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init();
	if (ret != 0) {
		fprintf(stderr, "Failed swupd initialization, exiting now.\n");
		return ret;
	}

	if (!init && !display_files) {
		fprintf(stderr, "Searching for '%s'\n\n", search_string);
	}
	if (display_files) {
		fprintf(stderr, "Displaying all bundles and their files:\n\n");
	}

	ret = download_manifests(&MoM);
	if (ret != 0) {
		if (ret == SWUPD_RECURSE_MANIFEST) {
			fprintf(stderr, "Warning: One or more manifests failed to download, search results will be partial.\n");
		} else {
			fprintf(stderr, "Error: Failed to download manifests\n");
			goto clean_exit;
		}
	}

	if (init) {
		fprintf(stderr, "Successfully retrieved manifests. Exiting\n");
		ret = SWUPD_OK;
		goto clean_exit;
	}

	/* Arbitrary upper limit to ensure we aren't getting handed garbage */
	if (!display_files &&
	    ((strlen(search_string) <= 0) || (strlen(search_string) > NAME_MAX))) {
		fprintf(stderr, "Error - search string invalid\n");
		ret = SWUPD_INVALID_OPTION;
		goto clean_exit;
	}

	if (ret == SWUPD_RECURSE_MANIFEST) {
		/* if not all manifests could be downloaded return SWUPD_RECURSE_MANIFEST */
		do_search(MoM, search_type, search_string);
	} else {
		ret = do_search(MoM, search_type, search_string);
	}

clean_exit:
	if (MoM) {
		free_manifest(MoM);
	}
	swupd_deinit();
	return ret;
}
