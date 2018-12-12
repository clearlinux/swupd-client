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
#include <unistd.h>

#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define VERIFY_NOPICKY 0

static char **bundles;

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd bundle-add [OPTION...] [bundle1 bundle2 (...)]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "   --skip-diskspace-check  Do not check free disk space before adding bundle\n");
	fprintf(stderr, "\n");
}

static const struct option prog_opts[] = {
	{ "list", no_argument, 0, 'l' },
	{ "skip-diskspace-check", no_argument, &skip_diskspace_check, 1 },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case 'l':
		fprintf(stderr, "Error: [-l, --list] option is deprecated, use\n"
				"bundle-list [-a|--all] sub-command instead.\n\n");
		exit(EXIT_FAILURE);
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

	if (argc <= optind) {
		fprintf(stderr, "Error: missing bundle(s) to be installed\n\n");
		return false;
	}

	bundles = argv + optind;

	return true;
}

int bundle_add_main(int argc, char **argv)
{
	if (!parse_options(argc, argv)) {
		print_help();
		return EINVALID_OPTION;
	}

	return install_bundles_frontend(bundles);
}
