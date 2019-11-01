#include "3rd_party_internal.h"
#include "config.h"
#include "lib/config_parser.h"
#include "lib/list.h"
#include "lib/sys.h"
#include "swupd.h"
#include <errno.h>
#include <stdlib.h>

struct global {
	void *value1;
	void *value2;
};

struct repo {
	char *repo_name;
	char *url;
};

struct list *repos_kv;

#define REPO_CONFIG_DIR "3rd_party"
#define REPO_CONFIG_CONFIG_FILE "repo.ini"
#define MAX_REPO_ALLOWED_FIELDS 2
#define MAX_GLOBAL_ALLOWED_FIELDS 2
#define MAX_FILE_PATH 1000

static char *repo_config_file_path;

bool parse_key_values(char *section, char *key, char *value, void UNUSED_PARAM *data)
{
	static int repo_field_count = 0;
	static int global_field_count = 0;
	bool ret = false;

	if (!strncmp(section, "REPO", strlen(section))) {
		if (key && value) {
			static struct repo *repo_var;
			if (repo_field_count == 0) {
				repo_var = (struct repo *)malloc(sizeof(struct repo));
			}
			if (!strncmp(key, "repo-name", strlen(key))) {
				repo_var->repo_name = strdup_or_die(value);
				repo_field_count++;
				ret = true;
			}

			if (!strncmp(key, "repo-url", strlen(key))) {
				repo_var->url = strdup_or_die(value);
				repo_field_count++;
				ret = true;
			}

			if (repo_field_count == MAX_REPO_ALLOWED_FIELDS) {
				repos_kv = list_append_data(repos_kv, repo_var);
				repo_field_count = 0;
			}
		}
	} else if (!strncmp(section, "GLOBAL", strlen(section))) {
		if (key && value) {
			static struct global *global_var;
			if (global_field_count > 2) {
				return false;
			}

			if (!strncmp(key, "value1", strlen(key))) {
				global_var->value1 = strdup_or_die(value);
				global_field_count++;
				ret = true;
			}

			if (!strncmp(key, "value2", strlen(key))) {
				global_var->value2 = strdup_or_die(value);
				global_field_count++;
				ret = true;
			}
		}
	}
	return ret;
}

static void generate_path()
{
	char *repo_config_dir;
	repo_config_dir = sys_path_join(globals.state_dir, REPO_CONFIG_DIR);
	repo_config_file_path = sys_path_join(repo_config_dir, REPO_CONFIG_CONFIG_FILE);
	free_string(&repo_config_dir);
}

static inline int ensure_repo_config_dir()
{
	char *repo_config_dir;
	int ret = 0;
	repo_config_dir = sys_dirname(repo_config_file_path);
	if (!is_dir(repo_config_dir)) {
		ret = mkdir_p(repo_config_dir);
		if (ret) {
			error("Failed to create repository config directory\n");
		}
	}

	free_string(&repo_config_dir);
	return ret;
}

static int write_section(char *name)
{
	int ret = 0, fd;
	ssize_t write_line_len;

	fd = open(repo_config_file_path, O_WRONLY | O_APPEND);
	if (fd < 0) {
		return -1;
	}

	write_line_len = strlen(name);

	if (write_line_len != write(fd, name, write_line_len)) {
		error("error writing to file %d", errno);
		ret = -1;
	}

	close(fd);
	return ret;
}

static int write_kv(char *key, char *value)
{
	char *write_line;
	int ret = 0;
	ssize_t write_line_len;

	int fd = open(repo_config_file_path, O_WRONLY | O_APPEND);
	if (fd < 0) {
		return -1;
	}

	string_or_die(&write_line, "%s=%s\n", key, value);
	write_line_len = strlen(write_line);

	if (write_line_len != write(fd, write_line, write_line_len)) {
		error("error writing to file %d", errno);
		ret = -1;
	}

	close(fd);
	free_string(&write_line);
	return ret;
}

static int write_empty_line()
{
	int ret = 0;
	char *write_line = "\n";
	ssize_t write_line_length = strlen(write_line);

	int fd = open(repo_config_file_path, O_WRONLY | O_APPEND);
	if (fd < 0) {
		return -1;
	}

	if (write_line_length != write(fd,
				       write_line,
				       write_line_length)) {
		error("error writing to file %d", errno);
		ret = -1;
	}

	close(fd);
	return ret;
}

static bool repo_config_read()
{
	return config_parse(repo_config_file_path, parse_key_values, NULL);
}

static void repo_config_read_free()
{
	list_free_list(repos_kv);
}

static int ensure_repo_config()
{
	int fd = open(repo_config_file_path, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd < 0 && errno != EEXIST) {
		error("Failed to create repo config file, error: %d", errno);
		return -1;
	}
	close(fd);

	return 0;
}

static int repo_config_init()
{
	int ret = 0;
	generate_path();
	ret = ensure_repo_config_dir();

	if (ret) {
		goto exit;
	}

	ret = ensure_repo_config();

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

static int repo_config_deinit()
{
	repo_config_read_free();
	free_string(&repo_config_file_path);
	return 0;
}

int add_repo_config(char *repo_name, char *repo_url)
{
	struct list *repo;
	int ret = 0;

	if (repo_config_init()) {
		ret = -1;
		goto exit;
	}

	repo = list_head(repos_kv);
	while (repo) {
		struct repo *repo_iter = repo->data;
		if (!strncmp(repo_iter->repo_name, repo_name, strlen(repo_name))) {
			error("The repo: %s already exists\n", repo_name);
			ret = -1;
		}
		repo = repo->next;
	}
	ret = write_section("[REPO]\n");
	ret = write_kv("repo-name", repo_name);
	ret = write_kv("repo-url", repo_url);
	ret = write_empty_line();

exit:
	repo_config_deinit();
	return ret;
}

int list_repos()
{
	struct list *repo;
	int ret = 0;

	if (repo_config_init()) {
		ret = -1;
		goto exit;
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

exit:
	repo_config_deinit();
	return ret;
}
