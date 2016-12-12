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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swupd.h"

#define VERIFY_PICKY 1

static char *bundle_name = NULL;

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [options] bundlename\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n");
	printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	printf("   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
	printf("   -v, --versionurl=[URL]  RFC-3986 encoded url for version string download\n");
	printf("   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
	printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	printf("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	printf("   -n, --nosigcheck       Do not attempt to enforce certificate or signature checks\n");
	printf("   -S, --statedir          Specify alternate swupd state directory\n");
	printf("\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "path", required_argument, 0, 'p' },
	{ "url", required_argument, 0, 'u' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "port", required_argument, 0, 'P' },
	{ "format", required_argument, 0, 'F' },
	{ "force", no_argument, 0, 'x' },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "statedir", required_argument, 0, 'S' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hxnp:u:c:v:P:F:S:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'p': /* default empty path_prefix removes on the running OS */
			if (!optarg || !set_path_prefix(optarg)) {
				printf("Invalid --path argument\n\n");
				goto err;
			}
			break;
		case 'u':
			if (!optarg) {
				printf("error: invalid --url argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			set_content_url(optarg);
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
		case 'S':
			if (!optarg || !set_state_dir(optarg)) {
				printf("Invalid --statedir argument\n\n");
				goto err;
			}
			break;
		case 'x':
			force = true;
			break;
		case 'n':
			sigcheck = false;
			break;
		default:
			printf("error: unrecognized option\n\n");
			goto err;
		}
	}

	if (argc != optind + 1) {
		printf("error: invalid arguments\n\n");
		goto err;
	}
	string_or_die(&bundle_name, "%s", argv[optind]);

	return true;
err:
	print_help(argv[0]);
	return false;
}

int bundle_remove_main(int argc, char **argv)
{
	int ret;

	copyright_header("bundle remover");

	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}

	ret = remove_bundle(bundle_name);
	free(bundle_name);

	return ret;
}
