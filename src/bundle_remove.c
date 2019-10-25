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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"

#define VERIFY_PICKY 1

static char **bundles;
static bool cmdline_option_force = false;

static void print_help(void)
{
	print("Usage:\n");
	print("   swupd bundle-remove [OPTION...] [bundle1 bundle2 (...)]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	print("Options:\n");
	print("   -x, --force             Removes a bundle along with all the bundles that depend on it\n");
	print("\n");
}

static const struct option prog_opts[] = {
	{ "force", no_argument, 0, 'x' },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case 'x':
		cmdline_option_force = optarg_to_bool(optarg);
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

	if (argc <= ind) {
		error("missing bundle(s) to be removed\n\n");
		return false;
	}

	bundles = argv + ind;

	return true;
}

enum swupd_code bundle_remove_main(int argc, char **argv)
{
	struct list *bundles_list = NULL;
	int ret;
	/*
	 * Steps for bundle-remove:
	 *
	 *  1) load_manifests
	 *  2) remove_files
	 */
	const int steps_in_bundle_remove = 2;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	progress_init_steps("bundle-remove", steps_in_bundle_remove);
	progress_next_step("load_manifests", PROGRESS_UNDEFINED);

	remove_set_option_force(cmdline_option_force);

	/* move the bundles provided in the command line into a
	 * list so it is easier to handle them */
	for (; *bundles; ++bundles) {
		char *bundle = *bundles;
		bundles_list = list_append_data(bundles_list, bundle);
	}
	bundles_list = list_head(bundles_list);
	ret = remove_bundles(bundles_list);
	list_free_list(bundles_list);

	progress_finish_steps(ret);
	return ret;
}
