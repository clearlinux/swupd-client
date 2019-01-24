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
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"

#define VERIFY_PICKY 1

static char **bundles;

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd bundle-remove [OPTION...] [bundle1 bundle2 (...)]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\n");
}

static const struct global_options opts = {
	NULL,
	0,
	NULL,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	if (argc <= ind) {
		fprintf(stderr, "Error: missing bundle(s) to be removed\n\n");
		return false;
	}

	bundles = argv + ind;

	return true;
}

enum swupd_code bundle_remove_main(int argc, char **argv)
{
	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	return remove_bundles(bundles);
}
