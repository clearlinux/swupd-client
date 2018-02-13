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
 *         Mario Alfredo Carrillo Arevalo <mario.alfredo.c.arevalo@intel.com>
 *         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <libgen.h>
#include <string.h>

#include "config.h"
#include "swupd.h"

static bool cmdline_option_all = false;
static char *cmdline_option_has_dep = NULL;
static char *cmdline_option_deps = NULL;

static void free_has_dep(void)
{
	free_string(&cmdline_option_has_dep);
}

static void free_deps(void)
{
	free_string(&cmdline_option_deps);
}

static void print_help(const char *name)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   swupd %s [options]\n\n", basename((char *)name));
	fprintf(stderr, "Help Options:\n");
	fprintf(stderr, "   -h, --help              Show help options\n");
	fprintf(stderr, "   -a, --all               List all available bundles for the current version of Clear Linux\n");
	fprintf(stderr, "   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	fprintf(stderr, "   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
	fprintf(stderr, "   -v, --versionurl=[URL]  RFC-3986 encoded url for version string download\n");
	fprintf(stderr, "   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	fprintf(stderr, "   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	fprintf(stderr, "   -n, --nosigcheck        Do not attempt to enforce certificate or signature checking\n");
	fprintf(stderr, "   -I, --ignore-time       Ignore system/certificate time when validating signature\n");
	fprintf(stderr, "   -S, --statedir          Specify alternate swupd state directory\n");
	fprintf(stderr, "   -C, --certpath          Specify alternate path to swupd certificates\n");
	fprintf(stderr, "   -d, --deps=[BUNDLE]     List bundles included by BUNDLE\n");
	fprintf(stderr, "   -D, --has-dep=[BUNDLE]  List dependency tree of all bundles which have BUNDLE as a dependency\n");

	fprintf(stderr, "\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "all", no_argument, 0, 'a' },
	{ "url", required_argument, 0, 'u' },
	{ "contenturl", required_argument, 0, 'c' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "path", required_argument, 0, 'p' },
	{ "format", required_argument, 0, 'F' },
	{ "nosigcheck", no_argument, 0, 'n' },
	{ "statedir", required_argument, 0, 'S' },
	{ "ignore-time", no_argument, 0, 'I' },
	{ "certpath", required_argument, 0, 'C' },
	{ "has-dep", required_argument, 0, 'D' },
	{ "deps", required_argument, 0, 'd' },

	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hanIu:c:v:p:F:S:C:d:D:", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'a':
			cmdline_option_all = true;
			break;
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
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				fprintf(stderr, "Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'n':
			sigcheck = false;
			break;
		case 'I':
			timecheck = false;
			break;
		case 'S':
			if (!optarg || !set_state_dir(optarg)) {
				fprintf(stderr, "Invalid --statedir argument\n\n");
				goto err;
			}
			break;
		case 'C':
			if (!optarg) {
				fprintf(stderr, "Invalid --certpath argument\n\n");
				goto err;
			}
			set_cert_path(optarg);
			break;
		case 'D':
			if (!optarg) {
				fprintf(stderr, "Invalid --has-dep argument\n\n");
				goto err;
			}
			string_or_die(&cmdline_option_has_dep, "%s", optarg);
			atexit(free_has_dep);
			break;
		case 'd':
			if (!optarg) {
				fprintf(stderr, "Invalid --deps argument\n\n");
				goto err;
			}
			string_or_die(&cmdline_option_deps, "%s", optarg);
			atexit(free_deps);
			break;
		default:
			fprintf(stderr, "error: unrecognized option\n\n");
			goto err;
		}
	}

	return true;
err:
	print_help(argv[0]);
	return false;
}

int bundle_list_main(int argc, char **argv)
{
	int lock_fd;
	int ret;

	copyright_header("bundle list");

	if (!parse_options(argc, argv)) {
		return EXIT_FAILURE;
	}

	ret = swupd_init(&lock_fd);
	if (ret != 0) {
		fprintf(stderr, "Error: Failed updater initialization. Exiting now\n");
		return ret;
	}

	if (cmdline_option_deps != NULL) {
		ret = show_included_bundles(cmdline_option_deps);
	} else if (cmdline_option_has_dep != NULL) {
		ret = show_bundle_reqd_by(cmdline_option_has_dep, cmdline_option_all);
	} else if (cmdline_option_all) {
		ret = list_installable_bundles();
	} else {
		ret = list_local_bundles();
	}

	swupd_deinit(lock_fd, NULL);

	return ret;
}
