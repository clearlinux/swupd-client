/*
 *   Software Updater - client side
 *
 *      Copyright © 2015-2016 Intel Corporation.
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
 */

#define _GNU_SOURCE // for basename()
#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swupd.h"
#include "swupd_build_opts.h"
#include "swupd_internal.h"

struct subcmd {
	char *name;
	char *doc;
	enum swupd_code (*mainfunc)(int, char **);
};

static struct subcmd commands[] = {
	{ "autoupdate", "Enable/disable automatic system updates", autoupdate_main },
	{ "bundle-add", "Install a new bundle", bundle_add_main },
	{ "bundle-remove", "Uninstall a bundle", bundle_remove_main },
	{ "bundle-list", "List installed bundles", bundle_list_main },
	{ "hashdump", "Dump the HMAC hash of a file", hashdump_main },
	{ "update", "Update to latest OS version", update_main },
	{ "verify", "Verify content for OS version", verify_main },
	{ "check-update", "Check if a new OS version is available", check_update_main },
	{ "search-file", "Command to search files in Clear Linux bundles", search_main },
	{ "info", "Show the version and the update URLs", info_main },
	{ "clean", "Clean cached files", clean_main },
	{ "mirror", "Configure mirror url for swupd content", mirror_main },
	{ "os-install", "Install Clear Linux OS to a blank partition or directory", install_main },
#ifdef EXTERNAL_MODULES_SUPPORT
	{ "search", "Searches for the best bundle to install a binary or library", binary_loader_main },
#endif
	{ 0 }
};

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ 0 }
};

static void print_help(const char *name)
{
	print("Usage:\n");
	print("    %s [OPTION...]\n", basename((char *)name));
	print(" or %s [OPTION...] SUBCOMMAND [OPTION...]\n\n", basename((char *)name));
	print("Help Options:\n");
	print("   -h, --help              Show help options\n");
	print("   -v, --version           Output version information and exit\n\n");
	print("Subcommands:\n");

	struct subcmd *entry = commands;

	while (entry->name != NULL) {
		print("   %-20s    %-30s\n", entry->name, entry->doc);
		entry++;
	}
	print("\n");
	print("To view subcommand options, run `%s SUBCOMMAND --help'\n", basename((char *)name));
}

/* this function prints the copyright message for the --version command */
static void copyright_header(void)
{
	print(PACKAGE " " VERSION "\n");
	print("   Copyright (C) 2012-2019 Intel Corporation\n");
	print("\n");
}

static void print_compile_opts(void)
{
	info("Compile-time options: %s\n", BUILD_OPTS);
	info("Compile-time configuration:\n%s\n", BUILD_CONFIGURE);
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
	while ((opt = getopt_long(argc, argv, "-hv", prog_opts, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'v':
			copyright_header();
			print_compile_opts();
			exit(EXIT_SUCCESS);
		case '\01':
			/* found a subcommand, or a random non-option argument */
			ret = subcmd_index(optarg);
			if (ret < 0) {
				error("unrecognized subcommand `%s'\n\n",
				      optarg);
				goto error;
			} else {
				*index = ret;
				return 0;
			}
		case '?':
			/* for unknown options, an error message is printed automatically */
			info("\n");
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

	/* Set locale to system locale
	 * Change from the standard (C) to system locale, so libarchive can
	 * handle filename conversions.
	 * More information on the libarchive problem:
	 * https://github.com/libarchive/libarchive/wiki/Filenames */
	setlocale(LC_ALL, "");
	if (parse_options(argc, argv, &index) < 0) {
		return SWUPD_INVALID_OPTION;
	}

	save_cmd(argv);
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	/* Reset optind to 0 (instead of the default value, 1) at this point,
	 * because option parsing is restarted for the given subcommand, and
	 * subcommand optstrings are not prefixed with "-", which is a GNU
	 * extension. See the getopt_long(3) NOTES section.
	 */
	optind = 0;

	ret = commands[index].mainfunc(argc - 1, argv + 1);

	return ret;
}
