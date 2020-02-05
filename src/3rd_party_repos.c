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
#include "config.h"
#include "signature.h"
#include "swupd.h"

#include <errno.h>

#ifdef THIRDPARTY

char *third_party_get_bin_dir(void)
{
	return sys_path_join(globals_bkp.path_prefix, SWUPD_3RD_PARTY_BIN_DIR);
}

char *third_party_get_binary_path(const char *binary_name)
{
	return str_or_die("%s%s/%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_BIN_DIR, binary_name);
}

static char *get_repo_config_path(void)
{
	return str_or_die("%s/%s/%s", globals_bkp.state_dir, SWUPD_3RD_PARTY_DIRNAME, "repo.ini");
}

static char *get_repo_content_path(const char *repo_name)
{
	return str_or_die("%s%s/%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_BUNDLES_DIR, repo_name);
}

static char *get_repo_state_dir(const char *repo_name)
{
	return str_or_die("%s/%s/%s", globals_bkp.state_dir, SWUPD_3RD_PARTY_DIRNAME, repo_name);
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

	repo = list_search(*repos, section, cmp_repo_name_string);
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
	if (repo) {
		free(repo->name);
		free(repo->url);
	}
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

	// write url
	ret = config_write_config(fp, "url", repo_url);

exit:
	if (fp) {
		fclose(fp);
	}

	free(repo_config_file_path);
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

	if (ret) {
		return ret;
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
	free_string(&repo_config_file_path);
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
	free(repo_dir);

	repo_dir = get_repo_state_dir(repo_name);
	ret = sys_rm_recursive(repo_dir);
	if (ret < 0 && ret != -ENOENT) {
		error("Failed to delete repository state directory\n");
		ret_code = ret;
	}
	free(repo_dir);

	return ret_code;
}

enum swupd_code third_party_set_repo(struct repo *repo, bool sigcheck)
{
	char *repo_state_dir;
	char *repo_path_prefix;
	char *repo_cert_path;

	set_content_url(repo->url);
	set_version_url(repo->url);

	/* set up swupd to use the certificate from the 3rd-party repository */
	string_or_die(&repo_cert_path, "%s%s/%s%s", globals_bkp.path_prefix, SWUPD_3RD_PARTY_BUNDLES_DIR, repo->name, CERT_PATH);
	set_cert_path(repo_cert_path);
	/* if --nosigcheck was used, we do not attempt any signature checking */
	if (sigcheck) {
		signature_deinit();
		if (!signature_init(globals.cert_path, NULL)) {
			signature_deinit();
			error("Unable to validate the certificate %s\n\n", repo_cert_path);
			free_string(&repo_cert_path);
			return SWUPD_SIGNATURE_VERIFICATION_FAILED;
		}
	}
	free_string(&repo_cert_path);

	repo_path_prefix = get_repo_content_path(repo->name);
	set_path_prefix(repo_path_prefix);
	free_string(&repo_path_prefix);

	/* make sure there are state directories for the 3rd-party
	 * repo if not there already */
	repo_state_dir = get_repo_state_dir(repo->name);
	if (create_state_dirs(repo_state_dir)) {
		free_string(&repo_state_dir);
		error("Unable to create the state directories for repository %s\n\n", repo->name);
		return SWUPD_COULDNT_CREATE_DIR;
	}
	set_state_dir(repo_state_dir);
	free_string(&repo_state_dir);

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
		mom = load_mom(version, false, NULL);
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
	set_state_dir(globals_bkp.state_dir);
	set_content_url(globals_bkp.content_url);
	set_version_url(globals_bkp.version_url);

	return ret_code;
}

enum swupd_code third_party_run_operation(struct list *bundles, const char *repo, run_operation_fn_t run_operation_fn)
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
			ret = run_operation_fn(bundle);
			if (ret) {
				/* if we have an error here it is ok to overwrite any previous error */
				ret_code = ret;
			}
			info("\n");

			/* return the global variables to the original values */
		next:
			set_path_prefix(globals_bkp.path_prefix);
			set_state_dir(globals_bkp.state_dir);
			set_content_url(globals_bkp.content_url);
			set_version_url(globals_bkp.version_url);
		}
	}

	/* free data */
clean_and_exit:
	list_free_list_and_data(repos, repo_free_data);

	return ret_code;
}

enum swupd_code third_party_run_operation_multirepo(const char *repo, run_operation_fn_t run_operation_fn, enum swupd_code expected_ret_code, const char *op_name, int op_steps)
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

		ret_code = run_operation_fn(NULL);

	} else {

		for (iter = repos; iter; iter = iter->next) {
			selected_repo = iter->data;

			/* set the appropriate variables for the selected 3rd-party repo */
			ret = third_party_set_repo(selected_repo, globals.sigcheck);
			if (ret) {
				ret_code = ret;
				goto clean_and_exit;
			}

			/* set the repo's header */
			third_party_repo_header(selected_repo->name);

			ret = run_operation_fn(NULL);
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
	list_free_list_and_data(repos, repo_free_data);
	free_string(&steps_title);

	return ret_code;
}

void third_party_repo_header(const char *repo_name)
{
	char *header = NULL;

	string_or_die(&header, " 3rd-Party Repo: %s", repo_name);
	print_header(header);
	free_string(&header);
}

#endif
