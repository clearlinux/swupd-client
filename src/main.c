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
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "swupd.h"

static bool cmd_line_status = false;

static const struct option prog_opts[] = {
	{ "download", no_argument, 0, 'd' },
	{ "help", no_argument, 0, 'h' },
	{ "url", required_argument, 0, 'u' },
	{ "port", required_argument, 0, 'P' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "status", no_argument, 0, 's' },
	{ "format", required_argument, 0, 'F' },
	{ "path", required_argument, 0, 'p' },
	{ "force", no_argument, 0, 'x' },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "ignore", no_argument, 0, 'I' },
	{ "statedir", required_argument, 0, 'S' },
	{ "certpath", required_argument, 0, 'C' },
	{ 0, 0, 0, 0 }
};

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [OPTION...]\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n\n");
	printf("Application Options:\n");
	printf("   -d, --download          Download all content, but do not actually install the update\n");
#warning "remove user configurable url when alternative exists"
	printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	printf("   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
#warning "remove user configurable content url when alternative exists"
	printf("   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
#warning "remove user configurable version url when alternative exists"
	printf("   -v, --versionurl=[URL]  RFC-3986 encoded url for version string download\n");
	printf("   -s, --status            Show current OS version and latest version available on server\n");
	printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	printf("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	printf("   -n, --nosigcheck        Do not attempt to enforce certificate or signature checking\n");
	printf("   -I, --ignore-certtime   Ignore system/certificate time when validating signature\n");
	printf("   -S, --statedir          Specify alternate swupd state directory\n");
	printf("   -C, --certpath          Specify alternate path to swupd certificates\n");
	printf("\n");
}

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hxnIdu:P:c:v:sF:p:S:C:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'd':
			download_only = true;
			break;
		case 'u':
			if (!optarg) {
				printf("Invalid --url argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			set_content_url(optarg);
			break;
		case 'P':
			if (sscanf(optarg, "%ld", &update_server_port) != 1) {
				printf("Invalid --port argument\n\n");
				goto err;
			}
			break;
		case 'c':
			if (!optarg) {
				printf("Invalid --contenturl argument\n\n");
				goto err;
			}
			set_content_url(optarg);
			break;
		case 'v':
			if (!optarg) {
				printf("Invalid --versionurl argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			break;
		case 's':
			cmd_line_status = true;
			break;
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				printf("Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'S':
			if (!optarg || !set_state_dir(optarg)) {
				printf("Invalid --statedir argument\n\n");
				goto err;
			}
			break;
		case 'p': /* default empty path_prefix updates the running OS */
			if (!optarg || !set_path_prefix(optarg)) {
				printf("Invalid --path argument\n\n");
				goto err;
			}
			break;
		case 'x':
			force = true;
			break;
		case 'n':
			sigcheck = false;
			break;
		case 'I':
			timecheck = false;
			break;
		case 'C':
			if (!optarg) {
				printf("Invalid --certpath argument\n\n");
				goto err;
			}
			set_cert_path(optarg);
			break;
		default:
			printf("Unrecognized option\n\n");
			goto err;
		}
	}

	return true;
err:
	print_help(argv[0]);
	return false;
}

/* return 0 if server_version is ahead of current_version,
 * 1 if the current_version is current or ahead
 * 2 if one or more versions can't be found
 */
static int print_versions()
{
	int current_version, server_version, ret = 0;

	check_root();
	(void)init_globals();
	swupd_curl_init();

	read_versions(&current_version, &server_version, path_prefix);

	if (current_version < 0) {
		printf("Cannot determine current OS version\n");
		ret = 2;
	} else {
		printf("Current OS version: %d\n", current_version);
	}

	if (server_version < 0) {
		printf("Cannot get latest the server version.Could not reach server\n");
		ret = 2;
	} else {
		printf("Latest server version: %d\n", server_version);
	}
	if ((ret == 0) && (server_version <= current_version)) {
		ret = 1;
	}

	telemetry(ret ? TELEMETRY_WARN : TELEMETRY_INFO,
		  "check",
		  "result=%d\n"
		  "version_current=%d\n"
		  "version_server=%d\n",
		  ret,
		  current_version,
		  server_version);

	return ret;
}

int update_main(int argc, char **argv)
{
	int ret = 0;
	copyright_header("software update");

	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}

	if (cmd_line_status) {
		ret = print_versions();
	} else {
		ret = main_update();
	}
	return ret;
}
