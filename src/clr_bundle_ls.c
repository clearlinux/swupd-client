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

#include "config.h"
#include "swupd.h"

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [options]\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n");
	printf("   -a, --all               List all available bundles for the current version of Clear Linux\n");
	printf("\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h'},
	{ "all",  no_argument, 0, 'a' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "ha", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'a':
			return list_installable_bundles();
			break;
		default:
			printf("error: unrecognized option\n\n");
			goto err;
		}
	}

	return true;
err:
	print_help(argv[0]);
	return false;
}

int bundle_list_main(int argc, char **argv)
{
	int current_version;
	int lock_fd;
	int ret;
	struct list *list_bundles = NULL;
	struct list *item = NULL;

	copyright_header("bundle list");

	if (!parse_options(argc, argv)) {
		return EXIT_FAILURE;
	}

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		printf("Error: Failed updater initialization. Exiting now\n");
		return ret;
	}

	current_version = get_current_version(path_prefix);
	if (current_version < 0) {
		printf("Error: Unable to determine current OS version\n");
		v_lockfile(lock_fd);
		return ECURRENT_VERSION;
	}

	read_local_bundles(&list_bundles);

	item = list_head(list_bundles);

	while (item) {
		printf("%s\n", (char *)item->data);
		item = item->next;
	}

	printf("Current OS version: %d\n", current_version);

	list_free_list(list_bundles);

	return ret;
}
