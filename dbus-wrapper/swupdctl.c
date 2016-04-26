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

#define _GNU_SOURCE // for basename()
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int bundle_add_main(int argc, char **argv);
int bundle_remove_main(int argc, char **argv);
int hashdump_main(int argc, char **argv);
int verify_main(int argc, char **argv);
int update_main(int argc, char **argv);
int check_update_main(int argc, char **argv);
int search_main(int argc, char **argv);

struct subcmd {
	char *name;
	char *doc;
	int (*mainfunc)(int, char **);
};

static const struct option prog_opts_main[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ 0 }
};

static void copyright_header(const char *name)
{
        printf("%s 1.0\n", name);
        printf("   Copyright (C) 2012-2016 Intel Corporation\n");
        printf("\n");
}

static struct subcmd commands[] = {
	{ "bundle-add", "Install a new bundle", bundle_add_main},
	{ "bundle-remove", "Uninstall a bundle", bundle_remove_main},
	{ "hashdump", "Dumps the HMAC hash of a file", hashdump_main},
	{ "update", "Update to latest OS version", update_main},
	{ "verify", "Verify content for OS version", verify_main},
	{ "check-update", "Checks if a new OS version is available", check_update_main},
	{ "search", "Search Clear Linux for a binary or library", search_main},
	{ 0 }
};

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("    %s [OPTION...]\n", basename((char *)name));
	printf(" or %s [OPTION...] SUBCOMMAND [OPTION...]\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n");
	printf("   -v, --version           Output version information and exit\n\n");
	printf("Subcommands:\n");

	struct subcmd *entry = commands;

	while (entry->name != NULL) {
		printf("   %-20s    %-30s\n", entry->name, entry->doc);
		entry++;
	}
	printf("\n");
	printf("To view subcommand options, run `%s SUBCOMMAND --help'\n", basename((char *)name));
}

static int subcmd_index(char *arg)
{
	struct subcmd *entry = commands;
	int i = 0;
	size_t input_len = strlen(arg);
	size_t cmd_len;

	while (entry->name != NULL) {
		cmd_len = strlen(entry->name);
		if (cmd_len == input_len && strcmp(arg, entry->name) == 0) {
			return i;
		}
		entry++;
		i++;
	}

	return -1;
}

static int parse_options(int argc, char **argv, int *index)
{
	int opt;
	int ret;

	/* The leading "-" in the optstring is required to preserve option parsing order */
	while ((opt = getopt_long(argc, argv, "-hv", prog_opts_main, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'v':
			copyright_header("swupdctl");
			exit(EXIT_SUCCESS);
		case '\01':
			/* found a subcommand, or a random non-option argument */
			ret = subcmd_index(optarg);
			if (ret < 0) {
				fprintf(stderr, "error: unrecognized subcommand `%s'\n\n",
					optarg);
				goto error;
			} else {
				*index = ret;
				return 0;
			}
		case '?':
			/* for unknown options, an error message is printed automatically */
			printf("\n");
			goto error;
		default:
			/* should be unreachable */
			break;
		}
	}

	/* no subcommands implies -h/--help */
	print_help(argv[0]);
	exit(EXIT_SUCCESS);
error:
	print_help(argv[0]);
	return -1;
}

int main(int argc, char **argv)
{
	int index;
	int ret;

	if (parse_options(argc, argv, &index) < 0) {
		ret = -1;
		goto finish;
	}

	/* Reset optind to 0 (instead of the default value, 1) at this point,
	 * because option parsing is restarted for the given subcommand, and
	 * subcommand optstrings are not prefixed with "-", which is a GNU
	 * extension. See the getopt_long(3) NOTES section.
	 */
	optind = 0;

	ret = commands[index].mainfunc(argc - 1, argv + 1);

finish:
	return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
