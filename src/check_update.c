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

#include "swupd.h"

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [options] bundlename\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n");
	printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	printf("   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
	printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	printf("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	printf("\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "url", required_argument, 0, 'u' },
	{ "port", required_argument, 0, 'P' },
	{ "format", required_argument, 0, 'F' },
	{ "force", no_argument, 0, 'x' },
	{ "path", required_argument, 0, 'p' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	set_format_string(NULL);

	while ((opt = getopt_long(argc, argv, "hxu:P:F:p:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'u':
			if (!optarg) {
				printf("error: invalid --url argument\n\n");
				goto err;
			}
			set_local_download(optarg);
			set_version_url(optarg);
			break;
		case 'P':
			if (sscanf(optarg, "%ld", &update_server_port) != 1) {
				printf("Invalid --port argument\n\n");
				goto err;
			}
			break;
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				printf("Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'p': /* default empty path_prefix checks the running OS */
			if (!optarg) {
				printf("Invalid --path argument\n\n");
				goto err;
			}
			if (path_prefix) { /* multiple -p options */
				free(path_prefix);
			}
			string_or_die(&path_prefix, "%s", optarg);
			break;
		case 'x':
			force = true;
			break;
		default:
			printf("error: unrecognized option\n\n");
			goto err;
		}
	}

	if (!init_globals()) {
		return false;
	}
	return true;
err:
	print_help(argv[0]);
	return false;
}

static int check_update()
{
	int current_version, server_version;

	check_root();
	swupd_curl_init();
	read_versions(&current_version, &server_version, path_prefix);

	if (server_version < 0) {
		printf("Cannot reach update server\n");
		return -1;
	} else if (current_version < 0) {
		printf("Unable to determine current OS version\n");
		return -1;
	} else {
		if (current_version != -1 && current_version < server_version) {
			printf("There is a new OS version available: %d\n", server_version);
			update_motd(server_version);
		} else if (current_version >= server_version) {
			printf("There are no updates available\n");
		}
		return 0;
	}
}

int check_update_main(int argc, char **argv)
{
	int ret;
	copyright_header("software update checker");

	if (!parse_options(argc, argv)) {
		free_globals();
		return EXIT_FAILURE;
	}

	ret = check_update();
	free_globals();

	return ret;
}
