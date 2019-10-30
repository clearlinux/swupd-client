#include "3rd_party_internal.h"
#include "config.h"
#include "lib/config_parser.h"
#include "lib/list.h"
#include "swupd.h"
#include <stdlib.h>

struct global {
	void *value1;
	void *value2;
};

struct repo {
	char *repo_name;
	char *url;
};

#define MAX_REPO_ALLOWED_FIELDS 2
#define MAX_GLOBAL_ALLOWED_FIELDS 2

struct list *repos_kv;

bool parse_key_values(char *section, char *key, char *value, void *data)
{
	static int repo_field_count = 0;
	static int global_field_count = 0;

	if (!strncmp(section, "REPO", strlen(section))) {
		if (key && value) {
			static struct repo *repo_var;
			if (repo_field_count == 0) {
				repo_var = (struct repo *)malloc(sizeof(struct repo));
			}
			if (!strncmp(key, "repo-name", strlen(key))) {
				repo_var->repo_name = strdup_or_die(value);
				repo_field_count++;
			}

			if (!strncmp(key, "repo-url", strlen(key))) {
				repo_var->url = strdup_or_die(value);
				repo_field_count++;
			}

			if (repo_field_count == MAX_REPO_ALLOWED_FIELDS) {
				repos_kv = list_append_data(repos_kv, repo_var);
				repo_field_count = 0;
			}
		}
		return true;
	} else if (!strncmp(section, "GLOBAL", strlen(section))) {
		if (key && value) {
			static struct global *global_var;
			if (global_field_count > 2) {
				return false;
			}

			if (!strncmp(key, "value1", strlen(key))) {
				global_var->value1 = strdup_or_die(value);
				global_field_count++;
			}

			if (!strncmp(key, "value2", strlen(key))) {
				global_var->value2 = strdup_or_die(value);
				global_field_count++;
			}
		}
		return true;
	}
	return false;
}

static inline char *get_repo_config_file()
{
	return sys_path_join(globals.state_dir, "/3rd_party/repo.ini");
}

static int create_repo_config(char *repo_config_file)
{
	int fd = open(repo_config_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	free_string(&repo_config_file);
	if (fd < 0) {
		return -1;
	}
	close(fd);

	return SWUPD_OK;
}

static inline int setup_repo_config()
{
	char *repo_config_file;
	int ret;
	repo_config_file = get_repo_config_file();

	sys_file_exists(repo_config_file);

	if (create_repo_config(repo_config_file)) {
		debug("Issue creating repo config file: %s in %s\n", repo_config_file, globals.state_dir);
		goto finish;
	}

finish:
	free_string(&repo_config_file);
	return ret;
}

static inline bool repo_config_exists(const char *repo_config_file)
{
	return sys_file_exists(repo_config_file);
}

int repo_config_init()
{
	char *repo_config_file;
	repo_config_file = get_repo_config_file();
	if (!repo_config_exists(repo_config_file)) {
		return -1;
	}

	config_parse(repo_config_file, parse_key_values, NULL);

	free_string(&repo_config_file);
	return 0;
}

int write_section(char *name)
{
	struct list *repo;

	repo = list_head(repos_kv);
	while (repo) {
		struct repo *repo_iter = repo->data;
		if (!strncmp(repo_iter->repo_name, name, strlen(name))) {
			error("The repo with %s already exists", name);
			return -1;
		}
		repo = repo->next;
	}
	return 0;
}

static inline char *get_repo_path(char *repo_name)
{
	return sys_path_join(THIRDPARTY_REPO_PREFIX, repo_name);
}

void repo_config_deinit()
{
	list_free_list(repos_kv);
}

int list_repos()
{
	struct list *repo;

	if (repo_config_init()) {
		return -1;
	}

	repo = list_head(repos_kv);
	while (repo) {
		struct repo *repo_iter = repo->data;
		info("Repo\n");
		info("--------\n");
		info("name:\t%s\n", repo_iter->repo_name);
		info("url:\t%s\n", repo_iter->url);
		info("\n");
		repo = repo->next;
	}

	repo_config_deinit();
	return 0;
}
