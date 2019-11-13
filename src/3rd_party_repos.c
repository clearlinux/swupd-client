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

static int repo_name_cmp(const void *repo, const void *name)
{
	return strcmp(((struct repo *)repo)->name, name);
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

	repo = list_search(*repos, section, repo_name_cmp);
	if (!repo) {
		repo = calloc(1, sizeof(struct repo));
		ON_NULL_ABORT(repo);
		repo->name = strdup_or_die(section);
		*repos = list_append_data(*repos, repo);
	}

	if (strcasecmp(key, "url") == 0) {
		if (repo->url) {
			free(repo->url);
		}
		repo->url = strdup_or_die(value);
	} else if (strcasecmp(key, "version") == 0) {
		if (repo->version) {
			free(repo->version);
		}
		repo->version = strdup_or_die(value);
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

void repo_free(struct repo *repo)
{
	free(repo->name);
	free(repo->url);
	free(repo->version);
	free(repo);
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

static int adjust_repo_config_file()
{
	struct list *repo;
	int ret = 0;
	FILE *fp = NULL;
	char *repo_config_file_path;

	repo_config_file_path = get_repo_config_path();
	// open it in trunc mode as we re-write all the contents again
	fp = fopen(repo_config_file_path, "w+");
	repo = list_head(repos);
	while (repo) {
		// If we are here, we are cleared to write to file
		struct repo *repo_temp = repo->data;
		ret = config_write_section(fp, repo_temp->name);
		if (ret) {
			goto exit;
		}

		// write url
		ret = config_write_config(fp, "url", repo_temp->url);
		repo = repo->next;
	}

exit:
	if (fp) {
		fclose(fp);
	}
	free_string(&repo_config_file_path);
	return ret;
}

/**
 * @brief This callback function helps remove_repo perform an item by item comparision
 * from the repos list.
 *
 * @param repo_item Each repo_item retreived from a list
 * @param repo_name_find A string containing the repo_name the user passed
 *
 * @returns 0 on success ie: a repo is found, any other value on a mismatch to
 * move to next element in list.
 */
int compare_repos(const void *repo_item, const void *repo_name_find)
{
	struct repo *repo = (struct repo *)repo_item;
	char *repo_name = (char *)repo_name_find;
	int ret = strncmp(repo->name, repo_name, strlen(repo->name));
	if (!ret) {
		info("Match found for repository: %s\n", repo_name);
	}
	return ret;
}

int remove_repo_from_config(char *repo_name)
{
	struct repo *repo;
	struct list *iter;
	int ret = 0;

	if (repo_config_init()) {
		ret = -1;
		goto exit;
	}

	iter = list_head(repos);
	repo = list_remove(repo_name, &iter, compare_repos);
	repos = iter;

	if (!repo) {
		info("Repository not found\n");
		ret = -1;
	} else {
		ret = adjust_repo_config_file();
		if (ret) {
			error("Failed while adjusting repository config file");
		}
	}

exit:
	repo_config_deinit();
	return ret;
}
#endif
