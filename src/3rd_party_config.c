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

#include "lib/config_file.h"
#include "lib/list.h"
#include "lib/sys.h"
#include "swupd.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef THIRDPARTY

/** @brief Internal Memory representation of a repo.  */
struct repo {
	char *name;
	char *url;
};

/** @brief List of repos. */
struct list *repos;

/**
 * @brief This function is called by the repo ini parse.
 *
 * @param section A string containing section of the file
 * @param key A string containing key
 * @param value A string containing value
 *
 * @returns True for every valid line which could be extracted, false otherwise
 */
bool parse_key_values(char *section, char *key, char *value, void UNUSED_PARAM *data)
{
	struct repo *repo;
	char *lkey;

	lkey = str_tolower(key);
	if (strcmp(lkey, "url") == 0) {
		repo = calloc(1, sizeof(struct repo));
		ON_NULL_ABORT(repo);
		repo->name = strdup_or_die(section);
		repo->url = strdup_or_die(value);
		repos = list_append_data(repos, repo);
		repos = list_head(repos);
	}
	free_string(&lkey);

	return true;
}

static char *get_repo_config_path(void)
{
	char *repo_config_file_path;
	string_or_die(&repo_config_file_path, "%s/%s/%s", globals.state_dir, "3rd_party", "repo.ini");
	return repo_config_file_path;
}

static bool repo_config_read(void)
{
	bool parse_ret = false;
	char *repo_config_file_path = get_repo_config_path();

	parse_ret = config_file_parse(repo_config_file_path, parse_key_values, NULL);
	free_string(&repo_config_file_path);
	return parse_ret;
}

/* TODO move to 3rd-party helpers */
static void repo_free(struct repo *repo)
{
	if (!repo) {
		return;
	}

	free_string(&repo->name);
	free_string(&repo->url);
	free(repo);
}

/* TODO move to 3rd-party helpers */
static void repo_free_data(void *data)
{
	struct repo *repo = (struct repo *)data;
	repo_free(repo);
}

static int third_party_config_create_dir(void)
{
	int ret = 0;
	char *repo_config_file_path = get_repo_config_path();
	char *repo_config_dir = sys_dirname(repo_config_file_path);

	ret = mkdir_p(repo_config_dir);
	if (ret) {
		error("Failed to create repository config directory\n");
	}

	free_string(&repo_config_dir);
	free_string(&repo_config_file_path);
	return ret;
}

static int third_party_config_create_file(void)
{
	char *repo_config_file_path = get_repo_config_path();
	int ret = 0;
	int fd = open(repo_config_file_path, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd < 0 && errno != EEXIST) {
		error("Failed to create repo config file, error: %d", errno);
		ret = -1;
	}

	close(fd);
	free_string(&repo_config_file_path);
	return ret;
}

int repo_config_init(void)
{
	int ret = 0;
	ret = third_party_config_create_dir();

	if (ret) {
		goto exit;
	}

	ret = third_party_config_create_file();

	if (ret) {
		goto exit;
	}

	if (!repo_config_read()) {
		error("Failed to read & parse repo config file");
		ret = -1;
	}

exit:
	return ret;
}

void repo_config_deinit(void)
{
	list_free_list_and_data(repos, repo_free_data);
}

/**
 * @brief This function performs the repo config operation part for repo add
 * It checks if a repo with repo_name already exists and if not creates an
 * entry in the repo config file.
 *
 * @param repo_name A string containing repo_name
 * @param repo_url A string containing repo_url
 *
 * @returns 0 on success ie: a repo with the name is successfully added to repo config
 * otherwise a -1 on any failure
 */
int add_repo_config(char *repo_name, char *repo_url)
{
	struct list *repo;
	int ret = 0;
	FILE *fp = NULL;
	char *repo_config_file_path;

	if (repo_config_init()) {
		ret = -1;
		goto exit;
	}

	repo = list_head(repos);
	while (repo) {
		struct repo *repo_iter = repo->data;
		if (!strncmp(repo_iter->name, repo_name, strlen(repo_name))) {
			error("The repo: %s already exists\n", repo_name);
			ret = -1;
			goto exit;
		}
		repo = repo->next;
	}

	// If we are here, we are cleared to write to file
	repo_config_file_path = get_repo_config_path();
	fp = fopen(repo_config_file_path, "a");

	ret = config_write_section(fp, repo_name);
	if (ret) {
		goto exit;
	}

	// write url
	ret = config_write_config(fp, "url", repo_url);

exit:
	if (fp) {
		fclose(fp);
	}
	free_string(&repo_config_file_path);
	repo_config_deinit();
	return ret;
}

#endif
