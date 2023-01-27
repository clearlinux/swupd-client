/*
 *   Software Updater - client side
 *
 *      Copyright Â© 2012-2020 Intel Corporation.
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

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lib/log.h"
#include "swupd.h"
#include "verifytime/verifytime.h"

static void print_help(void)
{
	print("Checks whether an update is available and prints out the information if so\n\n");
	print("Usage:\n");
	print("   swupd check-update [OPTION...]\n\n");
	// TODO: Add documentation explaining this command

	global_print_help();
}

static void print_format(int format)
{
	if (format > 0) {
		info_verbose(" (format %d)", format);
	} else {
		info_verbose(" (format unknown)");
	}
	info("\n");
}

/* Return 0 if there is an update available, 1 if not and > 1 on errors. */
enum swupd_code check_update(void)
{
	int current_version, server_version;
	enum swupd_code ret = SWUPD_OK;
	int server_format = -1;
	int current_format = -1;
	int latest_version_in_format = -1, format_in_format = -1, tmp_format = -1;

	current_version = get_current_version(globals.path_prefix);
	server_version = version_get_absolute_latest();

	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
	}

	if (server_version == -SWUPD_ERROR_SIGNATURE_VERIFICATION) {
		ret = SWUPD_SIGNATURE_VERIFICATION_FAILED;
		error("Unable to determine the server version as signature verification failed\n");
	} else if (server_version < 0) {
		error("Unable to determine the server version\n");
		ret = SWUPD_SERVER_CONNECTION_ERROR;
	}

	if (log_get_level() >= LOG_INFO_VERBOSE) {
		/* Retrieve latest_format */
		/* Sending latest server_version to get the latest format */
		server_format = get_server_format(server_version);
		/* Retrieve current format */
		current_format = get_current_format();
		latest_version_in_format = get_latest_version(NULL);
		if (latest_version_in_format == -SWUPD_ERROR_SIGNATURE_VERIFICATION) {
			ret = SWUPD_SIGNATURE_VERIFICATION_FAILED;
			error("Unable to determine the latest version in format as signature verification failed\n");
		} else if (latest_version_in_format < 0) {
			error("Unable to determine the latest version in format\n");
			ret = SWUPD_SERVER_CONNECTION_ERROR;
		} else {
			format_in_format = get_server_format(latest_version_in_format);
		}
	}

	if (current_version > 0) {
		info("Current OS version: %d", current_version);
		print_format(current_format);
	}

	if (server_version > 0) {
		info("Latest server version: %d", server_version);
		print_format(server_format);
	}

	if (latest_version_in_format > 0) {
		info_verbose("Latest version in format %s: %d", globals.format_string, latest_version_in_format);
		if (str_to_int(globals.format_string, &tmp_format) != 0 ||
		    tmp_format != format_in_format) {
			print_format(format_in_format);
		} else {
			info("\n");
		}
	}

	if (ret != SWUPD_OK) {
		return ret;
	}

	if (current_version < server_version) {
		info("There is a new OS version available: ");
		print("%d\n", server_version);
		return SWUPD_OK; /* update available */
	} else if (current_version >= server_version) {
		info("There are no updates available\n");
	}
	return SWUPD_NO; /* No update available */
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
enum swupd_code check_update_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_checkupdate = 0;
	/* there is no need to report in progress for check-update at this time */

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_NO_ROOT);
	if (ret != SWUPD_OK) {
		return ret;
	}

	progress_init_steps("check-update", steps_in_checkupdate);

	ret = check_update();

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
