/*
 *   Software Updater - client side
 *
 *      Copyright © 2013-2016 Intel Corporation.
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
 *         Timothy C. Pepper <timothy.c.pepper@linux.intel.com>
 *         cguiraud <christophe.guiraud@intel.com>
 *
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "swupd.h"

/* outputs the hash of a file */

static struct option opts[] = {
	{ "no-xattrs", 0, NULL, 'n' },
	{ "basepath", 1, NULL, 'b' },
	{ "help", 0, NULL, 'h' },
	{ 0, 0, NULL, 0 }
};

static void usage(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [OPTION...] filename\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n\n");
	printf("Application Options:\n");
	printf("   -n, --no-xattrs         Ignore extended attributes\n");
	printf("   -b, --basepath          Optional argument for leading path to filename\n");
	printf("\n");
	printf("The filename is the name as it would appear in a Manifest file.\n");
	printf("\n");
}

int hashdump_main(int argc, char **argv)
{
	struct file *file;
	char *fullname;
	int ret;

	file = calloc(1, sizeof(struct file));
	if (!file) {
		abort();
	}

	file->use_xattrs = true;

	while (1) {
		int c;
		int i;

		c = getopt_long(argc, argv, "nb:h", opts, &i);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'n':
			file->use_xattrs = false;
			break;
		case 'b':
			if (!optarg) {
				printf("Invalid --path argument\n\n");
				free(file);
				return EXIT_FAILURE;
			}
			set_path_prefix(optarg);
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
			break;
		default:
			usage(argv[0]);
			exit(-1);
			break;
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
		exit(-1);
	}

	// mk_full_filename expects absolute filenames (eg: from Manifest)
	if (argv[optind][0] == '/') {
		file->filename = strdup(argv[optind]);
		if (!file->filename) {
			abort();
		}
	} else {
		string_or_die(&file->filename, "/%s", argv[optind]);
	}

	set_path_prefix(NULL);

	printf("Calculating hash %s xattrs for: (%s) ... %s\n",
	       (file->use_xattrs ? "with" : "without"), path_prefix, file->filename);
	fullname = mk_full_filename(path_prefix, file->filename);
	printf("fullname=%s\n", fullname);
	populate_file_struct(file, fullname);
	ret = compute_hash(file, fullname);
	if (ret != 0) {
		printf("compute_hash() failed\n");
	} else {
		printf("%s\n", file->hash);
		if (file->is_dir) {
			if (is_directory_mounted(fullname)) {
				printf("!! dumped hash might not match a manifest hash because a mount is active\n");
			}
		}
	}

	free(fullname);
	free(file->filename);
	free(file);
	return 0;
}
