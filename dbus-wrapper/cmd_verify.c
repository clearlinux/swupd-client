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
#include <stdbool.h>
#include <string.h>

#include "option.h"
#include "list.h"
#include "helpers.h"
#include "dbus_client.h"
#include "log.h"

static const struct option prog_opts[] = {
        { "help", no_argument, 0, 'h' },
        { "manifest", required_argument, 0, 'm' },
        { "path", required_argument, 0, 'p' },
        { "url", required_argument, 0, 'u' },
        { "port", required_argument, 0, 'P' },
        { "contenturl", required_argument, 0, 'c' },
        { "versionurl", required_argument, 0, 'v' },
        { "fix", no_argument, 0, 'f' },
        { "install", no_argument, 0, 'i' },
        { "format", required_argument, 0, 'F' },
        { "quick", no_argument, 0, 'q' },
        { "force", no_argument, 0, 'x' },
        { "statedir", required_argument, 0, 'S' },
        { 0, 0, 0, 0 }
};

static void print_help(const char *name)
{
        printf("Usage:\n");
        printf("   swupd %s [OPTION...]\n\n", basename((char *)name));
        printf("Help Options:\n");
        printf("   -h, --help              Show help options\n\n");
        printf("Application Options:\n");
        printf("   -m, --manifest=M        Verify against manifest version M\n");
        printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
        printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
        printf("   -P, --port=[port #]     Port number to connect to at the url for version string and content file downloads\n");
        printf("   -c, --contenturl=[URL]  RFC-3986 encoded url for content file downloads\n");
        printf("   -v, --versionurl=[URL]  RFC-3986 encoded url for version file downloads\n");
        printf("   -f, --fix               Fix local issues relative to server manifest (will not modify ignored files)\n");
        printf("   -i, --install           Similar to \"--fix\" but optimized for install all files to empty directory\n");
        printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
        printf("   -q, --quick             Don't compare hashes, only fix missing files\n");
        printf("   -x, --force             Attempt to proceed even if non-critical errors found\n");
        printf("   -S, --statedir          Specify alternate swupd state directory\n");
        printf("\n");
}

static bool parse_options(int argc, char **argv, struct list **opts)
{
	int opt;
	int version = 0;
	bool install = false;
	bool fix = false;
	bool path_prefix = false;

        while ((opt = getopt_long(argc, argv, "hxm:p:u:P:c:v:fiF:qS:", prog_opts, NULL)) != -1) {
		command_option_t *option = NULL;
		int port = -1;
		bool bool_true = true;

                switch (opt) {
                case '?':
                case 'h':
                        print_help(argv[0]);
			exit(EXIT_SUCCESS);
                case 'm':
                        if (strcmp("latest", optarg) == 0) {
                                version = -1;
                        } else if (sscanf(optarg, "%i", &version) != 1) {
                                printf("Invalid --manifest argument\n\n");
                                return false;
                        }
			option = construct_command_option("manifest", TYPE_INT, &version);
			*opts = list_append_data(*opts, option);
                        break;
                case 'p': /* default empty path_prefix verifies the running OS */
                        if (!optarg) {
                                printf("Invalid --path argument\n\n");
                                return false;
                        }
			path_prefix = true;
			option = construct_command_option("path", TYPE_STRING, optarg);
			*opts = list_append_data(*opts, option);
                        break;
                case 'u':
                        if (!optarg) {
                                printf("Invalid --url argument\n\n");
                                return false;
                        }
			option = construct_command_option("url", TYPE_STRING, optarg);
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
                case 'c':
                        if (!optarg) {
                                printf("Invalid --contenturl argument\n\n");
                                return false;
                        }
			option = construct_command_option("contenturl", TYPE_STRING, optarg);
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
                case 'f':
			fix = true;
			option = construct_command_option("fix", TYPE_BOOL, &fix);
			*opts = list_append_data(*opts, option);
                        break;
                case 'i':
			install = true;
			option = construct_command_option("install", TYPE_BOOL, &install);
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
                case 'q':
			option = construct_command_option("quick", TYPE_BOOL, &bool_true);
			*opts = list_append_data(*opts, option);
                        break;
                case 'x':
			option = construct_command_option("force", TYPE_BOOL, &bool_true);
			*opts = list_append_data(*opts, option);
                        break;
                default:
                        printf("Unrecognized option\n\n");
                        return false;
                }
        }

        if (install) {
                if (version == 0) {
                        printf("--install option requires -m version option\n");
                        return false;
                }
                if (!path_prefix) {
                        printf("--install option requires --path option\n");
                        return false;
                }
                if (fix) {
                        printf("--install and --fix options are mutually exclusive\n");
                        return false;
                }
        } else if (version == -1) {
                printf("-m latest only supported with --install\n");
                return false;
        }

	return true;
}

int verify_main(int argc, char **argv)
{
	struct list *opts = NULL;
	int ret = -1;

	if (!parse_options(argc, argv, &opts)) {
		print_help(argv[0]);
		goto finish;
	}

        ret = dbus_client_call_method("Verify", opts, DBUS_CMD_NO_ARGS, NULL);

finish:
	list_free_list_and_data(opts, free_command_option);
	return ret;
}
