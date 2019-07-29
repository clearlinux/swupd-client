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

#define _GNU_SOURCE
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "swupd.h"

static void print_help(const char *name)
{
	print("Usage:\n");
	print("   swupd %s [options]\n\n", basename((char *)name));
	print("Help Options:\n");
	print("   -h, --help              Show help options\n");
	print("       --enable            enable autoupdates\n");
	print("       --disable           disable autoupdates\n");
	print("\n");
}

static int enable;
static int disable;

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "enable", no_argument, &enable, 1 },
	{ "disable", no_argument, &disable, 1 },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "h", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
			print_help(argv[0]);
			exit(SWUPD_INVALID_OPTION);
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 0: /* getopt_long has set the flag */
			break;
		default:
			error("unrecognized option\n\n");
			goto err;
		}
	}

	if (argc > optind) {
		error("unexpected arguments\n\n");
		goto err;
	}

	return true;
err:
	print_help(argv[0]);
	return false;
}

static void policy_warn(void)
{
	warn("disabling automatic updates may take you "
	     "out of compliance with your IT policy\n\n");
}

enum swupd_code autoupdate_main(int argc, char **argv)
{
	if (!parse_options(argc, argv)) {
		return SWUPD_INVALID_OPTION;
	}
	if (enable && disable) {
		error("Can not enable and disable at the same time\n");
		return SWUPD_INVALID_OPTION;
	}
	if (!systemctl_active()) {
		error("Systemd is inactive - unable to proceed\n");
		return SWUPD_SUBPROCESS_ERROR;
	}

	if (enable) {
		int rc;
		check_root();
		info("Running systemctl to enable updates\n");
		rc = systemctl_cmd("unmask", "--now", "swupd-update.service", "swupd-update.timer", NULL);
		if (rc != 0) {
			return SWUPD_SUBPROCESS_ERROR;
		}
		rc = systemctl_restart("swupd-update.timer");
		if (rc != 0) {
			return SWUPD_SUBPROCESS_ERROR;
		}

		return SWUPD_OK;
	} else if (disable) {
		int rc;
		check_root();
		policy_warn();
		info("Running systemctl to disable updates\n");
		rc = systemctl_cmd("mask", "--now", "swupd-update.service", "swupd-update.timer", NULL);
		if (rc) {
			return SWUPD_SUBPROCESS_ERROR;
		}
		return SWUPD_OK;
	} else {
		int rc1, rc2;
		const int STATUS_UNKNOWN = 4;
		rc1 = systemctl_cmd("is-enabled", "swupd-update.service", NULL);
		rc2 = systemctl_cmd("is-active", "swupd-update.timer", NULL);
		if (rc1 == SWUPD_OK && rc2 == SWUPD_OK) {
			print("Enabled\n");
			return SWUPD_OK;
		} else if (rc1 >= STATUS_UNKNOWN || rc2 >= STATUS_UNKNOWN) {
			/* systemctl returns 1,2, or 3 when program dead or not running */
			error("Unable to determine autoupdate status\n");
			return SWUPD_SUBPROCESS_ERROR;
		} else {
			print("Disabled\n");
			return SWUPD_NO;
		}
	}
}
