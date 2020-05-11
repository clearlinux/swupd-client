/*
 *   Software Updater - client side
 *
 *      Copyright © 2019-2020 Intel Corporation.
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
#include "swupd_lib/signature.h"
#include "swupd.h"

#include <errno.h>
#include <fcntl.h>

#ifdef THIRDPARTY

char *third_party_get_bin_dir(void)
{
	return sys_path_join("%s/%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_BIN_DIR);
}

char *third_party_get_binary_path(const char *binary_name)
{
	return sys_path_join("%s/%s/%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_BIN_DIR, binary_name);
}

static char *get_repo_config_path(void)
{
	return sys_path_join("%s/%s/%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_DIR, "repo.ini");
}

char *get_repo_content_path(const char *repo_name)
{
	return sys_path_join("%s/%s/%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_BUNDLES_DIR, repo_name);
}

static char *get_repo_state_dir(const char *repo_name)
{
	return sys_path_join("%s/%s/%s", globals_bkp.state_dir, SWUPD_3RD_PARTY_DIRNAME, repo_name);
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

	if (!section) {
		// 3rd party doesn't have any key to be set outside of a section
		// Ignoring it
		return true;
	}

	repo = list_search(*repos, section, cmp_repo_name_string);
	if (!repo) {
		repo = malloc_or_die(sizeof(struct repo));
		repo->name = strdup_or_die(section);
		*repos = list_append_data(*repos, repo);
	}

	if (strcasecmp(key, "url") == 0) {
		if (repo->url) {
			FREE(repo->url);
		}
		repo->url = strdup_or_die(value);
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
	FREE(repo_config_file_path);

	return repos;
}

void repo_free(struct repo *repo)
{
	if (repo) {
		FREE(repo->name);
		FREE(repo->url);
	}
	FREE(repo);
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
	char *repo_config_file = NULL;
	char *repo_config_file_path = NULL;
	FILE *fp = NULL;

	repos = third_party_get_repos();
	if (list_search(repos, repo_name, cmp_repo_name_string)) {
		error("The repository %s already exists\n", repo_name);
		ret = -EEXIST;
		goto exit;
	}
	if (list_search(repos, repo_url, cmp_repo_url_string)) {
		error("The specified URL %s is already assigned to another repository\n", repo_url);
		ret = -EEXIST;
		goto exit;
	}

	// If we are here, we are cleared to write to file
	repo_config_file = get_repo_config_path();
	repo_config_file_path = sys_dirname(repo_config_file);
	mkdir_p(repo_config_file_path);
	fp = fopen(repo_config_file, "a");
	if (!fp) {
		ret = -errno;
		goto exit;
	}

	// write name
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

	FREE(repo_config_file);
	FREE(repo_config_file_path);
	list_free_list_and_data(repos, repo_free_data);
	return ret;
}

static int write_repo(FILE *fp, struct repo *repo)
{
	int ret;

	ret = config_write_section(fp, repo->name);
	if (ret) {
		return ret;
	}

	if (repo->url) {
		ret = config_write_config(fp, "url", repo->url);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int overwrite_repo_config_file(struct list *repos)
{
	struct list *repo;
	int ret = 0;
	FILE *fp = NULL;
	char *repo_config_file_path;

	repo_config_file_path = get_repo_config_path();
	// open it in trunc mode as we re-write all the contents again
	fp = fopen(repo_config_file_path, "w");
	if (!fp) {
		error("Unable to open file %s for writing\n", repo_config_file_path);
		goto exit;
	}

	repo = repos;
	while (repo) {
		ret = write_repo(fp, repo->data);
		if (ret < 0) {
			error("Unable to write to file %s\n", repo_config_file_path);
			goto exit;
		}
		repo = repo->next;
	}

exit:
	if (fp) {
		fclose(fp);
	}
	FREE(repo_config_file_path);
	return ret;
}

int third_party_remove_repo(const char *repo_name)
{
	struct repo *repo;
	struct list *repos;
	int ret = 0;

	repos = third_party_get_repos();
	repo = list_remove((void *)repo_name, &repos, cmp_repo_name_string);

	if (!repo) {
		error("Repository not found\n");
		ret = -ENOENT;
		goto exit;
	}

	repo_free(repo);
	ret = overwrite_repo_config_file(repos);
	if (ret) {
		error("Failed while adjusting repository config file\n");
	}

exit:
	list_free_list_and_data(repos, repo_free_data);
	return ret;
}

int third_party_remove_repo_directory(const char *repo_name)
{
	char *repo_dir;
	int ret = 0;
	int ret_code = 0;

	//TODO: use a global function to get this value
	repo_dir = get_repo_content_path(repo_name);
	ret = sys_rm_recursive(repo_dir);
	if (ret < 0 && ret != -ENOENT) {
		error("Failed to delete repository directory\n");
		ret_code = ret;
	}
	FREE(repo_dir);

	repo_dir = get_repo_state_dir(repo_name);
	ret = sys_rm_recursive(repo_dir);
	if (ret < 0 && ret != -ENOENT) {
		error("Failed to delete repository state directory\n");
		ret_code = ret;
	}
	FREE(repo_dir);

	return ret_code;
}

enum swupd_code third_party_set_repo(struct repo *repo, bool sigcheck)
{
	char *repo_state_dir;
	char *repo_path_prefix;
	char *repo_cert_path;

	set_content_url(repo->url);
	set_version_url(repo->url);

	/* set up swupd to use the certificate from the 3rd-party repository,
	 * unless the user is specifying a path for the certificate to use */
	repo_cert_path = sys_path_join("%s/%s/%s/%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_BUNDLES_DIR, repo->name, CERT_PATH);
	if (!globals.user_defined_cert_path) {
		/* the user did not specify a cert, use repo's default */
		set_cert_path(repo_cert_path);
	}

	/* if --nosigcheck was used, we do not attempt any signature checking */
	if (sigcheck) {
		signature_deinit();
		if (!signature_init(globals.cert_path, NULL)) {
			signature_deinit();
			error("Unable to validate the certificate %s\n\n", repo_cert_path);
			FREE(repo_cert_path);
			return SWUPD_SIGNATURE_VERIFICATION_FAILED;
		}
	}
	FREE(repo_cert_path);

	repo_path_prefix = get_repo_content_path(repo->name);
	set_path_prefix(repo_path_prefix);
	FREE(repo_path_prefix);

	/* make sure there are state directories for the 3rd-party
	 * repo if not there already */
	repo_state_dir = get_repo_state_dir(repo->name);
	if (statedir_create_dirs(repo_state_dir)) {
		FREE(repo_state_dir);
		error("Unable to create the state directories for repository %s\n\n", repo->name);
		return SWUPD_COULDNT_CREATE_DIR;
	}
	statedir_set_path(repo_state_dir);
	FREE(repo_state_dir);

	return SWUPD_OK;
}

static enum swupd_code third_party_find_bundle(const char *bundle, struct list *repos, struct repo **found_repo)
{
	struct list *iter = NULL;
	int count = 0;
	int version;
	enum swupd_code ret_code = SWUPD_OK;

	info("Searching for bundle %s in the 3rd-party repositories...\n", bundle);
	for (iter = repos; iter; iter = iter->next) {
		struct repo *repo = iter->data;
		struct manifest *mom = NULL;
		struct file *file = NULL;

		/* set the appropriate content_dir and state_dir for the selected 3rd-party repo */
		ret_code = third_party_set_repo(repo, globals.sigcheck);
		if (ret_code != SWUPD_OK) {
			goto clean_and_exit;
		}

		/* get repo's version */
		version = get_current_version(globals.path_prefix);
		if (version < 0) {
			error("Unable to determine current version for repository %s\n\n", repo->name);
			ret_code = SWUPD_CURRENT_VERSION_UNKNOWN;
			goto clean_and_exit;
		}

		/* load the repo's MoM*/
		mom = load_mom(version, NULL);
		if (!mom) {
			error("Cannot load manifest MoM for 3rd-party repository %s version %i\n\n", repo->name, version);
			ret_code = SWUPD_COULDNT_LOAD_MOM;
			goto clean_and_exit;
		}

		/* search for the bundle in the MoM */
		file = mom_search_bundle(mom, bundle);
		if (file) {
			/* the bundle was found in this repo, keep a pointer to the repo */
			info("Bundle %s found in 3rd-party repository %s\n", bundle, repo->name);
			count++;
			*found_repo = repo;
		}
		manifest_free(mom);
	}

	/* if the bundle was not found or exists in more than one repo,
	 * return NULL */
	if (count == 0) {
		error("bundle %s was not found in any 3rd-party repository\n\n", bundle);
		ret_code = SWUPD_INVALID_BUNDLE;
	} else if (count > 1) {
		error("bundle %s was found in more than one 3rd-party repository\n", bundle);
		info("Please specify a repository using the --repo flag\n\n");
		ret_code = SWUPD_INVALID_OPTION;
	}

clean_and_exit:
	set_path_prefix(globals_bkp.path_prefix);
	statedir_set_path(globals_bkp.state_dir);
	set_content_url(globals_bkp.content_url);
	set_version_url(globals_bkp.version_url);

	return ret_code;
}

enum swupd_code third_party_run_operation(struct list *bundles, const char *repo, process_bundle_fn_t process_bundle_fn)
{
	struct list *repos = NULL;
	struct list *iter = NULL;
	struct repo *selected_repo = NULL;
	int ret;
	enum swupd_code ret_code = SWUPD_OK;

	/* load the existing 3rd-party repos from the repo.ini config file */
	repos = third_party_get_repos();

	/* try action on the bundles one by one */
	for (iter = bundles; iter; iter = iter->next) {
		char *bundle = iter->data;

		/* if the repo to be used was specified, use it, otherwise
		 * search for the bundle in all the 3rd-party repos */
		if (repo) {

			selected_repo = list_search(repos, repo, cmp_repo_name_string);
			if (!selected_repo) {
				error("3rd-party repository %s was not found\n\n", repo);
				ret_code = SWUPD_INVALID_REPOSITORY;
				goto clean_and_exit;
			}

		} else {

			/* search for the bundle in all repos */
			ret = third_party_find_bundle(bundle, repos, &selected_repo);
			if (ret) {
				ret_code = ret;
				/* something went wrong or wasn't found,
				 * go to the next bundle */
				continue;
			}
		}

		if (selected_repo) {

			/* set the appropriate content_dir and state_dir for the selected 3rd-party repo */
			ret = third_party_set_repo(selected_repo, globals.sigcheck);
			if (ret) {
				ret_code = ret;
				goto next;
			}

			/* perform appropriate action on the bundle */
			ret = process_bundle_fn(bundle);
			if (ret) {
				/* if we have an error here it is ok to overwrite any previous error */
				ret_code = ret;
			}
			info("\n");

			/* return the global variables to the original values */
		next:
			set_path_prefix(globals_bkp.path_prefix);
			statedir_set_path(globals_bkp.state_dir);
			set_content_url(globals_bkp.content_url);
			set_version_url(globals_bkp.version_url);
		}
	}

	/* free data */
clean_and_exit:
	list_free_list_and_data(repos, repo_free_data);

	return ret_code;
}

enum swupd_code third_party_run_operation_multirepo(const char *repo, process_bundle_fn_t process_bundle_fn, enum swupd_code expected_ret_code, const char *op_name, int op_steps)
{
	enum swupd_code ret_code = SWUPD_OK;
	enum swupd_code ret;
	struct list *repos = NULL;
	struct list *iter = NULL;
	struct repo *selected_repo = NULL;
	char *steps_title = NULL;
	int total_steps;

	string_or_die(&steps_title, "3rd-party-%s", op_name);

	/* load the existing 3rd-party repos from the repo.ini config file */
	repos = third_party_get_repos();

	/* initialize operation steps so progress can be reported */
	total_steps = repo ? op_steps : op_steps * list_len(repos);
	progress_init_steps(steps_title, total_steps);

	/* if the repo to be used was specified, use it,
	 * otherwise perform operation in all 3rd-party repos */
	if (repo) {

		selected_repo = list_search(repos, repo, cmp_repo_name_string);
		if (!selected_repo) {
			error("3rd-party repository %s was not found\n\n", repo);
			ret_code = SWUPD_INVALID_REPOSITORY;
			goto clean_and_exit;
		}

		/* set the appropriate variables for the selected 3rd-party repo */
		ret_code = third_party_set_repo(selected_repo, globals.sigcheck);
		if (ret_code) {
			goto clean_and_exit;
		}

		ret_code = process_bundle_fn(NULL);

	} else {

		for (iter = repos; iter; iter = iter->next) {
			selected_repo = iter->data;

			/* set the repo's header */
			third_party_repo_header(selected_repo->name);

			/* set the appropriate variables for the selected 3rd-party repo */
			ret = third_party_set_repo(selected_repo, globals.sigcheck);
			if (ret) {
				/* if one repo failed to set up, we can still try the rest of
				 * the repos, but keep the error */
				ret_code = ret;
				continue;
			}

			ret = process_bundle_fn(NULL);
			if (ret != expected_ret_code) {
				/* if the operation failed in any of the repos,
				 * keep the error */
				ret_code = ret;
			}

			info("\n\n");
		}
	}

	/* free data */
clean_and_exit:
	progress_finish_steps(ret_code);
	list_free_list_and_data(repos, repo_free_data);
	FREE(steps_title);

	return ret_code;
}

void third_party_repo_header(const char *repo_name)
{
	char *header = NULL;

	string_or_die(&header, " 3rd-Party Repository: %s", repo_name);
	print_header(header);
	if (log_is_quiet()) {
		print("[%s]\n", repo_name);
	}
	FREE(header);
}

bool third_party_file_is_binary(struct file *file)
{
	if (file->is_exported && is_binary(file->filename)) {
		return true;
	} else
		return false;
}

enum swupd_code third_party_process_files(struct list *files, const char *msg, const char *step, process_file_fn_t proc_file_fn)
{
	enum swupd_code ret, ret_code = SWUPD_OK;
	struct list *iter = NULL;
	struct file *file = NULL;
	int count = 0;
	int number_of_files = list_len(files);

	info("%s", msg);

	progress_next_step(step, PROGRESS_BAR);
	for (iter = list_head(files); iter; iter = iter->next) {
		file = iter->data;
		ret = proc_file_fn(file);
		if (ret) {
			/* at least one file failed to process */
			ret_code = ret;
		}
		count++;
		progress_report(count, number_of_files);
	}

	return ret_code;
}

enum swupd_code third_party_remove_binary(struct file *file)
{
	enum swupd_code ret_code = SWUPD_OK;
	int ret;
	char *script = NULL;
	char *binary = NULL;
	char *filename = NULL;

	if (!file || !third_party_file_is_binary(file)) {
		return ret_code;
	}

	filename = file->filename;
	script = third_party_get_binary_path(sys_basename(filename));
	binary = sys_path_join("%s/%s", globals.path_prefix, filename);

	if (!sys_file_exists(binary)) {
		ret = sys_rm(script);
		if (ret != 0 && ret != -ENOENT) {
			error("File %s could not be removed\n\n", script);
			ret_code = SWUPD_COULDNT_REMOVE_FILE;
		}
	}

	FREE(script);
	FREE(binary);

	return ret_code;
}

static enum swupd_code third_party_bin_directory_exist(void)
{
	enum swupd_code ret_code = SWUPD_OK;
	char *bin_directory = NULL;

	bin_directory = third_party_get_bin_dir();

	/* if the SWUPD_3RD_PARTY_BIN_DIR does not exist, attempt to create it */
	if (mkdir_p(bin_directory)) {
		error("The directory %s for 3rd-party content failed to be created\n", bin_directory);
		ret_code = SWUPD_COULDNT_CREATE_DIR;
		goto exit;
	}

	if (!sys_filelink_is_dir(bin_directory)) {
		error("The path %s for 3rd-party content exists but is not a directory\n", bin_directory);
		ret_code = SWUPD_COULDNT_CREATE_DIR;
	}

exit:
	FREE(bin_directory);
	return ret_code;
}

enum swupd_code third_party_create_wrapper_script(struct file *file)
{
	enum swupd_code ret_code = SWUPD_OK;
	int fd;
	FILE *fp = NULL;
	char *bin_directory = NULL;
	char *script = NULL;
	char *binary = NULL;
	char *third_party_bin_path = NULL;
	char *third_party_ld_path = NULL;
	char *third_party_xdg_data_path = NULL;
	char *third_party_xdg_conf_path = NULL;
	char *filename = NULL;
	mode_t mode = 0755;

	if (!file || !third_party_file_is_binary(file)) {
		return ret_code;
	}

	filename = file->filename;
	bin_directory = third_party_get_bin_dir();
	script = third_party_get_binary_path(sys_basename(filename));
	binary = sys_path_join("%s/%s", globals.path_prefix, filename);

	if (!sys_filelink_is_executable(binary)) {
		warn("File %s does not have 'execute' permission so it won't be exported\n", filename);
		goto close_and_exit;
	}

	if (sys_file_exists(script)) {
		/* the binary already exists, this condition should never happen
		 * since we checked before installing */
		ret_code = SWUPD_UNEXPECTED_CONDITION;
		error("There is already a binary called %s in %s, it will be skipped\n", sys_basename(filename), bin_directory);
		goto close_and_exit;
	}

	/* make sure the SWUPD_3RD_PARTY_BIN_DIR exist */
	ret_code = third_party_bin_directory_exist();
	if (ret_code) {
		goto close_and_exit;
	}

	/* open the file with mode set to 0755 */
	fd = open(script, O_RDWR | O_CREAT, mode);
	if (fd < 0) {
		error("The file %s failed to be created\n", script);
		ret_code = SWUPD_COULDNT_CREATE_FILE;
		goto close_and_exit;
	}
	fp = fdopen(fd, "w");
	if (!fp) {
		error("The file %s failed to be created\n", script);
		ret_code = SWUPD_COULDNT_CREATE_FILE;
		goto close_and_exit;
	}

	/* get the path for the 3rd-party content */
	third_party_bin_path = sys_path_join("%s/bin:%s/usr/bin:%s/usr/local/bin", globals.path_prefix, globals.path_prefix, globals.path_prefix);
	third_party_ld_path = sys_path_join("%s/usr/lib64:%s/usr/local/lib64", globals.path_prefix, globals.path_prefix);
	third_party_xdg_data_path = sys_path_join("%s/usr/share:%s/usr/local/share", globals.path_prefix, globals.path_prefix);
	third_party_xdg_conf_path = sys_path_join("%s/etc", globals.path_prefix);

	fprintf(fp, SCRIPT_TEMPLATE, third_party_bin_path, third_party_ld_path, third_party_xdg_data_path, third_party_xdg_conf_path, binary);

close_and_exit:
	if (fp) {
		fclose(fp);
	}
	FREE(binary);
	FREE(script);
	FREE(bin_directory);
	FREE(third_party_bin_path);
	FREE(third_party_ld_path);
	FREE(third_party_xdg_data_path);
	FREE(third_party_xdg_conf_path);

	return ret_code;
}

enum swupd_code third_party_update_wrapper_script(struct file *file)
{
	enum swupd_code ret_code = 0;
	int ret;
	char *bin_directory = NULL;
	char *script = NULL;
	char *binary = NULL;
	char *filename = NULL;

	if (!file || !third_party_file_is_binary(file)) {
		return ret_code;
	}

	filename = file->filename;
	bin_directory = third_party_get_bin_dir();
	script = third_party_get_binary_path(sys_basename(filename));
	binary = sys_path_join("%s/%s", globals.path_prefix, filename);

	/* the binary is either new or changed in the update, in either case
	 * we need to create the script */
	if (sys_file_exists(binary)) {
		/* if a script for the binary already exists, delete it */
		ret = sys_rm(script);
		if (ret && ret != -ENOENT) {
			ret_code = SWUPD_COULDNT_REMOVE_FILE;
			goto close_and_exit;
		}
		ret_code = third_party_create_wrapper_script(file);
		goto close_and_exit;
	}

	/* if the binary was removed during the update then we need to
	 * remove the script that exports it too */
	if (file->is_deleted) {
		ret = sys_rm(script);
		if (ret) {
			ret_code = SWUPD_COULDNT_REMOVE_FILE;
		}
	}

close_and_exit:
	FREE(binary);
	FREE(script);
	FREE(bin_directory);

	return ret_code;
}

#endif
