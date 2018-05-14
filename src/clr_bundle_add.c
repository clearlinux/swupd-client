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
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define VERIFY_NOPICKY 0

static char **bundles;

static void print_help(const char *name)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd %s [options] [bundle1 bundle2 (...)]\n\n", basename((char *)name));
	fprintf(stderr, "Help Options:\n");
	fprintf(stderr, "   -h, --help              Show help options\n");
	fprintf(stderr, "   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	fprintf(stderr, "   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
	fprintf(stderr, "   -v, --versionurl=[URL]  RFC-3986 encoded url for version string download\n");
	fprintf(stderr, "   -P, --port=[port #]        Port number to connect to at the url for version string and content file downloads\n");
	fprintf(stderr, "   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	fprintf(stderr, "   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	fprintf(stderr, "   -x, --force             Attempt to proceed even if non-critical errors found\n");
	fprintf(stderr, "   -n, --nosigcheck        Do not attempt to enforce certificate or signature checking\n");
	fprintf(stderr, "   -I, --ignore-time       Ignore system/certificate time when validating signature\n");
	fprintf(stderr, "   -S, --statedir          Specify alternate swupd state directory\n");
	fprintf(stderr, "   -C, --certpath          Specify alternate path to swupd certificates\n");
	fprintf(stderr, "   -t, --time              Show verbose time output for swupd operations\n");
	fprintf(stderr, "   -N, --no-scripts        Do not run the post-update scripts and boot update tool\n");
	fprintf(stderr, "   -b, --no-boot-update    Do not update the boot files using clr-boot-manager\n");
	fprintf(stderr, "\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "url", required_argument, 0, 'u' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "port", required_argument, 0, 'P' },
	{ "list", no_argument, 0, 'l' },
	{ "path", required_argument, 0, 'p' },
	{ "format", required_argument, 0, 'F' },
	{ "force", no_argument, 0, 'x' },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "ignore-time", no_argument, 0, 'I' },
	{ "statedir", required_argument, 0, 'S' },
	{ "certpath", required_argument, 0, 'C' },
	{ "time", no_argument, 0, 't' },
	{ "no-scripts", no_argument, 0, 'N' },
	{ "no-boot-update", no_argument, 0, 'b' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hxnIu:c:v:P:p:F:lS:tNbC:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'u':
			if (!optarg) {
				fprintf(stderr, "error: invalid --url argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			set_content_url(optarg);
			break;
		case 'c':
			if (!optarg) {
				fprintf(stderr, "Invalid --contenturl argument\n\n");
				goto err;
			}
			set_content_url(optarg);
			break;
		case 'v':
			if (!optarg) {
				fprintf(stderr, "Invalid --versionurl argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			break;
		case 'p': /* default empty path_prefix verifies the running OS */
			if (!optarg || !set_path_prefix(optarg)) {
				fprintf(stderr, "Invalid --path argument\n\n");
				goto err;
			}
			break;
		case 'P':
			if (sscanf(optarg, "%ld", &update_server_port) != 1) {
				fprintf(stderr, "Invalid --port argument\n\n");
				goto err;
			}
			break;
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				fprintf(stderr, "Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'S':
			if (!optarg || !set_state_dir(optarg)) {
				fprintf(stderr, "Invalid --statedir argument\n\n");
				goto err;
			}
			break;
		case 'l':
			fprintf(stderr, "error: [-l, --list] option is deprecated, use\n"
					"bundle-list [-a|--all] sub-command instead.\n\n");
			exit(EXIT_FAILURE);
		case 'x':
			force = true;
			break;
		case 'n':
			sigcheck = false;
			break;
		case 'I':
			timecheck = false;
			break;
		case 't':
			verbose_time = true;
			break;
		case 'N':
			no_scripts = true;
			break;
		case 'b':
			no_boot_update = true;
			break;
		case 'C':
			if (!optarg) {
				fprintf(stderr, "Invalid --certpath argument\n\n");
				goto err;
			}
			set_cert_path(optarg);
			break;
		default:
			fprintf(stderr, "error: unrecognized option\n\n");
			goto err;
		}
	}

	if (argc <= optind) {
		fprintf(stderr, "error: missing bundle(s) to be installed\n\n");
		goto err;
	}

	bundles = argv + optind;

	return true;
err:
	print_help(argv[0]);
	return false;
}

int bundle_add_main(int argc, char **argv)
{
	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}

	return install_bundles_frontend(bundles);
}
