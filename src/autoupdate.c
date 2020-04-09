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
 *         Icarus Sparry <icarus.w.sparry@intel.com>
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "swupd.h"

static void print_help(void)
{
	print("Enables or disables automatic updates, or reports current status\n\n");
	print("Usage:\n");
	print("   swupd autoupdate [OPTION...]\n\n");

	global_print_help();

	print("Options:\n");
	print("       --enable            enable autoupdates\n");
	print("       --disable           disable autoupdates\n");
	print("\n");
}

#define FLAG_ENABLE 2000
#define FLAG_DISABLE 2001
static bool cmdline_option_enable;
static bool cmdline_option_disable;

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "enable", no_argument, 0, FLAG_ENABLE },
	{ "disable", no_argument, 0, FLAG_DISABLE },
};

static bool parse_opt(int opt, UNUSED_PARAM char *optarg)
{
	switch (opt) {
	case 'h':
		print_help();
		exit(EXIT_SUCCESS);
	case FLAG_ENABLE:
		cmdline_option_enable = optarg_to_bool(optarg);
		return true;
	case FLAG_DISABLE:
		cmdline_option_disable = optarg_to_bool(optarg);
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
		error("unexpected arguments\n\n");
		return false;
	}

	if (cmdline_option_enable && cmdline_option_disable) {
		error("Can not use --enable and --disable options at the same time\n\n");
		return false;
	}

	return true;
}

static void policy_warn(void)
{
	warn("disabling automatic updates may take you "
	     "out of compliance with your IT policy\n\n");
}

enum swupd_code autoupdate_main(int argc, char **argv)
{

	if (!parse_options(argc, argv)) {
		print_help();
		return SWUPD_INVALID_OPTION;
	}

	if (!globals.path_prefix && !systemctl_active()) {
		error("Systemd is inactive - unable to proceed\n");
		return SWUPD_SUBPROCESS_ERROR;
	}

	if (cmdline_option_enable) {
		int rc;
		check_root();
		info("Running systemctl to enable updates\n");
		rc = systemctl_cmd_path(globals.path_prefix,
					"unmask", "--now", "swupd-update.service", "swupd-update.timer", NULL);
		if (rc != 0) {
			return SWUPD_SUBPROCESS_ERROR;
		}

		/* If globals.path_prefix is given and we have succeeded so far,
		 * make sense to return SWUPD_OK at this point. In that case,
		 * a swupd timer restart cannot be done as there is no
		 * system daemon to communicate for rootfs.
		 */
		if (globals.path_prefix) {
			warn("Running autoupdate with --path will not restart swupd-update.timer. This will have to be done manually\n");
			return SWUPD_OK;
		}

		rc = systemctl_cmd_path(globals.path_prefix, "restart", "swupd-update.timer", NULL);
		if (rc != 0) {
			return SWUPD_SUBPROCESS_ERROR;
		}

		return SWUPD_OK;
	} else if (cmdline_option_disable) {
		int rc;
		check_root();
		policy_warn();
		info("Running systemctl to disable updates\n");
		rc = systemctl_cmd_path(globals.path_prefix,
					"mask", "--now", "swupd-update.service", "swupd-update.timer", NULL);
		if (rc) {
			return SWUPD_SUBPROCESS_ERROR;
		}
		return SWUPD_OK;
	} else {
		int rc1, rc2;
		const int STATUS_UNKNOWN = 4;
		rc1 = systemctl_cmd_path(globals.path_prefix, "is-enabled", "swupd-update.service", NULL);
		rc2 = systemctl_cmd_path(globals.path_prefix, "is-active", "swupd-update.timer", NULL);
		if (rc1 == SWUPD_OK && rc2 == SWUPD_OK) {
			info("Enabled\n");
			return SWUPD_OK;
		} else if (rc1 >= STATUS_UNKNOWN || rc2 >= STATUS_UNKNOWN) {
			/* systemctl returns 1,2, or 3 when program dead or not running */
			error("Unable to determine autoupdate status\n");
			return SWUPD_SUBPROCESS_ERROR;
			// Swupd-service is unmasked, static but timer is inactive for --path
		} else if (rc1 == SWUPD_OK && rc2 != SWUPD_OK && globals.path_prefix) {
			info("Autoupdate is enabled, but the timer is not running\n");
			error("Unable to determine autoupdate status with --path option\n");
			return SWUPD_SUBPROCESS_ERROR;
		} else {
			info("Disabled\n");
			return SWUPD_NO;
		}
	}
}
