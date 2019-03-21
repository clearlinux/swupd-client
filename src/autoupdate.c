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

static int system_command(const char *cmd)
{
	int ret;

	ret = system(cmd);
	if (ret == -1) {
		/* it wasn't possible to create the shell process */
		return SWUPD_SUBPROCESS_ERROR;
	}

	ret = WEXITSTATUS(ret);

	if (ret == 126 || ret == 128) {
		/* the command invoked cannot execute or was not found */
		return SWUPD_SUBPROCESS_ERROR;
	}

	return ret;
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
	if (enable) {
		int rc;
		check_root();
		info("Running systemctl to enable updates\n");
		rc = system_command("/usr/bin/systemctl unmask --now swupd-update.service swupd-update.timer"
				    " && /usr/bin/systemctl restart swupd-update.timer > /dev/null");
		if (rc) {
			return SWUPD_SUBPROCESS_ERROR;
		}
		return SWUPD_OK;
	} else if (disable) {
		int rc;
		check_root();
		policy_warn();
		info("Running systemctl to disable updates\n");
		rc = system_command("/usr/bin/systemctl mask --now swupd-update.service swupd-update.timer > /dev/null");
		if (rc) {
			return SWUPD_SUBPROCESS_ERROR;
		}
		return SWUPD_OK;
	} else {
		/* In a container, "/usr/bin/systemctl" will return 1 with
		 * "Failed to connect to bus: No such file or directory"
		 * However /usr/bin/systemctl is-enabled ... will not fail, even when it
		 * should. Check that systemctl is working before reporting the output
		 * of is-enabled. */
		int rc = system_command("/usr/bin/systemctl > /dev/null 2>&1");
		if (rc) {
			error("Unable to determine autoupdate status\n");
			return SWUPD_SUBPROCESS_ERROR;
		}

		rc = system_command("/usr/bin/systemctl is-enabled swupd-update.service > /dev/null");
		switch (rc) {
		case SWUPD_OK:
			print("Enabled\n");
			break;
		case SWUPD_NO:
			print("Disabled\n");
			break;
		default:
			rc = SWUPD_SUBPROCESS_ERROR;
			error("Unable to determine autoupdate status\n");
		}
		return (rc);
	}
}
