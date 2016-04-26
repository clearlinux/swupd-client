/*
 *   Software Updater - D-Bus client for the daemon controlling
 *                      Clear Linux Software Update Client.
 *
 *      Copyright © 2016 Intel Corporation.
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
 *   Contact: Dmitry Rozhkov <dmitry.rozhkov@intel.com>
 */

#define _GNU_SOURCE

#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include "option.h"
#include "list.h"
#include "helpers.h"
#include "dbus_client.h"

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [options] bundlename\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n");
	printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	printf("   -v, --versionurl=[URL]  RFC-3986 encoded url for version string download\n");
	printf("   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
	printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	printf("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	printf("   -S, --statedir          Specify alternate swupd state directory\n");
	printf("\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "url", required_argument, 0, 'u' },
	{ "versionurl", required_argument, 0, 'v' },
	{ "port", required_argument, 0, 'P' },
	{ "format", required_argument, 0, 'F' },
	{ "force", no_argument, 0, 'x' },
	{ "path", required_argument, 0, 'p' },
	{ "statedir", required_argument, 0, 'S' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv, struct list **opts)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "hxu:v:P:F:p:S:", prog_opts, NULL)) != -1) {
		command_option_t *option = NULL;
		int port = -1;
		bool bool_true = true;

		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'u':
			if (!optarg) {
				printf("error: invalid --url argument\n\n");
				return false;
			}
			option = construct_command_option("url", TYPE_STRING, optarg);
			*opts = list_append_data(*opts, option);
			break;
		case 'v':
			if (!optarg) {
				printf("Invalid --versionurl argument\n\n");
				return false;
			}
			option = construct_command_option("versionurl", TYPE_STRING, optarg);
			*opts = list_append_data(*opts, option);
			break;
		case 'P':
			if (sscanf(optarg, "%i", &port) != 1) {
				printf("Invalid --port argument\n\n");
				return false;
			}
			option = construct_command_option("port", TYPE_INT, &port);
			*opts = list_append_data(*opts, option);
			break;
		case 'F':
			if (!optarg || !is_format_correct(optarg)) {
				printf("Invalid --format argument\n\n");
				return false;
			}
			option = construct_command_option("format", TYPE_STRING, optarg);
			*opts = list_append_data(*opts, option);
			break;
		case 'S':
			if (!optarg || !is_statedir_correct(optarg)) {
				printf("Invalid --statedir argument\n\n");
				return false;
			}
			option = construct_command_option("statedir", TYPE_STRING, optarg);
			*opts = list_append_data(*opts, option);
			break;
		case 'p': /* default empty path_prefix verifies the running OS */
			if (!optarg) {
				printf("Invalid --path argument\n\n");
				return false;
			}
			option = construct_command_option("path", TYPE_STRING, optarg);
			*opts = list_append_data(*opts, option);
			break;
		case 'x':
			option = construct_command_option("force", TYPE_BOOL, &bool_true);
			*opts = list_append_data(*opts, option);
			break;
		default:
			printf("error: unrecognized option\n\n");
			return false;
		}
	}

	return true;
}

int check_update_main(int argc, char **argv)
{
	struct list *opts = NULL;
	int ret = -1;

	if (!parse_options(argc, argv, &opts)) {
		print_help(argv[0]);
		goto finish;
	}
        ret = dbus_client_call_method("CheckUpdate", opts, DBUS_CMD_SINGLE_ARG, (argv + optind));

finish:
	list_free_list_and_data(opts, free_command_option);
	return ret;
}
