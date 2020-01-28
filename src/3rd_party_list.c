/*
 *   Software Updater - client side
 *
 *      Copyright © 2019 Intel Corporation.
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

#define _GNU_SOURCE
#include "3rd_party_repos.h"
#include "swupd.h"

#ifdef THIRDPARTY

static void print_help(void)
{
	/* TODO(castulo): we need to change this description to match that of the
	 * documentation once the documentation for this content is added */
	print("Lists the 3rd-party repositories defined in the system\n\n");
	print("Usage:\n");
	print("   swupd 3rd-party list\n\n");

	global_print_help();
	print("\n");
}

static const struct global_options opts = {
	NULL,
	0,
	NULL,
	print_help,
};

static bool parse_options(int argc, char **argv)
{
	int ind = global_parse_options(argc, argv, &opts);

	if (ind < 0) {
		return false;
	}

	if (argc > ind) {
		error("Unexpected arguments\n\n");
		return false;
	}

	return true;
}

static void list_repos(void)
{
	struct list *repos, *l;

	repos = third_party_get_repos();
	info("Swupd 3rd-party repositories:\n");
	for (l = repos; l; l = l->next) {
		struct repo *repo = l->data;
		info(" - ");
		print("%s: %s\n", repo->name, repo->url)
	}
	info("\nTotal: %d\n", list_len(repos));
	list_free_list_and_data(repos, repo_free_data);
}

enum swupd_code third_party_list_main(int argc, char **argv)
{
	enum swupd_code ret = SWUPD_OK;
	const int step_in_third_party_list = 0;

	if (!parse_options(argc, argv)) {
		print("\n");
		print_help();
		return SWUPD_INVALID_OPTION;
	}
	ret = swupd_init(SWUPD_ALL);
	if (ret != SWUPD_OK) {
		return ret;
	}
	progress_init_steps("third-party-list", step_in_third_party_list);

	list_repos();

	swupd_deinit();
	progress_finish_steps(ret);

	return ret;
}

#endif
