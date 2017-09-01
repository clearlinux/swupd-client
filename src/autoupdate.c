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
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd %s [options]\n\n", basename((char *)name));
	fprintf(stderr, "Help Options:\n");
	fprintf(stderr, "   -h, --help              Show help options\n");
	fprintf(stderr, "       --enable            enable autoupdates\n");
	fprintf(stderr, "       --disable           disable autoupdates\n");
	fprintf(stderr, "\n");
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
			exit(EINVALID_OPTION);
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 0: /* getopt_long has set the flag */
			break;
		default:
			fprintf(stderr, "Error: unrecognized option\n\n");
			goto err;
		}
	}

	if (argc > optind) {
		fprintf(stderr, "Error: unexpected arguements\n\n");
		goto err;
	}

	return true;
err:
	print_help(argv[0]);
	return false;
}

int autoupdate_main(int argc, char **argv)
{
	copyright_header("autoupdate settings");

	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}
	if (enable && disable) {
		fprintf(stderr, "Can not enable and disable at the same time\n");
		exit(EXIT_FAILURE);
	}
	if (enable) {
		int rc;
		check_root();
		fprintf(stderr, "Running systemctl to enable updates\n");
		rc = system("/usr/bin/systemctl unmask --now swupd-update.service swupd-update.timer"
			    " && /usr/bin/systemctl start swupd-update.timer > /dev/null");
		if (rc != -1) {
			rc = WEXITSTATUS(rc);
		}
		return (rc);
	} else if (disable) {
		int rc;
		check_root();
		fprintf(stderr, "Running systemctl to disable updates\n");
		rc = system("/usr/bin/systemctl mask --now swupd-update.service swupd-update.timer > /dev/null");
		if (rc != -1) {
			rc = WEXITSTATUS(rc);
		}
		return (rc);
	} else {
		/* In a container, "/usr/bin/systemctl" will return 1 with
		 * "Failed to connect to bus: No such file or directory"
		 * However /usr/bin/systemctl is-enabled ... will not fail, even when it
		 * should. Check that systemctl is working before reporting the output
		 * of is-enabled. */
		int rc = system("/usr/bin/systemctl > /dev/null 2>&1");
		if (rc != -1) {
			rc = WEXITSTATUS(rc);
		}

		if (rc) {
			fprintf(stderr, "Unable to determine autoupdate status\n");
			return rc;
		}

		rc = system("/usr/bin/systemctl is-enabled swupd-update.service > /dev/null");
		if (rc != -1) {
			rc = WEXITSTATUS(rc);
		}
		switch (rc) {
		case 0:
			printf("Enabled\n");
			break;
		case 1:
			printf("Disabled\n");
			break;
		default:
			fprintf(stderr, "Unable to determine autoupdate status\n");
		}
		return (rc);
	}
}
