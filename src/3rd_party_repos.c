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

#include "3rd_party_repos.h"
#include "swupd.h"

#include <errno.h>

#ifdef THIRDPARTY

static char *get_repo_config_path(void)
{
	return str_or_die("%s/%s/%s", globals.state_dir, "3rd_party", "repo.ini");
}

/**
 * @brief This function is called by the repo ini parse.
 *
 * @param section A string containing section of the file
 * @param key A string containing key
 * @param value A string containing value
 *
 * @returns True for every valid line which could be extracted, false otherwise
 */
static bool parse_key_values(char *section, char *key, char *value, void *data)
{
	struct repo *repo;
	struct list **repos = data;

	//TODO: Extend to support other fields on the config file too.
	//TODO: Don't load duplicated entries in the list
	if (strcasecmp(key, "url") == 0) {
		repo = calloc(1, sizeof(struct repo));
		ON_NULL_ABORT(repo);
		repo->name = strdup_or_die(section);
		repo->url = strdup_or_die(value);
		*repos = list_append_data(*repos, repo);
	}

	return true;
}

struct list *third_party_get_repos(void)
{
	char *repo_config_file_path = get_repo_config_path();
	struct list *repos = NULL;

	if (!config_file_parse(repo_config_file_path, parse_key_values, &repos)) {
		list_free_list_and_data(repos, repo_free_data);
		repos = NULL;
	}

	repos = list_head(repos);
	free_string(&repo_config_file_path);

	return repos;
}

static int repo_name_cmp(const void *repo, const void *name)
{
	return strcmp(((struct repo *)repo)->name, name);
}

void repo_free(struct repo *repo)
{
	free(repo->name);
	free(repo->url);
}

void repo_free_data(void *data)
{
	struct repo *repo = (struct repo *)data;
	repo_free(repo);
}

int third_party_add_repo(const char *repo_name, const char *repo_url)
{
	int ret = 0;
	struct list *repos;
	char *repo_config_file_path = NULL;
	FILE *fp = NULL;

	repos = third_party_get_repos();
	if (list_search(repos, repo_name, repo_name_cmp)) {
		error("The repo: %s already exists\n", repo_name);
		ret = -EEXIST;
		goto exit;
	}

	// If we are here, we are cleared to write to file
	repo_config_file_path = get_repo_config_path();
	fp = fopen(repo_config_file_path, "a");
	if (!fp) {
		ret = -errno;
	}

	// write name
	ret = config_write_section(fp, repo_name);
	if (ret) {
		goto exit;
	}

	// write url and version
	ret = config_write_config(fp, "url", repo_url);
	ret = config_write_config(fp, "version", "0");

exit:
	if (fp) {
		fclose(fp);
	}

	free(repo_config_file_path);
	list_free_list_and_data(repos, repo_free_data);
	return ret;
}

#endif
