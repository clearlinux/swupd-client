/*
 *   Software Updater - client side
 *
 *      Copyright © 2018-2020 Intel Corporation.
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
 */

#include "lib/log.h"
#include "swupd.h"

enum swupd_code print_update_conf_info(void)
{
	enum swupd_code ret = SWUPD_OK;
	char dist_string[LINE_MAX];
	int current_format = -1;

	if (log_get_level() >= LOG_INFO_VERBOSE) {
		/* Retrieve current format */
		current_format = get_current_format();
	}

	int current_version = get_current_version(globals.path_prefix);
	bool dist_string_found = get_distribution_string(globals.path_prefix, dist_string);

	// Set distribution string 'unknown' when not found.
	info("Distribution:      %s\n", dist_string_found ? dist_string : "unknown");

	if (current_version < 0) {
		error("Unable to determine current OS version\n");
		info("Installed version: ");
		print("unknown\n");
		ret = SWUPD_CURRENT_VERSION_UNKNOWN;
	} else {
		info("Installed version: ");
		print("%d", current_version);
		if (current_format > 0) {
			info_verbose(" (format %d)", current_format);
		} else {
			info_verbose(" (format unknown)");
		}
		print("\n");
	}
	info("Version URL:       %s\n", globals.version_url);
	info("Content URL:       %s\n", globals.content_url);

	return ret;
}

static void print_help(void)
{
	print("Shows the current OS version and the URLs used for updates\n\n");
	print("Usage:\n");
	print("   swupd info [OPTION...]\n\n");

	global_print_help();
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

	if (argc > ind) {
		error("Unexpected arguments\n\n");
		return false;
	}

	return true;
}

enum swupd_code info_main(int UNUSED_PARAM argc, char UNUSED_PARAM **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int steps_in_info = 0;
	/* there is no need to report in progress for init at this time */

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	ret = swupd_init(SWUPD_NO_ROOT);
	if (ret != SWUPD_OK) {
		return ret;
	}

	progress_init_steps("info", steps_in_info);
	ret = print_update_conf_info();

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}
