/*
 *   Software Updater - client side
 *
 *      Copyright © 2019-2025 Intel Corporation.
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

#include "cmds/swupd_cmds.h"
#include "swupd.h"

#ifdef THIRDPARTY

static struct subcmd third_party_commands[] = {
	{ "add", "Add third party repository", third_party_add_main },
	{ "remove", "Remove third party repository", third_party_remove_main },
	{ "list", "List third party repository", third_party_list_main },
	{ "info", "Show the version of third party repositories and the update URLs", third_party_info_main },
	{ "check-update", "Check if a new version of a third party repository is available", third_party_check_update_main },
	{ "update", "Update to latest version of a third party repository", third_party_update_main },
	{ "bundle-add", "Install a bundle from a third party repository", third_party_bundle_add_main },
	{ "bundle-remove", "Remove a bundle from a third party repository", third_party_bundle_remove_main },
	{ "bundle-list", "List bundles from a third party repository", third_party_bundle_list_main },
	{ "bundle-info", "Display information about a bundle in a third party repository", third_party_bundle_info_main },
	{ "diagnose", "Verify content from a third party repository", third_party_diagnose_main },
	{ "repair", "Repair local issues relative to a third party repository", third_party_repair_main },
	{ "clean", "Clean cached files of a third party repository", third_party_clean_main },
	{ 0 }
};

static void print_help(const char *name)
{
	print("Gives users the ability for handling custom content not provided upstream\n\n");
	print("Usage:\n");
	print("    swupd %s SUBCOMMAND [OPTION...]\n\n", sys_basename(name));
	print("Help Options:\n");
	print("   -h, --help              Show help options\n\n");
	print("Subcommands:\n");

	struct subcmd *entry = third_party_commands;

	while (entry->name != NULL) {
		print("   %-20s    %-30s\n", entry->name, entry->doc);
		entry++;
	}
	print("\n");
	print("To view subcommand options, run `swupd %s SUBCOMMAND --help'\n", sys_basename(name));
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ 0 }
};

static int parse_options(int argc, char **argv, int *index)
{
	int opt;
	int ret;

	/* The leading "-" in the optstring is required to preserve option parsing order */
	while ((opt = getopt_long(argc, argv, "-h", prog_opts, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case '\01':
			/* found a subcommand, or a random non-option argument */
			ret = subcmd_index(optarg, third_party_commands);
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

enum swupd_code third_party_main(int argc, char **argv)
{
	int index = 0, ret;

	if (parse_options(argc, argv, &index) < 0) {
		return SWUPD_INVALID_OPTION;
	}
	optind = 0;
	telemetry_disable();
	ret = third_party_commands[index].mainfunc(argc - 1, argv + 1);

	return ret;
}

#endif
