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

static bool use_prefix = false;
static int log_level = LOG_INFO;

static struct option opts[] = {
	{ "no-xattrs", 0, NULL, 'n' },
	{ "path", 1, NULL, 'p' },
	{ "help", 0, NULL, 'h' },
	{ "quiet", no_argument, &log_level, LOG_ERROR },
	{ "debug", no_argument, &log_level, LOG_DEBUG },
	{ 0, 0, NULL, 0 }
};

static void usage(const char *name)
{
	print("Usage:\n");
	print("   swupd %s [OPTION...] filename\n\n", basename((char *)name));
	print("Help Options:\n");
	print("   -h, --help              Show help options\n\n");
	print("Application Options:\n");
	print("   -n, --no-xattrs         Ignore extended attributes\n");
	print("   -p, --path=[PATH...]    Use [PATH...] for leading path to filename\n");
	print("\n");
	print("The filename is the name of a file on the filesystem.\n");
	print("\n");
}

enum swupd_code hashdump_main(int argc, char **argv)
{
	struct file file = { 0 };
	char *fullname = NULL;
	int ret;

	file.use_xattrs = true;

	while (1) {
		int c;

		c = getopt_long(argc, argv, "np:h", opts, NULL);
		if (c == -1) {
			break;
		}

		/* if an option that doesn't have a short option like --quiet or --debug
		 * was selected getopt_long returns 0 */
		if (c == 0) {
			set_log_level(log_level);
			continue;
		}

		switch (c) {
		case 'n':
			file.use_xattrs = false;
			break;
		case 'p':
			if (!set_path_prefix(optarg)) {
				error("Invalid --path/-p argument\n\n");
				return SWUPD_INVALID_OPTION;
			}
			use_prefix = true;
			break;
		case 'h':
			usage(argv[0]);
			return SWUPD_OK;
		default:
			usage(argv[0]);
			return SWUPD_INVALID_OPTION;
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
		return SWUPD_INVALID_OPTION;
	}

	if (!set_path_prefix(NULL)) {
		return SWUPD_INIT_GLOBALS_FAILED;
	}

	file.filename = strdup_or_die(argv[optind]);
	// Accept relative paths if no path_prefix set on command line
	if (use_prefix) {
		fullname = mk_full_filename(path_prefix, file.filename);
	} else {
		fullname = strdup_or_die(file.filename);
	}

	info("Calculating hash %s xattrs for: %s\n",
	     (file.use_xattrs ? "with" : "without"), fullname);

	populate_file_struct(&file, fullname);
	ret = compute_hash(&file, fullname);
	if (ret != 0) {
		warn("compute_hash() failed\n");
	} else {
		print("%s\n", file.hash);
		if (file.is_dir && is_directory_mounted(fullname)) {
			warn("!! dumped hash might not match a manifest "
			     "hash because a mount is active\n");
		}
	}

	free_string(&fullname);
	free_string(&file.filename);
	return SWUPD_OK;
}
