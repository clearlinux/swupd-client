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

/* this is a temp container to save parsed
 * options so if more than one bundle to remove
 * then we can restore the parsed options into
 * globals
 */
static struct opts {
	char *path_prefix;
	char *version_url;
	char *content_url;
	char *format_string;
	char *state_dir;
	char *cert_path;
	bool force;
	bool sigcheck;
}curopts = {NULL, NULL, NULL, NULL, NULL, NULL, false, true};

static char **bundles;

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [options] [bundle1, bundle2, ...]\n\n", basename((char *)name));
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
	printf("   -C, --certpath          Specify alternate path to swupd certificates\n");
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
	{ "certpath", required_argument, 0, 'C' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hxnp:u:c:v:P:F:S:C:", prog_opts, NULL)) != -1) {
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
			string_or_die(&curopts.path_prefix, "%s", optarg);
			break;
		case 'u':
			if (!optarg) {
				printf("error: invalid --url argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			set_content_url(optarg);
			string_or_die(&curopts.version_url, "%s", optarg);
			string_or_die(&curopts.content_url, "%s", optarg);
			break;
		case 'c':
			if (!optarg) {
				printf("Invalid --contenturl argument\n\n");
				goto err;
			}
			set_content_url(optarg);
			string_or_die(&curopts.content_url, "%s", optarg);
			break;
		case 'v':
			if (!optarg) {
				printf("Invalid --versionurl argument\n\n");
				goto err;
			}
			set_version_url(optarg);
			string_or_die(&curopts.version_url, "%s", optarg);
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
			string_or_die(&curopts.format_string, "%s", optarg);
			break;
		case 'S':
			if (!optarg || !set_state_dir(optarg)) {
				printf("Invalid --statedir argument\n\n");
				goto err;
			}
			string_or_die(&curopts.state_dir, "%s", optarg);
			break;
		case 'x':
			force = true;
			curopts.force = true;
			break;
		case 'n':
			sigcheck = false;
			curopts.sigcheck = false;
			break;
		case 'C':
			if (!optarg) {
				printf("Invalid --certpath argument\n\n");
				goto err;
			}
			set_cert_path(optarg);
			string_or_die(&curopts.cert_path, "%s", optarg);
			break;
		default:
			printf("error: unrecognized option\n\n");
			goto err;
		}
	}

	if (argc <= optind) {
		printf("error: missing bundle(s) to be removed\n\n");
		goto err;
	}

	bundles = argv + optind;

	return true;
err:
	print_help(argv[0]);
	return false;
}

static void reload_parsed_opts(void)
{
	if (curopts.path_prefix) {
		set_path_prefix(curopts.path_prefix);
	}

	if (curopts.version_url) {
		set_version_url(curopts.version_url);
	}

	if (curopts.content_url) {
		set_content_url(curopts.content_url);
	}

	if (curopts.format_string) {
		set_format_string(curopts.format_string);
	}

	if (curopts.state_dir) {
		set_state_dir(curopts.state_dir);
	}

	if (curopts.cert_path) {
		set_cert_path(curopts.cert_path);
	}

	force = curopts.force;
	sigcheck = curopts.sigcheck;

}

void static free_saved_opts(void)
{
	if (curopts.path_prefix != NULL) {
		free(curopts.path_prefix);
	}

	if (curopts.version_url != NULL) {
		free(curopts.version_url);
	}

	if (curopts.content_url != NULL) {
		free(curopts.content_url);
	}

	if (curopts.format_string != NULL) {
		free(curopts.format_string);
	}

	if (curopts.state_dir != NULL) {
		free(curopts.state_dir);
	}

	if (curopts.cert_path != NULL) {
		free(curopts.cert_path);
	}


}

int bundle_remove_main(int argc, char **argv)
{
	int ret = 0;
	int total = 0;
	int bad = 0;

	copyright_header("bundle remover");

	if (!parse_options(argc, argv)) {
		return EINVALID_OPTION;
	}

	for (; *bundles; ++bundles, total++) {
		printf("Removing bundle: %s\n", *bundles);

		if (remove_bundle(*bundles) != 0) {
			/* At least one bundle failed to be removed
			 * then for consistency return an error
			 * indicating that
			 */
			ret = EBUNDLE_REMOVE;
			bad++;
		}
		/* if we have more than one bundle to process then
		 * make sure to reload the parsed options since all
		 * globals are cleaned up at swupd_deinit()
		 */
		if (*bundles) {
			reload_parsed_opts();
		}
	}
	/* print some statistics */
	if (ret) {
		printf("%i bundle(s) of %i failed to remove\n", bad, total);
	} else {
		printf("%i bundle(s) were removed succesfully\n", total);
	}

	/* free any parsed opt saved for reloading */
	free_saved_opts();

	return ret;
}
