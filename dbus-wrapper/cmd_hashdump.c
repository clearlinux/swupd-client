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
	printf("   swupd %s [OPTION...] filename\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n\n");
	printf("Application Options:\n");
	printf("   -n, --no-xattrs         Ignore extended attributes\n");
	printf("   -b, --basepath          Optional argument for leading path to filename\n");
	printf("\n");
	printf("The filename is the name as it would appear in a Manifest file.\n");
	printf("\n");
}

static const struct option prog_opts[] = {
	{ "no-xattrs", 0, NULL, 'n' },
	{ "basepath", 1, NULL, 'b' },
	{ "help", 0, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv, struct list **opts)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "nb:h", prog_opts, NULL)) != -1) {
		command_option_t *option = NULL;
		bool bool_true = true;

		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'b':
			if (!optarg) {
				printf("Invalid --basepath argument\n\n");
				return false;
			}
			option = construct_command_option("basepath", TYPE_STRING, optarg);
			*opts = list_append_data(*opts, option);
			break;
		case 'n':
			option = construct_command_option("no-xattrs", TYPE_BOOL, &bool_true);
			*opts = list_append_data(*opts, option);
			break;
		default:
			printf("error: unrecognized option\n\n");
			return false;
		}
	}

	if (argc == optind) {
		printf("error: file name missing\n\n");
		return false;
	}

	return true;
}

int hashdump_main(int argc, char **argv)
{
	struct list *opts = NULL;
	int ret = -1;

	if (!parse_options(argc, argv, &opts)) {
		print_help(argv[0]);
		goto finish;
	}

        ret = dbus_client_call_method("HashDump", opts, DBUS_CMD_SINGLE_ARG, (argv + optind));

finish:
	list_free_list_and_data(opts, free_command_option);
	return ret;
}
