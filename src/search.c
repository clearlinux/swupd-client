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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>

#include "swupd.h"
#include "config.h"

char *search_string;
char search_type = '0';
bool display_all = false;   /* Show all hits, or simplify output and show just first bundle match */
bool display_files = false; /* Just display all files found in Manifest set */
bool init = false;

/* Supported default search paths */
char *lib_paths[] = {
	"/usr/lib",
	NULL
};

char *bin_paths[] = {
	"/usr/bin/",
	NULL
};

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("	swupd %s [Options] 'search_term'\n", basename((char *)name));
	printf("		'search_term': A substring of a binary, library or filename (default)\n");
	printf("		Return: Bundle name : filename matching search term\n\n");

	printf("Help Options:\n");
	printf("   -h, --help              Display this help\n");
	printf("   -l, --library           Search paths where libraries are located for a match\n");
	printf("   -b, --binary            Search paths where binaries are located for a match\n");
	printf("   -e, --everywhere        Search system-wide for a match\n");
	printf("   -a, --all               Display all matches. Default is to show the first only\n");
	printf("   -d, --display-files	   Output full file list, no search done\n");
	printf("   -i, --init              Download all manifests then return, no search done\n");
	printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	printf("\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "url", required_argument, 0, 'u' },
	{ "library", no_argument, 0, 'l' },
	{ "binary", no_argument, 0, 'b' },
	{ "everywhere", no_argument, 0, 'e' },
	{ "path", required_argument, 0, 'p' },
	{ "format", required_argument, 0, 'F' },
	{ "init", no_argument, 0, 'i' },
	{ "all", no_argument, 0, 'a' },
	{ "display-files", no_argument, 0, 'd' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	set_format_string(NULL);

	while ((opt = getopt_long(argc, argv, "hu:p:F:lbeiad", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(0);
		case 'u':
			if (!optarg) {
				printf("error: invalid --url argument\n\n");
				goto err;
			}
			if (version_server_urls[0]) {
				free(version_server_urls[0]);
			}
			if (content_server_urls[0]) {
				free(content_server_urls[0]);
			}
			string_or_die(&version_server_urls[0], "%s", optarg);
			string_or_die(&content_server_urls[0], "%s", optarg);

			break;
		case 'p': /* default empty path_prefix verifies the running OS */
			if (!optarg) {
				printf("Invalid --path argument\n\n");
				goto err;
			}
			if (path_prefix) {
				/* multiple -p options */
				free(path_prefix);
			}
			string_or_die(&path_prefix, "%s", optarg);

			break;
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				printf("Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'l':
			if (search_type != '0') {
				printf("Error, cannot specify multiple search types "
				       "(-l, -b, and -e are mutually exclusive)\n");
				goto err;
			}

			search_type = 'l';
			break;
		case 'i':
			init = true;
			break;
		case 'b':
			if (search_type != '0') {
				printf("Error, cannot specify multiple search types "
				       "(-l, -b, and -e are mutually exclusive)\n");
				goto err;
			}

			search_type = 'b';
			break;
		case 'e':
			if (search_type != '0') {
				printf("Error, cannot specify multiple search types "
				       "(-l, -b, and -e are mutually exclusive)\n");
				goto err;
			}

			search_type = 'e';
			break;
		case 'a':
			display_all = true;
			break;
		case 'd':
			display_files = true;
			break;
		default:
			printf("Error: unrecognized option: -'%c',\n\n", opt);
			goto err;
		}
	}

	if ((optind == argc) && (!init) && (!display_files)) {
		printf("Error: Search term missing\n\n");
		print_help(argv[0]);
		return false;
	}

	search_string = argv[optind];

	if (optind + 1 < argc) {
		printf("Error, only 1 search term supported at a time\n");
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
bool file_search(char *filename, char *path, char *search_term)
{
	char *pos;

	if (display_files) {
		printf("    %s\n", filename);
		return false;
	}

	pos = strstr(filename, path);
	if (pos == NULL) {
		return false;
	}

	/* match filename or substring of filename */
	if (strcasestr(pos + strlen(path), search_term)) {
		return true;
	}

	return false;
}

/* report_find()
 * Report out, respecting verbosity
 */
void report_find(char *bundle, char *file)
{
	printf("'%s'  :  '%s'\n", bundle, file);
}

/* do_search()
 * Description: Perform a lookup of the specified search string in all Clear manifests
 * for the current os release.
 */
void do_search(struct manifest *MoM, char search_type, char *search_term)
{
	struct list *list;
	struct list *sublist;
	struct file *file;
	struct file *subfile;
	struct manifest *subman = NULL;
	int i, ret;
	bool done_with_bundle, done = false;
	bool hit = false;

	list = MoM->manifests;
	while (list && !done) {
		file = list->data;
		list = list->next;
		done_with_bundle = false;

		/* Load sub-manifest */
		ret = load_manifests(file->last_change, file->last_change, file->filename, NULL, &subman);
		if (ret != 0) {
			printf("Failed to load manifest %s\n", file->filename);
			continue;
		}

		if (display_files) {
			/* Display bundle name. Marked up for pattern matchability */
			printf("--Bundle: %s--\n", file->filename);
		}

		/* Loop through sub-manifest, searching for files matching the desired pattern */
		sublist = subman->files;
		while (sublist && !done_with_bundle) {
			subfile = sublist->data;
			sublist = sublist->next;

			if (!subfile->is_file) {
				continue;
			}

			if (display_files) {
				/* Just display filename */
				file_search(subfile->filename, NULL, NULL);
			} else if (search_type == 'e') {
				/* Search for exact match, not path addition */
				if (file_search(subfile->filename, "", search_term)) {
					report_find(file->filename, subfile->filename);
					hit = true;
					if (!display_all) {
						done = true;
					}

					done_with_bundle = true;
					break;
				}
			} else if (search_type == 'l') {
				/* Check each supported library path for a match */
				for (i = 0; lib_paths[i] != NULL; i++) {
					if (file_search(subfile->filename, lib_paths[i], search_term)) {
						report_find(file->filename, subfile->filename);
						hit = true;
						if (!display_all) {
							done = true;
						}

						done_with_bundle = true;
						break;
					}
				}
			} else if (search_type == 'b') {
				/* Check each supported path for binaries */
				for (i = 0; bin_paths[i] != NULL; i++) {
					if (file_search(subfile->filename, bin_paths[i], search_term)) {
						report_find(file->filename, subfile->filename);
						hit = true;
						if (!display_all) {
							done = true;
						}

						done_with_bundle = true;
						break;
					}
				}
			} else {
				printf("Unrecognized search type. -b or -l supported\n");
				done = true;
				break;
			}
		}

		free_manifest(subman);
	}

	if (!hit) {
		printf("Search term not found.\n");
	}
}

static double query_total_download_size(struct list *list)
{
	double ret;
	double size_sum = 0;
	struct file *file;
	char *untard_file, *url;

	while (list) {
		file = list->data;
		list = list->next;

		string_or_die(&untard_file, "%s/%i/Manifest.%s", STATE_DIR, file->last_change,
			      file->filename);

		if (access(untard_file, F_OK) == -1) {
			/* Does not exist client-side. Must download */
			string_or_die(&url, "%s/%i/Manifest.%s.tar", preferred_content_url,
				      file->last_change, file->filename);

			ret = swupd_query_url_content_size(url);
			if (ret != -1) {
				/* Convert file size from bytes to MB */
				ret = ret / 1000000;
				size_sum += ret;
			} else {
				return ret;
			}
		}
	}

	free(untard_file);
	return size_sum;
}

/* download_manifests()
 * Description: To search Clear bundles for a particular entry, a complete set of
 *		manifests must be downloaded. This function does so, asynchronously, using
 *		the curl_multi interface */
int download_manifests(struct manifest **MoM)
{
	struct list *list;
	struct file *file;
	char *tarfile, *untard_file, *url;
	struct manifest *subMan = NULL;
	int current_version;
	int ret = 0;
	double size;

	current_version = read_version_from_subvol_file(path_prefix);
	swupd_curl_set_current_version(current_version);

	ret = load_manifests(current_version, current_version, "MoM", NULL, MoM);
	if (ret != 0) {
		printf("Cannot load official manifest MoM for version %i\n", current_version);
		return ret;
	}

	subscription_versions_from_MoM(*MoM, 0);

	list = (*MoM)->manifests;
	size = query_total_download_size(list);
	if (size == -1) {
		printf("Downloading manifests. Expect a delay, up to 100MB may be downloaded\n");
	} else if (size > 0) {
		printf("Downloading Clear Linux manifests\n");
		printf("   %.2f MB total...\n\n", size);
	}

	while (list) {
		file = list->data;
		list = list->next;

		create_and_append_subscription(file->filename);

		string_or_die(&untard_file, "%s/%i/Manifest.%s", STATE_DIR, file->last_change,
			      file->filename);

		string_or_die(&tarfile, "%s/%i/Manifest.%s.tar", STATE_DIR, file->last_change,
			      file->filename);

		if (access(untard_file, F_OK) == -1) {
			/* Do download */
			printf(" '%s' manifest...\n", file->filename);

			ret = load_manifests(current_version, file->last_change, file->filename, NULL, &subMan);
			if (ret != 0) {
				printf("Cannot load official manifest MoM for version %i\n", current_version);
			}

			free_manifest(subMan);
		}

		if (access(untard_file, F_OK) == -1) {
			string_or_die(&url, "%s/%i/Manifest.%s.tar", preferred_content_url, current_version,
				      file->filename);

			printf("Error: Failure reading from %s\n", url);
			free(url);
		}

		unlink(tarfile);
		free(untard_file);
		free(tarfile);
	}

	return ret;
}

int search_main(int argc, char **argv)
{
	int ret = 0;
	int lock_fd = 0;
	struct manifest *MoM = NULL;

	if (!parse_options(argc, argv) ||
	    create_required_dirs()) {
		return EXIT_FAILURE;
	}

	if (search_type == '0') {
		/* Default to a system-side search */
		search_type = 'e';
	}

	if (!init_globals()) {
		ret = EINIT_GLOBALS;
		goto clean_exit;
	}

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		printf("Failed swupd initialization, exiting now.\n");
		goto clean_exit;
	}

	if (!check_network()) {
		printf("Error: Network issue, unable to proceed with update\n");
		ret = EXIT_FAILURE;
		goto clean_exit;
	}

	if (!init) {
		printf("Searching for '%s'\n\n", search_string);
	}

	ret = download_manifests(&MoM);
	if (ret != 0) {
		printf("Error: Failed to download manifests\n");
		goto clean_exit;
	}

	if (init) {
		printf("Successfully retreived manifests. Exiting\n");
		ret = 0;
		goto clean_exit;
	}

	/* Arbitrary upper limit to ensure we aren't getting handed garbage */
	if (!display_files &&
	    ((strlen(search_string) <= 0) || (strlen(search_string) > NAME_MAX))) {
		printf("Error - search string invalid\n");
		ret = EXIT_FAILURE;
		goto clean_exit;
	}

	do_search(MoM, search_type, search_string);

clean_exit:
	free_manifest(MoM);
	free_globals();
	swupd_curl_cleanup();
	v_lockfile(lock_fd);

	return ret;
}
