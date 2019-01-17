/*
 *   Software Updater - client side
 *
 *      Copyright Â© 2012-2016 Intel Corporation.
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
 *
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lib/log.h"
#include "swupd.h"
#include "verifytime.h"

static void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd check-update [OPTION...]\n\n");
	//TODO: Add documentation explaining this command

	global_print_help();
}

/* Return 0 if there is an update available, nonzero if not */
static int check_update()
{
	int current_version, server_version;
	int ret;

	check_root();
	/* Check that our system time is reasonably valid before continuing,
	* or the certificate verification will fail with invalid time */
	if (timecheck) {
		if (!verify_time()) {
			return SWUPD_BAD_TIME;
		}
	}
	if (!init_globals()) {
		return SWUPD_INIT_GLOBALS_FAILED;
	}
	swupd_curl_init();

	ret = read_versions(&current_version, &server_version, path_prefix);
	if (ret != 0) {
		return ret;
	} else {
		info("Current OS version: %d\n", current_version);
		if (current_version < server_version) {
			info("There is a new OS version available: %d\n", server_version);
			update_motd(server_version);
			return SWUPD_OK; /* update available */
		} else if (current_version >= server_version) {
			info("There are no updates available\n");
		}
		return SWUPD_NO; /* No update available */
	}
}

static const struct global_options opts = {
	NULL,
	0,
	NULL,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
	int optind = global_parse_options(argc, argv, &opts);

	if (optind < 0) {
		return false;
	}

	if (argc > optind) {
		error("unexpected arguments\n\n");
		return false;
	}

	return true;
}

/* return 0 if update available, non-zero if not */
int check_update_main(int argc, char **argv)
{
	int ret;

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = check_update();
	free_globals();

	return ret;
}
