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
bool init = false;

static char search_type = '0';
static char scope = '0';

/* bundle_result contains the information to print along with
 * the internal relevancy score used to sort the output */
struct bundle_result {
	char bundle_name[BUNDLE_NAME_MAXLEN];
	long size;
	double score;
	struct list *files;
	bool is_tracked;
};

/* per-file scoring struct */
struct file_result {
	char filename[PATH_MAX + 1];
	double score;
};

static struct list *results;

/* add a bundle_result to the results list */
static void add_bundle_file_result(char *bundlename, char *filename, double score, struct manifest *man)
{
	struct bundle_result *bundle = NULL;
	struct file_result *file;
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
		results = list_append_data(results, bundle);
		strncpy(bundle->bundle_name, bundlename, BUNDLE_NAME_MAXLEN - 1);
		/* calculate bundle size by converting bytes->KB->MB */
		bundle->size = man->contentsize / 1024 / 1024;
		/* Arbitrarily assign an initial negative score based on how large the bundle is.
		 * This is set to negative 1/10th of the bundle size.
		 * NOTE this bundle->size does not include the bundle includes sizes */
		bundle->score = -0.1 * bundle->size;
		/* record if the bundle is tracked on the system */
		bundle->is_tracked = is_tracked_bundle(bundlename);
	}

	file = calloc(sizeof(struct file_result), 1);
	strncpy(file->filename, filename, PATH_MAX);
	bundle->files = list_append_data(bundle->files, file);
	file->score = score;
	bundle->score += score;
}

static int bundle_cmp(const void *a, const void *b)
{
	const struct bundle_result *A, *B;
	A = (struct bundle_result *)a;
	B = (struct bundle_result *)b;
	if (A->score > B->score) {
		return -1;
	} else if (A->score < B->score) {
		return 1;
	} else {
		return 0;
	}
}

static int file_cmp(const void *a, const void *b)
{
	const struct file_result *A, *B;
	A = (struct file_result *)a;
	B = (struct file_result *)b;
	if (A->score > B->score) {
		return -1;
	} else if (A->score < B->score) {
		return 1;
	} else {
		return 0;
	}
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

	list_free_list(br->files);
	free(br);
}

static void print_final_results(void)
{
	struct bundle_result *b;
	struct list *ptr;
	int counter = 0;

	ptr = results;
	/* TODO: make 5 a constant, potentially configurable during build or runtime
	 * we are basically printing the top 5 file matches for the top 5 bundles. */
	while (ptr && counter < 5) {
		struct list *ptr2;
		struct file_result *f;
		int counter2 = 0;

		b = ptr->data;
		ptr = ptr->next;
		counter++;
		printf("Bundle %s\t(%li Mb)%s\n",
		       b->bundle_name,
		       b->size,
		       b->is_tracked ? "\tinstalled" : "");
		ptr2 = b->files;
		while (ptr2 && counter2 < 5) {
			f = ptr2->data;
			ptr2 = ptr2->next;
			printf("\t%s\n", f->filename);
			counter2++;
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

static void print_help(const char *name)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "	swupd %s [Options] 'search_term'\n", basename((char *)name));
	fprintf(stderr, "		'search_term': A substring of a binary, library or filename (default)\n");
	fprintf(stderr, "		Return: Bundle name : filename matching search term\n\n");

	fprintf(stderr, "Help Options:\n");
	fprintf(stderr, "   -h, --help              Display this help\n");
	fprintf(stderr, "   -l, --library           Search paths where libraries are located for a match\n");
	fprintf(stderr, "   -b, --binary            Search paths where binaries are located for a match\n");
	fprintf(stderr, "   -s, --scope=[query type] 'b' or 'o' for first hit per (b)undle, or one hit total across the (o)s\n");
	fprintf(stderr, "   -d, --display-files	   Output full file list, no search done\n");
	fprintf(stderr, "   -i, --init              Download all manifests then return, no search done\n");
	fprintf(stderr, "   -I, --ignore-time       Ignore system/certificate time when validating signature\n");
	fprintf(stderr, "   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	fprintf(stderr, "   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
	fprintf(stderr, "   -v, --versionurl=[URL]  RFC-3986 encoded url for version string download\n");
	fprintf(stderr, "   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
	fprintf(stderr, "   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	fprintf(stderr, "   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	fprintf(stderr, "   -S, --statedir          Specify alternate swupd state directory\n");
	fprintf(stderr, "   -C, --certpath          Specify alternate path to swupd certificates\n");

	fprintf(stderr, "\nResults format:\n");
	fprintf(stderr, " 'Bundle Name'  :  'File matching search term'\n\n");
	fprintf(stderr, "\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "url", required_argument, 0, 'u' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "library", no_argument, 0, 'l' },
	{ "binary", no_argument, 0, 'b' },
	{ "scope", required_argument, 0, 's' },
	{ "port", required_argument, 0, 'P' },
	{ "path", required_argument, 0, 'p' },
	{ "format", required_argument, 0, 'F' },
	{ "init", no_argument, 0, 'i' },
	{ "ignore-time", no_argument, 0, 'I' },
	{ "display-files", no_argument, 0, 'd' },
	{ "statedir", required_argument, 0, 'S' },
	{ "certpath", required_argument, 0, 'C' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hu:c:v:P:p:F:s:lbiIdS:C:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(0);
		case 'u':
			if (!optarg) {
				fprintf(stderr, "error: invalid --url argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			set_content_url(optarg);
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
		case 'P':
			if (sscanf(optarg, "%ld", &update_server_port) != 1) {
				fprintf(stderr, "Invalid --port argument\n\n");
				goto err;
			}
			break;
		case 'p': /* default empty path_prefix verifies the running OS */
			if (!optarg || !set_path_prefix(optarg)) {
				fprintf(stderr, "Invalid --path argument\n\n");
				goto err;
			}
			break;
		case 's':
			if (!optarg || (strcmp(optarg, "b") && (strcmp(optarg, "o")))) {
				fprintf(stderr, "Invalid --scope argument. Must be 'b' or 'o'\n\n");
				goto err;
			}

			if (!strcmp(optarg, "b")) {
				scope = 'b';
			} else if (!strcmp(optarg, "o")) {
				scope = 'o';
			}

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
		case 'l':
			if (search_type != '0') {
				fprintf(stderr, "Error, cannot specify multiple search types "
						"(-l and -b are mutually exclusive)\n");
				goto err;
			}

			search_type = 'l';
			break;
		case 'i':
			init = true;
			break;
		case 'I':
			timecheck = false;
			break;
		case 'b':
			if (search_type != '0') {
				fprintf(stderr, "Error, cannot specify multiple search types "
						"(-l and -b are mutually exclusive)\n");
				goto err;
			}

			search_type = 'b';
			break;
		case 'd':
			display_files = true;
			break;
		case 'C':
			if (!optarg) {
				fprintf(stderr, "Invalid --certpath argument\n\n");
				goto err;
			}
			set_cert_path(optarg);
			break;
		default:
			fprintf(stderr, "Error: unrecognized option: -'%c',\n\n", opt);
			goto err;
		}
	}

	if ((optind == argc) && (!init) && (!display_files)) {
		fprintf(stderr, "Error: Search term missing\n\n");
		print_help(argv[0]);
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

err:
	print_help(argv[0]);
	return false;
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

double guess_score(char *bundle, char *file, char *search_term)
{
	double multiplier = 1.0;
	double score = 1.0;
	char dirname[PATH_MAX];
	char *bname;

	strcpy(dirname, file);
	// calculate basename
	bname = strrchr(dirname, '/');
	// set dirname to be just the dirname
	*bname = '\0';
	bname++;

	/* The following score and multiplier transformations are experimental
	 * heuristics and should be updated over time based on results and
	 * feedback from swupd search usage */

	/* dev bundles are less important */
	if (strstr(bundle, "-dev")) {
		multiplier /= 10;
	}

	/* files in /usr/bin are more important */
	if (strcmp(dirname, "/usr/bin") == 0) {
		score += 5;
	}

	/* files in /usr/include are not as important as other files */
	if (strcmp(dirname, "/usr/include") == 0) {
		multiplier /= 2;
	}

	/* if the search term is in the bundle name this is probably the bundle the
	 * user is looking for. Give it a slight bump. */
	if (strstr(bundle, search_term)) {
		score += 0.5;
	}

	/* slightly favor shorter filenames */
	score += (2.0 / strlen(file));

	/* if the search term matches the basename give it a substantial score bump */
	if (strcmp(bname, search_term) == 0) {
		multiplier *= 5;
	}

	return multiplier * score;
}

/* report_finds()
 * Report out, respecting verbosity
 */
static void report_find(char *bundle, char *file, char *search_term, struct manifest *man)
{
	double score;

	score = guess_score(bundle, file, search_term);
	//	printf("'%s'  :  '%s'   (%5.1f)\n", bundle, file, score);
	add_bundle_file_result(bundle, file, score, man);
}

/* do_search()
 * Description: Perform a lookup of the specified search string in all Clear manifests
 * for the current os release.
 */
static void do_search(struct manifest *MoM, char search_type, char *search_term)
{
	struct list *list;
	struct list *sublist;
	struct file *file;
	struct file *subfile;
	struct manifest *subman = NULL;
	int i;
	bool done_with_bundle, done_with_search = false;
	bool hit = false;
	long hit_count = 0;

	list = MoM->manifests;
	while (list && !done_with_search) {
		file = list->data;
		list = list->next;
		done_with_bundle = false;

		/* Load sub-manifest */
		subman = load_manifest(file->last_change, file->last_change, file, MoM, false);
		if (!subman) {
			fprintf(stderr, "Failed to load manifest %s\n", file->filename);
			continue;
		}

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
					report_find(file->filename, subfile->filename, search_term, subman);
					hit = true;
				}
			} else if (search_type == 'l') {
				/* Check each supported library path for a match */
				for (i = 0; lib_paths[i] != NULL; i++) {
					if (file_search(subfile->filename, lib_paths[i], search_term)) {
						report_find(file->filename, subfile->filename, search_term, subman);
						hit = true;
					}
				}
			} else if (search_type == 'b') {
				/* Check each supported path for binaries */
				for (i = 0; bin_paths[i] != NULL; i++) {
					if (file_search(subfile->filename, bin_paths[i], search_term)) {
						report_find(file->filename, subfile->filename, search_term, subman);
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

	if (!hit_count) {
		fprintf(stderr, "Search term not found.\n");
	}
	sort_results();
	print_final_results();
	list_free_list_and_data(results, free_bundle_result_data);
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

			ret = swupd_query_url_content_size(url);
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
static int download_manifests(struct manifest **MoM, struct list **subs)
{
	struct list *list = NULL;
	struct file *file = NULL;
	char *tarfile, *untard_file, *url = NULL;
	struct manifest *subMan = NULL;
	int current_version;
	int ret = 0;
	double size;
	bool did_download = false;

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		fprintf(stderr, "Error: Unable to determine current OS version\n");
		return ECURRENT_VERSION;
	}

	swupd_curl_set_current_version(current_version);

	*MoM = load_mom(current_version, false, false);
	if (!(*MoM)) {
		fprintf(stderr, "Cannot load official manifest MoM for version %i\n", current_version);
		return EMOM_NOTFOUND;
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
		list = list->next;

		create_and_append_subscription(subs, file->filename);

		string_or_die(&untard_file, "%s/%i/Manifest.%s", state_dir, file->last_change,
			      file->filename);

		string_or_die(&tarfile, "%s/%i/Manifest.%s.tar", state_dir, file->last_change,
			      file->filename);

		if (access(untard_file, F_OK) == -1) {
			/* Do download */
			subMan = load_manifest(current_version, file->last_change, file, *MoM, false);
			if (!subMan) {
				fprintf(stderr, "Cannot load official manifest MoM for version %i\n", current_version);
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

		unlink(tarfile);
		free_string(&untard_file);
		free_string(&tarfile);
	}

	if (did_download) {
		fprintf(stderr, "Completed manifests download.\n\n");
	}

	return ret;
}

int search_main(int argc, char **argv)
{
	int ret = 0;
	int lock_fd = 0;
	struct manifest *MoM = NULL;
	struct list *subs = NULL;

	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		fprintf(stderr, "Failed swupd initialization, exiting now.\n");
		return ret;
	}

	if (swupd_curl_check_network()) {
		fprintf(stderr, "Error: Network issue, unable to proceed with update\n");
		ret = ENOSWUPDSERVER;
		goto clean_exit;
	}

	if (!init) {
		fprintf(stderr, "Searching for '%s'\n\n", search_string);
	}

	ret = download_manifests(&MoM, &subs);
	if (ret != 0) {
		fprintf(stderr, "Error: Failed to download manifests\n");
		goto clean_exit;
	}

	if (init) {
		fprintf(stderr, "Successfully retreived manifests. Exiting\n");
		ret = 0;
		goto clean_exit;
	}

	/* Arbitrary upper limit to ensure we aren't getting handed garbage */
	if (!display_files &&
	    ((strlen(search_string) <= 0) || (strlen(search_string) > NAME_MAX))) {
		fprintf(stderr, "Error - search string invalid\n");
		ret = EXIT_FAILURE;
		goto clean_exit;
	}

	do_search(MoM, search_type, search_string);

clean_exit:
	free_manifest(MoM);
	swupd_deinit(lock_fd, &subs);
	return ret;
}
