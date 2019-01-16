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
 *         Mario Alfredo Carrillo Arevalo <mario.alfredo.c.arevalo@intel.com>
 *         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <libgen.h>
#include <string.h>

#include "config.h"
#include "swupd.h"

static bool cmdline_option_all = false;
static char *cmdline_option_has_dep = NULL;
static char *cmdline_option_deps = NULL;
static bool cmdline_local = true;

static void free_has_dep(void)
{
	free_string(&cmdline_option_has_dep);
}

static void free_deps(void)
{
	free_string(&cmdline_option_deps);
}

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd bundle-list [OPTIONS...]\n\n");

	//TODO: Add documentation explaining this command

	global_print_help();

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "   -a, --all               List all available bundles for the current version of Clear Linux\n");
	fprintf(stderr, "   -d, --deps=[BUNDLE]     List bundles included by BUNDLE\n");
	fprintf(stderr, "   -D, --has-dep=[BUNDLE]  List dependency tree of all bundles which have BUNDLE as a dependency\n");
	fprintf(stderr, "\n");
}

static const struct option prog_opts[] = {
	{ "all", no_argument, 0, 'a' },
	{ "deps", required_argument, 0, 'd' },
	{ "has-dep", required_argument, 0, 'D' },
};

static bool parse_opt(int opt, char *optarg)
{
	switch (opt) {
	case 'a':
		cmdline_option_all = true;
		cmdline_local = false;
		return true;
	case 'D':
		string_or_die(&cmdline_option_has_dep, "%s", optarg);
		atexit(free_has_dep);
		cmdline_local = false;
		return true;
	case 'd':
		string_or_die(&cmdline_option_deps, "%s", optarg);
		atexit(free_deps);
		cmdline_local = false;
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

	if (argc > ind) {
		fprintf(stderr, "Error: unexpected arguments\n\n");
		return false;
	}

	return true;
}

int bundle_list_main(int argc, char **argv)
{
	int ret;

	if (!parse_options(argc, argv)) {
		print_help();
		return EXIT_FAILURE;
	}

	ret = swupd_init();
	/* if swupd fails to initialize, the only list command we can still attempt is
	 * listing locally installed bundles (with the limitation of not showing what
	 * bundles are experimental) */
	if (ret != 0 && !cmdline_local) {
		fprintf(stderr, "Error: Failed updater initialization. Exiting now\n");
		return ret;
	}

	if (cmdline_local) {
		ret = list_local_bundles();
	} else if (cmdline_option_deps != NULL) {
		ret = show_included_bundles(cmdline_option_deps);
	} else if (cmdline_option_has_dep != NULL) {
		ret = show_bundle_reqd_by(cmdline_option_has_dep, cmdline_option_all);
	} else {
		ret = list_installable_bundles();
	}

	swupd_deinit();

	return ret;
}
